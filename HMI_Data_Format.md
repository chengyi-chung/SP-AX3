# HMI 資料格式與筆數定義

## 概述
本文檔說明傳送到 HMI 的資料格式以及如何計算資料筆數。

## 資料格式定義

### 格式 1: OnBnClickedIdcWorkGo() - 簡單格式
此格式用於傳送優化後的膠路點位到 HMI。

#### 資料結構
- **每筆資料包含**: (X1, Y, X2)
  - X1: 左手 X 軸座標 (1 個 uint16_t register)
  - Y: Y 軸座標 (1 個 uint16_t register)
  - X2: 右手 X 軸座標 (1 個 uint16_t register)

#### Register 分配
- X1 軸陣列: Register 14-43 (30 筆)
- Y 軸陣列: Register 44-73 (30 筆)
- X2 軸陣列: Register 74-103 (30 筆)
- HMI 固定接收 30 筆；ToolPath 只用前 25 點描述，第 26~30 筆重複最後一個有效描述點

#### 筆數計算
```cpp
// 每組 (X1, Y, X2) 算一筆
pointCount = min(25, min(PathLeft.size(), PathRight.size()));
// HMI 傳送固定 30 筆，pointCount 之後補最後一點
```

#### 範例
如果有 20 個有效點位：
- PC 端 `pointCount = 20`，但仍寫入固定 30 筆陣列範圍，未使用的位置重複第 20 點
- Register 14-33: X1[0] 到 X1[19]
- Register 44-63: Y[0] 到 Y[19]
- Register 74-93: X2[0] 到 X2[19]
- Register 34-43 / 64-73 / 94-103: 重複第 20 點

---

### 格式 2: SendToolPathDataA() - 進階格式
此格式用於傳送 3D 工具路徑資料到 PLC。

#### 資料結構
- **每筆資料包含**: (X, Y, Z)
  - X: X 軸座標 (2 個 uint16_t registers: 低位 + 高位)
  - Y: Y 軸座標 (2 個 uint16_t registers: 低位 + 高位)
  - Z: Z 軸座標 (2 個 uint16_t registers: 低位 + 高位)
  - **總計**: 每筆資料佔 6 個 uint16_t registers

#### Register 分配
資料以分軸方式傳送：
- X 軸區塊: Register 0, 2, 4, ... (每個點 2 個 registers)
- Y 軸區塊: Register 2, 4, 6, ... (每個點 2 個 registers)
- Z 軸區塊: Register 4, 6, 8, ... (每個點 2 個 registers)
- 總筆數: Register 40026

#### 筆數計算
```cpp
// 每組 (X, Y, Z) 算一筆
sizeOfArray = numPoints;  // 不是 numPoints * 6
// 實際傳輸的 register 數量 = sizeOfArray * 6
```

#### 資料編碼
每個座標值：
1. 原始單位: mm
2. 縮放: 乘以 100 (轉為 0.01mm 精度)
3. 編碼: 分為低位和高位兩個 uint16_t
   - 低位: value & 0xFFFF
   - 高位: (value >> 16) & 0xFFFF

#### 範例
如果有 100 個 3D 點位：
- Register 40026 = 100 (表示 100 筆資料)
- 實際傳輸 600 個 registers (100 筆 × 6 registers/筆)
- X 軸: 200 個 registers (100 個點 × 2)
- Y 軸: 200 個 registers (100 個點 × 2)
- Z 軸: 200 個 registers (100 個點 × 2)

---

## 重要原則

### ✓ 正確的計數方式
- **以邏輯資料單位計數**
  - 格式 1: 一組 (X1, Y, X2) = 1 筆
  - 格式 2: 一組 (X, Y, Z) = 1 筆

### ✗ 錯誤的計數方式
- ~~以 register 數量計數~~
- ~~以座標軸數量計數~~
- ~~以 uint16_t 陣列長度計數~~

### 程式碼中的體現
```cpp
// 格式 1 - OnBnClickedIdcWorkGo()
uint16_t actualDataCount = static_cast<uint16_t>(pointCount);
// HMI 目前不提供合法的筆數寫入 register，PC 端固定寫入 30 筆陣列範圍。
// 第 26~30 筆重複第 25 個 ToolPath 描述點。

// 格式 2 - SendToolPathDataA()
modbus_write_register(ctx, 40026, sizeOfArray);   // sizeOfArray = 點數，非 register 數
```

---

## Debug 輸出
在 Debug 模式下，`ToolPathTransform32A()` 會輸出：
```
總資料筆數 (Total Data Records): N 筆
總 Register 數量: N×6 個 (每筆 6 個)
資料筆數 0: X=... mm, Y=... mm, Z=... mm
資料筆數 1: X=... mm, Y=... mm, Z=... mm
...
```

這清楚地區分了「資料筆數」與「register 數量」的差異。

---

## 相關檔案
- `WorkTab.cpp`: 主要實作
  - `OnBnClickedIdcWorkGo()`: 格式 1 實作
  - `ToolPathTransform32A()`: 資料轉換
  - `SendToolPathDataA()`: 格式 2 實作
- `UAX/UAXTypes.h`: 資料結構定義
- `Modbus_Register_Map.md`: Register 對照表

---

## 版本歷史
- 2024-01: 初始版本，明確定義資料筆數計算方式
