from pathlib import Path

from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER, TA_LEFT
from reportlab.lib.pagesizes import A4, landscape
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import mm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (
    Flowable, KeepTogether, PageBreak, Paragraph, SimpleDocTemplate,
    Spacer, Table, TableStyle
)

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "output" / "pdf" / "SP_AX3_Image_to_PLC_Data_Flow_2026-08-25.pdf"
OUT.parent.mkdir(parents=True, exist_ok=True)

pdfmetrics.registerFont(TTFont("MSJH", r"C:\Windows\Fonts\msjh.ttc", subfontIndex=0))
pdfmetrics.registerFont(TTFont("MSJH-Bold", r"C:\Windows\Fonts\msjhbd.ttc", subfontIndex=0))

PAGE = landscape(A4)
BLUE = colors.HexColor("#175C8C")
LIGHT_BLUE = colors.HexColor("#EAF4FA")
GREEN = colors.HexColor("#2F7D5B")
LIGHT_GREEN = colors.HexColor("#EAF6F0")
ORANGE = colors.HexColor("#A65B13")
LIGHT_ORANGE = colors.HexColor("#FFF1E3")
GRAY = colors.HexColor("#52606D")
LIGHT_GRAY = colors.HexColor("#F3F5F7")
RED = colors.HexColor("#A53A3A")

styles = getSampleStyleSheet()
title = ParagraphStyle("title", fontName="MSJH-Bold", fontSize=22, leading=30,
                       textColor=BLUE, alignment=TA_CENTER, spaceAfter=8*mm)
h1 = ParagraphStyle("h1", fontName="MSJH-Bold", fontSize=16, leading=22,
                    textColor=BLUE, spaceAfter=4*mm)
h2 = ParagraphStyle("h2", fontName="MSJH-Bold", fontSize=11, leading=15,
                    textColor=GRAY, spaceBefore=2*mm, spaceAfter=2*mm)
body = ParagraphStyle("body", fontName="MSJH", fontSize=9.2, leading=14,
                      textColor=colors.HexColor("#24313A"))
small = ParagraphStyle("small", fontName="MSJH", fontSize=8, leading=11,
                       textColor=GRAY)
note = ParagraphStyle("note", fontName="MSJH", fontSize=9, leading=13,
                      textColor=RED, leftIndent=4*mm, rightIndent=4*mm)


class FlowDiagram(Flowable):
    def __init__(self, nodes, width=250*mm, box_h=18*mm, gap=8*mm):
        super().__init__()
        self.nodes = nodes
        self.width = width
        self.box_h = box_h
        self.gap = gap
        self.height = len(nodes) * box_h + (len(nodes)-1) * gap

    def draw(self):
        c = self.canv
        box_w = self.width * 0.82
        x = (self.width - box_w) / 2
        for idx, (heading, detail, palette) in enumerate(self.nodes):
            y = self.height - (idx+1)*self.box_h - idx*self.gap
            stroke, fill = palette
            c.setStrokeColor(stroke)
            c.setFillColor(fill)
            c.roundRect(x, y, box_w, self.box_h, 3*mm, fill=1, stroke=1)
            c.setFillColor(stroke)
            c.setFont("MSJH-Bold", 9.5)
            c.drawCentredString(self.width/2, y+self.box_h*0.62, heading)
            c.setFillColor(colors.HexColor("#24313A"))
            c.setFont("MSJH", 7.6)
            lines = detail.split("\n")
            base = y+self.box_h*0.22
            for li, line in enumerate(lines[:2]):
                c.drawCentredString(self.width/2, base-li*3.8*mm, line)
            if idx < len(self.nodes)-1:
                top_next = y-self.gap
                c.setStrokeColor(GRAY)
                c.setFillColor(GRAY)
                c.line(self.width/2, y, self.width/2, top_next+2.4*mm)
                c.line(self.width/2, top_next+2.4*mm, self.width/2-1.7*mm, top_next+5*mm)
                c.line(self.width/2, top_next+2.4*mm, self.width/2+1.7*mm, top_next+5*mm)


def p(text, style=body):
    return Paragraph(text, style)


def table(data, widths, header=True):
    t = Table(data, colWidths=widths, repeatRows=1 if header else 0, hAlign="LEFT")
    commands = [
        ("FONTNAME", (0,0), (-1,-1), "MSJH"),
        ("FONTSIZE", (0,0), (-1,-1), 8),
        ("LEADING", (0,0), (-1,-1), 11),
        ("VALIGN", (0,0), (-1,-1), "TOP"),
        ("GRID", (0,0), (-1,-1), 0.35, colors.HexColor("#C9D2D9")),
        ("ROWBACKGROUNDS", (0,1), (-1,-1), [colors.white, LIGHT_GRAY]),
        ("LEFTPADDING", (0,0), (-1,-1), 4),
        ("RIGHTPADDING", (0,0), (-1,-1), 4),
        ("TOPPADDING", (0,0), (-1,-1), 4),
        ("BOTTOMPADDING", (0,0), (-1,-1), 4),
    ]
    if header:
        commands += [
            ("BACKGROUND", (0,0), (-1,0), BLUE),
            ("TEXTCOLOR", (0,0), (-1,0), colors.white),
            ("FONTNAME", (0,0), (-1,0), "MSJH-Bold"),
        ]
    t.setStyle(TableStyle(commands))
    return t


def footer(canvas, doc):
    canvas.saveState()
    canvas.setStrokeColor(colors.HexColor("#D8DEE3"))
    canvas.line(15*mm, 11*mm, PAGE[0]-15*mm, 11*mm)
    canvas.setFont("MSJH", 7.5)
    canvas.setFillColor(GRAY)
    canvas.drawString(15*mm, 6.5*mm, "SP-AX3 影像至 PLC 路徑資料流程 | 2026-08-25")
    canvas.drawRightString(PAGE[0]-15*mm, 6.5*mm, f"第 {doc.page} 頁")
    canvas.restoreState()


doc = SimpleDocTemplate(str(OUT), pagesize=PAGE, leftMargin=16*mm, rightMargin=16*mm,
                        topMargin=14*mm, bottomMargin=16*mm,
                        title="SP-AX3 影像至 PLC 路徑資料流程")
story = []

story += [
    Spacer(1, 14*mm),
    p("SP-AX3 從影像邊緣到 PLC 路徑輸出的完整資料流程", title),
    p("資料型、座標原點、單位、左右軸向、CSV 與 Modbus TCP signed INT16 對應", h1),
    Spacer(1, 5*mm),
    FlowDiagram([
        ("1. 相機影像與 ROI", "cv::Mat / CV_8UC1；原點在影像左上；單位 pixel", (BLUE, LIGHT_BLUE)),
        ("2. 邊緣與原始工具路徑", "二值化、Mask、輪廓、曲率簡化、Offset；ToolPath", (GREEN, LIGHT_GREEN)),
        ("3. 左右路徑最佳化", "GluePath：PathLeft / PathRight；最多 25 組；共同 Y", (ORANGE, LIGHT_ORANGE)),
        ("4. 機械與 HMI 座標", "RefCenter 為原點；pixel → mm → mm×10 → rounded integer", (BLUE, LIGHT_BLUE)),
        ("5. CSV 與 Modbus TCP", "Y / X1 / X2 相同；負值以 INT16 two's complement 傳輸", (GREEN, LIGHT_GREEN)),
    ], box_h=19*mm, gap=7*mm),
    PageBreak(),
]

story += [p("1. 影像、校正、ROI 與邊緣路徑", h1)]
story += [FlowDiagram([
    ("Basler Camera", "uint8_t* → grabbedImage / m_mat；cv::Mat、CV_8UC1", (BLUE, LIGHT_BLUE)),
    ("固定單一畫格", "sourceFrame = m_mat.clone()；避免相機執行緒更新造成前後不一致", (BLUE, LIGHT_BLUE)),
    ("影像校正（若有效）", "undistortImage() / warpPerspective → pathSourceImage", (GREEN, LIGHT_GREEN)),
    ("建立 ROI Mask", "MaskX/Y/Width/Height → CV_8UC1；ROI=255，其餘=0", (ORANGE, LIGHT_ORANGE)),
    ("校正 Mask 與 RefCenter", "Mask 經 warpPerspective；RefCenter 經 transformPoint()", (ORANGE, LIGHT_ORANGE)),
    ("Offset 換算", "offsetPixel = OffsetValue(mm) / TransferFactor(mm/pixel)", (GREEN, LIGHT_GREEN)),
    ("GenerateToolPathByType", "Type 0/1/2 目前皆進入 GetToolPath_CurvatureOptimized_Mask()", (BLUE, LIGHT_BLUE)),
    ("toolPath.Path", "vector<cv::Point2d>；影像左上原點；X→右、Y→下；pixel", (GREEN, LIGHT_GREEN)),
], box_h=13*mm, gap=4*mm)]
story += [PageBreak()]

story += [p("2. 左右路徑分割與最佳化", h1)]
story += [Spacer(1, 3*mm)]
story += [FlowDiagram([
    ("OptimizeGluePath()", "輸入 vector<cv::Point2d> + ROIMask；輸出 GluePath", (BLUE, LIGHT_BLUE)),
    ("FilterByMask()", "只保留 effectiveRoiRect 內的輪廓點", (ORANGE, LIGHT_ORANGE)),
    ("SplitByCenter()", "由頂點到底點切成 chainA / chainB", (GREEN, LIGHT_GREEN)),
    ("左右判定", "averageX 較大 → PathRight；較小 → PathLeft", (GREEN, LIGHT_GREEN)),
    ("依 Y 建立側面輪廓", "右側取同 Y 最右點；左側取同 Y 最左點", (ORANGE, LIGHT_ORANGE)),
    ("同步取樣", "右側最多 25 點；左側依右側相同 Y 序列擬合", (BLUE, LIGHT_BLUE)),
    ("排序、過濾、正規化", "依 Y 遞增；限 ROI 與 25 組；左右點數及 Y 對齊", (GREEN, LIGHT_GREEN)),
    ("m_OptimizedGluePath", "PathLeft / PathRight：vector<cv::Point2d>；影像 pixel 座標", (BLUE, LIGHT_BLUE)),
], box_h=11.5*mm, gap=3.2*mm)]
story += [PageBreak()]

story += [p("3. 影像座標轉換為機械、HMI 與運動軸座標", h1)]
coord_data = [
    ["階段", "資料結構 / 型別", "轉換公式", "原點、方向與單位"],
    ["最佳化影像路徑", "m_OptimizedGluePath\nGluePath / cv::Point2d", "無", "影像左上；X→右、Y→下；pixel"],
    ["機械相對座標", "m_machineGluePath\nGluePath / cv::Point2d", "x = imageX - RefCenterX\ny = imageY - RefCenterY", "RefCenter；X→右、Y→下；pixel"],
    ["機械尺寸", "m_machineGluePath_mm", "x/y × TransferFactor", "RefCenter；mm"],
    ["HMI 暫存", "m_HMIGluePath_temp", "x/y × 10", "RefCenter；mm × 10"],
    ["HMI 整數", "m_HMIGluePath", "lround(x/y)", "RefCenter；整數；保留正負號"],
    ["運動軸", "Y, X1, X2", "Y = PathLeft.y\nX1 = -PathLeft.x\nX2 = PathRight.x", "X1 向左為正；X2 向右為正"],
]
story += [table([[p(c, small) for c in row] for row in coord_data],
                [34*mm, 53*mm, 68*mm, 94*mm])]
story += [Spacer(1, 5*mm), p("重要：X1/X2 是有方向的軸座標，不是距離值。若任一路徑跨越 RefCenter，對應值必須變成負數。", note)]
story += [Spacer(1, 4*mm), p("程式公式", h2)]
formula = [["欄位", "CSV 與 PLC 共用邏輯值"],
           ["Y", "m_HMIGluePath.PathLeft[i].y"],
           ["X1", "-m_HMIGluePath.PathLeft[i].x"],
           ["X2", "m_HMIGluePath.PathRight[i].x"]]
story += [table([[p(c, small) for c in row] for row in formula], [35*mm, 120*mm])]
story += [PageBreak()]

story += [p("4. WORK_GO、CSV 與 Modbus TCP 輸出", h1)]
story += [FlowDiagram([
    ("IDC_IDC_WORK_GO", "OnBnClickedIdcWorkGo()；先確認左右最佳化路徑有效", (BLUE, LIGHT_BLUE)),
    ("重建最新 HMI 座標", "ConvertToMachineCoordinates(effectiveReference)", (GREEN, LIGHT_GREEN)),
    ("PathDataOut.csv", "PathDataOut=1 時輸出 Y,X1,X2；不依賴 Modbus 是否在線", (ORANGE, LIGHT_ORANGE)),
    ("準備 30 筆暫存陣列", "vector<uint16_t> x1Regs / yRegs / x2Regs；最多 25 點", (BLUE, LIGHT_BLUE)),
    ("補足 HMI Buffer", "第 26～30 筆重複最後一個有效描述點", (BLUE, LIGHT_BLUE)),
    ("Signed INT16 編碼", "限制 -32768..32767；int16_t bit pattern 轉 uint16_t", (GREEN, LIGHT_GREEN)),
    ("Modbus TCP 寫入", "D14..D43=X1；D44..D73=Y；D74..D103=X2", (ORANGE, LIGHT_ORANGE)),
    ("PLC 解讀", "Holding Register 必須以 INT16 解讀；例如 -25 ↔ 0xFFE7", (GREEN, LIGHT_GREEN)),
], box_h=14*mm, gap=4.5*mm)]
story += [Spacer(1, 3*mm), p("CSV 是 PLC 運動座標的除錯鏡像：兩者使用相同 Y/X1/X2 公式。差異只在 Modbus 以 uint16_t 容器承載 INT16 的 two's complement 位元。", note)]
story += [PageBreak()]

story += [p("5. 主要程式位置與檢查重點", h1)]
refs = [
    ["功能", "檔案 / 函式", "檢查重點"],
    ["影像與 Mask", "WorkTab.cpp / OnBnClickedIdcWorkToolPath", "固定 sourceFrame；ROI 尺寸不可超出影像"],
    ["演算法路由", "GenerateToolPathByType", "Type 0/1/2 目前均沿用 Legacy 演算法"],
    ["邊緣路徑", "GetToolPath_CurvatureOptimized_Mask", "BinaryLower/Upper、Mask、offsetPixel"],
    ["左右分割", "GluePathOptimizer::SplitByCenter", "平均 X 大為右側，平均 X 小為左側"],
    ["最佳化", "GluePathOptimizer::OptimizePath", "最多 25 點、左右共同 Y"],
    ["座標轉換", "WorkTab::ConvertToMachineCoordinates", "RefCenter、TransferFactor、×10、lround"],
    ["CSV", "ExportPathData", "Y、X1=-Left.x、X2=Right.x；保留負值"],
    ["PLC", "OnBnClickedIdcWorkGo", "INT16 two's complement；PLC 必須用 signed word"],
]
story += [table([[p(c, small) for c in row] for row in refs], [42*mm, 88*mm, 115*mm])]
story += [Spacer(1, 6*mm), p("PLC 除錯核對方式", h2)]
checks = [
    "逐列比較 PathDataOut.csv 的 Y/X1/X2 與 PLC D44/D14/D74 對應索引。",
    "PLC 監看表須設定為 INT16；若以 UINT16 顯示，負數會呈現 65536-|value|。",
    "OffsetValue 改變後重新生成路徑並再次觸發 WORK_GO；CSV 每次覆寫最新一批資料。",
    "第 26～30 筆為填充資料，應重複第 25 個有效點，不代表新增路徑描述點。",
]
story += [p("<br/>".join(f"• {x}" for x in checks), body)]

doc.build(story, onFirstPage=footer, onLaterPages=footer)
print(OUT)
