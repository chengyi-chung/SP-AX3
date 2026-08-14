from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "vendor"))

from reportlab.lib import colors
from reportlab.lib.enums import TA_LEFT, TA_CENTER
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.units import mm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak, Flowable

ROOT = HERE.parents[1]
OUT = ROOT / "output" / "pdf" / "SP-AX3_Calibration校正操作手冊.pdf"
OUT.parent.mkdir(parents=True, exist_ok=True)

pdfmetrics.registerFont(TTFont("CJK", r"C:\Windows\Fonts\kaiu.ttf"))
NAVY = colors.HexColor("#183B56")
BLUE = colors.HexColor("#2377A5")
SKY = colors.HexColor("#EAF6FB")
GREEN = colors.HexColor("#277A53")
PALE_GREEN = colors.HexColor("#EAF6EF")
ORANGE = colors.HexColor("#C76B00")
PALE_ORANGE = colors.HexColor("#FFF4E5")
RED = colors.HexColor("#B42318")
PALE_RED = colors.HexColor("#FDECEC")
INK = colors.HexColor("#25313C")
MUTED = colors.HexColor("#637381")
LINE = colors.HexColor("#CCD6DF")
PALE = colors.HexColor("#F5F8FA")

S = {
    "title": ParagraphStyle("title", fontName="CJK", fontSize=25, leading=34, textColor=NAVY, spaceAfter=7*mm),
    "subtitle": ParagraphStyle("subtitle", fontName="CJK", fontSize=11, leading=18, textColor=MUTED, spaceAfter=9*mm),
    "h1": ParagraphStyle("h1", fontName="CJK", fontSize=17, leading=24, textColor=NAVY, spaceBefore=3*mm, spaceAfter=4*mm),
    "h2": ParagraphStyle("h2", fontName="CJK", fontSize=12.5, leading=18, textColor=BLUE, spaceBefore=3*mm, spaceAfter=2*mm),
    "body": ParagraphStyle("body", fontName="CJK", fontSize=9.6, leading=16, textColor=INK, spaceAfter=2.5*mm),
    "small": ParagraphStyle("small", fontName="CJK", fontSize=8.2, leading=13, textColor=MUTED),
    "step": ParagraphStyle("step", fontName="CJK", fontSize=10, leading=16, textColor=INK, spaceAfter=2*mm),
    "code": ParagraphStyle("code", fontName="CJK", fontSize=8.7, leading=14, textColor=NAVY, backColor=PALE, borderColor=LINE, borderWidth=.5, borderPadding=7, spaceAfter=3*mm),
}


def P(text, style="body"):
    return Paragraph(text, S[style])


def box(text, tone="info"):
    bg, border = {"info": (SKY, BLUE), "ok": (PALE_GREEN, GREEN), "warn": (PALE_ORANGE, ORANGE), "danger": (PALE_RED, RED)}[tone]
    st = ParagraphStyle("box"+tone, parent=S["body"], backColor=bg, borderColor=border, borderWidth=.8, borderPadding=8, spaceBefore=2*mm, spaceAfter=4*mm)
    return Paragraph(text, st)


def table(rows, widths, header=True):
    data = [[P(str(v), "small") for v in row] for row in rows]
    t = Table(data, colWidths=widths, repeatRows=1 if header else 0, hAlign="LEFT")
    cmds = [("GRID", (0,0), (-1,-1), .4, LINE), ("VALIGN", (0,0), (-1,-1), "TOP"),
            ("LEFTPADDING", (0,0), (-1,-1), 6), ("RIGHTPADDING", (0,0), (-1,-1), 6),
            ("TOPPADDING", (0,0), (-1,-1), 5), ("BOTTOMPADDING", (0,0), (-1,-1), 5)]
    if header: cmds += [("BACKGROUND", (0,0), (-1,0), NAVY), ("TEXTCOLOR", (0,0), (-1,0), colors.white)]
    for i in range(1 if header else 0, len(rows)):
        if i % 2 == 0: cmds.append(("BACKGROUND", (0,i), (-1,i), PALE))
    t.setStyle(TableStyle(cmds))
    return t


class TwoStageFlow(Flowable):
    def __init__(self):
        super().__init__(); self.width=168*mm; self.height=55*mm
    def draw(self):
        c=self.canv
        blocks=[(0, "A. Lens Calibration", "多張棋盤照片\n建立 calibration.yml", BLUE),
                (62, "B. Factor", "目前棋盤影像\n建立 mm/pixel", GREEN),
                (124, "C. Verify", "量測與 Tool Path\n確認方向與尺寸", ORANGE)]
        for x,title,desc,color in blocks:
            x*=mm; w=44*mm
            c.setFillColor(color); c.roundRect(x,25*mm,w,18*mm,3*mm,fill=1,stroke=0)
            c.setFillColor(colors.white); c.setFont("CJK",10); c.drawCentredString(x+w/2,35*mm,title)
            c.setFont("CJK",8.3)
            for j,line in enumerate(desc.splitlines()): c.drawCentredString(x+w/2,29*mm-j*4.5*mm,line)
        c.setStrokeColor(NAVY); c.setFillColor(NAVY); c.setLineWidth(1.4)
        for x in (44,106):
            c.line(x*mm,34*mm,(x+14)*mm,34*mm)
            p=c.beginPath(); p.moveTo((x+14)*mm,34*mm); p.lineTo((x+10)*mm,36*mm); p.lineTo((x+10)*mm,32*mm); p.close(); c.drawPath(p,fill=1,stroke=0)
        c.setFillColor(MUTED); c.setFont("CJK",8); c.drawCentredString(self.width/2,8*mm,"順序不可顛倒：先鏡頭去畸變，再計算實際尺寸比例")


def hf(canvas, doc):
    canvas.saveState(); w,h=A4
    canvas.setStrokeColor(LINE); canvas.line(20*mm,15*mm,w-20*mm,15*mm)
    canvas.setFont("CJK",7.5); canvas.setFillColor(MUTED)
    canvas.drawString(20*mm,9.5*mm,"SP-AX3 Calibration 校正操作手冊")
    canvas.drawRightString(w-20*mm,9.5*mm,f"第 {doc.page} 頁")
    canvas.restoreState()


story=[]
story += [Spacer(1,16*mm), P("SP-AX3 Calibration<br/>校正操作手冊","title")]
story += [P("依照現行 WorkTab / UAXVision 程式流程編製｜適用於鏡頭畸變校正與 Transfer Factor 校正","subtitle")]
story += [TwoStageFlow(), Spacer(1,5*mm)]
story += [box("<b>操作原則：</b>相機位置、鏡頭焦距、解析度或 ImageFlip 改變後，必須重新執行完整校正。校正期間請勿移動相機、鏡頭或治具。","warn")]
story += [P("快速操作總覽","h1")]
story += [table([
    ["階段","操作","完成判定"],
    ["1. 準備","固定相機，拍攝至少 3 張不同角度的棋盤格照片。","照片清楚、同尺寸、棋盤遍布畫面不同位置。"],
    ["2. Calibration","在 Work 頁按 Calibration，多選校正照片。","顯示校正完成與 RMS，應用程式目錄出現 calibration.yml。"],
    ["3. Factor","載入或拍攝一張棋盤影像，按 Factor，輸入單格 mm 後框選多格。","顯示 TransferFactor，SystemConfig.ini 已更新。"],
    ["4. 驗證","以已知尺寸工件或棋盤檢查中心與邊緣。","尺寸誤差及路徑方向符合設備規格。"],
], [30*mm,70*mm,65*mm])]

story += [PageBreak(), P("1. 校正前準備","h1")]
story += [P("1.1 設備條件","h2")]
for item in [
    "相機與鏡頭已安裝完成，機構鎖固且不會晃動。",
    "Camera ID、Camera Width、Camera Height 與實際取像設定一致。",
    "ImageFlip 已設定為正式生產方向；校正後不要再更改。",
    "鏡頭焦距、光圈及對焦已完成；校正後不要調整焦距或鏡頭位置。",
    "棋盤格平整、不反光、無折痕，且單格實際尺寸已用可靠量具確認。",
]: story.append(P("□ "+item))
story += [P("1.2 校正照片拍攝規則","h2")]
story += [table([
    ["項目","操作要求"],
    ["數量","程式至少需要 3 張成功角點影像；實務建議準備 10-20 張。"],
    ["尺寸","所有照片必須使用完全相同的解析度。"],
    ["位置","涵蓋中心、四角與畫面邊緣，避免全部集中在中央。"],
    ["姿態","包含正面、上下傾斜、左右傾斜與不同距離；每張姿態需有變化。"],
    ["品質","棋盤完整可見、角點清楚、曝光均勻，避免模糊、過曝與強烈反光。"],
    ["格式","程式選擇器支援 JPG、JPEG、PNG、BMP。"],
], [32*mm,133*mm])]
story += [box("棋盤規格目前由程式自動嘗試，預設為 9×6 內部角點、25.0 單格尺寸。畫面上的角點數是黑白格交界點數，不是方格數。","info")]
story += [P("1.3 ROI 使用注意","h2")]
story += [P("按 Calibration 時，如果系統的 MaskWidth 與 MaskHeight 大於 0，程式只在該 Mask 區域內尋找棋盤角點。若校正板不完全位於 Mask 中，建議先把 Mask 設為涵蓋完整棋盤，或使 Mask 尺寸無效以改用全圖搜尋。畫面上的 ROI 勾選框主要控制顯示，實際搜尋範圍取自系統 Mask 數值。")]

story += [PageBreak(), P("2. 執行鏡頭畸變校正（Calibration）","h1")]
steps=[
    ["1","進入 Work 頁面","確認右側可看到 Calibration、Factor、ROI、Center 按鈕。"],
    ["2","確認正式參數","確認相機解析度、ImageFlip 與 Mask；校正後不得任意變更。"],
    ["3","按 Calibration","系統開啟「Select calibration images」多選視窗。"],
    ["4","選取全部照片","在同一資料夾可一次多選；完成後按開啟。若取消會顯示「未選取任何影像」。"],
    ["5","等待角點與模型計算","程式逐張讀圖、搜尋角點並計算 fisheye 模型。操作期間不要關閉程式。"],
    ["6","查看預覽","成功時出現 Calibration Grid Preview；綠色 FOUND 代表找到角點，紅色 NOT FOUND 代表該張未採用。"],
    ["7","確認完成訊息","記錄 RMS 值與儲存路徑。成功後 calibration.yml 會寫入 EXE 所在目錄。"],
]
story += [table([["步驟","畫面操作","確認重點"]]+steps,[15*mm,48*mm,102*mm])]
story += [P("完成訊息範例","h2"), P("校正完成，RMS=0.xxx px<br/>儲存於: ...\\calibration.yml","code")]
story += [box("<b>品質判讀：</b>程式在 RMS > 0.8 px 時只輸出警告，仍可能保存檔案。建議現場標準先以 RMS ≤ 0.8 px 為基本條件；若超過，重新拍攝更多角度與位置差異較大的照片。","warn")]
story += [P("校正後立即確認","h2")]
for item in ["calibration.yml 的修改時間為本次操作時間。","主畫面影像可正常顯示，沒有空白或例外訊息。","直線在校正後看起來較直，且中心與邊緣沒有明顯異常拉伸。"]: story.append(P("□ "+item))

story += [PageBreak(), P("3. 執行比例校正（Factor）","h1")]
story += [box("Factor 必須在 Calibration 成功後執行。它計算的是 <b>mm/pixel</b>，不是鏡頭畸變參數。","info")]
steps2=[
    ["1","準備目前影像","使用 Grab 或 Load Image 取得一張清楚的棋盤影像；主畫面不可為空。"],
    ["2","按 Factor","程式先載入 calibration.yml 並對目前影像去畸變。"],
    ["3","輸入單格長度","在輸入視窗填入棋盤『單一方格邊長』，單位 mm；預設為 25.0。"],
    ["4","等待角點偵測","成功後主畫面切換為校正後預覽，並顯示偵測到的棋盤角點。"],
    ["5","框選多個格點","在主影像區按住滑鼠拖拉，框住連續的多個角點；橫向或縱向至少跨越 1 格。"],
    ["6","放開滑鼠","程式自動計算平均單格像素距離與 TransferFactor，無需再按確認。"],
    ["7","記錄結果","確認完成視窗中的格數、單格長度、平均像素與 mm/pixel，並確認 INI 已更新。"],
]
story += [table([["步驟","畫面操作","確認重點"]]+steps2,[15*mm,48*mm,102*mm])]
story += [P("計算公式","h2"), P("TransferFactor (mm/pixel) = 單格實際長度 (mm) ÷ 平均相鄰角點距離 (pixel)","code")]
story += [box("框選建議：框住中央且清楚的 3×3 格以上區域，包含多個水平與垂直間距。不要只框單一角點，也不要讓選框落在沒有角點的位置。","ok")]

story += [PageBreak(), P("4. 結果確認與生產前驗證","h1")]
story += [P("4.1 Factor 完成訊息","h2")]
story += [table([
    ["欄位","應確認內容"],
    ["橫向格數 / 縱向格數","與實際框選跨越的格數合理一致。"],
    ["單格長度","等於輸入的棋盤單格真實尺寸。"],
    ["平均單格像素","必須大於 0，且同一設備重複操作結果應接近。"],
    ["TransferFactor","單位為 mm/pixel；已寫入 m_SystemPara 與 SystemConfig.ini。"],
    ["INI 路徑","應指向 EXE 所在目錄的 SystemConfig.ini。"],
], [52*mm,113*mm])]
story += [P("4.2 必做驗證","h2")]
for item in [
    "重新啟動程式，確認 calibration.yml 可自動載入且畫面正常。",
    "在棋盤中心量測至少 5 格，以計算結果換算後與真實尺寸比較。",
    "在畫面左、右、上、下邊緣各量測一次，確認誤差沒有明顯擴大。",
    "產生測試 Tool Path，確認路徑與校正後影像、ROI 及實際機械方向一致。",
    "以低速、離開工件的安全高度試跑；確認 X/Y 方向與移動距離後才進入生產。",
]: story.append(P("□ "+item))
story += [box("<b>安全要求：</b>程式在找不到校正或去畸變失敗時，部分 Tool Path 流程可能退回原始影像繼續計算。首次校正、更換相機或變更 ImageFlip 後，禁止直接高速生產，必須先做離線與低速驗證。","danger")]
story += [P("4.3 建議紀錄","h2")]
story += [table([
    ["日期","設備/相機","解析度","ImageFlip","棋盤規格","RMS","Factor","操作員"],
    ["____/____/____","________","____×____","____","____×____ / ____ mm","____ px","____ mm/px","________"],
], [22*mm,23*mm,23*mm,19*mm,31*mm,17*mm,18*mm,17*mm])]

story += [PageBreak(), P("5. 異常訊息與處理方式","h1")]
story += [table([
    ["畫面訊息 / 現象","可能原因","處理方式"],
    ["未選取任何影像","取消選檔或沒有完成選取。","重新按 Calibration 並多選校正照片。"],
    ["Cannot read one or more calibration images","檔案損壞、格式異常或路徑不可讀。","移除問題照片，確認可由一般圖片程式開啟。"],
    ["Calibration images must all have the same size","照片解析度不一致。","只保留同一相機、同一解析度的照片。"],
    ["Not enough valid calibration images","成功找到角點的照片少於 3 張。","改善對焦/曝光，增加不同位置與角度的照片；檢查 Mask 是否限制搜尋。"],
    ["校正資料儲存失敗","EXE 目錄不可寫、檔案被鎖定或路徑異常。","關閉占用 calibration.yml 的程式，確認目錄權限後重試。"],
    ["請先載入影像","按 Factor 時 m_mat 為空。","先按 Grab 或 Load Image。"],
    ["無法載入校正參數檔 calibration.yml","尚未 Calibration、檔案遺失或內容無效。","重新完成 Calibration，確認檔案位於 EXE 目錄。"],
    ["無法在校正後影像中偵測到棋盤格角點","目前 Factor 影像模糊、棋盤不完整或規格不符。","重新取一張清晰、完整、曝光正常的棋盤影像。"],
    ["框選區域太小 / 沒有有效角點","拖曳範圍不足或沒有包住角點。","重新框選連續多個角點，至少跨越 1 格。"],
    ["無法從所選區域計算角點間距","選取的角點無法形成相鄰格。","改框較大的矩形區域，建議至少 3×3 格。"],
], [52*mm,55*mm,58*mm])]

story += [PageBreak(), P("6. 何時必須重新校正","h1")]
for item in [
    "更換相機或鏡頭。",
    "調整鏡頭焦距、對焦位置或相機與工件距離。",
    "相機支架、治具或工作平面移動。",
    "Camera Width / Height 或取像解析度改變。",
    "ImageFlip 或相機安裝方向改變。",
    "校正後直線仍明顯彎曲，或中心與邊緣量測誤差差異過大。",
    "Tool Path 與影像、ROI 或實際機械方向不一致。",
]: story.append(P("• "+item))
story += [P("重新校正順序","h2"), P("固定設備 → 確認解析度與 ImageFlip → 重新拍攝照片 → Calibration → Factor → 尺寸驗證 → 低速試跑","code")]
story += [P("7. 操作員最終檢查表","h1")]
checks=[
    "相機、鏡頭與治具已鎖固。", "校正照片同解析度且涵蓋中心與四角。",
    "Calibration 完成，已記錄 RMS。", "calibration.yml 已建立且時間正確。",
    "Factor 使用正確的單格 mm。", "SystemConfig.ini 已更新 TransferFactor。",
    "中心與邊緣尺寸驗證合格。", "Tool Path、ROI、X/Y 方向驗證合格。",
    "已完成安全高度低速試跑。", "校正紀錄已填寫並保存。",
]
story += [table([["確認","項目","確認","項目"]]+[["□",checks[i],"□",checks[i+1]] for i in range(0,len(checks),2)], [13*mm,69*mm,13*mm,70*mm])]
story += [Spacer(1,5*mm), box("本手冊依目前程式行為編製。正式導入前，請由設備工程師依機台精度、安全規範與實際棋盤尺寸設定可接受的 RMS、Factor 重複性及尺寸誤差標準。","info")]

doc=SimpleDocTemplate(str(OUT),pagesize=A4,leftMargin=20*mm,rightMargin=20*mm,topMargin=18*mm,bottomMargin=20*mm,
                      title="SP-AX3 Calibration 校正操作手冊",author="Codex",subject="依程式流程編製的校正操作手冊")
doc.build(story,onFirstPage=hf,onLaterPages=hf)
print(OUT)
