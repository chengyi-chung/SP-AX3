from pathlib import Path
import re

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
from xml.sax.saxutils import escape

ROOT = Path(r"D:\Git Repository\chengyi-chung\SP-AX3")
SOURCE = ROOT / "DOC" / "ToolPath_Flow_Design_Review.md"
OUTPUT = ROOT / "output" / "pdf" / "ToolPath_Flow_Design_Review.pdf"
FONT = Path(r"C:\Windows\Fonts\NotoSansTC-VF.ttf")

OUTPUT.parent.mkdir(parents=True, exist_ok=True)
pdfmetrics.registerFont(TTFont("NotoTC", str(FONT)))

PAGE_W, PAGE_H = A4


class ReviewDoc(BaseDocTemplate):
    def __init__(self, filename):
        super().__init__(
            filename,
            pagesize=A4,
            leftMargin=18 * mm,
            rightMargin=18 * mm,
            topMargin=20 * mm,
            bottomMargin=17 * mm,
            title="SP-AX3 ToolPath 流程設計審查",
            author="SP-AX3 Engineering",
        )
        frame = Frame(
            self.leftMargin, self.bottomMargin,
            self.width, self.height,
            id="content",
            leftPadding=0, rightPadding=0, topPadding=0, bottomPadding=0,
        )
        self.addPageTemplates(PageTemplate(id="main", frames=[frame], onPage=self._page))

    def _page(self, canvas, doc):
        canvas.saveState()
        canvas.setFont("NotoTC", 7.5)
        canvas.setFillColor(colors.HexColor("#64748B"))
        canvas.drawString(18 * mm, PAGE_H - 11 * mm, "SP-AX3 / ToolPath Design Review")
        canvas.drawRightString(PAGE_W - 18 * mm, 9 * mm, f"Page {doc.page}")
        canvas.setStrokeColor(colors.HexColor("#CBD5E1"))
        canvas.setLineWidth(0.4)
        canvas.line(18 * mm, PAGE_H - 13 * mm, PAGE_W - 18 * mm, PAGE_H - 13 * mm)
        canvas.restoreState()


styles = getSampleStyleSheet()
base = dict(fontName="NotoTC", textColor=colors.HexColor("#1E293B"))
TITLE = ParagraphStyle("TitleTC", parent=styles["Title"], fontSize=23, leading=30,
                       alignment=TA_CENTER, spaceAfter=8 * mm, **base)
SUBTITLE = ParagraphStyle("Subtitle", fontName="NotoTC", fontSize=10, leading=15,
                          alignment=TA_CENTER, textColor=colors.HexColor("#64748B"), spaceAfter=12 * mm)
H1 = ParagraphStyle("H1", fontName="NotoTC", fontSize=15, leading=21,
                    textColor=colors.HexColor("#0F3D5E"), spaceBefore=7 * mm,
                    spaceAfter=3 * mm, keepWithNext=True)
H2 = ParagraphStyle("H2", fontName="NotoTC", fontSize=12, leading=18,
                    textColor=colors.HexColor("#155E75"), spaceBefore=5 * mm,
                    spaceAfter=2 * mm, keepWithNext=True)
BODY = ParagraphStyle("BodyTC", fontName="NotoTC", fontSize=9.2, leading=15,
                      textColor=colors.HexColor("#263238"), spaceAfter=2.2 * mm)
BULLET = ParagraphStyle("BulletTC", parent=BODY, leftIndent=5 * mm,
                        firstLineIndent=-3.2 * mm, bulletIndent=1.5 * mm, spaceAfter=1.2 * mm)
CODE = ParagraphStyle("CodeTC", fontName="NotoTC", fontSize=7.6, leading=11,
                      leftIndent=4 * mm, rightIndent=4 * mm, borderPadding=5,
                      backColor=colors.HexColor("#F1F5F9"), borderColor=colors.HexColor("#CBD5E1"),
                      borderWidth=0.5, borderRadius=2, spaceBefore=2 * mm, spaceAfter=3 * mm)
CALLOUT = ParagraphStyle("Callout", parent=BODY, leftIndent=4 * mm, rightIndent=4 * mm,
                         borderPadding=6, backColor=colors.HexColor("#FFF7ED"),
                         borderColor=colors.HexColor("#FB923C"), borderWidth=0.8,
                         spaceBefore=2 * mm, spaceAfter=3 * mm)


def inline(text):
    text = escape(text)
    text = re.sub(r"`([^`]+)`", r'<font color="#7C2D12">\1</font>', text)
    text = re.sub(r"\*\*([^*]+)\*\*", r"<b>\1</b>", text)
    return text


def parse_table(lines):
    rows = []
    for line in lines:
        cells = [c.strip() for c in line.strip().strip("|").split("|")]
        if all(re.fullmatch(r":?-{3,}:?", c.replace(" ", "")) for c in cells):
            continue
        rows.append([Paragraph(inline(c), BODY) for c in cells])
    if not rows:
        return Spacer(1, 1)
    widths = [42 * mm, 48 * mm, 25 * mm, 56 * mm][:len(rows[0])]
    if len(rows[0]) != 4:
        widths = [174 * mm / len(rows[0])] * len(rows[0])
    table = Table(rows, colWidths=widths, repeatRows=1, hAlign="LEFT")
    table.setStyle(TableStyle([
        ("FONTNAME", (0, 0), (-1, -1), "NotoTC"),
        ("BACKGROUND", (0, 0), (-1, 0), colors.HexColor("#DCEAF2")),
        ("TEXTCOLOR", (0, 0), (-1, 0), colors.HexColor("#0F3D5E")),
        ("GRID", (0, 0), (-1, -1), 0.45, colors.HexColor("#94A3B8")),
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("LEFTPADDING", (0, 0), (-1, -1), 5),
        ("RIGHTPADDING", (0, 0), (-1, -1), 5),
        ("TOPPADDING", (0, 0), (-1, -1), 4),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 4),
        ("ROWBACKGROUNDS", (0, 1), (-1, -1), [colors.white, colors.HexColor("#F8FAFC")]),
    ]))
    return table


def build_story(markdown):
    lines = markdown.splitlines()
    story = [Spacer(1, 12 * mm), Paragraph("SP-AX3 ToolPath", TITLE),
             Paragraph("流程設計審查", TITLE),
             Paragraph("供機構、視覺、軟體與電控設計工程師共同檢閱", SUBTITLE),
             Table([[Paragraph("文件目的", H2), Paragraph("說明現行 ToolPath 端到端流程、座標契約、Modbus 輸出與設計風險。", BODY)],
                    [Paragraph("審查基準", H2), Paragraph("目前工作區程式碼與 SystemConfig.ini 設計。", BODY)]],
                   colWidths=[35 * mm, 125 * mm], style=TableStyle([
                       ("BOX", (0, 0), (-1, -1), 0.7, colors.HexColor("#94A3B8")),
                       ("INNERGRID", (0, 0), (-1, -1), 0.4, colors.HexColor("#CBD5E1")),
                       ("BACKGROUND", (0, 0), (0, -1), colors.HexColor("#E0F2FE")),
                       ("VALIGN", (0, 0), (-1, -1), "TOP"),
                       ("LEFTPADDING", (0, 0), (-1, -1), 7),
                       ("RIGHTPADDING", (0, 0), (-1, -1), 7),
                       ("TOPPADDING", (0, 0), (-1, -1), 7),
                       ("BOTTOMPADDING", (0, 0), (-1, -1), 7),
                   ])), PageBreak()]

    i = 1  # skip markdown title
    paragraph = []
    in_code = False
    code_lines = []

    def flush_paragraph():
        nonlocal paragraph
        if paragraph:
            text = " ".join(x.strip() for x in paragraph).strip()
            if text:
                style = CALLOUT if text.startswith("目前可正常編譯") else BODY
                story.append(Paragraph(inline(text), style))
            paragraph = []

    while i < len(lines):
        line = lines[i]
        if line.startswith("```"):
            flush_paragraph()
            if in_code:
                story.append(Paragraph("<br/>".join(escape(x).replace(" ", "&nbsp;") for x in code_lines), CODE))
                code_lines = []
                in_code = False
            else:
                in_code = True
            i += 1
            continue
        if in_code:
            code_lines.append(line)
            i += 1
            continue
        if line.startswith("| ") and i + 1 < len(lines) and lines[i + 1].startswith("|"):
            flush_paragraph()
            table_lines = []
            while i < len(lines) and lines[i].startswith("|"):
                table_lines.append(lines[i])
                i += 1
            story.append(parse_table(table_lines))
            story.append(Spacer(1, 3 * mm))
            continue
        if line.startswith("## "):
            flush_paragraph()
            story.append(Paragraph(inline(line[3:]), H1))
        elif line.startswith("### "):
            flush_paragraph()
            story.append(Paragraph(inline(line[4:]), H2))
        elif re.match(r"^[-*] ", line):
            flush_paragraph()
            story.append(Paragraph(inline(line[2:]), BULLET, bulletText="•"))
        elif re.match(r"^\d+\. ", line):
            flush_paragraph()
            m = re.match(r"^(\d+)\. (.*)$", line)
            story.append(Paragraph(inline(m.group(2)), BULLET, bulletText=m.group(1) + "."))
        elif not line.strip():
            flush_paragraph()
        else:
            paragraph.append(line)
        i += 1
    flush_paragraph()
    return story


doc = ReviewDoc(str(OUTPUT))
doc.build(build_story(SOURCE.read_text(encoding="utf-8")))
print(OUTPUT)
