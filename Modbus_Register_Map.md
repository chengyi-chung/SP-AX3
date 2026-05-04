# Modbus Register Map

## 1. SystemConfigA

說明：
- 啟動時由 `SystemConfig.ini` 載入到 `m_SystemPara`
- `MachineTab::OpenModBus()` 連線成功後，將這一段寫入 HMI
- `WorkTab::SyncHmiData()` 之後也會由 HMI 讀回主程式

對應程式：
- [MachineTab.cpp](/D:/Git%20Repository/chengyi-chung/SP-AX3/MachineTab.cpp)
- [WorkTab.cpp](/D:/Git%20Repository/chengyi-chung/SP-AX3/WorkTab.cpp)
- [UAX.cpp](/D:/Git%20Repository/chengyi-chung/SP-AX3/UAX/UAX.cpp)

| Modbus Register | INI Key | `SystemConfigA` 成員 | 備註 |
|---|---|---|---|
| 139 | `ImageBinary` | `ImageBinary` | `uint16_t` |
| 140 | `CreateToolPath` | `CreateToolPath` | `uint16_t` |
| 141 | `DispalyToolPath` | `DispalyToolPath` | 依原程式拼字 |
| 142 | `DisplayROI` | `DisplayROI` | `uint16_t` |
| 143 | `DisplayRefLine` | `DisplayRefLine` | `uint16_t` |
| 144 | `TabWork` | `TabWork` | `uint16_t` |
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

## 2. SystemConfig.ini 有讀取但目前不寫入 HMI 的欄位

這些值會進入 `m_SystemPara`，但不在 `139~156` 這段 Modbus block 內：

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
| 157 | `CreateToolPath` | `uint16_t` 控制是否建立工具路徑 |
| 158 | `RecipeID` | `uint16_t` |
| 159 | `CurrentProduction` | `uint16_t` |
| 160 | `Set_temperature0` | `uint16_t` |
| 161 | `Temperature0` | `uint16_t` |
| 162 | `Set_Temperature1` | `uint16_t` |
| 163 | `Temperature1` | `uint16_t` |
| 164 | `Set_temperature2` | `uint16_t` |
| 165 | `Temperature2` | `uint16_t` |
| 166 | `Servo_ALE0` | `uint16_t` |
| 167 | `Servo_ALE1` | `uint16_t` |
| 168 | `Servo_ALE2` | `uint16_t` |
| 169 | `Servo_ALE3` | `uint16_t` |
| 170 | `i_ProcessingTimeCount` | `uint16_t` |
| 171 | `i_SystemTimeCount` | `uint16_t` |
| 172 | `MachineID` | `uint16_t` |
| 173 | `MachineModel` | `uint16_t` |
| 174 | `Alm_tem_not_reach` | `uint8_t` |
| 175 | `flag_AL_overload` | `uint8_t` |
| 176 | `Alm_airPressureLow` | `uint8_t` |
| 177 | `flag_AL_emergency` | `uint8_t` |
| 178 | `flag_AL_midside_sensor` | `uint8_t` |
| 179 | `Alm_ManualY_GoOut` | `uint8_t` |
| 180 | `MachineStatus` | `uint8_t` |
| 181 | `WorkingMode` | `uint8_t` |
| 182 | `p19` 高 16-bit | `float` 拆高位 |
| 183 | `p19` 低 16-bit | `float` 拆低位 |

## 4. 目前同步方向

| 區塊 | 位址 | 方向 | 說明 |
|---|---|---|---|
| `SystemConfigA` | `139~156` | `SystemConfig.ini -> HMI` | 啟動連線後寫入 |
| `SystemConfigA` | `139~156` | `HMI -> 主程式` | timer 讀回更新 `m_SystemPara` |
| `MemStruct_SP` | `157~182` | `HMI -> 主程式` | timer 讀回更新 `m_MemStruct_SP` |
| `MemStruct_SP` | `157~182` | 啟動時不主動寫入 | 避免覆蓋執行期資料 |

## 5. 備註

- `HMI_ID` / `PLC_ID` 目前保留在 `SystemConfigA` 與 `ini` 內，但 Modbus 讀寫已停用。
- 先前規劃的 `HMI_ID = 160~185` 會和 `MemStruct_SP = 157~182` 重疊，因此目前不建議啟用。
