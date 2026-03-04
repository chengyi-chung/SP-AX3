#pragma once
#pragma once
#include <vector>
#include <opencv2/core.hpp>

// ──────────────────────────────────────────────
//  資料結構定義
// ──────────────────────────────────────────────

struct ToolPath
{
    cv::Point2d            Offset;       // 工具路徑的整體偏移量
    std::vector<cv::Point2d> Path;       // 工具路徑上的點序列
    std::vector<int>       numClusters;  // 每個點所屬的輪廓/叢集編號
};

struct GluePath
{
    std::vector<cv::Point2d> PathRight;  // 右側膠水路徑（最終輸出）
    std::vector<cv::Point2d> PathLeft;   // 左側膠水路徑（Y與右側完全相同）
};

struct ROIMask
{
    int MaskX;       // ROI 左上角 X
    int MaskY;       // ROI 左上角 Y
    int MaskWidth;   // ROI 寬度
    int MaskHeight;  // ROI 高度
    int RefCenterX;  // 分割左右的參考中心 X（僅用於分割，不再用於鏡射）
    int RefCenterY;  // 參考中心 Y（保留未來使用）
};

// ──────────────────────────────────────────────
//  膠水路徑優化器
// ──────────────────────────────────────────────

class GluePathOptimizer
{
public:
    explicit GluePathOptimizer(const ROIMask& roi);

    /**
     * @brief 【最終版】主要優化流程
     *
     * 符合實際機械行為（單Y軸帶動兩個X軸）：
     *   1. FilterByMask
     *   2. SplitByCenter
     *   3. 先對主要側（右側）做擬合 → 決定標準Y序列
     *   4. 再對次要側（左側）使用「相同Y序列」做擬合
     *   5. 輸出 PathRight 與 PathLeft（Y值完全一致）
     */
    void OptimizePath(const std::vector<cv::Point2d>& inputPath,
        GluePath& optimizedPath,
        int                              shoeType = 1);

private:
    ROIMask mROI;

    // ── 步驟函式 ───────────────────────────────────────────────────────────
    std::vector<cv::Point2d> FilterByMask(const std::vector<cv::Point2d>& inputPath) const;
    void SplitByCenter(const std::vector<cv::Point2d>& maskedPath,
        std::vector<cv::Point2d>& rightPts,
        std::vector<cv::Point2d>& leftPts) const;

    static std::vector<cv::Point2d> SortByY(const std::vector<cv::Point2d>& pts);

    // 對主要側做擬合（決定標準Y序列）
    static std::vector<cv::Point2d> FitCurve(const std::vector<cv::Point2d>& pts,
        int maxPoints = 30);

    // 使用指定的Y序列對另一側做擬合（核心修正）
    static std::vector<cv::Point2d> FitCurveAtGivenY(const std::vector<cv::Point2d>& pts,
        const std::vector<double>& targetYs);

    // 多項式工具函式
    static std::vector<double> PolyFit(const std::vector<double>& y,
        const std::vector<double>& x,
        int degree);
    static double PolyEval(const std::vector<double>& coeffs, double y);
};