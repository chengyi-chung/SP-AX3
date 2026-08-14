from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "vendor"))

from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER, TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import mm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak,
    KeepTogether, Flowable
)

ROOT = HERE.parents[1]
OUT = ROOT / "output" / "pdf" / "SP-AX3_Calibration流程架構分析.pdf"
OUT.parent.mkdir(parents=True, exist_ok=True)

FONT_PATH = r"C:\Windows\Fonts\kaiu.ttf"
pdfmetrics.registerFont(TTFont("CJK", FONT_PATH))

NAVY = colors.HexColor("#17324D")
BLUE = colors.HexColor("#2474A6")
CYAN = colors.HexColor("#EAF5FA")
PALE = colors.HexColor("#F4F7F9")
ORANGE = colors.HexColor("#D97706")
RED = colors.HexColor("#B42318")
GREEN = colors.HexColor("#277A53")
INK = colors.HexColor("#25313C")
MUTED = colors.HexColor("#5D6B78")
LINE = colors.HexColor("#CBD5DF")


class FlowDiagram(Flowable):
    def __init__(self, width=170*mm, height=78*mm):
        super().__init__()
        self.width, self.height = width, height

    def draw(self):
        c = self.canv
        lanes = [
            ("WorkTab UI", "選取影像 / ROI / 顯示 / 例外處理", 0, NAVY),
            ("UAXVision", "角點偵測 / fisheye 校正 / 去畸變", 1, BLUE),
            ("持久化", "calibration.yml / SystemConfig.ini", 2, GREEN),
        ]
        lane_w = 51*mm
        gap = 7*mm
        for title, text, idx, color in lanes:
            x = idx*(lane_w+gap)
            c.setFillColor(color)
            c.roundRect(x, self.height-18*mm, lane_w, 13*mm, 3*mm, fill=1, stroke=0)
            c.setFillColor(colors.white); c.setFont("CJK", 10)
            c.drawCentredString(x+lane_w/2, self.height-10.5*mm, title)
            c.setFillColor(PALE); c.setStrokeColor(LINE)
            c.roundRect(x, 5*mm, lane_w, 48*mm, 2*mm, fill=1, stroke=1)
            c.setFillColor(INK); c.setFont("CJK", 8.5)
            for j, line in enumerate(text.split(" / ")):
                c.drawCentredString(x+lane_w/2, 42*mm-j*8*mm, line)
        c.setStrokeColor(ORANGE); c.setFillColor(ORANGE); c.setLineWidth(1.5)
        for idx in (0, 1):
            x1 = idx*(lane_w+gap)+lane_w
            x2 = x1+gap
            y = 29*mm
            c.line(x1+1*mm, y, x2-2*mm, y)
            p = c.beginPath(); p.moveTo(x2-2*mm, y+1.8*mm); p.lineTo(x2+1*mm, y); p.lineTo(x2-2*mm, y-1.8*mm); p.close()
            c.drawPath(p, fill=1, stroke=0)
        c.setFillColor(MUTED); c.setFont("CJK", 8)
        c.drawCentredString(self.width/2, 0, "控制流程由左至右；校正參數載入後反向供 WorkTab 顯示與路徑計算使用")


base = getSampleStyleSheet()
styles = {
    "title": ParagraphStyle("title", fontName="CJK", fontSize=25, leading=33, textColor=NAVY, alignment=TA_LEFT, spaceAfter=7*mm),
    "subtitle": ParagraphStyle("subtitle", fontName="CJK", fontSize=11, leading=18, textColor=MUTED, spaceAfter=9*mm),
    "h1": ParagraphStyle("h1", fontName="CJK", fontSize=17, leading=24, textColor=NAVY, spaceBefore=4*mm, spaceAfter=4*mm),
    "h2": ParagraphStyle("h2", fontName="CJK", fontSize=12.5, leading=18, textColor=BLUE, spaceBefore=3*mm, spaceAfter=2*mm),
    "body": ParagraphStyle("body", fontName="CJK", fontSize=9.6, leading=16, textColor=INK, spaceAfter=2.5*mm),
    "small": ParagraphStyle("small", fontName="CJK", fontSize=8.2, leading=13, textColor=MUTED),
    "code": ParagraphStyle("code", fontName="CJK", fontSize=8.5, leading=14, textColor=NAVY, backColor=PALE, borderColor=LINE, borderWidth=.5, borderPadding=6, spaceAfter=3*mm),
    "callout": ParagraphStyle("callout", fontName="CJK", fontSize=9.4, leading=16, textColor=INK, backColor=CYAN, borderColor=BLUE, borderWidth=.7, borderPadding=8, spaceBefore=2*mm, spaceAfter=4*mm),
}


def P(text, style="body"):
    return Paragraph(text, styles[style])


def bullets(items):
    return [P("• " + item, "body") for item in items]


def table(rows, widths, header=True):
    data = [[P(str(v), "small") for v in row] for row in rows]
    t = Table(data, colWidths=widths, repeatRows=1 if header else 0, hAlign="LEFT")
    cmd = [
        ("FONTNAME", (0,0), (-1,-1), "CJK"), ("VALIGN", (0,0), (-1,-1), "TOP"),
        ("GRID", (0,0), (-1,-1), .4, LINE), ("LEFTPADDING", (0,0), (-1,-1), 6),
        ("RIGHTPADDING", (0,0), (-1,-1), 6), ("TOPPADDING", (0,0), (-1,-1), 5),
        ("BOTTOMPADDING", (0,0), (-1,-1), 5),
    ]
    if header:
        cmd += [("BACKGROUND", (0,0), (-1,0), NAVY), ("TEXTCOLOR", (0,0), (-1,0), colors.white)]
    for r in range(1 if header else 0, len(rows)):
        if r % 2 == 0: cmd.append(("BACKGROUND", (0,r), (-1,r), PALE))
    t.setStyle(TableStyle(cmd))
    return t


def header_footer(canvas, doc):
    canvas.saveState()
    w, h = A4
    canvas.setStrokeColor(LINE); canvas.line(20*mm, 15*mm, w-20*mm, 15*mm)
    canvas.setFont("CJK", 7.5); canvas.setFillColor(MUTED)
    canvas.drawString(20*mm, 9.5*mm, "SP-AX3 Calibration 流程架構分析")
    canvas.drawRightString(w-20*mm, 9.5*mm, f"第 {doc.page} 頁")
    canvas.restoreState()


story = []
story += [Spacer(1, 18*mm), P("SP-AX3 Calibration<br/>流程架構分析", "title")]
story += [P("鏡頭畸變校正、像素比例校正與 Tool Path 座標鏈之程式架構說明", "subtitle")]
story += [FlowDiagram(), Spacer(1, 8*mm)]
story += [P("文件摘要", "h1")]
story += [P("目前有效的 Calibration 核心由 <b>WorkTab</b> 負責 UI 與流程協調，<b>UAXVision</b> 負責棋盤角點偵測、魚眼模型計算與影像去畸變。校正結果寫入 calibration.yml；pixel 到 mm 的比例則另由 Factor 流程計算並寫入 SystemConfig.ini。", "callout")]
story += [table([
    ["校正層次", "主要輸出", "用途"],
    ["鏡頭畸變校正", "cameraMatrix、distCoeffs", "把魚眼影像轉為幾何較正確的影像"],
    ["比例校正", "TransferFactor (mm/pixel)", "將像素距離換算為機械距離"],
    ["路徑座標轉換", "Machine / HMI GluePath", "將校正後影像路徑轉為設備使用座標"],
], [37*mm, 58*mm, 70*mm])]

story += [PageBreak(), P("1. 模組責任與資料邊界", "h1")]
story += [table([
    ["模組", "責任", "關鍵位置"],
    ["WorkTab", "按鈕事件、ROI、ImageFlip、顯示、Factor 框選、Tool Path 串接", "WorkTab.cpp:739、1851、2629、3550、3940"],
    ["UAXVision", "棋盤規格、角點偵測、fisheye 校正、去畸變、YAML 存取", "UAX/UAXVision.cpp:14-395、451-495"],
    ["SystemConfig", "保存 TransferFactor、Mask、ImageFlip 等運行參數", "SystemConfig.ini / m_SystemPara"],
    ["Calibration Dialog", "目前僅為空白 MFC 對話框，非核心校正實作", "Calibration.cpp"],
    ["MV Calibration", "獨立子專案，目前主要管理 Calibration Image 資料夾與檔案清單", "MV Calibration/MV CalibrationDlg.cpp"],
], [34*mm, 82*mm, 49*mm])]
story += [P("核心資料流", "h2")]
story += [P("WorkTab UI → UAXVision::calibrate() → cameraMatrix / distCoeffs → calibration.yml → UAXVision::undistortImage() → 校正後顯示與 Tool Path。", "code")]

story += [P("2. 鏡頭校正建立流程", "h1")]
steps = [
    ("1", "選取影像", "Calibration 按鈕進入 OnBnClickedMfcbtnWorkImgCalibrate()，以 Windows 多選檔案對話框取得 JPG、PNG 或 BMP。"),
    ("2", "設定搜尋 ROI", "Mask 有效時呼叫 setCalibrationROI()；此 ROI 只限制棋盤角點搜尋，不代表校正後裁切。"),
    ("3", "嘗試棋盤規格", "依序嘗試目前規格、9×6、8×6、7×6、11×8、10×7、10×8、6×4、5×4；數字為內部角點數。"),
    ("4", "處理每張影像", "確認尺寸一致，先用 findChessboardCornersSB()，失敗再用 findChessboardCorners()，最後以 cornerSubPix() 細化。"),
    ("5", "魚眼模型求解", "至少三張影像成功後執行 cv::fisheye::calibrate()，先用條件檢查；若資料 ill-conditioned，移除 CALIB_CHECK_COND 後重試。"),
    ("6", "預覽與儲存", "顯示最多四張角點預覽，將 cameraMatrix、distCoeffs、imageWidth、imageHeight 寫入應用程式目錄的 calibration.yml。"),
]
story += [table([["步驟", "階段", "行為"]] + [list(x) for x in steps], [15*mm, 36*mm, 114*mm])]

story += [PageBreak(), P("3. 角點偵測與魚眼模型細節", "h1")]
story += bullets([
    "預設棋盤內部角點為 9×6，單格尺寸為 25.0；理想平面點以 (j×squareSize, i×squareSize, 0) 建立。",
    "ROI 模式下先在局部影像偵測，再將角點加回 ROI 位移，恢復為完整影像座標。",
    "影像尺寸不一致的校正照片會被略過；有效角點影像少於三張即失敗。",
    "嚴格模式旗標為 CALIB_RECOMPUTE_EXTRINSIC、CALIB_FIX_SKEW、CALIB_CHECK_COND。",
    "RMS 為 pixel 單位；大於 0.8 僅輸出警告，目前不阻止儲存。",
])
story += [P("成功輸出", "h2")]
story += [table([
    ["資料", "內容", "目前持久化"],
    ["cameraMatrix", "3×3 相機內參矩陣", "是"],
    ["distCoeffs", "4×1 魚眼畸變係數", "是"],
    ["imageSize", "校正影像寬高", "是"],
    ["boardSize / squareSize", "棋盤規格與實際單格尺寸", "否"],
    ["RMS / 模型版本 / 相機 ID", "品質與適用性追蹤資訊", "否"],
    ["Calibration ROI", "建立參數時的角點搜尋區域", "否"],
], [43*mm, 76*mm, 46*mm])]
story += [P("注意：重新啟動後只從 YAML 還原內參、畸變與影像尺寸；棋盤規格仍回到建構子的 9×6 / 25.0 預設值。", "callout")]

story += [P("4. 校正資料載入與顯示", "h1")]
story += [P("WorkTab 初始化即嘗試載入 calibration.yml。isCalibrated() 沒有獨立狀態旗標，而是依焦距值是否仍接近單位矩陣，以及畸變係數是否全零來判斷。", "body")]
story += [P("m_mat（目前顯示方向） → ApplyInverseConfiguredFlip → 原始相機方向 → undistortImage → 校正後原始方向 → ApplyConfiguredFlip → 校正後顯示方向", "code")]
story += [P("去畸變使用 estimateNewCameraMatrixForUndistortRectify() 與 fisheye::undistortImage()；預設 balance = 0.8，用於權衡視野保留與黑邊。", "body")]

story += [PageBreak(), P("5. Calibration 在 Tool Path 中的位置", "h1")]
story += [table([
    ["階段", "影像 / 資料", "座標特性"],
    ["相機取像後", "m_mat", "可能已套用 ImageFlip"],
    ["校正輸入", "rawOrientationImage", "已逆轉 ImageFlip，回到校正參數的原始方向"],
    ["路徑來源", "pathSourceImage = undistortedRaw", "原始相機方向且已去畸變"],
    ["顯示來源", "correctedImage", "去畸變後再套回顯示方向"],
    ["路徑擷取前", "imgClone", "目前再以 cv::flip(..., -1) 旋轉 180°"],
    ["比例換算", "TransferFactor", "mm/pixel"],
], [35*mm, 65*mm, 65*mm])]
story += [P("Mask 也會以 ApplyInverseConfiguredFlip() 轉回原始相機方向，使 pathMask 與 pathSourceImage 對齊。若校正不存在、輸出為空或 OpenCV 拋出例外，流程退回原始 m_mat 與原始 Mask，Tool Path 不會中止。", "body")]
story += [P("座標風險", "h2")]
story += [P("流程同時包含 ImageFlip、inverse flip、去畸變、Mask inverse flip、imgClone 再旋轉 180°，且部分 ApplyConfiguredFlipToToolPath / GluePath 程式已被註解。這是最需要以測試圖與 CSV 比對驗證的區域。", "callout")]

story += [P("6. Factor 比例校正流程", "h1")]
story += [P("Factor 按鈕由 OnBnClickedMfcbtnWorkImgFactor() 處理。它不是重新求鏡頭參數，而是在已去畸變的棋盤影像上取得 pixel 與 mm 的比例。", "body")]
story += [table([
    ["順序", "動作"],
    ["1", "確認已有影像與 calibration.yml，並輸入單格實際長度（預設 25 mm）。"],
    ["2", "將影像還原到原始相機方向並去畸變，再偵測棋盤角點。"],
    ["3", "切換至 Factor 選取模式，由使用者框選多個格點。"],
    ["4", "統計框選區內相鄰角點的平均單格像素距離。"],
    ["5", "計算 TransferFactor = 單格實際長度(mm) ÷ 平均單格像素距離(pixel)。"],
    ["6", "將 TransferFactor 寫入 SystemConfig.ini，供 Offset 與機械座標換算。"],
], [18*mm, 147*mm])]

story += [PageBreak(), P("7. 主要問題與改善優先序", "h1")]
story += [table([
    ["優先序", "問題", "建議"],
    ["P0", "Tool Path 座標鏈包含多次 flip，且部分點座標翻轉被註解。", "建立固定測試圖，逐階段輸出影像與角點 CSV，驗證影像、Mask、路徑與機械方向。"],
    ["P1", "YAML 未保存棋盤規格、RMS、相機 ID、ImageFlip 與版本。", "擴充 calibration.yml schema，載入時做完整相容性驗證。"],
    ["P1", "目前輸入解析度與 calibration.yml 的 imageSize 不一致時仍可能繼續。", "載入或去畸變前拒絕不相容尺寸，或依縮放比例同步調整內參。"],
    ["P1", "自動嘗試多個 board size，可能接受語意錯誤但可收斂的規格。", "由 UI 明確指定棋盤規格；自動偵測只作輔助並要求確認。"],
    ["P2", "Calibration Dialog、MV Calibration 與 WorkTab 三套入口責任模糊。", "保留單一正式入口，或明確將獨立工具定位為影像採集器。"],
    ["P2", "RMS > 0.8 只警告，仍可覆寫現有參數。", "加入品質門檻與舊檔備份，顯示有效影像數及每張重投影誤差。"],
], [17*mm, 72*mm, 76*mm])]
story += [P("建議驗證基準", "h2")]
story += bullets([
    "同一棋盤在畫面中心與四角的單格 mm/pixel 誤差是否落在允許範圍。",
    "ImageFlip = 0 / 90 / 180 / 270（若支援）時，ROI、角點、路徑與機械方向是否一致。",
    "更換解析度、相機或棋盤規格後，舊 calibration.yml 是否被正確拒絕。",
    "校正失敗回退原圖時，UI 是否明確提示，而不是讓操作員誤以為已使用校正影像。",
])

story += [P("8. 結論", "h1")]
story += [P("SP-AX3 的 Calibration 是兩階段架構：UAXVision 先建立並套用魚眼鏡頭模型，WorkTab 再透過 Factor 流程建立 mm/pixel 尺度。兩者共同供 Tool Path 使用。現有計算主幹完整，但持久化資訊與座標方向管理仍是主要技術風險；優先完成座標鏈測試與 YAML 相容性驗證，可顯著提升校正結果的可追溯性與設備安全性。", "callout")]
story += [Spacer(1, 4*mm), P("分析基準：目前工作區原始碼；主要檔案為 WorkTab.cpp、WorkTab.h、UAX/UAXVision.cpp、UAX/UAXVision.h、Calibration.cpp 與 MV Calibration/MV CalibrationDlg.cpp。", "small")]

doc = SimpleDocTemplate(
    str(OUT), pagesize=A4, rightMargin=20*mm, leftMargin=20*mm,
    topMargin=18*mm, bottomMargin=20*mm,
    title="SP-AX3 Calibration 流程架構分析",
    author="Codex",
    subject="SP-AX3 鏡頭校正、Factor 與 Tool Path 架構分析",
)
doc.build(story, onFirstPage=header_footer, onLaterPages=header_footer)
print(OUT)
