from pathlib import Path
from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER, TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.units import mm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak,
    KeepTogether, Flowable
)

ROOT = Path(r"D:\Git Repository\chengyi-chung\SP-AX3")
OUT = ROOT / "output" / "pdf" / "SP-AX3_Coordinate_Conversion_Flow.pdf"
OUT.parent.mkdir(parents=True, exist_ok=True)

font_path = Path(r"C:\Windows\Fonts\msjh.ttc")
font_bold_path = Path(r"C:\Windows\Fonts\msjhbd.ttc")
pdfmetrics.registerFont(TTFont("CJK", str(font_path), subfontIndex=0))
pdfmetrics.registerFont(TTFont("CJKB", str(font_bold_path), subfontIndex=0))

NAVY = colors.HexColor("#153A5B")
BLUE = colors.HexColor("#246B9E")
CYAN = colors.HexColor("#DDF2F8")
GREEN = colors.HexColor("#DFF2E5")
ORANGE = colors.HexColor("#FCE8D2")
GRAY = colors.HexColor("#F2F4F6")
DARK = colors.HexColor("#24313A")

styles = getSampleStyleSheet()
styles.add(ParagraphStyle(name="CJKTitle", fontName="CJKB", fontSize=22, leading=29,
                          textColor=NAVY, alignment=TA_CENTER, spaceAfter=10))
styles.add(ParagraphStyle(name="CJKSub", fontName="CJK", fontSize=10, leading=16,
                          textColor=colors.HexColor("#52616B"), alignment=TA_CENTER))
styles.add(ParagraphStyle(name="H1C", fontName="CJKB", fontSize=15, leading=21,
                          textColor=NAVY, spaceBefore=4, spaceAfter=8))
styles.add(ParagraphStyle(name="H2C", fontName="CJKB", fontSize=11.5, leading=17,
                          textColor=BLUE, spaceBefore=6, spaceAfter=5))
styles.add(ParagraphStyle(name="BodyC", fontName="CJK", fontSize=9.5, leading=16,
                          textColor=DARK, spaceAfter=5))
styles.add(ParagraphStyle(name="SmallC", fontName="CJK", fontSize=8, leading=12,
                          textColor=DARK))
styles.add(ParagraphStyle(name="CodeC", fontName="CJK", fontSize=8.5, leading=14,
                          backColor=GRAY, borderPadding=7, textColor=colors.HexColor("#172B3A"),
                          spaceBefore=4, spaceAfter=7))
styles.add(ParagraphStyle(name="NoteC", fontName="CJK", fontSize=8.5, leading=14,
                          backColor=CYAN, borderPadding=8, textColor=NAVY, spaceBefore=5, spaceAfter=8))


class FlowDiagram(Flowable):
    def __init__(self, width=174*mm, height=185*mm):
        super().__init__()
        self.width, self.height = width, height

    def draw_box(self, c, x, y, w, h, title, detail, fill):
        c.setFillColor(fill)
        c.setStrokeColor(BLUE)
        c.setLineWidth(0.8)
        c.roundRect(x, y, w, h, 5, fill=1, stroke=1)
        c.setFillColor(NAVY)
        c.setFont("CJKB", 9)
        c.drawCentredString(x+w/2, y+h-13, title)
        c.setFillColor(DARK)
        c.setFont("CJK", 7.2)
        lines = detail.split("\n")
        yy = y+h-26
        for line in lines:
            c.drawCentredString(x+w/2, yy, line)
            yy -= 10

    def arrow(self, c, x1, y1, x2, y2):
        c.setStrokeColor(BLUE)
        c.setFillColor(BLUE)
        c.setLineWidth(1.2)
        c.line(x1, y1, x2, y2)
        import math
        a = math.atan2(y2-y1, x2-x1)
        s = 5
        pts = [(x2, y2),
               (x2-s*math.cos(a-0.5), y2-s*math.sin(a-0.5)),
               (x2-s*math.cos(a+0.5), y2-s*math.sin(a+0.5))]
        p = c.beginPath(); p.moveTo(*pts[0]); p.lineTo(*pts[1]); p.lineTo(*pts[2]); p.close()
        c.drawPath(p, fill=1, stroke=0)

    def draw(self):
        c = self.canv
        W, H = self.width, self.height
        bw, bh = 74*mm, 21*mm
        cx = (W-bw)/2
        ys = [H-24*mm, H-52*mm, H-80*mm, H-108*mm, H-136*mm, H-164*mm]
        boxes = [
            ("影像與路徑產生", "校正／方向修正／Mask／ROI\nOffsetValue ÷ TransferFactor → pixel", CYAN),
            ("m_OptimizedGluePath", "影像左上角為原點\n單位：pixel；最多 25 組描述點", GREEN),
            ("座標原點平移", "Xc = X - RefCenterX\nYc = Y - RefCenterY", ORANGE),
            ("選擇性旋轉", "IsMachineRotate=1：套用角度 θ\n=0：θ 視為 0°", ORANGE),
            ("單位與格式轉換", "× TransferFactor → mm\n× 10、四捨五入 → 0.1 mm 整數", ORANGE),
            ("HMI / Modbus TCP", "X1：D14 起；Y：D44 起；X2：D74 起\nINT16；固定傳送 30 筆", GREEN),
        ]
        for i, (t,d,f) in enumerate(boxes):
            self.draw_box(c, cx, ys[i], bw, bh, t, d, f)
            if i < len(boxes)-1:
                self.arrow(c, W/2, ys[i], W/2, ys[i+1]+bh)

        # Picture Control side branch from optimized path.
        sx, sy = cx+bw, ys[1]+bh/2
        bx, by, sw, sh = W-48*mm, ys[1]-2*mm, 46*mm, 26*mm
        self.arrow(c, sx, sy, bx, sy)
        self.draw_box(c, bx, by, sw, sh, "Picture Control", "保持原始影像座標\n黃：raw；紅：Left；綠：Right", GRAY)
        c.setFont("CJK", 7.5); c.setFillColor(BLUE)
        c.drawString(cx+bw+3*mm, sy+3*mm, "顯示分支")


def header_footer(canvas, doc):
    canvas.saveState()
    canvas.setStrokeColor(colors.HexColor("#CCD6DD")); canvas.line(18*mm, 16*mm, 192*mm, 16*mm)
    canvas.setFont("CJK", 7.5); canvas.setFillColor(colors.HexColor("#667680"))
    canvas.drawString(18*mm, 10*mm, "SP-AX3 座標轉換與路徑資料流程")
    canvas.drawRightString(192*mm, 10*mm, f"第 {doc.page} 頁")
    canvas.restoreState()


doc = SimpleDocTemplate(str(OUT), pagesize=A4, rightMargin=18*mm, leftMargin=18*mm,
                        topMargin=18*mm, bottomMargin=22*mm,
                        title="SP-AX3 座標轉換、單位轉換與路徑資料流程")
story = []
story += [Spacer(1, 18*mm), Paragraph("SP-AX3 座標轉換、單位轉換<br/>與路徑資料產生顯示流程", styles["CJKTitle"]),
          Paragraph("Picture Control 保持原始影像座標；HMI 接收完成平移、旋轉與單位換算後的最終路徑", styles["CJKSub"]),
          Spacer(1, 18*mm)]

summary_data = [
    ["顯示端", "Picture Control 顯示 m_OptimizedGluePath，單位 pixel，不套用機械座標轉換。"],
    ["傳送端", "WORK_GO 前重新建立 m_HMIGluePath，再透過 Modbus TCP 傳送。"],
    ["轉換順序", "RefCenter 平移 → 選擇性旋轉 → pixel 轉 mm → 0.1 mm 整數化。"],
]
t = Table(summary_data, colWidths=[28*mm, 136*mm])
t.setStyle(TableStyle([("FONTNAME",(0,0),(-1,-1),"CJK"),("FONTSIZE",(0,0),(-1,-1),9),
                       ("LEADING",(0,0),(-1,-1),15),("BACKGROUND",(0,0),(0,-1),NAVY),
                       ("TEXTCOLOR",(0,0),(0,-1),colors.white),("FONTNAME",(0,0),(0,-1),"CJKB"),
                       ("VALIGN",(0,0),(-1,-1),"MIDDLE"),("GRID",(0,0),(-1,-1),0.5,colors.HexColor("#C9D4DB")),
                       ("ROWBACKGROUNDS",(1,0),(-1,-1),[colors.white,GRAY]),("LEFTPADDING",(0,0),(-1,-1),7),
                       ("RIGHTPADDING",(0,0),(-1,-1),7),("TOPPADDING",(0,0),(-1,-1),8),("BOTTOMPADDING",(0,0),(-1,-1),8)]))
story += [t, Spacer(1, 14*mm), Paragraph("核心結論", styles["H1C"]),
          Paragraph("畫面上的點用來核對影像與路徑相對位置；真正送往 HMI 的資料來自 m_HMIGluePath，已完成機械座標與單位處理。", styles["NoteC"]),
          PageBreak(), Paragraph("一、完整流程圖", styles["H1C"]), FlowDiagram(), PageBreak()]

story += [Paragraph("二、參數與單位", styles["H1C"])]
params = [
    ["參數", "單位／值", "用途"],
    ["OffsetValue", "mm", "工具路徑向內縮距離，不是機械座標平移量。"],
    ["TransferFactor", "mm/pixel", "相機 pixel 與實際 mm 的比例。"],
    ["RefCenterX / RefCenterY", "pixel", "影像中的機械原點，也是旋轉中心。"],
    ["CameraToMachineAngle", "degree", "相機座標軸相對機械座標軸的角度。"],
    ["IsMachineRotate", "0 或 1", "1 套用角度旋轉；0 不旋轉。"],
]
t = Table(params, colWidths=[43*mm, 31*mm, 90*mm], repeatRows=1)
t.setStyle(TableStyle([("FONTNAME",(0,0),(-1,-1),"CJK"),("FONTNAME",(0,0),(-1,0),"CJKB"),
                       ("FONTSIZE",(0,0),(-1,-1),8.5),("LEADING",(0,0),(-1,-1),13),
                       ("BACKGROUND",(0,0),(-1,0),NAVY),("TEXTCOLOR",(0,0),(-1,0),colors.white),
                       ("ROWBACKGROUNDS",(0,1),(-1,-1),[colors.white,GRAY]),
                       ("GRID",(0,0),(-1,-1),0.5,colors.HexColor("#BCCAD2")),("VALIGN",(0,0),(-1,-1),"TOP"),
                       ("LEFTPADDING",(0,0),(-1,-1),6),("RIGHTPADDING",(0,0),(-1,-1),6),
                       ("TOPPADDING",(0,0),(-1,-1),7),("BOTTOMPADDING",(0,0),(-1,-1),7)]))
story += [t, Spacer(1, 7*mm), Paragraph("1. 路徑內縮", styles["H2C"]),
          Paragraph("OffsetPixel = OffsetValue(mm) ÷ TransferFactor(mm/pixel)", styles["CodeC"]),
          Paragraph("路徑演算法在影像座標中使用 OffsetPixel 進行內縮，完成後整理左右路徑、依 Y 排序、限制 ROI，最多保留 25 組描述點，形成 m_OptimizedGluePath。", styles["BodyC"]),
          Paragraph("2. 機械座標平移與旋轉", styles["H2C"]),
          Paragraph("Xc = P.x - RefCenterX<br/>Yc = P.y - RefCenterY<br/><br/>IsMachineRotate=1：<br/>Xm = Xc × cosθ - Yc × sinθ<br/>Ym = Xc × sinθ + Yc × cosθ<br/><br/>IsMachineRotate=0：Xm = Xc，Ym = Yc", styles["CodeC"]),
          Paragraph("輸出採機械原點座標，不再加回 RefCenter。因此 P(RefCenterX, RefCenterY) 會轉成 PM(0,0)。", styles["NoteC"]),
          Paragraph("3. Pixel 轉 mm，再轉 HMI 整數", styles["H2C"]),
          Paragraph("Machine_mm = Machine_pixel × TransferFactor<br/>HMI = round(Machine_mm × 10)", styles["CodeC"]),
          Paragraph("HMI 整數的 1 代表 0.1 mm。例如 23.4856 mm 會轉為 round(234.856) = 235。", styles["BodyC"]),
          PageBreak()]

story += [Paragraph("三、顯示資料與傳送資料", styles["H1C"])]
display = [
    ["階段", "資料物件", "原點與單位", "用途"],
    ["原始／優化路徑", "toolPath、m_OptimizedGluePath", "影像左上角；pixel", "Picture Control 顯示"],
    ["平移／旋轉後", "m_machineGluePath", "RefCenter；pixel", "機械座標中間值"],
    ["實際尺寸", "m_machineGluePath_mm", "RefCenter；mm", "實際機械距離"],
    ["HMI 暫存", "m_HMIGluePath_temp", "RefCenter；mm × 10", "整數化前資料"],
    ["HMI 最終", "m_HMIGluePath", "RefCenter；0.1 mm 整數", "Modbus 傳送資料"],
]
t = Table(display, colWidths=[29*mm, 50*mm, 43*mm, 42*mm], repeatRows=1)
t.setStyle(TableStyle([("FONTNAME",(0,0),(-1,-1),"CJK"),("FONTNAME",(0,0),(-1,0),"CJKB"),
                       ("FONTSIZE",(0,0),(-1,-1),7.8),("LEADING",(0,0),(-1,-1),12),
                       ("BACKGROUND",(0,0),(-1,0),NAVY),("TEXTCOLOR",(0,0),(-1,0),colors.white),
                       ("ROWBACKGROUNDS",(0,1),(-1,-1),[colors.white,GRAY]),
                       ("GRID",(0,0),(-1,-1),0.5,colors.HexColor("#BCCAD2")),("VALIGN",(0,0),(-1,-1),"TOP"),
                       ("LEFTPADDING",(0,0),(-1,-1),5),("RIGHTPADDING",(0,0),(-1,-1),5),
                       ("TOPPADDING",(0,0),(-1,-1),7),("BOTTOMPADDING",(0,0),(-1,-1),7)]))
story += [t, Spacer(1, 7*mm), Paragraph("Picture Control", styles["H2C"]),
          Paragraph("Picture Control 保持原始影像座標：黃色為 raw toolPath、紅色為 PathLeft、綠色為 PathRight。它不套用 RefCenter 平移、角度旋轉或 TransferFactor，避免畫面與相機影像失去對應。", styles["BodyC"]),
          Paragraph("WORK_GO 傳送前", styles["H2C"]),
          Paragraph("每次按下 WORK_GO，都從最新的 m_OptimizedGluePath 重新執行 ConvertToMachineCoordinates()。因此 RefCenter、IsMachineRotate、CameraToMachineAngle 與 TransferFactor 的最新值會在送出前生效。", styles["BodyC"]),
          Paragraph("X1、X2 與共用 Y 軸", styles["H2C"]),
          Paragraph("左右路徑轉換後，PathLeft.Y 會同步為 PathRight.Y，形成共用 Y。最終軸向為：", styles["BodyC"]),
          Paragraph("X1 = -m_HMIGluePath.PathLeft.X<br/>Y = m_HMIGluePath.PathRight.Y（與 PathLeft.Y 相同）<br/>X2 = m_HMIGluePath.PathRight.X", styles["CodeC"]),
          Paragraph("Modbus TCP 傳送", styles["H2C"])]
modbus = [["軸", "起始 Address", "資料筆數", "格式"], ["X1", "14", "30", "INT16 / WORD bit pattern"],
          ["Y", "44", "30", "INT16 / WORD bit pattern"], ["X2", "74", "30", "INT16 / WORD bit pattern"]]
t = Table(modbus, colWidths=[30*mm, 42*mm, 38*mm, 54*mm])
t.setStyle(TableStyle([("FONTNAME",(0,0),(-1,-1),"CJK"),("FONTNAME",(0,0),(-1,0),"CJKB"),
                       ("FONTSIZE",(0,0),(-1,-1),8.5),("BACKGROUND",(0,0),(-1,0),NAVY),
                       ("TEXTCOLOR",(0,0),(-1,0),colors.white),("ALIGN",(0,0),(-1,-1),"CENTER"),
                       ("ROWBACKGROUNDS",(0,1),(-1,-1),[colors.white,GRAY]),
                       ("GRID",(0,0),(-1,-1),0.5,colors.HexColor("#BCCAD2")),
                       ("TOPPADDING",(0,0),(-1,-1),7),("BOTTOMPADDING",(0,0),(-1,-1),7)]))
story += [t, Spacer(1, 5*mm), Paragraph("實際路徑最多使用 25 個描述點；HMI 固定接收 30 筆，第 26 至 30 筆重複最後一個有效點。負值以 two's complement 寫入 WORD，數值限制於 -32768 至 32767。", styles["NoteC"]),
          Spacer(1, 5*mm), Paragraph("四、最終確認", styles["H1C"]),
          Paragraph("Picture Control 顯示原始影像 pixel 座標；傳送至 HMI 的資料則已完成 RefCenter 原點平移、IsMachineRotate 控制的角度旋轉、TransferFactor 的 pixel-to-mm 換算、0.1 mm 整數化，以及 X1/Y/X2 軸向整理。", styles["NoteC"])]

doc.build(story, onFirstPage=header_footer, onLaterPages=header_footer)
print(OUT)
