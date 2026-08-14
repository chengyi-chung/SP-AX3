# SP-AX3 ToolPath 流程設計審查

## 1. 範圍與結論

目前 ToolPath 流程包含兩種觸發來源，但共用同一套產生與傳送函式：

- 手動：使用者按下 `Tool Path`，再按下 `GO`。
- 自動：HMI 將 `CreateToolPath` 設為 1，程式自動取像、停止取像、產生路徑並送出。

`ToolPathType` 已建立三路分派：

- `0`：現有算法。
- `1`：新算法 1 預留入口，目前回退到現有算法。
- `2`：新算法 2 預留入口，目前回退到現有算法。
- 缺值、空值或超出 0～2：使用 0。

目前可正常編譯，但在啟用影像校正時存在一項高優先問題：`Tool Path` 階段使用校正後參考點轉換座標，`GO` 階段卻再次使用未校正的 `referenceX/referenceY` 重建座標，最終送出的 HMI 座標可能偏移。

## 2. 端到端資料流

```text
Camera / Load Image
        |
        v
      m_mat                         原始灰階影像，pixel
        |
        +--> ROI 邊界檢查
        +--> OffsetValue / TransferFactor = offsetPixel
        +--> calibration.yml（若有效）
        |       |- 校正影像
        |       |- 校正 ROI mask
        |       `- 校正 reference point
        v
 GenerateToolPathByType(ToolPathType)
        |
        +-- 0 --> GenerateLegacyToolPath
        +-- 1 --> GenerateToolPathNewAlgorithm1 --> 暫時回退 legacy
        `-- 2 --> GenerateToolPathNewAlgorithm2 --> 暫時回退 legacy
        |
        v
 GetToolPath_CurvatureOptimized_Mask
        |- ROI mask
        |- BinaryLower / BinaryUpper
        |- erosion(offsetPixel)
        |- external contours
        `- toolPath.Path（影像左上原點，pixel）
        |
        v
 OptimizeGluePath
        |- ROI 過濾
        |- 依 RefCenterX 分左右側
        |- 建立左右 Y profile
        |- EntryPointX 決定右側起點
        |- 右側取樣 25 點
        `- 左側依右側 Y 對齊
        |
        v
 Sort + ROI filter + max 25 + pair normalization
        |
        v
 m_OptimizedGluePath                 影像座標，pixel
        |
        v
 ConvertToMachineCoordinates
        |- m_machineGluePath          參考點相對座標，pixel
        |- m_machineGluePath_mm       相對座標，mm
        |- m_HMIGluePath_temp         mm x 10
        `- m_HMIGluePath              四捨五入後整數
        |
        v
 GO / Modbus
        |- X1: D14～D43，取左側 abs(X)
        |- Y : D44～D73，取左側 Y
        |- X2: D74～D103，取右側 abs(X)
        `- 固定 30 筆；有效最多 25 筆，其餘重複最後一點
```

## 3. 觸發流程

### 3.1 手動流程

1. `OnBnClickedIdcWorkToolPath()` 驗證影像與 ROI。
2. 產生並保存 `m_OptimizedGluePath`。
3. 使用者按 `GO`。
4. `OnBnClickedIdcWorkGo()` 再次重建機械/HMI 座標並寫入 Modbus。

### 3.2 HMI 自動流程

`HandleAutoCreateToolPathRequest()` 監看 `CreateToolPath == 1`：

1. 若尚未取像，清除舊影像與路徑後啟動 Grab。
2. 有影像且 Grab 尚在執行時，先停止 Grab。
3. Grab 停止後呼叫 `OnBnClickedIdcWorkToolPath()`。
4. 左右路徑皆有效才呼叫 `OnBnClickedIdcWorkGo()`。
5. 清除 Register 139 `Grab` 與 Register 157 `CreateToolPath`。

## 4. 影像前處理與校正

### 4.1 ROI

ROI 來自 `MaskX/MaskY/MaskWidth/MaskHeight`。程式建立與影像同尺寸的 8-bit mask，ROI 內設為 255，其餘為 0。

### 4.2 Offset

```text
offsetPixel = OffsetValue(mm) / TransferFactor(mm/pixel)
```

若 `TransferFactor <= 0`，目前直接使用 0 pixel，不做 inward erosion。

### 4.3 校正

若 `calibration.yml` 可載入：

- `m_mat` 經 `undistortImage()` 得到路徑來源影像。
- ROI mask 同樣經校正，再 threshold 成二值 mask。
- 校正後 mask 的 non-zero bounding rectangle 成為 `effectiveRoiRect`。
- `referenceX/referenceY` 經 `transformPoint()` 成為 `effectiveReference`。

若 OpenCV 校正拋出例外，會回退到原影像、原 mask 與原參考點。

## 5. ToolPathType 分派

`GenerateToolPathByType()` 是算法選擇的唯一入口。新算法應只修改各自函式，不應在 UI handler 再增加 switch。

建議新算法遵守相同輸入輸出契約：

- 輸入影像與 mask 必須同尺寸。
- 輸出 `ToolPath.Path` 使用影像左上為原點。
- 單位固定為 pixel。
- 不在算法內轉成 mm 或 HMI 座標。
- 失敗時清空 output，並回傳明確狀態；目前介面為 `void`，後續建議改為結果型別。

## 6. 現有算法（ToolPathType=0）

`GetToolPath_CurvatureOptimized_Mask()` 執行：

1. 轉灰階。
2. 套用 ROI mask。
3. 使用 `BinaryLower～BinaryUpper` 執行 `cv::inRange()`。
4. 將 `offsetPixel` 四捨五入為 erosion 次數，以 3x3 矩形 kernel 內縮。
5. `findContours(RETR_EXTERNAL, CHAIN_APPROX_TC89_L1)` 取得外輪廓。
6. 目前呼叫參數 `enableCurvatureOptimization=false`，因此不執行 Douglas-Peucker 簡化，保留原 contour 點。
7. 所有外輪廓點依 contours 回傳順序串接到同一個 `ToolPath.Path`。

## 7. 左右路徑最佳化

`OptimizeGluePath()` 建立 `GluePathOptimizer`，固定傳入 `shoeType=2`。目前 optimizer 內部以 `(void)shoeType` 忽略此參數。

主要流程：

1. ROI 過濾。
2. 依中心參考線切成左、右 raw points。
3. 左右各自建立依 Y 的 profile。
4. 在右側 profile 找 `EntryPointX` 對應起點。
5. 右側 profile 取樣為 25 點。
6. 左側依右側各點的 Y 插值，形成成對路徑。
7. 最後一點替換成左右底部點，並強制左右 Y 相同。

WorkTab 隨後再次：

- 正規化左右點數。
- 依 Y 由小到大排序。
- 僅保留左右點都位於 ROI 的 pair。
- 限制最多 25 pair。
- 再次正規化左右點數與 Y。

## 8. 座標系與單位

| 變數 | 原點 | 單位 | 用途 |
|---|---|---:|---|
| `toolPath.Path` | 影像左上 | pixel | contour 原始點 |
| `m_OptimizedGluePath` | 影像左上 | pixel | 左右成對路徑 |
| `m_machineGluePath` | `effectiveReference` 或 `referenceX/Y` | pixel | 機械相對座標 |
| `m_machineGluePath_mm` | 同上 | mm | pixel × TransferFactor |
| `m_HMIGluePath_temp` | 同上 | 0.1 mm | mm × 10 |
| `m_HMIGluePath` | 同上 | integer | 四捨五入後 HMI 值 |

轉換公式：

```text
machineX_px = imageX_px - referenceX_px
machineY_px = imageY_px - referenceY_px
machine_mm   = machine_px * TransferFactor
hmi          = round(machine_mm * 10)
```

## 9. HMI / Modbus 輸出

輸出固定使用 30-word buffer：

- X1：Address 14，左側路徑 `abs(x)`。
- Y：Address 44，左側路徑 `y`。
- X2：Address 74，右側路徑 `abs(x)`。

有效描述點最多 25；第 26～30 筆重複最後有效點。轉成 register 時負值被壓到 0，大於 65535 被壓到 65535。

## 10. 設計審查發現

### P1：校正參考點在 GO 階段被原始參考點覆蓋

`Tool Path` 階段呼叫：

```cpp
ConvertToMachineCoordinates(effectiveReference.x, effectiveReference.y);
```

但 `GO` 階段又呼叫無參數版本：

```cpp
ConvertToMachineCoordinates(); // 使用 referenceX/referenceY
```

這會清空並重建所有衍生路徑。因此啟用校正時，最終 Modbus 資料不一定使用校正後參考點。建議保存本次生成所使用的 effective reference，GO 必須使用同一份生成上下文。

### P1：手動連續取像時存在影像一致性風險

Grab thread 會替換 `m_mat`；ToolPath handler 直接多次讀取 `m_mat`，沒有先在同一把 mutex 下 clone 一份完整 frame。自動流程會先停 Grab，手動流程則不保證。建議 ToolPath 開始時鎖定後 clone 成局部 `sourceFrame`，後續全部使用該 frame。

### P1：空 raw path 檢查被註解

`GetToolPath_CurvatureOptimized_Mask()` 後原本的 empty check 位於註解區。雖然後續通常會得到空 GluePath 並由 GO 擋下，但錯誤位置延後，且 UI 不會直接告知「ROI 無路徑」。建議恢復檢查並回傳具體失敗原因。

### P2：GO 固定使用 station ID 1

`OnBnClickedIdcWorkGo()` 使用 `const int stationID = 1`，沒有使用 `m_SystemPara.StationID`。若部署站號不是 1，路徑可能送往錯誤 slave。

### P2：ToolPathType 1/2 尚未有不同算法

目前兩個預留入口都回退現有算法。這是安全的相容行為，但 UI/記錄應清楚顯示實際執行的是 fallback，否則測試人員可能誤認新算法已生效。

### P2：多輪廓直接串接

現有 extractor 將所有 external contours 直接 append 至單一 vector，沒有選最大輪廓或建立 contour 間斷點。若 ROI 中有多個物件或雜訊輪廓，optimizer 可能將不相連輪廓視為同一路徑。

### P2：固定 shoeType=2 且目前被忽略

呼叫端固定傳 2，optimizer 又忽略參數。若鞋型方向未來需要影響主導側，應把鞋型納入設定並建立測試案例；否則可移除參數避免誤導。

### P2：X1/X2 左右定義需統一

Debug CSV helper 將 `PathRight` 標為 X1、`PathLeft` 標為 X2；Modbus 寫入則是 X1=Left、X2=Right。需以 HMI/電控規格為唯一依據統一命名，避免設計檢閱與現場診斷相反。

### P2：INI 既有拼字不一致

writer 使用 `DispayROI`，reader 使用 `DisplayROI`。此欄位寫回後可能無法在下次啟動讀回。雖非 ToolPathType 新增造成，但會影響 ToolPath ROI 顯示設定的持久化。

### P3：錯誤回報介面不足

路徑 extractor 與 optimizer 多為 `void`；部分失敗只清空 vector 或輸出 stderr。建議統一回傳：成功、錯誤碼、訊息、raw/filtered/final point count，方便 HMI 與測試記錄。

## 11. 建議的下一版介面

```cpp
struct ToolPathGenerationContext {
    int algorithmType;
    cv::Mat sourceFrame;
    cv::Mat roiMask;
    cv::Rect effectiveRoi;
    cv::Point2d effectiveReference;
    double transferFactorMmPerPixel;
    double offsetPixel;
    int binaryLower;
    int binaryUpper;
};

struct ToolPathGenerationResult {
    bool success;
    CString errorMessage;
    ToolPath rawPath;
    GluePath optimizedPath;
    GluePath hmiPath;
};
```

產生與 GO 應共享同一個 immutable context/result，避免兩階段使用不同影像、參考點或設定值。

## 12. 建議驗收案例

1. `ToolPathType` 缺值、空值、0、1、2、負值及大於 2。
2. 相機 frame 與 Load Image 不同尺寸。
3. 校正開/關時，同一特徵點的 HMI 座標可預期且一致。
4. ROI 剛好貼齊影像四邊、ROI 寬高為 0、ROI 超界。
5. ROI 中無物件、單一物件、多物件與小雜訊。
6. `TransferFactor` 為 0、負值及正常值。
7. 左右側其中一側沒有點。
8. 少於 25、等於 25、多於 25 pair。
9. StationID 非 1。
10. 手動連續取像時同時按 Tool Path。
11. Modbus 在 X1、Y、X2 任一 block 寫入失敗。
12. HMI 驗證 X1/X2 與實際左/右機構定義。

