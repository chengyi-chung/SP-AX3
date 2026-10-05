# Modbus 路徑輸出 CSV

從主程式所在目錄讀取 `ModbusPath_YYYY-MM-DD.csv`。Release 與 Debug 各有自己的檔案；每日追加，不覆寫前次批次，不受 PathDataOut 開關影響。

目前記錄 GO／自動建立路徑共用的 OnBnClickedIdcWorkGo 三軸寫入：X1=14..43、Y=44..73、X2=74..103；位址為 libmodbus 使用的零起算位址，FC16。不是所有設定／控制暫存器或其他測試傳送函式的通用網路封包紀錄。

每批正常有 180 列：PREPARED 90 列是呼叫寫入前保存的緩衝區；RESULT 90 列是三次寫入的結果。用 batch_id 篩選，再用 phase=RESULT 檢視結果。

- timestamp_local：批次開始的電腦本機時間，不是每一個網路封包的時間。
- ip、port、unit_id：連線設定與本次使用站號。
- axis、address_zero_based、point_1based：軸、位址、1 起算點號。
- padded=1：重複最後有效點的補點。
- raw_uint16／raw_hex：傳送緩衝區中的原始 WORD，包含既有範圍截斷結果。
- signed_int16：以二補數還原的有號數；command_mm 為該值除以 10 的 PC 命令解讀。
- status=SUCCESS：libmodbus 回傳完整 30 registers；FAILED：錯誤或非完整回傳；NOT_ATTEMPTED：前軸失敗或連線失敗，該軸沒有呼叫寫入。
- returned_count：-2 未嘗試、-1 錯誤、30 完整成功。errno／error 保存寫入錯誤。

只有 PREPARED 而沒有 RESULT 代表沒有完整結果紀錄，不能推定未送出或成功。SUCCESS 也不代表 PLC 已採用整批路徑或馬達已到位，本功能未加入 PLC 回讀。

若傳送前无法寫入 CSV，取消本批傳送並提示；若傳送後結果寫入失敗，顯示警告，因為命令可能已經送出。記錄持續追加，需定期封存。啟動後需實際執行 GO 才會產生檔案；建置與離線測試不會產生真實 PLC 傳送紀錄。
