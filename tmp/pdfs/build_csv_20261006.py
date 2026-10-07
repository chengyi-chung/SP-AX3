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
OUT=ROOT/'output/pdf/AX3_CSV與Modbus欄位說明_2026-10-06.pdf'
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

p('AX3 CSV 除錯資料\n與 Modbus 欄位說明','title')
p('文件日期：2026-10-06　｜　適用目錄：SP-AX3 / x64 / Debug','small')
p('本文件說明 Debug 目錄內 9 份 CSV 的用途、座標單位及軸向，完整解釋 ModbusPath_2026-10-06.csv 的 19 個欄位，並以已核對的實際批次說明讀法。')
h('閱讀前先確認')
p('tool_*.csv 的 X1／X2 欄名與正式送往 PLC 的軸定義不同。PathDataOut.csv 和 ModbusPath CSV 才使用正式軸定義。不可直接把各檔同名欄位當成同一個軸。')
p('本次紀錄：1 個批次、180 列資料；三軸寫入均回報 SUCCESS。這代表 Modbus 寫入 API 成功，不代表 PLC 已採用整批路徑或伺服已到位。')
h('1. 資料產生流程')
p('tool_raw_path.csv\n↓ 左右配對、排序及 ROI 過濾\ntool_final_glue_path.csv\n↓ 保存至程式成員\ntool_optimized_glue_path.csv\n↓ 減去參考原點、旋轉\ntool_machine_glue_path.csv\n↓ × TransferFactor\ntool_machine_glue_path_mm.csv\n↓ × 10\ntool_HMIGluePath_temp.csv\n↓ 四捨五入\ntool_HMIGluePath.csv\n↓ 正式軸向轉換\nPathDataOut.csv\n↓ 整數編碼、補至 30 點、實際寫入\nModbusPath_2026-10-06.csv')

page()
h('2. 每一份 CSV 的用途')
table([
 ['檔案／資料筆數','用途與單位'],
 ['tool_raw_path.csv\n50 筆','路徑算法輸出，影像座標，單位 pixel。已經過二值化、鞋型選取與內縮，不是相機原始像素清單。'],
 ['tool_final_glue_path.csv\n25 筆','左右配對、排序、ROI 過濾及点數限制後的路徑，仍為影像座標，單位 pixel。'],
 ['tool_optimized_glue_path.csv\n25 筆','保存至 m_OptimizedGluePath 的資料。本次與 final 檔完全相同，不是再做一次最佳化。'],
 ['tool_machine_glue_path.csv\n25 筆','以參考點為原點並旋轉後的座標，單位仍為 pixel。'],
 ['tool_machine_glue_path_mm.csv\n25 筆','前一步乘上 TransferFactor，單位 mm，適合檢查幾何尺寸。'],
 ['tool_HMIGluePath_temp.csv\n25 筆','mm 座標乘以 10，尚未取整；每一數值單位代表 0.1 mm。'],
 ['tool_HMIGluePath.csv\n25 筆','前一步四捨五入，單位 0.1 mm；仍使用 Debug 欄位定義。'],
 ['PathDataOut.csv\n25 筆','GO 階段的正式軸向，欄位 Y,X1,X2，單位 0.1 mm。不含補點，也不代表傳送成功。'],
 ['ModbusPath_2026-10-06.csv\n180 筆','實際傳送緩衝區與 Modbus 寫入結果，包含三軸各 30 點、補點與原始 WORD。'],
],[73,97])
h('寫入時機與保存方式')
p('tool_*.csv 在 Debug 建立 ToolPath 時覆寫。PathDataOut.csv 在 GO 階段且啟用 PathDataOut 設定時覆寫。ModbusPath CSV 不受該設定影響，按每日檔名追加，保留不同批次。')
p('本次 tool_*.csv 的修改時間為 11:41:13，PathDataOut 與 Modbus 紀錄為 11:41:29。GO 會重新讀取角度並轉換座標，因此不同時間的輸出不一定相同；本次已逐點核對相符。','small')

page()
h('3. Debug 欄位與正式軸向')
table([['tool_raw_path.csv 欄位','意義'],['Index','由 0 開始的路徑點索引。'],['X／Y','影像水平／垂直座標，單位 pixel。'],['Cluster','程式提供的群組編號；本次 -1 表示沒有提供群組編號，不代表錯誤。']],[55,115])
p('其餘六份 tool_*.csv 均使用 Index,X1,Y,X2，但匯出函式實際內容如下：')
table([['CSV 欄名','實際內容'],['Index','由 0 開始的左右配對索引。'],['X1','右側 PathRight.x。'],['Y','右側／共用 Y。'],['X2','左側 PathLeft.x，尚未轉成左軸正方向。']],[35,135])
p('從 tool_HMIGluePath.csv 轉成正式傳送值：\n正式 X1 = -Debug 欄位 X2\n正式 X2 = Debug 欄位 X1\n正式 Y = Debug 欄位 Y')
h('本次第一點：沿各階段追查')
table([['階段','右側 X','共用 Y','左側 X'],['影像 pixel','716.003','172.002','680.996'],['機械 pixel','16.985','50.137','-17.890'],['機械 mm','3.989','11.775','-4.202'],['HMI temp','39.889','117.750','-42.015'],['HMI 整數','40','118','-42']],[53,39,39,39])
p('因此 PathDataOut 第一列為 Y=118、X1=42、X2=40。兩種欄位表示同一組位置，不是資料傳錯。CSV 小數只保留三位，階段间可能有輸出四捨五入差異。')
p('機械與 HMI 階段已套用目前「左側 Y 改成右側 Y」的處理，無法由這些檔案還原覆寫前的左側 Y。','small')

page()
h('4. Modbus CSV 欄位：批次與位址')
p('以下依 CSV 欄位順序說明。每列對應一個暫存器的紀錄，不是一次獨立 Modbus 寫入。')
table([
 ['欄位','意義與本次實例'],
 ['1. timestamp_local','電腦本機批次開始時間。本次 2026-10-06 11:41:29.122。同批兩個 phase 使用相同時間，不是各封包完成時間。'],
 ['2. batch_id','時間、程序 ID、程序內批次序號組合。本次為 2026-10-06 11:41:29.122-45304-1。不是 Modbus Transaction ID。'],
 ['3. phase','PREPARED：傳送前資料；RESULT：完成寫入嘗試後的結果。'],
 ['4. ip','連線設定 IP，本次 192.168.1.10。不能只由此欄判定端點是 PLC 本體或 HMI 閘道。'],
 ['5. port','TCP 連接埠，本次 502。'],
 ['6. unit_id','Modbus Unit Identifier／站號，本次 1。'],
 ['7. function_code','十進位功能碼 16，也就是 0x10，寫入多個 Holding Registers。'],
 ['8. axis','正式傳送軸：X1、Y 或 X2。'],
 ['9. address_zero_based','libmodbus 使用的零起算位址。X1=14~43、Y=44~73、X2=74~103；不是直接顯示為 4xxxx 的參考編號。'],
 ['10. point_1based','該軸內的點號，從 1 到 30。三軸相同點號組成一組座標。'],
 ['11. padded','0：有效描述點；1：重複最後有效點的補點。本次第 26~30 點為補點。'],
],[49,121])
h('位址與點號的對應')
p('X1 位址 = 14 + 點號 - 1\nY 位址 = 44 + 點號 - 1\nX2 位址 = 74 + 點號 - 1')

page()
h('5. Modbus CSV 欄位：數值與結果')
table([
 ['欄位','意義與注意事項'],
 ['12. raw_uint16','實際寫入緩衝區的無號 16 位元 WORD，範圍 0~65535。包含既有取整、範圍截斷及補點結果。負座標以二補數表達。'],
 ['13. raw_hex','同一個 WORD 的十六進位表示。例如 42 = 0x002A，不是另一份座標。'],
 ['14. signed_int16','按二補數解讀的有號值，範圍 -32768~32767，單位 0.1 mm。'],
 ['15. command_mm','signed_int16 ÷ 10。例如 42 → 4.2 mm。這是 PC 命令的解讀，不是伺服回授位置。'],
 ['16. status','PREPARED、SUCCESS、FAILED 或 NOT_ATTEMPTED，搭配 phase 判讀。'],
 ['17. returned_count','該軸整次寫入 API 的回傳值：30 完整成功、-1 錯誤、-2 未嘗試。該軸 30 列會重複同一回傳值。其他非完整回傳值列為 FAILED。'],
 ['18. errno','寫入失敗當下保存的錯誤碼。本次 0。不能單憑 0 判斷成功，必須看 status；非完整回傳也可能 errno=0。'],
 ['19. error','錯誤文字。本次空白；可能記錄連線失敗、Modbus 錯誤或 Short write。'],
],[49,121])
h('phase 與 status 對照')
table([['phase','status','判讀'],['PREPARED','PREPARED','已保存待送資料，不表示已送達。'],['RESULT','SUCCESS','該軸寫入完整回傳 30 registers。'],['RESULT','FAILED','寫入出錯或回傳不足 30。'],['RESULT','NOT_ATTEMPTED','該軸未呼叫寫入，例如前軸失敗或連線未建立。']],[32,43,95])
p('只有 PREPARED、沒有 RESULT：表示結果紀錄不完整，不能推定成功或未送出。SUCCESS 也不代表 PLC 已採用整批路徑，更不代表馬達到位；目前沒有 PLC 回讀或運動完成確認。')
p('負數示例（本批未出現）：raw_uint16=65526、raw_hex=0xFFF6、signed_int16=-10、command_mm=-1.0。','small')

page()
h('6. 本次紀錄核對結果')
table([['項目','核對結果'],['批次','1 個：2026-10-06 11:41:29.122-45304-1'],['資料列','PREPARED 90 列 + RESULT 90 列，共 180 列'],['有效／補點','各軸 25 個有效點 + 5 個補點'],['寫入結果','X1、Y、X2 均 SUCCESS，returned_count=30'],['負數','本批沒有負座標'],['資料一致性','所有 PREPARED／RESULT 座標與 PathDataOut 相符，含補點']],[45,125])
p('180 列不代表傳送 180 個暫存器；同一批 90 個暫存器分別記錄了準備值與結果。三軸各呼叫一次寫入，每次 30 個暫存器。')
h('第一組正式座標')
table([['軸','位址','raw_uint16','raw_hex','signed_int16','mm'],['X1','14','42','0x002A','42','4.2'],['Y','44','118','0x0076','118','11.8'],['X2','74','40','0x0028','40','4.0']],[18,23,35,32,38,24])
p('第 25 組：X1=501（50.1 mm）、Y=2236（223.6 mm）、X2=159（15.9 mm）。第 26~30 組重複此組，padded=1。')
h('7. 實體標定與除錯讀法')
p('先依 batch_id 選取本次批次，再檢查三軸 RESULT 是否全部成功。繪製有效路徑時，使用 phase=RESULT、status=SUCCESS、padded=0，依 point_1based 組合三軸。若其中一軸失敗，不能把剩下的列當作完整路徑。')
p('以平面 X 向右為正、Y 方向與程式一致：\n左側點 = (-X1.command_mm, Y.command_mm)\n右側點 = ( X2.command_mm, Y.command_mm)\n本次第一組：左側 (-4.2, 11.8) mm；右側 (4.0, 11.8) mm。')
p('不要把 raw_uint16 直接當 mm，也不要把 X1、X2 的正值都畫在原點右側。先由 Modbus CSV 確認傳送值，再向前比對 mm 路徑與影像路徑，以區分計算、通訊解讀或機構執行的問題。')
p('依據：x64/Debug 下本次 9 份 CSV；WorkTab.cpp 的 CSV 匯出與 GO 傳送程式；ModbusPathCsv.h。CSV 是持續更新的工作檔，本文件記錄的是上述批次的檢查結果。','small')

def footer(c,d):
 c.saveState(); c.setStrokeColor(colors.HexColor('#CCD9E1')); c.line(20*mm,18*mm,190*mm,18*mm)
 c.setFont('CJK',8); c.setFillColor(navy); c.drawString(20*mm,12*mm,'AX3 CSV 與 Modbus 欄位說明  |  2026-10-06'); c.drawRightString(190*mm,12*mm,str(d.page)); c.restoreState()
doc=SimpleDocTemplate(str(OUT),pagesize=A4,rightMargin=20*mm,leftMargin=20*mm,topMargin=18*mm,bottomMargin=24*mm,title='AX3 CSV 與 Modbus 欄位說明',author='SP-AX3 技術文件')
doc.build(story,onFirstPage=footer,onLaterPages=footer)
print(OUT)

