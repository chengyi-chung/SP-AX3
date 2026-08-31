from pathlib import Path
from reportlab.lib import colors
from reportlab.lib.pagesizes import A3, landscape
from reportlab.lib.units import mm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.pdfgen import canvas

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "output" / "pdf" / "SP_AX3_Image_to_PLC_Mermaid_Source_2026-08-25.pdf"
OUT.parent.mkdir(parents=True, exist_ok=True)

pdfmetrics.registerFont(TTFont("MSJH", r"C:\Windows\Fonts\msjh.ttc", subfontIndex=0))
pdfmetrics.registerFont(TTFont("MSJH-Bold", r"C:\Windows\Fonts\msjhbd.ttc", subfontIndex=0))

SOURCE = r'''```mermaid
flowchart TD
    A["Basler Camera<br/>影像緩衝區 uint8_t*"] --> B["grabbedImage / m_mat<br/>cv::Mat, CV_8UC1<br/>原始相機影像座標<br/>原點：左上；X→右；Y→下；單位：pixel"]

    B --> C{"已載入 Homography<br/>校正資料？"}
    C -- 否 --> D["pathSourceImage = sourceFrame<br/>cv::Mat<br/>原始影像座標"]
    C -- 是 --> E["m_vision.undistortImage()<br/>cv::warpPerspective"]
    E --> F["pathSourceImage / correctedImage<br/>cv::Mat<br/>校正後影像座標<br/>原點：左上；單位：pixel"]

    G["SystemConfig.ini<br/>MaskX, MaskY<br/>MaskWidth, MaskHeight<br/>型別：int；單位：pixel"] --> H["建立 mask<br/>cv::Mat, CV_8UC1<br/>ROI 內=255；其餘=0"]
    H --> I{"影像是否校正？"}
    I -- 否 --> J["pathMask = mask<br/>effectiveRoiRect = roiRect"]
    I -- 是 --> K["undistortImage(mask)<br/>threshold + findNonZero"]
    K --> L["pathMask：校正後 Mask<br/>effectiveRoiRect：cv::Rect"]

    M["RefCenterX / RefCenterY<br/>int；原始影像 pixel"] --> N{"影像是否校正？"}
    N -- 否 --> O["effectiveReference<br/>cv::Point2d(RefCenterX, RefCenterY)"]
    N -- 是 --> P["m_vision.transformPoint()"]
    P --> Q["effectiveReference<br/>校正後機械原點<br/>cv::Point2d；pixel"]

    D --> R
    F --> R
    J --> R
    L --> R

    S["OffsetValue<br/>float；單位 mm"] --> T["offsetPixel = OffsetValue / TransferFactor"]
    U["TransferFactor<br/>float；單位 mm/pixel"] --> T
    T --> R
    V["BinaryLower / BinaryUpper<br/>int；灰階門檻"] --> R

    R["GenerateToolPathByType()<br/>ToolPathType 0/1/2"]
    R --> W["目前三種型別均進入<br/>GetToolPath_CurvatureOptimized_Mask()"]
    W --> X["二值化、Mask 限制<br/>尋找輪廓/影像邊緣<br/>曲率簡化與 inward offset"]
    X --> Y["toolPath.Path<br/>std::vector&lt;cv::Point2d&gt;<br/>原點：影像左上<br/>X→右；Y→下；單位：pixel"]

    Y --> Z["OptimizeGluePath()<br/>GluePathOptimizer::OptimizePath()"]
    J --> Z
    L --> Z
    Z --> AA["FilterByMask()<br/>只保留 effectiveRoiRect 內點"]
    AA --> AB["SplitByCenter()<br/>輪廓由頂點至底點切成兩鏈"]
    AB --> AC["比較兩鏈 averageX<br/>平均 X 大 = PathRight<br/>平均 X 小 = PathLeft"]
    AC --> AD["BuildSideProfileByY()<br/>右側取同 Y 最右點<br/>左側取同 Y 最左點"]
    AD --> AE["右側取樣最多 25 點<br/>左側依相同 Y 序列擬合"]
    AE --> AF["finalPath / m_OptimizedGluePath<br/>GluePath<br/>PathLeft、PathRight：vector&lt;cv::Point2d&gt;<br/>影像左上原點；pixel<br/>左右路徑 Y 完全一致"]

    AF --> AG["SortGluePathByAscendingY()<br/>由上到下排序"]
    AG --> AH["FilterGluePathByRoiAndLimit()<br/>只留 ROI 內點；最多 25 組"]
    AH --> AI["NormalizeGluePathPairCount()<br/>左右點數一致、Y 對齊"]

    O --> AJ
    Q --> AJ
    AI --> AJ
    AJ["ConvertToMachineCoordinates()"]
    AJ --> AK["m_machineGluePath<br/>cv::Point2d；pixel<br/>machineX = imageX - effectiveReferenceX<br/>machineY = imageY - effectiveReferenceY<br/>原點：RefCenter<br/>X→右為正；Y→下為正"]
    AK --> AL["m_machineGluePath_mm<br/>cv::Point2d；mm<br/>X/Y × TransferFactor"]
    AL --> AM["m_HMIGluePath_temp<br/>cv::Point2d；mm × 10<br/>X/Y × 10"]
    AM --> AN["m_HMIGluePath<br/>cv::Point2d，值已 lround<br/>邏輯上為整數運動座標<br/>仍保留相對 RefCenter 的正負號"]

    AN --> AO{"IDC_IDC_WORK_GO<br/>PathDataOut = 1？"}
    AO -- 是 --> AP["PathDataOut.csv<br/>來源：m_HMIGluePath<br/>Y = PathLeft.y<br/>X1 = -PathLeft.x<br/>X2 = PathRight.x<br/>保留跨越 RefCenter 的負值"]
    AO -- 否 --> AQ["不輸出 CSV"]

    AN --> AR["建立 PLC 三組暫存陣列<br/>vector&lt;uint16_t&gt;<br/>每組固定 30 筆"]
    AR --> AS["前 25 筆運動描述點<br/>Y = PathLeft.y<br/>X1 = -PathLeft.x，向左為正<br/>X2 = PathRight.x，向右為正"]
    AS --> AT["不足 30 筆時<br/>第 26～30 筆重複最後一點"]
    AT --> AU["toSignedReg()<br/>lround 後限制於<br/>-32768 ～ 32767"]
    AU --> AV["int16_t → uint16_t<br/>負數以 two's complement<br/>保存相同 16-bit bit pattern"]

    AV --> AW{"Modbus TCP<br/>連線成功？"}
    AW -- 否 --> AX["CSV 已先產生<br/>顯示連線失敗<br/>PLC 不更新"]
    AW -- 是 --> AY["D14～D43：X1，30個 register<br/>modbus_write_registers()"]
    AY --> AZ["D44～D73：Y，30個 register"]
    AZ --> BA["D74～D103：X2，30個 register"]
    BA --> BB["PLC 端必須以 INT16 解讀<br/>CSV -25 ↔ Register 65511 / 0xFFE7<br/>PLC INT16 = -25"]
```'''

page_w, page_h = landscape(A3)
c = canvas.Canvas(str(OUT), pagesize=(page_w, page_h))
c.setTitle("SP-AX3 Mermaid Source - 2026-08-25")

lines = SOURCE.splitlines()
per_page = 43
pages = (len(lines) + per_page - 1) // per_page

for page_idx in range(pages):
    c.setFillColor(colors.HexColor("#175C8C"))
    c.setFont("MSJH-Bold", 16)
    c.drawString(15*mm, page_h-15*mm, "SP-AX3 影像至 PLC 資料流程 - Mermaid 原始碼（未渲染）")
    c.setFillColor(colors.HexColor("#52606D"))
    c.setFont("MSJH", 8)
    c.drawRightString(page_w-15*mm, page_h-15*mm, f"2026-08-25 | 第 {page_idx+1}/{pages} 頁")

    y = page_h-25*mm
    c.setFillColor(colors.HexColor("#202B33"))
    c.setFont("MSJH", 7.2)
    start = page_idx * per_page
    for n, line in enumerate(lines[start:start+per_page], start=start+1):
        c.setFillColor(colors.HexColor("#83919B"))
        c.drawRightString(14*mm, y, f"{n:03d}")
        c.setFillColor(colors.HexColor("#202B33"))
        c.drawString(17*mm, y, line)
        y -= 5.6*mm

    c.setStrokeColor(colors.HexColor("#D8DEE3"))
    c.line(15*mm, 10*mm, page_w-15*mm, 10*mm)
    c.setFillColor(colors.HexColor("#52606D"))
    c.setFont("MSJH", 7)
    c.drawString(15*mm, 5.5*mm, "內容為可複製的 Mermaid 語法，未轉換成圖形。")
    c.showPage()

c.save()
print(OUT)
