# Modbus Register Map

## 1. SystemFunction / SystemConfigA

說明：
- 啟動時由 `SystemConfig.ini` 載入到 `m_SystemPara`
- Register 139~144 為 HMI SystemFunction 監控區，由 `WorkTab::SyncHmiData()` 讀取並觸發 UI 動作
- `MachineTab::OpenModBus()` 連線成功後初始化 Register 145~159，角度另寫入 Register 186

對應程式：
- [MachineTab.cpp](/D:/Git%20Repository/chengyi-chung/SP-AX3/MachineTab.cpp)
- [WorkTab.cpp](/D:/Git%20Repository/chengyi-chung/SP-AX3/WorkTab.cpp)
- [UAX.cpp](/D:/Git%20Repository/chengyi-chung/SP-AX3/UAX/UAX.cpp)

| Modbus Register | INI Key | 程式成員 | 備註 |
|---|---|---|---|
| 139 | - | `SystemFunction.Grab` | 0=Stop Grab, 1=Grab |
| 140 | - | `SystemFunction.ImageBinary` | 預留，0/1 |
| 141 | - | `SystemFunction.DisplayROI` | 0=隱藏 ROI, 1=顯示 ROI |
| 142 | - | `SystemFunction.DisplayRefLine` | 0=隱藏 center cross, 1=顯示 center cross |
| 143 | - | `SystemFunction.DiplayPath` | 預留，0/1 |
| 144 | - | `SystemFunction.TabStatus` | 0=Working, 2=SystemPara, 4=Modbus TCP |
| 145 | `OffsetValue` | `OffsetValue` | 寫入前 `lround()` |
| 146 | `BinaryUpper` | `BinaryUpper` | `uint16_t` |
| 147 | `BinaryLower` | `BinaryLower` | `uint16_t` |
| 148 | `MaskX` | `MaskX` | `uint16_t` |
| 149 | `MaskY` | `MaskY` | `uint16_t` |
| 150 | `MaskWidth` | `MaskWidth` | `uint16_t` |
| 151 | `MaskHeight` | `MaskHeight` | `uint16_t` |
| 152 | `StationID` | `StationID` | `uint16_t` |
| 153 | `CameraID` | `CameraID` | `uint16_t` |
| 154 | `RefCenterX` | `RefCenterX` | `uint16_t` |
| 155 | `RefCenterY` | `RefCenterY` | `uint16_t` |
| 156 | `ImageFlip` | `ImageFlip` | `short -> uint16_t` |
| 157 | `CreateToolPath` | `SystemConfigA.CreateToolPath` | CreatePath flag，1=建立路徑，完成後清回 0 |
| 158 | `Binary` | `SystemConfigA.Binary` | 0/1 二值影像顯示 |
| 159 | `SaveINI` | `SystemConfigA.SaveINI` | 1=儲存設定，完成後清回 0 |
| 186 | `CameraToMachineAngle` | `SystemConfigA.CameraToMachineAngle` | WORD，單位為度；相機座標相對機械座標角度，預設 0 |

## 2. SystemConfig.ini 有讀取但目前不寫入 HMI 的欄位

這些值會進入 `m_SystemPara`，但不在 `139~159` 與 `186` 的 Modbus 映射內：

| INI Key | `SystemConfigA` 成員 |
|---|---|
| `IpAddress` | `IpAddress` |
| `Port` | `Port` |
| `TransferFactor` | `TransferFactor` |
| `MACKey` | `MACKey` |
| `GoldenKey` | `GoldenKey` |
| `HMI_ID` | `HMI_ID` |
| `PLC_ID` | `PLC_ID` |
| `CameraWidth` | `CameraWidth` |
| `CameraHeight` | `CameraHeight` |
| `CameraSerialNumber` | `CameraSerialNumber` |
| `MachineType` | `MachineType` |

## 3. MemStruct_SP

說明：
- 這段視為 HMI / PLC 執行期資料
- 啟動後由 HMI 讀回主程式
- `WorkTab::SyncHmiData()` 週期同步到 `m_MemStruct_SP`

對應程式：
- [MachineTab.cpp](/D:/Git%20Repository/chengyi-chung/SP-AX3/MachineTab.cpp)
- [WorkTab.cpp](/D:/Git%20Repository/chengyi-chung/SP-AX3/WorkTab.cpp)

| Modbus Register | `MemStruct_SP` 成員 | 備註 |
|---|---|---|
| 114 | `RecipeID` | `uint16_t` |
| 115 | `CurrentProduction` | `uint16_t` |
| 116 | `Set_temperature0` | `uint16_t` |
| 117 | `Temperature0` | `uint16_t` |
| 118 | `Set_Temperature1` | `uint16_t` |
| 119 | `Temperature1` | `uint16_t` |
| 120 | `Set_temperature2` | `uint16_t` |
| 121 | `Temperature2` | `uint16_t` |
| 122 | `Servo_ALE0` | `uint16_t` |
| 123 | `Servo_ALE1` | `uint16_t` |
| 124 | `Servo_ALE2` | `uint16_t` |
| 125 | `Servo_ALE3` | `uint16_t` |
| 126 | `p19` | `值 x100，WORD` |
| 127 | `i_ProcessingTimeCount` | `uint16_t` |
| 128 | `i_SystemTimeCount` | `uint16_t` |
| 129 | `MachineID` | `uint16_t` |
| 130 | `MachineModel` | `uint16_t` |
| 131 | `Alm_tem_not_reach` | `uint8_t` |
| 132 | `flag_AL_overload` | `uint8_t` |
| 133 | `Alm_airPressureLow` | `uint8_t` |
| 134 | `flag_AL_emergency` | `uint8_t` |
| 135 | `flag_AL_midside_sensor` | `uint8_t` |
| 136 | `Alm_ManualY_GoOut` | `uint8_t` |
| 137 | `MachineStatus` | `uint8_t` |
| 138 | `WorkingMode` | `uint8_t` |

## 4. 目前同步方向

| 區塊 | 位址 | 方向 | 說明 |
|---|---|---|---|
| `SystemConfigA` | `139~159`、`186` | `SystemConfig.ini -> HMI` | 啟動連線後寫入 |
| `SystemConfigA` | `139~159`、`186` | `HMI -> 主程式` | timer 讀回更新 `m_SystemPara` |
| `MemStruct_SP` | `114~138` | `HMI -> 主程式` | timer 讀回更新 `m_MemStruct_SP` |
| `MemStruct_SP` | `114~138` | 啟動時不主動寫入 | 避免覆蓋執行期資料 |

## 5. 備註

- `HMI_ID` / `PLC_ID` 目前保留在 `SystemConfigA` 與 `ini` 內，但 Modbus 讀寫已停用。
- Address 159 保留既有 `SaveINI` 觸發功能；座標角度參數配置於 Address 186。
