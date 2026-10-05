# PLC 匯出符號清單

來源：Press machine CCD.Device.Application.xml。共 17 群組、367 個符號；不代表完整程式變數，亦不包含當前值或 Modbus 位址。

## GlobVar_Constant

| 變數 | IEC 型別 | 存取 | 註解 |
|---|---|---|---|
| wLMaxX | WORD | ReadWrite | X軸左像素最大值 |
| wLMinX | WORD | ReadWrite | X軸左像素最小值 |
| wRMaxX | WORD | ReadWrite | X軸右像素最大值 |
| wRMinX | WORD | ReadWrite | X軸右像素最小值 |

## GlobVar_DIO

| 變數 | IEC 型別 | 存取 | 註解 |
|---|---|---|---|
| bX0 | BOOL | ReadWrite | DI點(模組64點已用52點,12點待命) / 手動模式SW |
| bX1 | BOOL | ReadWrite | 全自動模式SW |
| bX10 | BOOL | ReadWrite | 腳踏剎車開關FB(B接點) |
| bX11 | BOOL | ReadWrite | 膝頂開關(手動送膠)PB |
| bX12 | BOOL | ReadWrite | 擦膠機構SW |
| bX13 | BOOL | ReadWrite | 緊急停止PB |
| bX14 | BOOL | ReadWrite | 氣壓開關 |
| bX15 | BOOL | ReadWrite | 左中幫上升近接開關 |
| bX16 | BOOL | ReadWrite | 左中幫下降近接開關 |
| bX17 | BOOL | ReadWrite | 右中幫上升近接開關 |
| bX18 | BOOL | ReadWrite | 右中幫下降近接開關 |
| bX19 | BOOL | ReadWrite | 小碼撥桿 |
| bX2 | BOOL | ReadWrite | 第4.5組指壓塊不使用SW |
| bX20 | BOOL | ReadWrite | 左爪後移近接開關 |
| bX21 | BOOL | ReadWrite | 大碼撥桿 |
| bX22 | BOOL | ReadWrite | 右爪後移近接開關 |
| bX23 | BOOL | ReadWrite | 左右腳切換近接開關 |
| bX24 | BOOL | ReadWrite | 馬達啟動信號 |
| bX25 | BOOL | ReadWrite | 馬達過載OL |
| bX26 | BOOL | ReadWrite | 撐台上升停止點近接開關 |
| bX27 | BOOL | ReadWrite | 撐台內拉停止點近接開關 |
| bX28 | BOOL | ReadWrite | 左重疊結幫近接開關 |
| bX29 | BOOL | ReadWrite | 右重疊結幫近接開關 |
| bX3 | BOOL | ReadWrite | 第5組指壓塊不使用SW |
| bX30 | BOOL | ReadWrite | 撐台二次壓開關 |
| bX31 | BOOL | ReadWrite | 左膠桿溫度AL2 |
| bX32 | BOOL | ReadWrite | 右膠桿溫度AL2 |
| bX33 | BOOL | ReadWrite | 快速校點確認訊號(不需要) |
| bX34 | BOOL | ReadWrite | 左爪子上升近接開關 |
| bX35 | BOOL | ReadWrite | 右爪子上升近接開關 |
| bX36 | BOOL | ReadWrite | 左爪上升SW/左膠嘴左移SW |
| bX37 | BOOL | ReadWrite | 左爪下降SW/左膠嘴右移SW |
| bX38 | BOOL | ReadWrite | 右爪上升SW/右膠嘴右移SW |
| bX39 | BOOL | ReadWrite | 右爪下降SW/右膠嘴左移SW |
| bX4 | BOOL | ReadWrite | 膠臂手動推出SW |
| bX40 | BOOL | ReadWrite | 左爪後移SW/膠臂移至上一點SW |
| bX41 | BOOL | ReadWrite | 左爪前移SW/膠臂移至下一點SW |
| bX42 | BOOL | ReadWrite | 右爪後移SW/膠臂移至上一點SW |
| bX43 | BOOL | ReadWrite | 右爪前移SW/膠臂移至下一點SW/快速校點確認 |
| bX44 | BOOL | ReadWrite | 爪子同動(前進)SW |
| bX45 | BOOL | ReadWrite | 爪子同動(後移)SW |
| bX46 | BOOL | ReadWrite | 左中幫上升SW |
| bX47 | BOOL | ReadWrite | 左中幫下降SW |
| bX48 | BOOL | ReadWrite | 右中幫上升SW |
| bX49 | BOOL | ReadWrite | 右中幫下降SW |
| bX5 | BOOL | ReadWrite | 中幫模式SW |
| bX50 | BOOL | ReadWrite | 中幫同動(上升)SW |
| bX51 | BOOL | ReadWrite | 中幫同動(下降)SW |
| bX52 | BOOL | ReadWrite | 膠桿上升近接開關 |
| bX53 | BOOL | ReadWrite | 膠量檢知微動開關 |
| bX54 | BOOL | ReadWrite | 防夾光電開關 |
| bX55 | BOOL | ReadWrite | 待命4 |
| bX56 | BOOL | ReadWrite | 待命5 |
| bX57 | BOOL | ReadWrite | 待命6 |
| bX58 | BOOL | ReadWrite | 待命7 |
| bX59 | BOOL | ReadWrite | 待命8 |
| bX6 | BOOL | ReadWrite | 後幫模式SW |
| bX60 | BOOL | ReadWrite | 待命9 |
| bX61 | BOOL | ReadWrite | 待命10 |
| bX62 | BOOL | ReadWrite | 待命11 |
| bX63 | BOOL | ReadWrite | 待命12 |
| bX7 | BOOL | ReadWrite | 雙手按鈕(左)PB |
| bX8 | BOOL | ReadWrite | 雙手按鈕(右)PB |
| bX9 | BOOL | ReadWrite | 腳踏啟動開關FB |
| bY0 | BOOL | ReadWrite | DO點(模組64點已用44點,20點待命) / 左爪前移 |
| bY1 | BOOL | ReadWrite | 左爪後移 |
| bY10 | BOOL | ReadWrite | 第4組指壓塊內移 |
| bY11 | BOOL | ReadWrite | 第5組指壓塊內移 |
| bY12 | BOOL | ReadWrite | 指壓塊1次低壓 |
| bY13 | BOOL | ReadWrite | 指壓塊2次高壓 |
| bY14 | BOOL | ReadWrite | 指壓塊3次低壓 |
| bY15 | BOOL | ReadWrite | 掃刀 |
| bY16 | BOOL | ReadWrite | 撐台1次上升 |
| bY17 | BOOL | ReadWrite | 撐台2次上升 |
| bY18 | BOOL | ReadWrite | 撐台下降 |
| bY19 | BOOL | ReadWrite | 撐台下降洩壓 |
| bY2 | BOOL | ReadWrite | 右爪前移 |
| bY20 | BOOL | ReadWrite | 撐台1次高壓內拉 |
| bY21 | BOOL | ReadWrite | 撐台2次低壓內拉 |
| bY22 | BOOL | ReadWrite | 撐台2次內拉慢速(三次壓) |
| bY23 | BOOL | ReadWrite | 撐台後退 |
| bY24 | BOOL | ReadWrite | 束緊器低壓 |
| bY25 | BOOL | ReadWrite | 束緊器高壓 |
| bY26 | BOOL | ReadWrite | 左爪內拉 |
| bY27 | BOOL | ReadWrite | 膠桿浮動/控制膠桿浮動 |
| bY28 | BOOL | ReadWrite | 膠桿下降 |
| bY29 | BOOL | ReadWrite | 定位桿 |
| bY3 | BOOL | ReadWrite | 右爪後移 |
| bY30 | BOOL | ReadWrite | 爪子夾緊 |
| bY31 | BOOL | ReadWrite | 左爪上升 |
| bY32 | BOOL | ReadWrite | 左爪下降 |
| bY33 | BOOL | ReadWrite | 右爪上升 |
| bY34 | BOOL | ReadWrite | 右爪下降 |
| bY35 | BOOL | ReadWrite | 爪子升降慢速 |
| bY36 | BOOL | ReadWrite | 頂針 |
| bY37 | BOOL | ReadWrite | 右爪內拉 |
| bY38 | BOOL | ReadWrite | 第4組指壓塊不使用 |
| bY39 | BOOL | ReadWrite | 第5組指壓塊不使用 |
| bY4 | BOOL | ReadWrite | 左中幫上升 |
| bY40 | BOOL | ReadWrite | 左氣壓偏擺 |
| bY41 | BOOL | ReadWrite | 右氣壓偏擺 |
| bY42 | BOOL | ReadWrite | 送膠第一段速度(不需要) |
| bY43 | BOOL | ReadWrite | 送膠第二段速度(不需要) |
| bY44 | BOOL | ReadWrite | 三色燈(綠燈) |
| bY45 | BOOL | ReadWrite | 三色燈(黃燈) |
| bY46 | BOOL | ReadWrite | 三色燈(紅燈) |
| bY47 | BOOL | ReadWrite | 蜂鳴器 |
| bY48 | BOOL | ReadWrite | 待命5 |
| bY49 | BOOL | ReadWrite | 待命6 |
| bY5 | BOOL | ReadWrite | 左中幫下降 |
| bY50 | BOOL | ReadWrite | 待命7 |
| bY51 | BOOL | ReadWrite | 待命8 |
| bY52 | BOOL | ReadWrite | 待命9 |
| bY53 | BOOL | ReadWrite | 待命10 |
| bY54 | BOOL | ReadWrite | 待命11 |
| bY55 | BOOL | ReadWrite | 待命12 |
| bY56 | BOOL | ReadWrite | 待命13 |
| bY57 | BOOL | ReadWrite | 待命14 |
| bY58 | BOOL | ReadWrite | 待命15 |
| bY59 | BOOL | ReadWrite | 待命16 |
| bY6 | BOOL | ReadWrite | 右中幫上升 |
| bY60 | BOOL | ReadWrite | 待命17 |
| bY61 | BOOL | ReadWrite | 待命18 |
| bY62 | BOOL | ReadWrite | 待命19 |
| bY63 | BOOL | ReadWrite | 待命20 |
| bY7 | BOOL | ReadWrite | 右中幫下降 |
| bY8 | BOOL | ReadWrite | 第1組指壓塊內移 |
| bY9 | BOOL | ReadWrite | 第2.3組指壓塊內移 |

## GlobVar_Recipe

| 變數 | IEC 型別 | 存取 | 註解 |
|---|---|---|---|
| aLineRecipe | ARRAY [400..550] OF WORD | ReadWrite | 人機線條緩衝區 |
| aMScaleBuffer | ARRAY [0..2, 0..1] OF LREAL | ReadWrite | 級放主要陣列 |
| aRPointBuffer | ARRAY [0..199] OF WORD | ReadWrite | 讀取人機配方陣列 |
| aRScaleBuffer | ARRAY [0..1] OF LREAL | ReadWrite | 級放陣列讀取緩衝區 |
| bHMIStart | BOOL | ReadWrite | 人機進入畫面判斷旗標 |
| byGauge | WORD | ReadWrite | 級放規格 |
| wDeg | WORD | ReadWrite | 旋轉角度 |

## PRG_AllInit

| 變數 | IEC 型別 | 存取 | 註解 |
|---|---|---|---|
| bReInit | BOOL | ReadWrite | 回復出廠值訊號 |
| wSub1 | WORD | ReadWrite | 次版號 |
| wSub2 | WORD | ReadWrite | 子版號 |
| wVersion | WORD | ReadWrite | 主版號 |

## PRG_CCD

| 變數 | IEC 型別 | 存取 | 註解 |
|---|---|---|---|
| aCCD | ARRAY [14..103] OF WORD | ReadWrite |  |
| bCapture | BOOL | ReadWrite | 拍攝訊號 |

## PRG_Change

| 變數 | IEC 型別 | 存取 | 註解 |
|---|---|---|---|
| bCLJUUse | BOOL | ReadWrite | 左腰上拉選擇鏡像(半自動/自動) |
| bCRJUUse | BOOL | ReadWrite | 右腰上拉選擇鏡像(半自動/自動) |
| bLOffset | BOOL | ReadWrite | 左偏功能接點 |
| bROffset | BOOL | ReadWrite | 右偏功能接點 |
| wLOffset | WORD | ReadWrite | 左偏移位置(人機) |
| wROffset | WORD | ReadWrite | 右偏移位置(人機) |

## PRG_IOMapping

| 變數 | IEC 型別 | 存取 | 註解 |
|---|---|---|---|
| b1BT | BOOL | ReadWrite | 結束點位選擇開關1 |
| b1PressDo | BOOL | ReadWrite | 第一指壓塊允許發送訊號 |
| b1SPBlock | BOOL | ReadWrite | 第一組指壓塊內移(手動) |
| b23PressDo | BOOL | ReadWrite | 第二三指壓塊允許發送訊號 |
| b23SPBlock | BOOL | ReadWrite | 第二三組指壓塊內移(手動) |
| b2BT | BOOL | ReadWrite | 結束點位選擇開關2 |
| b3BT | BOOL | ReadWrite | 結束點位選擇開關3 |
| b4BT | BOOL | ReadWrite | 結束點位選擇開關4 |
| b4PressDo | BOOL | ReadWrite | 第四指壓塊允許發送訊號 |
| b4SPBlock | BOOL | ReadWrite | 第四組指壓塊內移(手動) |
| b5BT | BOOL | ReadWrite | 結束點位選擇開關5 |
| b5PressDo | BOOL | ReadWrite | 第五指壓塊允許發送訊號 |
| b5SPBlock | BOOL | ReadWrite | 第五組指壓塊內移(手動) |
| b6BT | BOOL | ReadWrite | 結束點位選擇開關6 |
| b7BT | BOOL | ReadWrite | 結束點位選擇開關7 |
| bAJaw | BOOL | ReadWrite | 夾爪前後選用訊號 |
| bAPress | BOOL | ReadWrite | 中幫上下選用訊號 |
| bAxisErr | BOOL | ReadWrite | 軸異常訊號 |
| bBGlue | BOOL | ReadWrite | 後根補膠功能開啟訊號 |
| bChangeFlag | BOOL | ReadWrite | 配方儲存避免誤判訊號 |
| bLaserUse | BOOL | ReadWrite | 雷射使用訊號 |
| bNoAirInGlue | BOOL | ReadWrite | 擦膠氣壓無開啟異常訊號 |
| bNoGlue | BOOL | ReadWrite | 開啟擦膠後膠量檢知訊號 |
| bOnePush | BOOL | ReadWrite | 一次按鈕 |
| bPressInit | BOOL | ReadWrite | 第一指指壓塊未回復訊號 |
| bPreX2JogB | BOOL | ReadWrite | X2軸點動閉訊號 |
| bPreX2JogF | BOOL | ReadWrite | X2軸點動開訊號 |
| bPreXJogB | BOOL | ReadWrite | X軸點動閉訊號 |
| bPreXJogF | BOOL | ReadWrite | X軸點動開訊號 |
| bRecipeR | BOOL | ReadWrite | 配方讀取時關閉類比回填 |
| bSAPlatform | BOOL | ReadWrite | 撐台慢速內拉(半自動/自動) |
| bSFastF | BOOL | ReadWrite | 膠桿快速前進(手動) |
| bSFolder | BOOL | ReadWrite | 夾爪夾緊接點(手動) |
| bSFPlatform | BOOL | ReadWrite | 撐台快速內拉接點(手動) |
| bSFPlatformD | BOOL | ReadWrite | 撐台快速下降接點(手動) |
| bSFPlatformU | BOOL | ReadWrite | 撐台快速上升訊號(手動) |
| bSGlueNB | BOOL | ReadWrite | 膠桿後退(手動) |
| bSGlueNOff | BOOL | ReadWrite | 同動膠嘴閉(手動) |
| bSGlueNOn | BOOL | ReadWrite | 同動膠嘴開(手動) |
| bSGluePD | BOOL | ReadWrite | 膠桿下降訊號(手動) |
| bSGluePos | BOOL | ReadWrite | 同動膠嘴閉至擦膠點 |
| bSingleSection | BOOL | ReadWrite | 單節模式(2/3步) |
| bSJawD | BOOL | ReadWrite | 夾爪下降(手動) |
| bSJawPull | BOOL | ReadWrite | 夾爪內拉訊號(手動) |
| bSJawU | BOOL | ReadWrite | 夾爪上升(手動) |
| bSLGlueNOn | BOOL | ReadWrite | 左膠嘴開(手動) |
| bSPBlock2PHigh | BOOL | ReadWrite | 指壓塊2次高壓(手動) |
| bSPBlock3PHigh | BOOL | ReadWrite | 指壓塊3次高壓(手動) |
| bSPBlockPLow | BOOL | ReadWrite | 指壓塊低壓(手動) |
| bSPlatformB | BOOL | ReadWrite | 撐台後退接點(手動) |
| bSPPoleD | BOOL | ReadWrite | 定位桿下降接點(手動) |
| bSPrePos | BOOL | ReadWrite | 同動膠嘴開至預備點 |
| bSRGlueNOn | BOOL | ReadWrite | 右膠嘴開(手動) |
| bSSlowF | BOOL | ReadWrite | 膠桿慢速前進(手動) |
| bSSPlatform | BOOL | ReadWrite | 撐台慢速內拉接點(手動) |
| bSSPlatformD | BOOL | ReadWrite | 撐台慢速下降接點(手動) |
| bSSPlatformU | BOOL | ReadWrite | 撐台慢速上升訊號(手動) |
| bSSweeping | BOOL | ReadWrite | 掃刀接點(手動) |
| bSThimble | BOOL | ReadWrite | 頂針控制接點(手動) |
| bSTigHig | BOOL | ReadWrite | 束緊器高壓訊號(手動) |
| bSTigLow | BOOL | ReadWrite | 束緊器低壓訊號(手動) |
| bSweepLimit | BOOL | ReadWrite | 中幫模式掃刀限制功能 |
| bThirPlatF | BOOL | ReadWrite | 三次內拉訊號 |
| iLaserMax | INT | ReadWrite | 雷射最大值 |
| iLaserMin | INT | ReadWrite | 雷射原點 |
| iLaserP | INT | ReadWrite | 雷射當前位置 |
| iPressB | INT | ReadWrite | 後幫結束位置 |
| iPressM | INT | ReadWrite | 中幫開始位置 |
| lrJogYB | LREAL | ReadWrite | Y軸回退速度 |
| wAnLJawMax | WORD | ReadWrite | 左爪電阻尺最大值 |
| wAnLJawMin | WORD | ReadWrite | 左爪電阻尺最小值 |
| wAnLMax | WORD | ReadWrite | 左中幫電阻尺最大值 |
| wAnLMin | WORD | ReadWrite | 左中幫電阻尺最小值 |
| wAnRJawMax | WORD | ReadWrite | 右爪電阻尺最大值 |
| wAnRJawMin | WORD | ReadWrite | 右爪電阻尺最小值 |
| wAnRMax | WORD | ReadWrite | 右中幫電阻尺最大值 |
| wAnRMin | WORD | ReadWrite | 右中幫電阻尺最小值 |
| wAxisYfsetP0 | WORD | ReadWrite | Y軸設定位置0 |
| wAxisYfsetP1 | WORD | ReadWrite | Y軸設定位置1 |
| wAxisYfsetP2 | WORD | ReadWrite | Y軸設定位置2 |
| wAxisYfsetP3 | WORD | ReadWrite | Y軸設定位置3 |
| wGlueRun | WORD | ReadWrite | 出膠馬達時間 |
| wMaMode | WORD | ReadWrite | 0:手動/1:半自動/2:全自動 |
| wMaStat | WORD | ReadWrite | 0:正常/1:溫度未到達/2:機器急停/3:機器故障 |
| wRestartT | WORD | ReadWrite | 重啟時間 |

## PRG_Main

| 變數 | IEC 型別 | 存取 | 註解 |
|---|---|---|---|
| b1PressOffUSE | BOOL | ReadWrite | 第一指壓塊關閉使用訊號 |
| bAPole2USE | BOOL | ReadWrite | 定位桿二次使用訊號 |
| bAPole3USE | BOOL | ReadWrite | 定位桿三次使用訊號 |
| bCReset | BOOL | ReadWrite | 重置工件完成計數訊號 |
| bCycle | BOOL | ReadWrite | 循環測試訊號 |
| bInsidePress | BOOL | ReadWrite | 窄內腰功能開啟訊號 |
| bJawUandPull | BOOL | ReadWrite | 夾爪內拉與上拉優先選擇開關 |
| bJawUSE | BOOL | ReadWrite | 夾爪選用選擇開關 |
| bPressUSE | BOOL | ReadWrite | 指壓塊使用功能訊號 |
| bRstlwcom | BOOL | ReadWrite | 重置出機完成計數 |
| bSPlatformD | BOOL | ReadWrite | 掃刀出,撐台延時計時功能 |
| lwCom | LWORD | ReadWrite | 出機後總加工次數 |
| lwComPress | LWORD | ReadWrite | 機台加工次數 |
| w1APBlockT | WORD | ReadWrite | 第一組指壓塊時間(自動) |
| w1PBlockT | WORD | ReadWrite | 第一組指壓塊時間(半自動) |
| w1PressOffT | WORD | ReadWrite | 第一指壓塊關閉時間 |
| w23APBlockT | WORD | ReadWrite | 第二三組指壓塊時間(自動) |
| w23PBlockT | WORD | ReadWrite | 第二三組指壓塊時間(半自動) |
| w3PressT | WORD | ReadWrite | 三次壓計時時間 |
| w4APBlockT | WORD | ReadWrite | 第四組指壓塊時間(自動) |
| w4PBlockT | WORD | ReadWrite | 第四組指壓塊時間(半自動) |
| w5APBlockT | WORD | ReadWrite | 第五組指壓塊時間(自動) |
| w5PBlockT | WORD | ReadWrite | 第五組指壓塊時間(半自動) |
| wFolderT | WORD | ReadWrite | 夾爪放開時間 |
| wJawLF | WORD | ReadWrite | 左夾爪前移時間 |
| wJawPull | WORD | ReadWrite | 夾爪內拉時間 |
| wJawRF | WORD | ReadWrite | 右夾爪前移時間 |
| wJawUT | WORD | ReadWrite | 夾爪上升時間 |
| wJawUU | WORD | ReadWrite | 夾爪上拉延遲計時器 |
| wLJawUUse | WORD | ReadWrite | 左腰上拉時間 |
| wMotionReset | WORD | ReadWrite | 膠桿回待機點時間 |
| wPBlock2PH | WORD | ReadWrite | 指壓塊2次高壓時間 |
| wPBlockPLow | WORD | ReadWrite | 指壓塊低壓時間 |
| wPlatformB | WORD | ReadWrite | 撐台後退時間 |
| wPoleDT | WORD | ReadWrite | 定位桿二次定位下沉時間 |
| wPoleR3 | WORD | ReadWrite | 定位桿三次下降回復延遲時間 |
| wPressT | WORD | ReadWrite | 指壓塊壓著時間 |
| wRJawUUse | WORD | ReadWrite | 右腰上拉時間 |
| wSPlatformD | WORD | ReadWrite | 掃刀出,撐台延時時間 |
| wSPlatformU | WORD | ReadWrite | 撐台慢速上升時間 |
| wSTig | WORD | ReadWrite | 束緊器延時束緊時間 |
| wSTigOff | WORD | ReadWrite | 束緊器放開時間 |
| wTimeStart | WORD | ReadWrite | 計時時間 |

## PRG_MotionControl

| 變數 | IEC 型別 | 存取 | 註解 |
|---|---|---|---|
| aBufferPoint | ARRAY [0..29, 0..2] OF BOOL | ReadWrite | New點位到位旗標 |
| bGlueUSE | BOOL | ReadWrite | 擦膠使用訊號 |
| bPCounter | BOOL | ReadWrite | 鞋子點位計數訊號 |
| bXUse | BOOL | ReadWrite | X軸使用訊號 |
| iGluePos | INT | ReadWrite | Y軸出膠位置 |
| iGluePosX | INT | ReadWrite | X軸出膠位置 |
| iGluePosX2 | INT | ReadWrite | X2軸出膠位置 |
| iOutGluePos | INT | ReadWrite | 提早斷膠位置 |
| iPrePos | INT | ReadWrite | Y軸預備點 |
| iPrePosX | INT | ReadWrite | X軸預備點 |
| iPrePosX2 | INT | ReadWrite | X2軸預備點 |
| iYGluePos1 | INT | ReadWrite | 第一段擦膠位置 |
| iYGluePos2 | INT | ReadWrite | 第二段擦膠位置 |
| lrGule | LREAL | ReadWrite | 送膠速度 |
| lrIntV0 | LREAL | ReadWrite | 彎曲處運動速度 |
| lrIntV1 | LREAL | ReadWrite | 前段運動速度 |
| lrIntV2 | LREAL | ReadWrite | 中段運動速度 |
| lrIntV3 | LREAL | ReadWrite | 後段運動速度 |
| lrPreV | LREAL | ReadWrite | 回預備點速度 |
| wGluePos | WORD | ReadWrite | 中後/後幫擦膠點停留時間 |
| wGluePosX5 | WORD | ReadWrite | 中幫擦膠點停留時間 |

## PRG_MotionConvert

| 變數 | IEC 型別 | 存取 | 註解 |
|---|---|---|---|
| wAxisX2factP | DINT | ReadWrite | X2軸當下位置(mm) |
| wAxisXfactP | DINT | ReadWrite | X軸當下位置(mm) |
| wAxisYfactP | DINT | ReadWrite | Y軸當下位置(mm) |

## PRG_MotionInit

| 變數 | IEC 型別 | 存取 | 註解 |
|---|---|---|---|
| bGetHPos | BOOL | ReadWrite | 獲取Y軸原點座標 |
| bSetHPos | BOOL | ReadWrite | 設定Y軸原點訊號 |
| bStartHome | BOOL | ReadWrite | 回原點訊號 |
| strSerialNumber | STRING | ReadWrite | 產品序號 |

## PRG_MultiPMove

| 變數 | IEC 型別 | 存取 | 註解 |
|---|---|---|---|
| aLChose | ARRAY [14..43] OF BOOL | ReadWrite | 多點偏移左半邊選擇點位陣列 |
| aRChose | ARRAY [74..103] OF BOOL | ReadWrite | 多點偏移右半邊選擇點位陣列 |
| bLAdd | BOOL | ReadWrite | 多點偏移左半邊遞增接點 |
| bLAllChose | BOOL | ReadWrite | 左半邊全部勾選訊號 |
| bLRotate | BOOL | ReadWrite | 順時針旋轉訊號 |
| bLSub | BOOL | ReadWrite | 多點偏移左半邊遞減接點 |
| bRAdd | BOOL | ReadWrite | 多點偏移右半邊遞增接點 |
| bRAllChose | BOOL | ReadWrite | 右半邊全部勾選訊號 |
| bRRotate | BOOL | ReadWrite | 逆時針旋轉訊號 |
| bRSub | BOOL | ReadWrite | 多點偏移右半邊遞減接點 |

## PRG_QuickCorr

| 變數 | IEC 型別 | 存取 | 註解 |
|---|---|---|---|
| bQuickCorr | BOOL | ReadWrite | 快速校點模式 |
| bWQuiP | BOOL | ReadWrite | 快速校正點位寫入/移至下一點 |

## PRG_Read

| 變數 | IEC 型別 | 存取 | 註解 |
|---|---|---|---|
| bCErr | BOOL | ReadWrite | C軸異常訊號 |
| bX2Err | BOOL | ReadWrite | X2軸異常訊號 |
| bXErr | BOOL | ReadWrite | X軸異常訊號 |
| bYErr | BOOL | ReadWrite | Y軸異常訊號 |
| dwAALID | DWORD | ReadWrite | A軸異常訊息 |
| dwCALID | DWORD | ReadWrite | C軸異常訊息 |
| dwXALID | DWORD | ReadWrite | X軸異常訊息 |
| dwYALID | DWORD | ReadWrite | Y軸異常訊息 |

## PRG_Recipe

| 變數 | IEC 型別 | 存取 | 註解 |
|---|---|---|---|
| aPToInter | ARRAY [0..29, 0..2] OF LREAL | ReadWrite | 運動點位陣列 |
| bClear | BOOL | ReadWrite | 配方清除訊號 |
| bPointRecipeR | BOOL | ReadWrite | 點位配方讀取輸出訊號 |
| bScaleW | BOOL | ReadWrite | 級放配方寫入輸出訊號 |
| bZeroArray | BOOL | ReadWrite | 判斷陣列是否有資料旗標 |

## PRG_Stop

| 變數 | IEC 型別 | 存取 | 註解 |
|---|---|---|---|
| bReset | BOOL | ReadWrite | 重置異常訊號 |

## PRG_Zoom

| 變數 | IEC 型別 | 存取 | 註解 |
|---|---|---|---|
| aZoomEU | ARRAY [0..26] OF BOOL | ReadWrite | 歐規級放觸發訊號(人機) |
| aZoomTW | ARRAY [0..18] OF BOOL | ReadWrite | 中/日規級放觸發訊號(人機) |
| aZoomUS | ARRAY [0..23] OF BOOL | ReadWrite | 美規級放觸發訊號(人機) |
| lrEUNowX | LREAL | ReadWrite | 歐規當前X軸級放數值 |
| lrEUNowY | LREAL | ReadWrite | 歐規當前Y軸級放數值 |
| lrStanLev | LREAL | ReadWrite | 標準楦頭等級 |
| lrStanLevTran | LREAL | ReadWrite | 經轉換後的標準楦頭等級 |
| lrTWNowX | LREAL | ReadWrite | 中日規當前X軸級放數值 |
| lrTWNowY | LREAL | ReadWrite | 中日規當前Y軸級放數值 |
| lrUSNowX | LREAL | ReadWrite | 美規當前X軸級放數值 |
| lrUSNowY | LREAL | ReadWrite | 美規當前Y軸級放數值 |
| lrXScale | LREAL | ReadWrite | X軸級放距離 |
| lrYScale | LREAL | ReadWrite | Y軸級放距離 |
| strLevelHMI | STRING | ReadWrite | 級放等級映射至人機 |
| wEUMX0 | LREAL | ReadWrite |  |
| wEUMX1 | LREAL | ReadWrite |  |
| wEUMX2 | LREAL | ReadWrite |  |
| wEUMY0 | LREAL | ReadWrite |  |
| wEUMY1 | LREAL | ReadWrite |  |
| wEUMY2 | LREAL | ReadWrite |  |
| wLevel | WORD | ReadWrite | 級放等級 |
| wLevelHMI | WORD | ReadWrite | 人機顯示級放等級 |
| wTWMX0 | LREAL | ReadWrite |  |
| wTWMX1 | LREAL | ReadWrite |  |
| wTWMX2 | LREAL | ReadWrite |  |
| wTWMY0 | LREAL | ReadWrite |  |
| wTWMY1 | LREAL | ReadWrite |  |
| wTWMY2 | LREAL | ReadWrite |  |
| wUSMX0 | LREAL | ReadWrite |  |
| wUSMX1 | LREAL | ReadWrite |  |
| wUSMX2 | LREAL | ReadWrite |  |
| wUSMY0 | LREAL | ReadWrite |  |
| wUSMY1 | LREAL | ReadWrite |  |
| wUSMY2 | LREAL | ReadWrite |  |
