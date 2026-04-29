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
void GluePathOptimizer::OptimizePath(
    const std::vector<cv::Point2d>& inputPath,
    GluePath& optimizedPath,
    int shoeType)
{
    std::vector<cv::Point2d> maskedPath = FilterByMask(inputPath);
    if (maskedPath.empty()) {
        optimizedPath.PathRight.clear();
        optimizedPath.PathLeft.clear();
        return;
    }

    std::vector<cv::Point2d> rightRaw;
    std::vector<cv::Point2d> leftRaw;
    SplitByCenter(maskedPath, rightRaw, leftRaw);

    std::vector<cv::Point2d>* primaryRawPtr = NULL;
    std::vector<cv::Point2d>* secondaryRawPtr = NULL;
    bool isLeftPrimary = false;

    if (shoeType == 2) {
        primaryRawPtr = &leftRaw;
        secondaryRawPtr = &rightRaw;
        isLeftPrimary = true;
    }
    else if (shoeType == 1) {
        primaryRawPtr = &rightRaw;
        secondaryRawPtr = &leftRaw;
        isLeftPrimary = false;
    }
    else if (rightRaw.size() >= leftRaw.size()) {
        primaryRawPtr = &rightRaw;
        secondaryRawPtr = &leftRaw;
        isLeftPrimary = false;
    }
    else {
        primaryRawPtr = &leftRaw;
        secondaryRawPtr = &rightRaw;
        isLeftPrimary = true;
    }

    if (primaryRawPtr == NULL || primaryRawPtr->empty()) {
        optimizedPath.PathRight.clear();
        optimizedPath.PathLeft.clear();
        return;
    }

    std::vector<cv::Point2d> primarySmoothPts = FitCurve(*primaryRawPtr, 30);

    std::vector<double> targetYs;
    targetYs.reserve(primarySmoothPts.size());
    for (size_t i = 0; i < primarySmoothPts.size(); ++i) {
        targetYs.push_back(primarySmoothPts[i].y);
    }

    std::vector<cv::Point2d> secondarySmoothPts;
    if (secondaryRawPtr != NULL && !secondaryRawPtr->empty()) {
        secondarySmoothPts = FitCurveAtGivenY(*secondaryRawPtr, targetYs);
    }
    else {
        secondarySmoothPts = primarySmoothPts;
    }

    primarySmoothPts = FilterByMask(primarySmoothPts);
    secondarySmoothPts = FilterByMask(secondarySmoothPts);

    if (primarySmoothPts.empty() || secondarySmoothPts.empty()) {
        optimizedPath.PathRight.clear();
        optimizedPath.PathLeft.clear();
        return;
    }

    if (isLeftPrimary) {
        optimizedPath.PathLeft = primarySmoothPts;
        optimizedPath.PathRight = secondarySmoothPts;
    }
    else {
        optimizedPath.PathRight = primarySmoothPts;
        optimizedPath.PathLeft = secondarySmoothPts;
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

    std::vector<cv::Point2d> sortedPts = pts;
    std::sort(sortedPts.begin(), sortedPts.end(),
        [](const cv::Point2d& a, const cv::Point2d& b) { return a.y < b.y; });

    std::vector<cv::Point2d> uniquePts;
    uniquePts.reserve(sortedPts.size());
    if (!sortedPts.empty()) {
        uniquePts.push_back(sortedPts[0]);
        for (size_t i = 1; i < sortedPts.size(); ++i) {
            if (std::abs(sortedPts[i].y - uniquePts.back().y) > 1e-6) {
                uniquePts.push_back(sortedPts[i]);
            }
            else {
                uniquePts.back().x = (uniquePts.back().x + sortedPts[i].x) / 2.0;
            }
        }
    }

    std::vector<cv::Point2d> result;
    result.reserve(targetYs.size());

    if (uniquePts.size() < 2) {
        for (size_t i = 0; i < targetYs.size(); ++i) {
            const double y = targetYs[i];
            double bestX = uniquePts.empty() ? 0.0 : uniquePts[0].x;
            double minDist = uniquePts.empty() ? 1e9 : std::abs(uniquePts[0].y - y);
            for (size_t j = 0; j < uniquePts.size(); ++j) {
                const cv::Point2d& p = uniquePts[j];
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

    for (size_t i = 0; i < targetYs.size(); ++i) {
        const double ty = targetYs[i];
        double x;
        if (ty <= uniquePts.front().y) {
            x = uniquePts[0].x;
        }
        else if (ty >= uniquePts.back().y) {
            x = uniquePts.back().x;
        }
        else {
            size_t upperIdx = 1;
            while (upperIdx < uniquePts.size() && uniquePts[upperIdx].y < ty) {
                ++upperIdx;
            }
            const size_t lowerIdx = upperIdx - 1;

            const double y0 = uniquePts[lowerIdx].y;
            const double y1 = uniquePts[upperIdx].y;
            const double x0 = uniquePts[lowerIdx].x;
            const double x1 = uniquePts[upperIdx].x;

            const double dy = y1 - y0;
            if (std::abs(dy) <= 1e-9) {
                x = (x0 + x1) * 0.5;
            }
            else {
                const double t = (ty - y0) / dy;
                x = x0 + t * (x1 - x0);
            }
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
        // Match cv::Rect semantics: left/top inclusive, right/bottom exclusive.
        if (pt.x >= xMin && pt.x < xMax &&
            pt.y >= yMin && pt.y < yMax)
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
