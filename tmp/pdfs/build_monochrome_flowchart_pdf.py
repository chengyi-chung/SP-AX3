from pathlib import Path
from reportlab.lib import colors
from reportlab.lib.pagesizes import A3
from reportlab.lib.units import mm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.pdfgen import canvas

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "output" / "pdf" / "SP_AX3_Image_to_PLC_Monochrome_Flowchart_2026-08-25.pdf"
OUT.parent.mkdir(parents=True, exist_ok=True)

pdfmetrics.registerFont(TTFont("MSJH", r"C:\Windows\Fonts\msjh.ttc", subfontIndex=0))
pdfmetrics.registerFont(TTFont("MSJH-Bold", r"C:\Windows\Fonts\msjhbd.ttc", subfontIndex=0))

W, H = A3
INK = colors.HexColor("#34383B")
LINE = colors.HexColor("#777C80")
BORDER = colors.HexColor("#D4D7D9")


def text_lines(c, cx, cy, lines, size=10, bold_first=True, leading=4.8*mm):
    lines = list(lines)
    total = (len(lines)-1) * leading
    y = cy + total/2
    for i, line in enumerate(lines):
        c.setFont("MSJH-Bold" if bold_first and i == 0 else "MSJH", size if i == 0 else size-1)
        c.setFillColor(INK)
        c.drawCentredString(cx, y, line)
        y -= leading


def box(c, cx, top, lines, width=118*mm, height=27*mm):
    x = cx-width/2
    y = top-height
    c.setFillColor(colors.white)
    c.setStrokeColor(BORDER)
    c.setLineWidth(0.7)
    c.roundRect(x, y, width, height, 4*mm, fill=1, stroke=1)
    text_lines(c, cx, y+height/2, lines)
    return (cx, y, cx, top)


def diamond(c, cx, top, lines, width=62*mm, height=38*mm):
    cy = top-height/2
    pts = [(cx, top), (cx+width/2, cy), (cx, top-height), (cx-width/2, cy)]
    path = c.beginPath()
    path.moveTo(*pts[0])
    for p in pts[1:]: path.lineTo(*p)
    path.close()
    c.setFillColor(colors.white)
    c.setStrokeColor(BORDER)
    c.setLineWidth(0.7)
    c.drawPath(path, fill=1, stroke=1)
    text_lines(c, cx, cy, lines, size=10, bold_first=False)
    return (cx, top-height, cx, top)


def arrow(c, x1, y1, x2, y2, label=None):
    c.setStrokeColor(LINE)
    c.setFillColor(LINE)
    c.setLineWidth(0.8)
    c.line(x1, y1, x2, y2+2.2*mm)
    c.line(x2, y2, x2-2.0*mm, y2+3.2*mm)
    c.line(x2, y2, x2+2.0*mm, y2+3.2*mm)
    if label:
        c.setFont("MSJH", 8)
        c.setFillColor(INK)
        c.drawString(x1+2*mm, (y1+y2)/2, label)


def branch(c, start, left_target, right_target, left_label="否", right_label="是"):
    sx, sy = start
    lx, ly = left_target
    rx, ry = right_target
    mid_y = sy-9*mm
    c.setStrokeColor(LINE); c.setFillColor(LINE); c.setLineWidth(0.8)
    c.line(sx, sy, sx, mid_y)
    c.line(sx, mid_y, lx, mid_y)
    c.line(lx, mid_y, lx, ly+2.2*mm)
    c.line(lx, ly, lx-2*mm, ly+3.2*mm); c.line(lx, ly, lx+2*mm, ly+3.2*mm)
    c.line(sx, mid_y, rx, mid_y)
    c.line(rx, mid_y, rx, ry+2.2*mm)
    c.line(rx, ry, rx-2*mm, ry+3.2*mm); c.line(rx, ry, rx+2*mm, ry+3.2*mm)
    c.setFont("MSJH", 8); c.setFillColor(INK)
    c.drawCentredString((sx+lx)/2, mid_y+2*mm, left_label)
    c.drawCentredString((sx+rx)/2, mid_y+2*mm, right_label)


def header(c, title, page):
    c.setFont("MSJH-Bold", 17)
    c.setFillColor(INK)
    c.drawString(20*mm, H-18*mm, title)
    c.setFont("MSJH", 8)
    c.setFillColor(LINE)
    c.drawRightString(W-20*mm, H-18*mm, f"2026-08-25 | 第 {page}/3 頁")
    c.setStrokeColor(BORDER)
    c.line(20*mm, H-22*mm, W-20*mm, H-22*mm)


def footer(c):
    c.setStrokeColor(BORDER)
    c.line(20*mm, 13*mm, W-20*mm, 13*mm)
    c.setFont("MSJH", 7.5); c.setFillColor(LINE)
    c.drawString(20*mm, 8*mm, "SP-AX3 從影像邊緣至 PLC 路徑資料流程 - 黑白流程圖")


c = canvas.Canvas(str(OUT), pagesize=A3)
c.setTitle("SP-AX3 Image to PLC Monochrome Flowchart - 2026-08-25")
cx = W/2

# Page 1: image, mask, calibration, edge path
header(c, "1. 影像、Mask、校正與邊緣路徑", 1)
n1 = box(c, cx, H-31*mm, ["Basler Camera", "uint8_t* → grabbedImage / m_mat", "cv::Mat, CV_8UC1；影像左上原點；pixel"], height=29*mm)
n2 = box(c, cx, H-72*mm, ["固定單一畫格", "sourceFrame = m_mat.clone()", "避免相機執行緒更新造成資料前後不一致"])
arrow(c, n1[0], n1[1], n2[0], n2[3])
n3 = diamond(c, cx, H-112*mm, ["Homography", "校正有效？"])
arrow(c, n2[0], n2[1], n3[0], n3[3])
left = box(c, W*0.29, H-169*mm, ["未校正影像", "pathSourceImage = sourceFrame", "原始影像座標"], width=92*mm)
right = box(c, W*0.71, H-169*mm, ["校正影像", "undistortImage() / warpPerspective", "校正後影像座標"], width=92*mm)
branch(c, (n3[0], n3[1]), (left[0], left[3]), (right[0], right[3]))
n4 = box(c, cx, H-215*mm, ["SystemConfig.ini → 建立 Mask", "MaskX / MaskY / MaskWidth / MaskHeight", "cv::Mat, CV_8UC1；ROI=255；其餘=0"], height=30*mm)
arrow(c, left[0], left[1], n4[0]-20*mm, n4[3])
arrow(c, right[0], right[1], n4[0]+20*mm, n4[3])
n5 = diamond(c, cx, H-260*mm, ["Mask 與 RefCenter", "需要校正？"])
arrow(c, n4[0], n4[1], n5[0], n5[3])
left2 = box(c, W*0.29, H-317*mm, ["原始 ROI / 原點", "effectiveRoiRect = roiRect", "effectiveReference = RefCenter"], width=94*mm)
right2 = box(c, W*0.71, H-317*mm, ["校正 ROI / 原點", "Mask：warpPerspective", "RefCenter：transformPoint()"], width=94*mm)
branch(c, (n5[0], n5[1]), (left2[0], left2[3]), (right2[0], right2[3]))
n6 = box(c, cx, H-363*mm, ["Offset 與邊緣路徑", "offsetPixel = OffsetValue(mm) / TransferFactor(mm/pixel)", "GetToolPath_CurvatureOptimized_Mask() → toolPath.Path"], height=31*mm)
arrow(c, left2[0], left2[1], n6[0]-20*mm, n6[3])
arrow(c, right2[0], right2[1], n6[0]+20*mm, n6[3])
footer(c); c.showPage()

# Page 2: optimization and coordinate conversion
header(c, "2. 左右路徑最佳化與座標轉換", 2)
tops = [31, 70, 109, 148, 187, 226, 265, 304, 343, 382]
nodes = [
    ["toolPath.Path", "std::vector<cv::Point2d>", "影像左上原點；X→右、Y→下；pixel"],
    ["OptimizeGluePath() / FilterByMask()", "只保留 effectiveRoiRect 內點"],
    ["SplitByCenter()", "由頂點到底點切成 chainA / chainB"],
    ["左右判定", "averageX 大 → PathRight；averageX 小 → PathLeft"],
    ["同步取樣與正規化", "最多 25 組；左右共同 Y；依 Y 遞增排序"],
    ["m_OptimizedGluePath", "GluePath：PathLeft / PathRight", "影像左上原點；pixel"],
    ["m_machineGluePath", "x = imageX - RefCenterX；y = imageY - RefCenterY", "RefCenter 原點；pixel；保留正負號"],
    ["m_machineGluePath_mm", "machine x/y × TransferFactor", "RefCenter 原點；mm"],
    ["m_HMIGluePath_temp", "machine mm × 10", "單位：mm × 10"],
    ["m_HMIGluePath", "lround(x/y)；邏輯整數運動座標", "相對 RefCenter；保留正負號"],
]
prev = None
for top_mm, lines in zip(tops, nodes):
    cur = box(c, cx, H-top_mm*mm, lines, height=27*mm)
    if prev: arrow(c, prev[0], prev[1], cur[0], cur[3])
    prev = cur
footer(c); c.showPage()

# Page 3: Work Go, CSV, Modbus, PLC
header(c, "3. WORK_GO、CSV 與 PLC Modbus TCP 輸出", 3)
tops = [31, 71, 111, 151, 191, 231, 271, 311, 351]
nodes = [
    ["IDC_IDC_WORK_GO", "OnBnClickedIdcWorkGo()", "確認 PathLeft / PathRight 有有效資料"],
    ["重建最新 HMI 座標", "ConvertToMachineCoordinates(effectiveReference)"],
    ["PathDataOut.csv", "PathDataOut=1 時輸出；Modbus 離線仍產生", "Y=Left.y；X1=-Left.x；X2=Right.x"],
    ["建立 PLC 暫存陣列", "x1Regs / yRegs / x2Regs：std::vector<uint16_t>", "最多 25 點；第 26～30 筆重複最後有效點"],
    ["共同運動座標公式", "Y=m_HMIGluePath.PathLeft.y", "X1=-PathLeft.x；X2=PathRight.x；保留負值"],
    ["Signed INT16 編碼", "lround；限制 -32768～32767", "int16_t bit pattern → uint16_t Holding Register"],
    ["Modbus TCP 寫入 X1", "D14～D43；30 個 register"],
    ["Modbus TCP 寫入 Y / X2", "D44～D73 = Y；D74～D103 = X2", "modbus_write_registers()"],
    ["PLC 以 INT16 解讀", "CSV -25 ↔ Register 65511 / 0xFFE7", "PLC signed INT16 = -25；CSV 與 PLC 邏輯值一致"],
]
prev = None
for top_mm, lines in zip(tops, nodes):
    cur = box(c, cx, H-top_mm*mm, lines, height=28*mm)
    if prev: arrow(c, prev[0], prev[1], cur[0], cur[3])
    prev = cur
footer(c); c.showPage()

c.save()
print(OUT)
