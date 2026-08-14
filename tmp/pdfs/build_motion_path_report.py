from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent / "vendor"))
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

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "output" / "pdf" / "SP-AX3_運動路徑架構分析.pdf"
OUT.parent.mkdir(parents=True, exist_ok=True)

FONT = r"C:\Windows\Fonts\kaiu.ttf"
pdfmetrics.registerFont(TTFont("Kai", FONT))

NAVY = colors.HexColor("#18324A")
BLUE = colors.HexColor("#2878A8")
PALE = colors.HexColor("#EAF4F8")
LIGHT = colors.HexColor("#F5F7F9")
ORANGE = colors.HexColor("#E58A36")
RED = colors.HexColor("#B74343")
GRAY = colors.HexColor("#5E6B75")

styles = getSampleStyleSheet()
styles.add(ParagraphStyle(name="TTitle", fontName="Kai", fontSize=24, leading=31,
                          textColor=NAVY, alignment=TA_LEFT, spaceAfter=8))
styles.add(ParagraphStyle(name="Sub", fontName="Kai", fontSize=10.5, leading=17,
                          textColor=GRAY, spaceAfter=10))
styles.add(ParagraphStyle(name="H1C", fontName="Kai", fontSize=16, leading=22,
                          textColor=NAVY, spaceBefore=10, spaceAfter=7))
styles.add(ParagraphStyle(name="H2C", fontName="Kai", fontSize=12.5, leading=18,
                          textColor=BLUE, spaceBefore=7, spaceAfter=5))
styles.add(ParagraphStyle(name="BodyC", fontName="Kai", fontSize=10, leading=16,
                          textColor=colors.HexColor("#26343E"), spaceAfter=5))
styles.add(ParagraphStyle(name="SmallC", fontName="Kai", fontSize=8.3, leading=12,
                          textColor=GRAY))
styles.add(ParagraphStyle(name="CellC", fontName="Kai", fontSize=8.7, leading=12,
                          textColor=colors.HexColor("#26343E")))
styles.add(ParagraphStyle(name="CellHead", fontName="Kai", fontSize=9, leading=12,
                          textColor=colors.white, alignment=TA_CENTER))
styles.add(ParagraphStyle(name="Risk", fontName="Kai", fontSize=9.5, leading=15,
                          textColor=colors.HexColor("#3C2B26")))

def P(text, style="BodyC"):
    return Paragraph(text, styles[style])

def bullet(text):
    return P("• " + text)

class FlowDiagram(Flowable):
    def __init__(self, width=170*mm, height=100*mm):
        super().__init__(); self.width=width; self.height=height
    def draw(self):
        c=self.canv
        boxes=[
            ("相機影像", "m_mat"), ("校正與 ROI", "去畸變／遮罩"),
            ("原始輪廓", "ToolPath, pixel"), ("左右路徑優化", "GluePath, ≤25對"),
            ("機械座標", "reference 原點／mm"), ("HMI／PLC", "×10、30筆")]
        bw=49*mm; bh=21*mm; gapx=8*mm; gapy=18*mm
        positions=[]
        for row in range(3):
            y=self.height-(row+1)*bh-row*gapy
            positions.extend([(7*mm,y),(7*mm+bw+gapx,y)])
        for i,((title,sub),(x,y)) in enumerate(zip(boxes,positions)):
            c.setFillColor(PALE if i<4 else colors.HexColor("#FFF2E6"))
            c.setStrokeColor(BLUE if i<4 else ORANGE); c.setLineWidth(1.2)
            c.roundRect(x,y,bw,bh,3*mm,fill=1,stroke=1)
            c.setFillColor(NAVY); c.setFont("Kai",11); c.drawCentredString(x+bw/2,y+12*mm,title)
            c.setFillColor(GRAY); c.setFont("Kai",8); c.drawCentredString(x+bw/2,y+6*mm,sub)
        def arrow(x1,y1,x2,y2):
            c.setStrokeColor(GRAY); c.setFillColor(GRAY); c.setLineWidth(1)
            c.line(x1,y1,x2,y2)
            import math
            a=math.atan2(y2-y1,x2-x1); s=2.2*mm
            pts=[(x2,y2),(x2-s*math.cos(a-.45),y2-s*math.sin(a-.45)),
                 (x2-s*math.cos(a+.45),y2-s*math.sin(a+.45))]
            p=c.beginPath(); p.moveTo(*pts[0]); p.lineTo(*pts[1]); p.lineTo(*pts[2]); p.close(); c.drawPath(p,fill=1)
        # snake flow
        arrow(7*mm+bw,positions[0][1]+bh/2,positions[1][0],positions[1][1]+bh/2)
        arrow(positions[1][0]+bw/2,positions[1][1],positions[2][0]+bw/2,positions[2][1]+bh)
        arrow(7*mm+bw,positions[2][1]+bh/2,positions[3][0],positions[3][1]+bh/2)
        arrow(positions[3][0]+bw/2,positions[3][1],positions[4][0]+bw/2,positions[4][1]+bh)
        arrow(7*mm+bw,positions[4][1]+bh/2,positions[5][0],positions[5][1]+bh/2)

def footer(canvas, doc):
    canvas.saveState()
    canvas.setStrokeColor(colors.HexColor("#D5DDE3")); canvas.line(18*mm,14*mm,192*mm,14*mm)
    canvas.setFont("Kai",7.5); canvas.setFillColor(GRAY)
    canvas.drawString(18*mm,9*mm,"SP-AX3 運動路徑架構分析")
    canvas.drawRightString(192*mm,9*mm,f"第 {doc.page} 頁")
    canvas.restoreState()

story=[]
story += [Spacer(1,15*mm), P("SP-AX3 運動路徑架構分析", "TTitle"),
          P("影像輪廓 → 左右成對路徑 → 機械座標 → HMI／PLC", "Sub")]
summary=Table([[P("核心結論", "CellHead")],
               [P("目前架構已清楚分成影像提取、左右同步優化、座標轉換與 Modbus 輸出四層；但影像旋轉後的 ROI mask、多輪廓串接，以及註解與實際取樣演算法不一致，是最需要優先驗證的三項風險。", "BodyC")]], colWidths=[174*mm])
summary.setStyle(TableStyle([("BACKGROUND",(0,0),(-1,0),NAVY),("BACKGROUND",(0,1),(-1,-1),LIGHT),
                             ("BOX",(0,0),(-1,-1),0.8,NAVY),("LEFTPADDING",(0,0),(-1,-1),8),
                             ("RIGHTPADDING",(0,0),(-1,-1),8),("TOPPADDING",(0,0),(-1,-1),7),
                             ("BOTTOMPADDING",(0,0),(-1,-1),7)]))
story += [summary, Spacer(1,8*mm), P("1. 整體資料流程", "H1C"), FlowDiagram(),
          P("主入口：WorkTab::OnBnClickedIdcWorkToolPath()（WorkTab.cpp:2595）", "SmallC"), PageBreak()]

story += [P("2. 階段式架構", "H1C")]
sections=[
 ("2.1 入口與參數", ["檢查來源影像、父視窗與 ROI 邊界。",
  "讀取 OffsetValue、TransferFactor、BinaryLower、BinaryUpper 與影像翻轉設定。",
  "內縮距離：offsetPixel = OffsetValue(mm) ÷ TransferFactor(mm/pixel)。",
  "程式位置：WorkTab.cpp:2595-2627。"]),
 ("2.2 影像校正與方向", ["顯示影像先還原成相機原始方向，再執行鏡頭去畸變。",
  "校正影像重新套用顯示翻轉；取路徑影像 imgClone 又額外旋轉 180°。",
  "校正失敗時退回原始 m_mat。程式位置：WorkTab.cpp:2629-2666。"]),
 ("2.3 原始 ToolPath", ["灰階化、套 ROI mask、依上下限 inRange 二值化。",
  "以 3×3 kernel 腐蝕 offsetPixel 次，再用 RETR_EXTERNAL 提取外輪廓。",
  "輪廓點依序加入 toolPath.Path；座標仍是影像 pixel。",
  "目前 enableCurvatureOptimization=false，因此 epsilonFactor=0.0008 不生效。",
  "程式位置：UAX/UAX.cpp:344-456。"]),
 ("2.4 左右膠路優化", ["ROI 再過濾後，從最上點到最下點沿輪廓正反方向形成兩條 chain。",
  "依平均 X 判定左右；相同 Y 的右側保留最大 X，左側保留最小 X。",
  "右側依 EntryPointX 決定入口，最多取樣 25 點；左側依右側 Y 線性插值。",
  "最後強制 PathLeft[i].y = PathRight[i].y。",
  "程式位置：UAX/GluePathOptimizer.cpp:322-390、555-648。"]),
 ("2.5 最終整理", ["補齊左右點數、按平均 Y 遞增排序、再次剔除 ROI 外的成對點。",
  "最多保留 25 對描述點。程式位置：WorkTab.cpp:246-361、2728-2738。"]),
]
for h,items in sections:
    story.append(P(h,"H2C")); story.extend(bullet(x) for x in items)

story += [PageBreak(), P("3. 座標系與輸出", "H1C"),
 P("座標轉換由 WorkTab::ConvertToMachineCoordinates() 執行（WorkTab.cpp:3871）。每次產生路徑及送出前都會重新建立衍生座標，避免相同筆數但內容已變更時送出舊資料。")]
coord=[
 [P("階段","CellHead"),P("資料","CellHead"),P("公式／定義","CellHead")],
 [P("影像", "CellC"),P("m_OptimizedGluePath","CellC"),P("左上角為原點，單位 pixel","CellC")],
 [P("機械 pixel","CellC"),P("m_machineGluePath","CellC"),P("X=x-referenceX；Y=y-referenceY","CellC")],
 [P("機械 mm","CellC"),P("m_machineGluePath_mm","CellC"),P("machine pixel × TransferFactor","CellC")],
 [P("HMI 暫存","CellC"),P("m_HMIGluePath_temp","CellC"),P("machine mm × 10","CellC")],
 [P("HMI 最終","CellC"),P("m_HMIGluePath","CellC"),P("四捨五入為整數座標","CellC")],
]
t=Table(coord,colWidths=[29*mm,55*mm,90*mm],repeatRows=1)
t.setStyle(TableStyle([("BACKGROUND",(0,0),(-1,0),NAVY),("GRID",(0,0),(-1,-1),0.5,colors.HexColor("#CBD5DC")),
                       ("VALIGN",(0,0),(-1,-1),"MIDDLE"),("ROWBACKGROUNDS",(0,1),(-1,-1),[colors.white,LIGHT]),
                       ("LEFTPADDING",(0,0),(-1,-1),6),("RIGHTPADDING",(0,0),(-1,-1),6),
                       ("TOPPADDING",(0,0),(-1,-1),6),("BOTTOMPADDING",(0,0),(-1,-1),6)]))
story += [Spacer(1,3*mm),t,Spacer(1,7*mm),P("3.1 Modbus 輸出", "H2C")]
modbus=[
 [P("資料","CellHead"),P("暫存器","CellHead"),P("來源／處理","CellHead")],
 [P("X1","CellC"),P("D14-D43","CellC"),P("左側 X 的絕對值","CellC")],
 [P("Y","CellC"),P("D44-D73","CellC"),P("左右共用 Y","CellC")],
 [P("X2","CellC"),P("D74-D103","CellC"),P("右側 X 的絕對值","CellC")],
]
mt=Table(modbus,colWidths=[28*mm,42*mm,104*mm],repeatRows=1)
mt.setStyle(TableStyle([("BACKGROUND",(0,0),(-1,0),BLUE),("GRID",(0,0),(-1,-1),0.5,colors.HexColor("#CBD5DC")),
                        ("VALIGN",(0,0),(-1,-1),"MIDDLE"),("LEFTPADDING",(0,0),(-1,-1),6),
                        ("TOPPADDING",(0,0),(-1,-1),6),("BOTTOMPADDING",(0,0),(-1,-1),6)]))
story += [mt,Spacer(1,4*mm),bullet("最多使用前 25 對路徑描述點；HMI 固定接收 30 筆，第 26-30 筆重複最後有效點。"),
          bullet("送出位置：WorkTab.cpp:2948-3056。")]

story += [PageBreak(),P("4. 架構風險與改善優先序", "H1C")]
risks=[
 ("P0", "影像與 ROI mask 可能不同座標系", "imgClone 被旋轉 180°，但 pathMask 沒有同步旋轉。非中心對稱 ROI 可能遮到錯誤區域。", "同步旋轉 mask，並用影像疊圖測試 ROI 四角。"),
 ("P0", "多輪廓被串成單一封閉序列", "findContours 可能回傳多條輪廓，程式直接 append；SplitByCenter 卻假設輸入是一條連續環。", "先選主輪廓，或以 contour 為單位分別優化。"),
 ("P1", "註解與實際演算法不一致", "註解稱弧長參數化保留 U 型，但實作是依 Y profile 與索引取樣，後續又全域按 Y 排序。", "明確決定採 Y 單調路徑或保留輪廓拓撲，並同步更新註解與測試。"),
 ("P1", "shoeType 參數未生效", "傳入 shoeType=2，但 OptimizePath 直接忽略，主導側固定為右側。", "依鞋型選擇主導側，或移除無效參數。"),
 ("P1", "負座標處理不對稱", "X 取絕對值；負 Y 則在 uint16 轉換時被截成 0，可能讓多個點重疊。", "定義 PLC 座標規格，採偏移、帶符號暫存器或一致的絕對值策略。"),
 ("P2", "入口只定義於右側", "EntryPointX 只裁切右側，左側完全依右側 Y 插值。", "確認雙頭機構是否需要獨立入口與安全接近段。"),
]
for pri,title,desc,act in risks:
    color=RED if pri=="P0" else ORANGE if pri=="P1" else BLUE
    tb=Table([[P(pri,"CellHead"),P(title,"Risk")],["",P(desc+"<br/><font color='#2878A8'>建議：</font>"+act,"Risk")]],
             colWidths=[16*mm,158*mm])
    tb.setStyle(TableStyle([("BACKGROUND",(0,0),(0,-1),color),("BACKGROUND",(1,0),(-1,-1),LIGHT),
                            ("SPAN",(0,0),(0,1)),("BOX",(0,0),(-1,-1),0.6,color),
                            ("VALIGN",(0,0),(-1,-1),"MIDDLE"),("ALIGN",(0,0),(0,-1),"CENTER"),
                            ("LEFTPADDING",(1,0),(-1,-1),7),("RIGHTPADDING",(1,0),(-1,-1),7),
                            ("TOPPADDING",(0,0),(-1,-1),4),("BOTTOMPADDING",(0,0),(-1,-1),4)]))
    story += [KeepTogether([tb,Spacer(1,2*mm)])]

story += [Spacer(1,4*mm),P("5. 建議驗證順序", "H1C"),
 bullet("以偏離影像中心的 ROI 測試旋轉前後 mask 是否一致。"),
 bullet("建立含兩個獨立物件的測試影像，確認不會跨輪廓連線。"),
 bullet("使用 U 型、凹型與左右腳樣本，比較點序是否符合實際運動需求。"),
 bullet("測試 referenceY 位於路徑上方與下方時，HMI Y 是否出現大量 0。"),
 bullet("確認 25 點描述與補滿 30 筆後，PLC 插補及停止行為符合設備規格。"),
 Spacer(1,3*mm),KeepTogether([P("結論", "H2C"),
 P("現有程式已具備完整的資料分層與 HMI 傳輸管線；改善重點不是重新設計全部架構，而是先消除座標系與輪廓拓撲的不確定性，再決定路徑應採 Y 單調掃描或保留原始輪廓順序。")])]

doc=SimpleDocTemplate(str(OUT), pagesize=A4, rightMargin=18*mm,leftMargin=18*mm,
                      topMargin=17*mm,bottomMargin=18*mm,title="SP-AX3 運動路徑架構分析",
                      author="Codex")
doc.build(story,onFirstPage=footer,onLaterPages=footer)
print(OUT)
