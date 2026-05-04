#include "GluePathOptimizer.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <opencv2/core.hpp>

namespace
{
    std::vector<cv::Point2d> FilterProfileByYRange(
        const std::vector<cv::Point2d>& profile,
        double yMin,
        double yMax)
    {
        std::vector<cv::Point2d> filtered;
        filtered.reserve(profile.size());
        for (size_t i = 0; i < profile.size(); ++i) {
            if (profile[i].y >= yMin && profile[i].y <= yMax) {
                filtered.push_back(profile[i]);
            }
        }
        return filtered;
    }

    std::vector<cv::Point2d> BuildSideProfileByY(
        const std::vector<cv::Point2d>& rawPts,
        bool keepRightMost)
    {
        if (rawPts.empty()) {
            return std::vector<cv::Point2d>();
        }

        std::vector<cv::Point2d> sortedPts = rawPts;
        std::sort(sortedPts.begin(), sortedPts.end(),
            [](const cv::Point2d& a, const cv::Point2d& b) {
                if (a.y != b.y) {
                    return a.y < b.y;
                }
                return a.x < b.x;
            });

        std::vector<cv::Point2d> profile;
        profile.reserve(sortedPts.size());

        size_t i = 0;
        while (i < sortedPts.size()) {
            const double currentY = sortedPts[i].y;
            cv::Point2d chosen = sortedPts[i];
            size_t j = i + 1;
            while (j < sortedPts.size() && std::abs(sortedPts[j].y - currentY) <= 1e-6) {
                if (keepRightMost) {
                    if (sortedPts[j].x > chosen.x) {
                        chosen = sortedPts[j];
                    }
                }
                else {
                    if (sortedPts[j].x < chosen.x) {
                        chosen = sortedPts[j];
                    }
                }
                ++j;
            }

            profile.push_back(chosen);
            i = j;
        }

        return profile;
    }

    std::vector<cv::Point2d> SampleProfilePoints(
        const std::vector<cv::Point2d>& profile,
        size_t maxPoints)
    {
        if (profile.empty() || maxPoints == 0) {
            return std::vector<cv::Point2d>();
        }

        if (profile.size() <= maxPoints) {
            return profile;
        }

        std::vector<cv::Point2d> sampled;
        sampled.reserve(maxPoints);
        for (size_t i = 0; i < maxPoints; ++i) {
            const size_t index = static_cast<size_t>(
                std::lround(static_cast<double>(i) * static_cast<double>(profile.size() - 1) /
                    static_cast<double>(maxPoints - 1)));
            if (sampled.empty() ||
                std::abs(profile[index].y - sampled.back().y) > 1e-6 ||
                std::abs(profile[index].x - sampled.back().x) > 1e-6) {
                sampled.push_back(profile[index]);
            }
        }
        return sampled;
    }

    cv::Point2d FindNearestPointOnPath(
        const cv::Point2d& target,
        const std::vector<cv::Point2d>& candidates)
    {
        if (candidates.empty()) {
            return target;
        }

        size_t bestIndex = 0;
        double bestDist2 = std::numeric_limits<double>::max();
        for (size_t i = 0; i < candidates.size(); ++i) {
            const double dx = candidates[i].x - target.x;
            const double dy = candidates[i].y - target.y;
            const double dist2 = dx * dx + dy * dy;
            if (dist2 < bestDist2) {
                bestDist2 = dist2;
                bestIndex = i;
            }
        }
        return candidates[bestIndex];
    }

    void SnapPointsToRawPath(
        std::vector<cv::Point2d>& smoothedPts,
        const std::vector<cv::Point2d>& rawPts)
    {
        for (size_t i = 0; i < smoothedPts.size(); ++i) {
            smoothedPts[i] = FindNearestPointOnPath(smoothedPts[i], rawPts);
        }
    }
}

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
        primaryRawPtr = &rightRaw;
        secondaryRawPtr = &leftRaw;
        isLeftPrimary = false;
    }
    else if (shoeType == 1) {
        primaryRawPtr = &leftRaw;
        secondaryRawPtr = &rightRaw;
        isLeftPrimary = true;
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

    const bool primaryIsRightSide = !isLeftPrimary;
    const bool secondaryIsRightSide = isLeftPrimary;

    std::vector<cv::Point2d> primaryProfile =
        BuildSideProfileByY(*primaryRawPtr, primaryIsRightSide);
    std::vector<cv::Point2d> secondaryProfile =
        BuildSideProfileByY(*secondaryRawPtr, secondaryIsRightSide);

    if (!primaryProfile.empty() && !secondaryProfile.empty()) {
        const double commonYMin = (std::max)(primaryProfile.front().y, secondaryProfile.front().y);
        const double commonYMax = (std::min)(primaryProfile.back().y, secondaryProfile.back().y);
        if (commonYMin <= commonYMax) {
            primaryProfile = FilterProfileByYRange(primaryProfile, commonYMin, commonYMax);
            secondaryProfile = FilterProfileByYRange(secondaryProfile, commonYMin, commonYMax);
        }
    }

    std::vector<cv::Point2d> primarySmoothPts =
        SampleProfilePoints(primaryProfile, 30);

    std::vector<double> targetYs;
    targetYs.reserve(primarySmoothPts.size());
    for (size_t i = 0; i < primarySmoothPts.size(); ++i) {
        targetYs.push_back(primarySmoothPts[i].y);
    }

    std::vector<cv::Point2d> secondarySmoothPts;
    if (!secondaryProfile.empty() && !targetYs.empty()) {
        secondarySmoothPts = FitCurveAtGivenY(secondaryProfile, targetYs);

        // Keep the secondary side anchored to its own profile endpoints so the
        // first/last point is not lost by interpolation drift near the contour ends.
        if (!secondarySmoothPts.empty()) {
            secondarySmoothPts.front().x = secondaryProfile.front().x;
            secondarySmoothPts.back().x = secondaryProfile.back().x;
        }
    }
    else {
        secondarySmoothPts = primarySmoothPts;
    }

    primarySmoothPts = FilterByMask(primarySmoothPts);
    secondarySmoothPts = FilterByMask(secondarySmoothPts);

    const size_t pairCount = (std::min)(primarySmoothPts.size(), secondarySmoothPts.size());
    if (pairCount == 0) {
        optimizedPath.PathRight.clear();
        optimizedPath.PathLeft.clear();
        return;
    }

    primarySmoothPts.resize(pairCount);
    secondarySmoothPts.resize(pairCount);

    if (isLeftPrimary) {
        optimizedPath.PathLeft = primarySmoothPts;
        optimizedPath.PathRight = secondarySmoothPts;
    }
    else {
        optimizedPath.PathRight = primarySmoothPts;
        optimizedPath.PathLeft = secondarySmoothPts;
    }

    for (size_t i = 0; i < pairCount; ++i) {
        const double sharedY = isLeftPrimary
            ? optimizedPath.PathLeft[i].y
            : optimizedPath.PathRight[i].y;
        optimizedPath.PathRight[i].y = sharedY;
        optimizedPath.PathLeft[i].y = sharedY;
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
    if (pts.empty()) return {};
    if (pts.size() < 2 || targetCount <= 0) return pts;

    const std::vector<double> arcLengths = ComputeArcLengthParam(pts);
    const double totalLength = arcLengths.empty() ? 0.0 : arcLengths.back();
    if (totalLength <= 1e-9) {
        return pts;
    }

    std::vector<cv::Point2d> result;
    result.reserve(targetCount);

    for (int i = 0; i < targetCount; ++i) {
        const double targetLength = (targetCount > 1)
            ? (totalLength * static_cast<double>(i)) / static_cast<double>(targetCount - 1)
            : 0.0;

        size_t upperIdx = 1;
        while (upperIdx < arcLengths.size() && arcLengths[upperIdx] < targetLength) {
            ++upperIdx;
        }

        if (upperIdx >= arcLengths.size()) {
            result.push_back(pts.back());
            continue;
        }

        const size_t lowerIdx = upperIdx - 1;
        const double s0 = arcLengths[lowerIdx];
        const double s1 = arcLengths[upperIdx];
        const cv::Point2d& p0 = pts[lowerIdx];
        const cv::Point2d& p1 = pts[upperIdx];

        double alpha = 0.0;
        if (std::abs(s1 - s0) > 1e-9) {
            alpha = (targetLength - s0) / (s1 - s0);
        }

        result.emplace_back(
            p0.x + alpha * (p1.x - p0.x),
            p0.y + alpha * (p1.y - p0.y));
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
    if (maskedPath.empty()) {
        return;
    }

    if (maskedPath.size() == 1) {
        rightPts.push_back(maskedPath.front());
        return;
    }

    size_t topIdx = 0;
    size_t bottomIdx = 0;
    for (size_t i = 1; i < maskedPath.size(); ++i) {
        if (maskedPath[i].y < maskedPath[topIdx].y ||
            (std::abs(maskedPath[i].y - maskedPath[topIdx].y) <= 1e-6 &&
                maskedPath[i].x < maskedPath[topIdx].x)) {
            topIdx = i;
        }

        if (maskedPath[i].y > maskedPath[bottomIdx].y ||
            (std::abs(maskedPath[i].y - maskedPath[bottomIdx].y) <= 1e-6 &&
                maskedPath[i].x < maskedPath[bottomIdx].x)) {
            bottomIdx = i;
        }
    }

    std::vector<cv::Point2d> chainA;
    std::vector<cv::Point2d> chainB;

    size_t idx = topIdx;
    while (true) {
        chainA.push_back(maskedPath[idx]);
        if (idx == bottomIdx) {
            break;
        }
        idx = (idx + 1) % maskedPath.size();
    }

    idx = topIdx;
    while (true) {
        chainB.push_back(maskedPath[idx]);
        if (idx == bottomIdx) {
            break;
        }
        idx = (idx == 0) ? (maskedPath.size() - 1) : (idx - 1);
    }

    auto averageX = [](const std::vector<cv::Point2d>& chain) -> double {
        if (chain.empty()) {
            return 0.0;
        }
        double sumX = 0.0;
        for (size_t i = 0; i < chain.size(); ++i) {
            sumX += chain[i].x;
        }
        return sumX / static_cast<double>(chain.size());
    };

    if (averageX(chainA) >= averageX(chainB)) {
        rightPts = chainA;
        leftPts = chainB;
    }
    else {
        rightPts = chainB;
        leftPts = chainA;
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
