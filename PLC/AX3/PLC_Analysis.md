# AX3 PLC 靜態資料分析

分析日期：2026-09-16。來源：本目錄 PLC 檔案，並交叉比對目前工作目錄的 WorkTab.cpp、HMI_Data_Format.md、Modbus_Register_Map.md；PC 程式含既有未提交修改，本報告反映目前檔案內容。

## 1. 結論與分析範圍

這是具有視覺取像、伺服送膠、鞋型配方、級放與結幫機構的 DIADesigner-AX／CODESYS 專案。可直接解析的資料為符號 XML 與 TGS 內的專案描述，共 17 個符號群組、367 個匯出變數（陣列以一個變數計），全部標為 ReadWrite。

主 .project 為二進位容器，未能當作 ZIP 開啟；本次未解析其程式本體。以下功能分類依符號名稱與註解，不能視為已驗證的 PLC 呼叫順序。未連線設備、下載程式、寫入暫存器或修改原始 PLC 檔案。

無法由現有可讀資料確認：PLC 精確硬體型號、Task 週期與優先序、完整 POU 實作、實體 I/O 位址、通訊映射、軸參數、保持變數、計時單位及安全互鎖實作。AX3 是目錄名称，不能單憑它認定硬體型號。

## 2. 檔案與版本

| 檔案 | 可確認用途／內容 |
|---|---|
| Press machine CCD.project | 主專案二進位檔，約 2.30 MB |
| Press machine CCD.Device.Application.xml | 匯出符號、型別、陣列上下界與註解；不含程式邏輯或目前數值 |
| Press machine CCD.tgs | ZIP 容器，包含 Project.json 及符號 XML；內部 XML 與外部 XML 位元組完全一致 |
| *.compileinfo、*.bootinfo | 編譯／啟動相關二進位資料，各 21,481,555 bytes；未解析其內容 |
| *.precompilecache、*.opt、*.bootinfo_guids、*.project.~u | 快取、設定及其他輔助檔；不是可直接閱讀的 PLC 程式來源 |

XML 記錄 profile 為 DIADesigner-AX 1.10+，compiler/lmm 為 3.5.18.50，runtimeid 為 3.5.18.30，SymbolConfigObject/libversion 為 4.3.0.0。這些是檔案記錄，非線上設備版本確認。

TGS 的標題為 Servo-motor feeding with visual system，namespace 為 SP-666 Series；描述指出由 SP-656 的液壓送料改為伺服送料並增加視覺。專案版本 1.0.0.0，Device 記錄版本 1.0.7.4，兩者不可混為應用程式或韌體版本。Compiled、Downloaded 為 true，ChangedAfterCodeGenerated 為 false；這些匯出時的旗標不能證明目前現場 PLC 與檔案一致。

## 3. 功能組成

| 群組 | 匯出變數數 | 依名稱與註解判讀的職責 |
|---|---:|---|
| GlobVar_Constant | 4 | 左右 X 像素上下限 |
| GlobVar_DIO | 128 | bX0–bX63、bY0–bY63 |
| GlobVar_Recipe | 7 | 人機配方、線條、級放緩衝區與旋轉角度 |
| PRG_AllInit | 4 | 回復出廠設定與版本欄位 |
| PRG_CCD | 2 | 拍攝訊號及 CCD 資料陣列 |
| PRG_Change | 6 | 左右選用／鏡像及偏移 |
| PRG_IOMapping | 85 | 手動命令、功能選用、機台模式／狀態、感測器校正參數 |
| PRG_Main | 43 | 夾爪、指壓塊、撐台、定位桿等時序參數與加工計數 |
| PRG_MotionControl | 21 | X／X2／Y 預備點、出膠點、路徑速度與到位旗標 |
| PRG_MotionConvert | 3 | X／X2／Y 目前位置，DINT，註解單位 mm |
| PRG_MotionInit | 4 | 原點設定、回原點、產品序號 |
| PRG_MultiPMove | 10 | 左右各 30 點選取與偏移／旋轉命令 |
| PRG_QuickCorr | 2 | 快速校點及下一點／寫入訊號 |
| PRG_Read | 8 | 軸錯誤旗標與錯誤碼 |
| PRG_Recipe | 5 | 運動點位、配方清除／讀取、級放寫入 |
| PRG_Stop | 1 | 異常重置 |
| PRG_Zoom | 34 | 歐規、中日規、美規鞋碼級放 |

功能關係可概念化為「取像／路徑輸入 → 配方與級放／偏移 → 運動點位 → 運動與機構控制」，但 PRG_CCD 到 PRG_Recipe 的資料複製、轉換方式及先後順序尚無程式本體可驗證。

## 4. CCD 與 PC 資料介面

PLC 明確宣告 PRG_CCD.aCCD 為 ARRAY [14..103] OF WORD，合計 90 個 WORD、180 bytes；PRG_Recipe.aPToInter 為 ARRAY [0..29, 0..2] OF LREAL，合計 90 個 LREAL、720 bytes。另有同維度 BOOL 到位陣列 aBufferPoint。

PC 的 WorkTab::OnBnClickedIdcWorkGo() 使用以下配置：

| PC Modbus 起迄位址 | 筆數 | 寫入內容 |
|---|---:|---|
| 14–43 | 30 | X1 = -PathLeft.x |
| 44–73 | 30 | 共用 Y = PathLeft.y |
| 74–103 | 30 | X2 = PathRight.x |

PC 最多採用 25 個描述點，未用欄位補最後一個有效點至 30 筆；若只有 20 點，則第 21–30 點均補末點。資料容量及索引與 PLC 符號相符，但 XML 陣列索引不是 Modbus 映射證據，實際經 HMI 或直接進 PLC 的映射仍須查設備設定。

傳送前 PC 讀取位址 186 的相機到機械角度，再重建座標。XML 的 GlobVar_Recipe.wDeg 只有「旋轉角度」註解，不能直接認定它就是位址 186。

現行此傳送函式會四捨五入，再限制至 -32768..32767，轉成 int16_t 後以 uint16_t 位元模式送出；此處沒有乘以 100。上游座標比例及 PLC 最終工程單位需查轉換實作。HMI_Data_Format.md 另述的 32 位元 XYZ、每點 6 registers、乘以 100 的格式，不可套用到這個 90 WORD 介面。

## 5. 需優先確認的項目

### A. 有號座標解讀（高優先）

PC 明確允許負座標，但 aCCD 宣告為 WORD。WORD 作為傳輸位元容器本身不代表錯誤；關鍵是 PLC 是否先依 INT16 解讀，再轉為 LREAL。例如 -100 送出為 0xFF9C，若被當成無號數轉換，會變成 65436。需查 PRG_CCD／PRG_Recipe 的實作，確認完整轉型路徑與比例。

### B. 整批資料一致性（高優先）

PC 依序分三次寫入 X1、Y、X2。任一後續寫入失敗時，前段資料已可能更新；PC mutex 只保護 PC 的共用連線，不能阻止 PLC 掃描中途讀取。函式最後呼叫 ClearCreateToolPathRequest，但目前資料不足以證明 PLC 以它作為整批提交條件。需確認 PLC／HMI 是否有接收緩衝、完成旗標及失敗時保留舊完整路徑的機制。

### C. 站號使用不一致

同一 PC 傳送函式讀取角度時使用 m_SystemPara.StationID，寫路徑時固定 stationID = 1。若設定值不是 1，兩段交易使用不同站號；影響取決於伺服器的站號路由設定。這是 PC 端可直接確認的不一致，非 PLC 邏輯缺陷判定。

### D. 點數與補點規則

PLC 匯出陣列可容納 30 點，但不能据此認定它只執行前 25 點。需核對 PLC 是否處理全部 30 點、重複尾點是否附帶停留／送膠／到位觸發，以及有無額外有效點數或結束條件。

### E. I/O 註解過期

DI 總註解寫「已用52點、12點待命」，逐點卻已命名至 bX54，只有 bX55–63 共 9 點標待命；其中 bX33 又註明不需要。DO 總註解寫「已用44點、20點待命」，但 bY44–47 已配置三色燈及蜂鳴器，只有 bY48–63 共16點標待命，bY42–43 另註明不需要。這是文件不一致，不能用標籤直接推定實際接線數。

### F. 匯出存取權與命名

367 個符號全部標為 ReadWrite，包含 I/O、狀態及復位命令。這說明匯出介面的權限設定，並不證明已啟用網路服務或匿名存取。需依實際 HMI／OPC UA 使用需求區分可寫命令與唯讀狀態。

PRG_Read 中存在 bX2Err，但錯誤碼欄位是 dwAALID／dwCALID／dwXALID／dwYALID，需核對 A 與 X2 的對應，不能依命名猜測軸映射。

## 6. I/O 與模式重點

- bX7／bX8：左右雙手按鈕；bX9：腳踏啟動；bX10：腳踏剎車（B接點）。
- bX13：緊急停止；bX14：氣壓；bX25：馬達過載；bX31／bX32：左右膠桿溫度 AL2；bX54：防夾光電。
- bY0–7：左右夾爪前後及中幫升降；bY8–14：指壓塊與壓力段；bY15：掃刀。
- bY16–25：撐台及束緊器；bY26–41：夾爪、膠桿、定位桿、頂針與偏擺等。
- wMaMode：0 手動、1 半自動、2 全自動。
- wMaStat：0 正常、1 溫度未到達、2 急停、3 故障。

上述均是符號註解，未驗證實際輸入極性、互鎖、急停回路或狀態優先級。

## 7. 深入程式審查所需資料

以相容版本 DIADesigner-AX 開啟主專案，匯出包含實作的 PLCopen XML／文字 POU，並保留 Device Tree、Task Configuration、I/O Mapping、通訊映射及軸設定。優先取得 PRG_CCD、PRG_Recipe、PRG_MotionControl、PRG_Stop、PRG_Main、PRG_IOMapping。

取得後即可追查：WORD→INT→LREAL 轉換、資料提交握手、30點運動與補點行為、模式切換、異常復歸，以及各軸故障時如何停止相關動作。本次沒有執行建置、模擬或機台動作測試。

完整匯出符號清單見 PLC_Symbols.md。
