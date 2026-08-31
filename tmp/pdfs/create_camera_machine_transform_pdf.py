from pathlib import Path

from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER, TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import mm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (
    BaseDocTemplate, Frame, PageTemplate, Paragraph, Spacer, Table, TableStyle,
    PageBreak, KeepTogether
)


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "output" / "pdf" / "相機座標轉機械座標_X1_X2共用Y軸.pdf"
OUT.parent.mkdir(parents=True, exist_ok=True)

font_candidates = [
    Path(r"C:\Windows\Fonts\msjh.ttc"),
    Path(r"C:\Windows\Fonts\msjh.ttf"),
    Path(r"C:\Windows\Fonts\mingliu.ttc"),
]
font_path = next((p for p in font_candidates if p.exists()), None)
if font_path is None:
    raise FileNotFoundError("找不到可用的中文字型")

pdfmetrics.registerFont(TTFont("CJK", str(font_path), subfontIndex=0))
pdfmetrics.registerFont(TTFont("CJK-Bold", str(font_path), subfontIndex=0))

PAGE_W, PAGE_H = A4
NAVY = colors.HexColor("#17324D")
BLUE = colors.HexColor("#2878A5")
PALE = colors.HexColor("#EDF5F8")
INK = colors.HexColor("#25313B")
MUTED = colors.HexColor("#61717D")
RED = colors.HexColor("#B34040")


def header_footer(canvas, doc):
    canvas.saveState()
    canvas.setFillColor(NAVY)
    canvas.rect(0, PAGE_H - 15 * mm, PAGE_W, 15 * mm, stroke=0, fill=1)
    canvas.setFont("CJK", 8.5)
    canvas.setFillColor(colors.white)
    canvas.drawString(18 * mm, PAGE_H - 9.5 * mm, "SP-AX3｜相機座標至機械座標轉換")
    canvas.setStrokeColor(colors.HexColor("#D7E1E7"))
    canvas.line(18 * mm, 14 * mm, PAGE_W - 18 * mm, 14 * mm)
    canvas.setFillColor(MUTED)
    canvas.drawString(18 * mm, 9 * mm, "技術說明文件")
    canvas.drawRightString(PAGE_W - 18 * mm, 9 * mm, f"第 {doc.page} 頁")
    canvas.restoreState()


doc = BaseDocTemplate(
    str(OUT), pagesize=A4,
    leftMargin=18 * mm, rightMargin=18 * mm,
    topMargin=23 * mm, bottomMargin=20 * mm,
    title="相機座標轉機械座標 - X1、X2 獨立且共用 Y 軸",
    author="Codex",
)
frame = Frame(doc.leftMargin, doc.bottomMargin, doc.width, doc.height, id="body")
doc.addPageTemplates(PageTemplate(id="main", frames=[frame], onPage=header_footer))

styles = getSampleStyleSheet()
title = ParagraphStyle("TitleCJK", fontName="CJK-Bold", fontSize=22, leading=30,
                       textColor=NAVY, alignment=TA_LEFT, spaceAfter=8 * mm)
subtitle = ParagraphStyle("Subtitle", fontName="CJK", fontSize=11, leading=18,
                          textColor=MUTED, spaceAfter=8 * mm)
h1 = ParagraphStyle("H1", fontName="CJK-Bold", fontSize=15, leading=21,
                    textColor=NAVY, spaceBefore=5 * mm, spaceAfter=3 * mm)
h2 = ParagraphStyle("H2", fontName="CJK-Bold", fontSize=11.5, leading=17,
                    textColor=BLUE, spaceBefore=3 * mm, spaceAfter=2 * mm)
body = ParagraphStyle("Body", fontName="CJK", fontSize=10.2, leading=17,
                      textColor=INK, spaceAfter=2.5 * mm)
formula = ParagraphStyle("Formula", fontName="CJK", fontSize=11.5, leading=20,
                         textColor=NAVY, alignment=TA_CENTER, leftIndent=6 * mm,
                         rightIndent=6 * mm, spaceBefore=2 * mm, spaceAfter=2 * mm,
                         borderColor=colors.HexColor("#C9DDE7"), borderWidth=0.7,
                         borderPadding=7, backColor=PALE)
note = ParagraphStyle("Note", fontName="CJK", fontSize=9.7, leading=16,
                      textColor=INK, leftIndent=4 * mm, rightIndent=4 * mm,
                      borderColor=BLUE, borderWidth=0, borderLeftWidth=3,
                      borderPadding=6, backColor=colors.HexColor("#F6FAFC"))
small = ParagraphStyle("Small", fontName="CJK", fontSize=8.8, leading=14, textColor=MUTED)

story = []
story += [
    Spacer(1, 8 * mm),
    Paragraph("相機座標轉機械座標", title),
    Paragraph("X1、X2 各自獨立運動，並共用同一條 Y 軸", subtitle),
    Paragraph("目的", h1),
    Paragraph(
        "建立相機座標 P(X1, X2, Y) 與機械座標 PM(X1, X2, Y) 的關係，並說明當相機座標系與機械座標系存在旋轉角 θ 時，共用 Y 軸所造成的幾何限制。",
        body,
    ),
    Paragraph("機構與符號定義", h1),
]

data = [
    [Paragraph("符號", h2), Paragraph("定義", h2)],
    [Paragraph("P = (X1, X2, Y)", body), Paragraph("同一相機座標系中的控制／量測座標", body)],
    [Paragraph("PM = (XM1, XM2, YM)", body), Paragraph("機械控制座標；XM1、XM2 獨立，YM 為共用 Y 軸", body)],
    [Paragraph("ΔX、ΔY", body), Paragraph("相機原點在機械座標中的平移量", body)],
    [Paragraph("θ", body), Paragraph("相機座標軸相對機械座標軸的逆時針旋轉角", body)],
]
t = Table(data, colWidths=[52 * mm, 118 * mm], repeatRows=1)
t.setStyle(TableStyle([
    ("BACKGROUND", (0, 0), (-1, 0), colors.HexColor("#DCEBF2")),
    ("GRID", (0, 0), (-1, -1), 0.5, colors.HexColor("#C3D2DA")),
    ("VALIGN", (0, 0), (-1, -1), "MIDDLE"),
    ("LEFTPADDING", (0, 0), (-1, -1), 7),
    ("RIGHTPADDING", (0, 0), (-1, -1), 7),
    ("TOPPADDING", (0, 0), (-1, -1), 6),
    ("BOTTOMPADDING", (0, 0), (-1, -1), 6),
]))
story += [t, Spacer(1, 4 * mm)]

story += [
    Paragraph("基本二維剛體轉換", h1),
    Paragraph("對任一相機點 (X, Y)，其機械座標為：", body),
    Paragraph("XM = X cosθ - Y sinθ + ΔX<br/>YM = X sinθ + Y cosθ + ΔY", formula),
    Paragraph("角度若以度數輸入，計算三角函數前必須先轉為弧度：θrad = θdeg × π / 180。", note),
    PageBreak(),
    Paragraph("套用至 X1、X2 與共用 Y 軸", h1),
    Paragraph("同一組機構座標可視為兩個位於相同 Y 指令位置的工作點：", body),
    Paragraph("P1 = (X1, Y)　　P2 = (X2, Y)", formula),
    Paragraph("兩點分別進行剛體轉換：", body),
    Paragraph("XM1 = X1 cosθ - Y sinθ + ΔX<br/>YM1 = X1 sinθ + Y cosθ + ΔY", formula),
    Paragraph("XM2 = X2 cosθ - Y sinθ + ΔX<br/>YM2 = X2 sinθ + Y cosθ + ΔY", formula),
    Paragraph("共用 Y 軸的精確條件", h1),
    Paragraph("因實際機構只有一個 YM，若 X1 與 X2 要同時精確對應，必須滿足 YM1 = YM2。", body),
    Paragraph("YM1 = YM2  =>  (X1 - X2) sinθ = 0", formula),
    Paragraph("因此，只有 θ = 0 或 X1 = X2 時，單一共用 Y 軸才可以讓兩個工作點同時完全符合旋轉後的目標位置。", note),
    Paragraph("重要結論", h1),
    Paragraph(
        "一般情況下，若 θ ≠ 0 且 X1 ≠ X2，便不存在一個能同時精確滿足兩點的 PM = (XM1, XM2, YM)。原因不是公式不足，而是旋轉後兩個工作點需要不同的 Y 位置，但機構只有一個共用 Y 自由度。",
        body,
    ),
    PageBreak(),
    Paragraph("實際控制方式", h1),
    Paragraph("方式 A：X1、X2 分開作業（精確）", h2),
    Paragraph("當一次只有一個 X 軸執行對位時，使用該作業軸計算共用 Y，可得到精確結果。", body),
    Paragraph("X1 作業：<br/>XM1 = X1 cosθ - Y sinθ + ΔX<br/>YM = X1 sinθ + Y cosθ + ΔY", formula),
    Paragraph("X2 作業：<br/>XM2 = X2 cosθ - Y sinθ + ΔX<br/>YM = X2 sinθ + Y cosθ + ΔY", formula),
    Paragraph("未作業的 X 軸可維持目前位置，或移動至預先設定的安全位置。", note),
    Paragraph("方式 B：兩軸同時作業（近似）", h2),
    Paragraph("若兩軸必須同時作業，需指定共同 Y 的參考 X 位置 XR：", body),
    Paragraph("XM1 = X1 cosθ - Y sinθ + ΔX<br/>XM2 = X2 cosθ - Y sinθ + ΔX<br/>YM = XR sinθ + Y cosθ + ΔY", formula),
    Paragraph("相對於各自精確 Y 位置的誤差為：", body),
    Paragraph("EY1 = (X1 - XR) sinθ<br/>EY2 = (X2 - XR) sinθ", formula),
    Paragraph("若選 XR = (X1 + X2) / 2，最大誤差會平均分配：", body),
    Paragraph("EY1 = (X1 - X2) sinθ / 2<br/>EY2 = -EY1", formula),
    Paragraph("此方法僅為折衷近似。允許與否應依設備精度、θ 大小、X1-X2 距離及製程容差判定。", note),
    PageBreak(),
    Paragraph("建議程式模型", h1),
    Paragraph("建議在控制程式中明確區分「單軸精確模式」與「雙軸近似模式」，避免把近似結果誤認為完整剛體轉換。", body),
]

code_rows = [
    [Paragraph("模式", h2), Paragraph("共同 Y 計算", h2), Paragraph("特性", h2)],
    [Paragraph("X1 作業", body), Paragraph("YM = X1 sinθ + Y cosθ + ΔY", body), Paragraph("X1 精確", body)],
    [Paragraph("X2 作業", body), Paragraph("YM = X2 sinθ + Y cosθ + ΔY", body), Paragraph("X2 精確", body)],
    [Paragraph("雙軸同時", body), Paragraph("YM = XR sinθ + Y cosθ + ΔY", body), Paragraph("存在可計算的 Y 誤差", body)],
]
tt = Table(code_rows, colWidths=[36 * mm, 88 * mm, 46 * mm], repeatRows=1)
tt.setStyle(TableStyle([
    ("BACKGROUND", (0, 0), (-1, 0), colors.HexColor("#DCEBF2")),
    ("GRID", (0, 0), (-1, -1), 0.5, colors.HexColor("#C3D2DA")),
    ("VALIGN", (0, 0), (-1, -1), "MIDDLE"),
    ("LEFTPADDING", (0, 0), (-1, -1), 6),
    ("RIGHTPADDING", (0, 0), (-1, -1), 6),
    ("TOPPADDING", (0, 0), (-1, -1), 6),
    ("BOTTOMPADDING", (0, 0), (-1, -1), 6),
]))
story += [tt, Spacer(1, 5 * mm)]
story += [
    Paragraph("工程判斷重點", h1),
    Paragraph("1. 確認 θ 的正負方向與座標軸方向；影像 Y 軸若向下，必須先處理軸向反轉。", body),
    Paragraph("2. 分開作業時，以目前作業的 X 軸計算共用 Y。", body),
    Paragraph("3. 同時作業時，先定義 XR，再以 EY1、EY2 驗證是否落在製程容差內。", body),
    Paragraph("4. 若要求兩點在 θ ≠ 0 時仍同時完全重合，機構必須增加獨立 Y 自由度，或透過機械／相機安裝調整使 θ 接近 0。", body),
    Spacer(1, 5 * mm),
    Paragraph("摘要", h1),
    Paragraph(
        "P(X1, X2, Y) 並不是單一二維點，而是兩個獨立 X 軸加上一個共用 Y 軸的機構狀態。相機座標旋轉後，兩個 X 位置通常會要求不同的 Y 修正。因此，分開作業可精確轉換；同時作業只能在特定條件下精確，否則必須採用明確定義且可量化誤差的近似策略。",
        note,
    ),
]

doc.build(story)
print(OUT)
