#include "GluePathOptimizer.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <opencv2/core.hpp>

// ══════════════════════════════════════════════════════════════════════════════
//  建構子
// ══════════════════════════════════════════════════════════════════════════════
GluePathOptimizer::GluePathOptimizer(const ROIMask& roi)
    : mROI(roi)
{}

// ══════════════════════════════════════════════════════════════════════════════
//  【方案1 最終版】OptimizePath – 使用弧長參數化（專治 U 型路徑）
// ══════════════════════════════════════════════════════════════════════════════
/**
 * @brief 優化膠水路徑（方案1：弧長參數化版）
 *        1. 移除 SortByY（避免 U 型路徑被打散）
 *        2. 對主導側使用累積弧長 t 做參數化 → 保留原始走向與 U 型形狀
 *        3. 仍保留「標準 Y 序列」同步機制，讓次要側強制跟隨相同高度
 *
 * @param inputPath     原始輸入點序列（必須是 GetToolPath_CurvatureOptimized_Mask 輸出的「已排序路徑」）
 * @param optimizedPath 輸出結構
 * @param shoeType      鞋型：1=右腳（右側為主），2=左腳（左側為主）
 */
void GluePathOptimizer::OptimizePath(const std::vector<cv::Point2d>& inputPath,
    GluePath& optimizedPath,
    int shoeType)
{
    // Step 1: 濾除不在 ROI 內的雜點
    std::vector<cv::Point2d> maskedPath = FilterByMask(inputPath);
    if (maskedPath.empty()) {
        optimizedPath.PathRight.clear();
        optimizedPath.PathLeft.clear();
        return;
    }

    // Step 2: 分割左右原始點
    std::vector<cv::Point2d> rightRaw, leftRaw;
    SplitByCenter(maskedPath, rightRaw, leftRaw);

    // ────────────────────────────────────────────────
    // 【核心邏輯：決定主導側】
    // ────────────────────────────────────────────────
    std::vector<cv::Point2d>* primaryRawPtr = nullptr;
    std::vector<cv::Point2d>* secondaryRawPtr = nullptr;
    bool isLeftPrimary = false;

    if (shoeType == 2) {          // 左腳
        primaryRawPtr = &leftRaw;
        secondaryRawPtr = &rightRaw;
        isLeftPrimary = true;
    }
    else if (shoeType == 1) {     // 右腳
        primaryRawPtr = &rightRaw;
        secondaryRawPtr = &leftRaw;
        isLeftPrimary = false;
    }                                                                                           
    else {
        // 自動模式（建議正式版務必傳正確 shoeType）
        if (rightRaw.size() >= leftRaw.size()) {
            primaryRawPtr = &rightRaw;
            secondaryRawPtr = &leftRaw;
            isLeftPrimary = false;
        }
        else {
            primaryRawPtr = &leftRaw;
            secondaryRawPtr = &rightRaw;
            isLeftPrimary = true;
        }
    }

    if (primaryRawPtr->empty()) {
        optimizedPath.PathRight.clear();
        optimizedPath.PathLeft.clear();
        return;
    }

    // Step 3: 對主要側擬合（方案1重點：弧長參數化，保留 U 型原始順序）
    //         不再使用 SortByY！直接使用 GetToolPath 輸出的原始順序
    std::vector<cv::Point2d> primarySmooth = FitCurve(*primaryRawPtr, 30);

    // 抽出標準 Y 序列（仍保留原需求：左右 Y 對齊）
    std::vector<double> standardYs;
    standardYs.reserve(primarySmooth.size());
    for (const auto& p : primarySmooth) {
        standardYs.push_back(p.y);
    }

    // Step 4: 次要側強制跟隨相同 Y 值
    std::vector<cv::Point2d> secondarySmooth;
    if (!secondaryRawPtr->empty()) {
        secondarySmooth = FitCurveAtGivenY(*secondaryRawPtr, standardYs);
    }
    else {
        secondarySmooth = primarySmooth;
    }

    // Step 5: 寫回輸出
    if (isLeftPrimary) {
        optimizedPath.PathLeft = std::move(primarySmooth);
        optimizedPath.PathRight = std::move(secondarySmooth);
    }
    else {
        optimizedPath.PathRight = std::move(primarySmooth);
        optimizedPath.PathLeft = std::move(secondarySmooth);
    }
}

// ══════════════════════════════════════════════════════════════════════════════
//  新增：計算累積弧長參數（方案1核心）
// ══════════════════════════════════════════════════════════════════════════════
std::vector<double>
GluePathOptimizer::ComputeArcLengthParam(const std::vector<cv::Point2d>& pts)
{
    if (pts.empty()) return {};
    std::vector<double> s(pts.size(), 0.0);
    for (size_t i = 1; i < pts.size(); ++i) {
        double dx = pts[i].x - pts[i - 1].x;
        double dy = pts[i].y - pts[i - 1].y;
        s[i] = s[i - 1] + std::hypot(dx, dy);   // 歐氏距離累加
    }
    return s;
}

// ══════════════════════════════════════════════════════════════════════════════
//  FitCurve（已改為弧長參數化版本）
// ══════════════════════════════════════════════════════════════════════════════
std::vector<cv::Point2d>
GluePathOptimizer::FitCurve(const std::vector<cv::Point2d>& pts, int targetCount)
{
    if (pts.size() < 3) return pts;   // 點太少直接返回

    // 計算弧長參數 t
    //auto s = ComputeArcLengthParam(pts);
    auto s = ComputeArcLengthParam(pts);
    double sTotal = s.back();

    std::vector<double> xs, ys;
    xs.reserve(pts.size());
    ys.reserve(pts.size());
    for (const auto& p : pts) {
        xs.push_back(p.x);
        ys.push_back(p.y);
    }

    // 分別對 x(s) 和 y(s) 做 3 次多項式擬合（對 U 型極穩定）
    std::vector<double> coeffX = PolyFit(s, xs, 4);
    std::vector<double> coeffY = PolyFit(s, ys, 4);

    std::vector<cv::Point2d> result;
    result.reserve(targetCount);

    for (int i = 0; i < targetCount; ++i) {
        double t = (targetCount > 1) ? (sTotal * i) / (targetCount - 1) : 0.0;
        double x = PolyEval(coeffX, t);
        double y = PolyEval(coeffY, t);
        result.emplace_back(x, y);
    }
    return result;
}

// ══════════════════════════════════════════════════════════════════════════════
//  FitCurveAtGivenY – 改用更穩定的方法（避免全局多項式失效）
// ══════════════════════════════════════════════════════════════════════════════
std::vector<cv::Point2d>
GluePathOptimizer::FitCurveAtGivenY(const std::vector<cv::Point2d>& pts,
    const std::vector<double>& targetYs)
{
    if (pts.empty() || targetYs.empty()) return {};

    // 先按 y 排序次要側原始點（這次排序是為了插值，不是破壞順序）
    std::vector<cv::Point2d> sortedPts = pts;
    std::sort(sortedPts.begin(), sortedPts.end(),
        [](const cv::Point2d& a, const cv::Point2d& b) { return a.y < b.y; });

    // 移除重複 y（如果有完全相同 y，取平均 x 或第一個）
    std::vector<cv::Point2d> uniquePts;
    uniquePts.reserve(sortedPts.size());
    if (!sortedPts.empty()) {
        uniquePts.push_back(sortedPts[0]);
        for (size_t i = 1; i < sortedPts.size(); ++i) {
            if (std::abs(sortedPts[i].y - uniquePts.back().y) > 1e-6) {  // 避免浮點重複
                uniquePts.push_back(sortedPts[i]);
            }
            else {
                // 有重複 y，取平均 x（可改成其他策略）
                uniquePts.back().x = (uniquePts.back().x + sortedPts[i].x) / 2.0;
            }
        }
    }

    if (uniquePts.size() < 2) {
        // 點太少，直接回傳 targetYs 上最近的點
        std::vector<cv::Point2d> result;
        for (double y : targetYs) {
            double bestX = uniquePts.empty() ? 0.0 : uniquePts[0].x;
            double minDist = uniquePts.empty() ? 1e9 : std::abs(uniquePts[0].y - y);
            for (const auto& p : uniquePts) {
                double dist = std::abs(p.y - y);
                if (dist < minDist) {
                    minDist = dist;
                    bestX = p.x;
                }
            }
            result.emplace_back(bestX, y);
        }
        return result;
    }

    // 逐一對 targetYs 做線性插值（最穩健）
    std::vector<cv::Point2d> result;
    result.reserve(targetYs.size());

    size_t idx = 0;  // 用來加速查找
    for (double ty : targetYs) {
        // 找到 ty 所在的區間 [uniquePts[idx].y, uniquePts[idx+1].y]
        while (idx + 1 < uniquePts.size() && uniquePts[idx + 1].y < ty) {
            ++idx;
        }

        double x;
        if (idx + 1 >= uniquePts.size()) {
            // ty > 最大 y → 最後一段外插（可改成 clamp 或最近鄰）
            x = uniquePts.back().x;
        }
        else if (ty <= uniquePts[0].y) {
            // ty < 最小 y → 第一段外插
            x = uniquePts[0].x;
        }
        else {
            // 線性插值
            double y0 = uniquePts[idx].y;
            double y1 = uniquePts[idx + 1].y;
            double x0 = uniquePts[idx].x;
            double x1 = uniquePts[idx + 1].x;

            double t = (ty - y0) / (y1 - y0);
            x = x0 + t * (x1 - x0);
        }

        result.emplace_back(x, ty);
    }

    return result;
}

// ══════════════════════════════════════════════════════════════════════════════
//  其餘函式（FilterByMask、SplitByCenter、PolyFit、PolyEval）完全不變
// ══════════════════════════════════════════════════════════════════════════════
std::vector<cv::Point2d>
GluePathOptimizer::FilterByMask(const std::vector<cv::Point2d>& inputPath) const
{
    std::vector<cv::Point2d> result;
    result.reserve(inputPath.size());

    const double xMin = static_cast<double>(mROI.MaskX);
    const double xMax = static_cast<double>(mROI.MaskX + mROI.MaskWidth);
    const double yMin = static_cast<double>(mROI.MaskY);
    const double yMax = static_cast<double>(mROI.MaskY + mROI.MaskHeight);

    for (const auto& pt : inputPath)
    {
        if (pt.x >= xMin && pt.x <= xMax &&
            pt.y >= yMin && pt.y <= yMax)
        {
            result.push_back(pt);
        }
    }
    return result;
}

void GluePathOptimizer::SplitByCenter(const std::vector<cv::Point2d>& maskedPath,
    std::vector<cv::Point2d>& rightPts,
    std::vector<cv::Point2d>& leftPts) const
{
    rightPts.clear();
    leftPts.clear();
    const double cx = static_cast<double>(mROI.RefCenterX);
    for (const auto& pt : maskedPath)
    {
        if (pt.x >= cx)
            rightPts.push_back(pt);
        else
            leftPts.push_back(pt);
    }
}

std::vector<double>
GluePathOptimizer::PolyFit(const std::vector<double>& y,
    const std::vector<double>& x,
    int degree)
{
    assert(y.size() == x.size() && !y.empty());
    const int n = static_cast<int>(y.size());
    const int d = degree + 1;

    cv::Mat A(n, d, CV_64F);
    for (int i = 0; i < n; ++i) {
        double val = 1.0;
        for (int j = 0; j < d; ++j) {
            A.at<double>(i, j) = val;
            val *= y[i];
        }
    }

    cv::Mat b(n, 1, CV_64F);
    for (int i = 0; i < n; ++i)
        b.at<double>(i, 0) = x[i];

    cv::Mat c;
    cv::solve(A, b, c, cv::DECOMP_SVD);

    std::vector<double> coeffs(d);
    for (int j = 0; j < d; ++j)
        coeffs[j] = c.at<double>(j, 0);
    return coeffs;
}

double GluePathOptimizer::PolyEval(const std::vector<double>& coeffs, double var)
{
    double result = 0.0;
    double power = 1.0;
    for (double c : coeffs) {
        result += c * power;
        power *= var;
    }
    return result;
}
