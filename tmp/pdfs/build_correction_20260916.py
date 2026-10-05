from pathlib import Path
from xml.sax.saxutils import escape
from reportlab.lib import colors
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.pagesizes import A4
from reportlab.lib.units import mm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak

ROOT=Path(r'D:\Git Repository\chengyi-chung\SP-AX3')
OUT=ROOT/'output/pdf/AX3_伺服座標補正重構說明_2026-09-16.pdf'
OUT.parent.mkdir(parents=True,exist_ok=True)
pdfmetrics.registerFont(TTFont('CJK',r'C:\Windows\Fonts\msjh.ttc',subfontIndex=0))
pdfmetrics.registerFont(TTFont('CJKB',r'C:\Windows\Fonts\msjhbd.ttc',subfontIndex=0))
navy=colors.HexColor('#173C56')
styles={
 'title':ParagraphStyle('title',fontName='CJKB',fontSize=23,leading=32,textColor=navy,spaceAfter=12),
 'h':ParagraphStyle('h',fontName='CJKB',fontSize=14,leading=21,textColor=navy,spaceBefore=12,spaceAfter=8),
 'p':ParagraphStyle('p',fontName='CJK',fontSize=10,leading=17,spaceAfter=8,wordWrap='CJK'),
 'small':ParagraphStyle('small',fontName='CJK',fontSize=8.5,leading=14,spaceAfter=6,wordWrap='CJK'),
 'cell':ParagraphStyle('cell',fontName='CJK',fontSize=9,leading=14,wordWrap='CJK'),
}
story=[]
def p(t,style='p'): story.append(Paragraph(escape(t).replace('\n','<br/>'),styles[style]))
def h(t): p(t,'h')
def table(rows,widths):
 t=Table([[Paragraph(escape(str(v)),styles['cell']) for v in row] for row in rows],colWidths=[w*mm for w in widths],repeatRows=1,hAlign='LEFT')
 t.setStyle(TableStyle([('BACKGROUND',(0,0),(-1,0),colors.HexColor('#DFEBF1')),('VALIGN',(0,0),(-1,-1),'TOP'),('LEFTPADDING',(0,0),(-1,-1),7),('RIGHTPADDING',(0,0),(-1,-1),7),('TOPPADDING',(0,0),(-1,-1),6),('BOTTOMPADDING',(0,0),(-1,-1),6),('LINEBELOW',(0,0),(-1,0),.6,navy),('LINEBELOW',(0,1),(-1,-1),.3,colors.HexColor('#D8E0E6'))]))
 story.append(t); story.append(Spacer(1,8))
def page(): story.append(PageBreak())

p('AX3 伺服座標補正\n重構說明','title')
p('日期：2026-09-16　｜　設計說明文件　｜　依目前 PC 程式及 PLC 匯出符號整理','small')
p('目的：將送入伺服前的座標處理拆成可追查、可驗證的階段，避免補正重複套用、單位混淆及三軸資料不一致。')
h('建議的完整流程')
p('影像座標 → 機械座標 → 共用 Y 路徑 → 軸向座標 → 機構補正 → 範圍檢查 → 伺服命令')
p('核心原則：幾何轉換、機構誤差補正、通訊編碼分開處理，每一種補正只執行一次。本文為重構方案，尚未修改或部署程式。')
h('1. 目前已實作的轉換')
table([
 ['階段','目前處理','單位'],
 ['原點平移','影像點減去校正後參考點','pixel'],
 ['角度轉換','依 CameraToMachineAngle 旋轉；受 IsMachineRotate 控制','pixel'],
 ['比例換算','乘上 TransferFactor','mm'],
 ['HMI 比例','乘以 10，再四捨五入','每單位 0.1 mm'],
 ['共用 Y','左側 Y 直接改成右側 Y','同上'],
 ['軸向定義','X1 取左側 X 的負值，X2 保留右側 X','同上'],
 ['傳送','signed 16-bit 位元格式，分三組寫入','WORD'],
],[30,111,29])
p('主要實作：WorkTab.cpp 的 ConvertToMachineCoordinates() 與 OnBnClickedIdcWorkGo()。傳送函式本身未乘比例，但上游已乘以 10；PLC 是否依相同倍率還原，仍須確認程式本體。')
p('目前這條 PC 傳送路徑可見原點、角度、比例及軸向轉換，未看到依伺服實測誤差建立的逐軸補正模型。','small')

page()
h('2. 第一項重構：區分三種補正')
table([
 ['類別','解決的問題','適合位置'],
 ['視覺校正','影像透視、像素比例、相機安裝角度','PC 影像／座標轉換層'],
 ['製程調整','膠路內縮、鞋碼級放、左右偏移','路徑／配方層'],
 ['機構補正','伺服命令與實際膠嘴位置的偏差','最終軸命令產生前'],
],[28,83,59])
p('例如：膠路向鞋緣內縮 2 mm 是製程要求；膠嘴實際位置比命令多走 0.4 mm 才是機構誤差。若用同一個 Offset 修正兩件事，更換鞋型或重新校正相機後，既有補正可能失效。')
h('3. 第二項重構：中間計算統一使用 mm')
p('內部保存浮點數毫米座標，直到通訊封裝才轉成整數，避免在不同層次重複縮放或過早取整。')
p('u = x_pixel - x_ref\nv = y_pixel - y_ref\nx_m = s × (u × cosθ - v × sinθ)\ny_m = s × (u × sinθ + v × cosθ)')
p('s 為 TransferFactor，單位 mm/pixel；θ 為相機座標轉到機械座標的角度。參考點與路徑點必須位於同一個校正後影像座標系。')
p('目前 TransferFactor 無效時會退回 1.0。正式送出流程應改為停止產生命令並回報校正參數無效，因為退回 1.0 等同假設一個像素就是一毫米。')
p('影像 Y 向下為正；旋轉正方向必須以實際機台定義驗證，不可直接套用一般 Y 向上圖形的直覺。')
h('4. 第三項重構：先旋轉，再建立共用 Y')
p('目前左右路徑先在影像座標中配對，旋轉後把左側 Y 強制設成右側 Y。相同影像 Y 的兩點，旋轉後通常不再有相同機械 Y：')
p('y_m,R - y_m,L = s × (u_R - u_L) × sinθ')
p('只要左右有間距且角度不為零，就會出現 Y 差。直接覆寫左側 Y，沒有重算該 Y 位置應有的左側 X，因此可能改變左側曲線形狀。')

page()
h('共用 Y 的重建步驟')
p('1. 保留左右完整曲線或足夠密集的點。\n2. 左右曲線分別轉到機械 mm 座標。\n3. 在機械座標中決定共同的 Y_i。\n4. 分別求兩條曲線在 Y_i 的交點或插值。\n5. 組成真正對應同一個 Y 的三軸點。')
p('P_i = ( X1(Y_i), Y_i, X2(Y_i) )')
p('若曲線對同一個 Y 有多個交點，應依路徑方向與連續性選擇分支；不可只依 Y 排序後任意插值。')
h('5. 第四項重構：獨立的逐軸機構補正')
p('完成幾何與製程調整後，先取得「希望膠嘴實際到達的位置」，再反算伺服應收到的命令。X1、X2、Y 各自保存模型；即使左右機構外觀對稱，也不能預設共用同一組係數。')
table([
 ['軸','建議參數','用途'],
 ['X1','比例、零點、必要時補正表','左膠嘴位置'],
 ['X2','比例、零點、必要時補正表','右膠嘴位置'],
 ['Y','比例、零點、必要時補正表','共用前後位置'],
],[20,91,59])
p('若實測模型為 p_actual = a × q_command + b，目標位置為 p_target，則：')
p('q_command = (p_target - b) / a')
p('重點是反算模型，不是把量測誤差直接加上去。模型係數及量測資料必須使用一致單位。')
h('計算示例（非本機實測）')
p('命令 100 mm，實際到 101.2 mm；假設多點擬合得到 p = 1.01q + 0.2。要實際到 100 mm，應命令：\nq = (100 - 0.2) / 1.01 ≈ 98.812 mm。')
p('單筆量測本身不足以識別比例與零點兩個參數；以上係數僅用於解釋反算方式。','small')

page()
h('補正模型選擇')
table([
 ['模型','適用情況','注意事項'],
 ['固定偏移','全行程誤差近似固定','無法修正比例誤差'],
 ['線性模型','誤差隨行程近似線性變化','需多點量測驗證'],
 ['分段線性補正表','各位置誤差不均勻','檢查單調性、插值範圍'],
 ['方向相關模型','正反向有不同誤差','處理換向，避免命令跳變'],
],[38,65,67])
p('只有實測顯示 X 誤差明顯隨 Y 改變時，才考慮二維補正表。目前資料不足以支持直接採用更複雜模型。')
h('6. 第五項重構：指定唯一補正執行端')
table([
 ['PC 責任','PLC 責任'],
 ['影像校正、輪廓、膠路內縮','接收並驗證完整點位'],
 ['相機到機械座標轉換','完成 PLC 端配方／級放等剩餘處理'],
 ['共用 Y 重新取樣','最終逐軸機構補正'],
 ['編碼與整批傳送','限位檢查、運動命令與異常處理'],
],[85,85])
p('建議將機構補正放在 PLC，讓手動定位、配方及 CCD 路徑等來源共用規則。實際插入位置必須位於最後一次目標座標修改之後、交付運動命令之前。')
p('不能只依名称就斷定放進 PRG_MotionConvert；目前該群組匯出的資料只看得到目前位置欄位。若伺服驅動器已啟用位置補償，也需纳入同一份設定定義，避免 PC、PLC、驅動器重複補正。')
h('資料契約')
p('每一層需明確定義座標原點、正方向、單位、有效範圍，以及已套用的補正項目。PC 傳送值若是名義位置，PLC 就以名義位置解碼；若協議改成補正後命令，則不能再次套用相同補正。')

page()
h('7. 第六項重構：封裝前驗證，禁止靜默截斷')
p('目前 PC 將超出 signed 16-bit 範圍的值限制到邊界，可能出現「傳送成功，但座標已被改變」。建議改為整批拒絕並指出點號、軸及原因。')
table([
 ['檢查項目','不通過時處理'],
 ['NaN／無限值','拒絕整批'],
 ['校正倍率或模型係數無效','拒絕整批'],
 ['超出補正模型量測範圍','拒絕或依明訂策略處理'],
 ['補正後超出軸行程','拒絕整批，指出點號與軸'],
 ['編碼後超出 INT16','拒絕整批，不截斷'],
 ['點位突跳或左右配對失敗','拒絕整批'],
],[85,85])
p('若協議確認為每單位 0.1 mm：r = round(10 × q)。INT16 可表達約 -3276.8 至 3276.7 mm；這只是通訊數值範圍，實際機械行程必須另外限制。')
p('PLC 的 WORD 接收值必須按約定解讀為有號座標，再換算成 mm。例如負數不能直接當作無號 WORD 數值放大或轉成 LREAL。')
h('8. 第七項重構：整批接收後才允許使用')
p('目前 X1、Y、X2 分三次寫入。建議區分待接收區與執行區，執行中的路徑不直接被新資料覆寫。')
p('接收完整路徑\n↓\n確認序號、有效點數、資料完整性\n↓\n最終補正與範圍驗證\n↓\n切換為可執行路徑\n↓\n回覆已接受的序號')
p('一次寫入連續 90 個 WORD 可減少分批問題，但是否能一致交付 PLC 應用程式，仍須由接收與提交機制保證。')
p('新版協議應納入有效點數。現行最多 25 點補成 30 點，需先確認重複末點是否再次觸發停留或送膠。')

page()
h('9. 驗證方式與驗收重點')
table([
 ['驗證項目','通過條件'],
 ['等價性','角度為零、補正為單位模型時，新舊輸出一致'],
 ['共用 Y','非零角度時，左右 X 來自同一機械 Y 的曲線位置'],
 ['符號與倍率','正負座標、零點、邊界值的 PC 編碼與 PLC 解碼一致'],
 ['模型反算','命令代回量測模型後，接近期望位置'],
 ['整批一致性','任一段傳輸中斷，不採用混合新舊資料'],
 ['實測驗收','以未參與擬合的點，檢查最大／平均誤差及正反向差異'],
],[34,136])
p('驗收容許誤差應依機台製程需求與量測能力訂定；本文未替現場指定數值。數學回代正確不能代替實際膠嘴落點量測。')
h('建議實施順序')
p('1. 確認 PLC 解碼、單位與最終補正插入位置。\n2. 修正旋轉後的共用 Y 配對。\n3. 統一 mm 中間計算並延後整數編碼。\n4. 依實測導入 X1、X2、Y 的獨立補正模型。\n5. 完成整批提交與異常情境驗證。\n6. 以獨立量測點完成實測驗收。')
h('依據與限制')
p('本文件將本次討論的重構說明整理成 PDF。主要依據為目前工作目錄中的 WorkTab.cpp，以及 PLC/AX3/Press machine CCD.Device.Application.xml。現有流程的行為以程式內容為準；本文所列重構流程為建議設計。')
p('原始 PLC .project 為二進位專案，本次尚未取得可讀 POU 實作。因此 PLC 的實際補正公式、Task 順序、運動功能塊與通訊提交條件均待確認，不能將建議方案視為既有 PLC 邏輯。')
p('參考位置：\n• WorkTab.cpp：ConvertToMachineCoordinates()，約第 4430 行。\n• WorkTab.cpp：OnBnClickedIdcWorkGo()，約第 3386 行。\n• PLC/AX3/PLC_Analysis.md：PLC 靜態資料分析。\n• PLC/AX3/PLC_Symbols.md：匯出符號清單。','small')

def footer(c,d):
 c.saveState(); c.setStrokeColor(colors.HexColor('#CCD9E1')); c.line(20*mm,18*mm,190*mm,18*mm)
 c.setFont('CJK',8); c.setFillColor(navy); c.drawString(20*mm,12*mm,'AX3 伺服座標補正重構說明  |  2026-09-16'); c.drawRightString(190*mm,12*mm,str(d.page)); c.restoreState()
doc=SimpleDocTemplate(str(OUT),pagesize=A4,rightMargin=20*mm,leftMargin=20*mm,topMargin=18*mm,bottomMargin=24*mm,title='AX3 伺服座標補正重構說明',author='SP-AX3 技術文件')
doc.build(story,onFirstPage=footer,onLaterPages=footer)
print(OUT)
