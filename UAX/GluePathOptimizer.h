#pragma once
#include "UAXTypes.h"
#include <vector>
#include <opencv2/core.hpp>




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
    /**
 * @brief 優化膠水路徑：根據鞋型（左腳/右腳）決定主導側，
 *        對主導側進行平滑擬合得到標準 Y 序列，
 *        次要側強制跟隨相同 Y 值進行擬合，確保左右路徑 Y 對齊。
 *
 * @param inputPath     原始輸入點序列（通常來自影像邊緣或分割結果，可能包含雜訊）
 * @param optimizedPath 輸出結構，包含優化後的 PathLeft 與 PathRight
 * @param shoeType      鞋型：1=右腳（右側為主），2=左腳（左側為主），其他=自動依點數判斷
 * @param preserveSynchronizedEndpoints 保留輸入路徑的同步端點，不以獨立底點覆寫最後一組
 */
    void OptimizePath(const std::vector<cv::Point2d>& inputPath,
        GluePath& optimizedPath,
        int shoeType = 2,
        bool preserveSynchronizedEndpoints = false);

    // Apply the same legacy final-point rule used by ToolPathType 0:
    // within the bottom band, X2 takes the rightmost point and X1 the leftmost.
    static void ApplyLegacyBottomPoints(
        const std::vector<cv::Point2d>& rightSource,
        const std::vector<cv::Point2d>& leftSource,
        GluePath& path);
    static void ApplyLegacyBottomPointsFromContour(
        const std::vector<cv::Point2d>& contour,
        GluePath& path);
    // ── 新增這一行宣告 ──
   // std::vector<double> ComputeArcLengthParam(const std::vector<cv::Point2d>& pts);
    static std::vector<double> ComputeArcLengthParam(const std::vector<cv::Point2d>& pts);

private:
    ROIMask mROI;

    // ── 步驟函式 ───────────────────────────────────────────────────────────
    std::vector<cv::Point2d> FilterByMask(const std::vector<cv::Point2d>& inputPath) const;
    static void SplitByCenter(const std::vector<cv::Point2d>& maskedPath,
        std::vector<cv::Point2d>& rightPts,
        std::vector<cv::Point2d>& leftPts);

    static std::vector<cv::Point2d> SortByY(const std::vector<cv::Point2d>& pts);

    // 對主要側做擬合（決定標準Y序列）
    static std::vector<cv::Point2d> FitCurve(const std::vector<cv::Point2d>& pts,
        int maxPoints = 25);

    // 使用指定的Y序列對另一側做擬合（核心修正）
    static std::vector<cv::Point2d> FitCurveAtGivenY(const std::vector<cv::Point2d>& pts,
        const std::vector<double>& targetYs);

    // 多項式工具函式
    static std::vector<double> PolyFit(const std::vector<double>& y,
        const std::vector<double>& x,
        int degree);
    static double PolyEval(const std::vector<double>& coeffs, double y);
};
