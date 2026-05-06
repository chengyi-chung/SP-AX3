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

    cv::Point2d FindEntryPointOnProfile(
        const std::vector<cv::Point2d>& profile,
        double entryPointX)
    {
        if (profile.empty()) {
            return cv::Point2d(entryPointX, 0.0);
        }

        cv::Point2d nearest = profile.front();
        double nearestDx = std::abs(profile.front().x - entryPointX);

        for (size_t i = 1; i < profile.size(); ++i) {
            const cv::Point2d& prev = profile[i - 1];
            const cv::Point2d& curr = profile[i];
            const double prevDx = prev.x - entryPointX;
            const double currDx = curr.x - entryPointX;

            if (std::abs(currDx) < nearestDx) {
                nearest = curr;
                nearestDx = std::abs(currDx);
            }

            if (prevDx == 0.0) {
                return cv::Point2d(entryPointX, prev.y);
            }

            if ((prevDx < 0.0 && currDx > 0.0) ||
                (prevDx > 0.0 && currDx < 0.0) ||
                currDx == 0.0) {
                const double dx = curr.x - prev.x;
                if (std::abs(dx) <= 1e-9) {
                    return cv::Point2d(entryPointX, (prev.y + curr.y) * 0.5);
                }

                const double t = (entryPointX - prev.x) / dx;
                return cv::Point2d(entryPointX, prev.y + t * (curr.y - prev.y));
            }
        }

        nearest.x = entryPointX;
        return nearest;
    }

    std::vector<cv::Point2d> BuildProfileFromEntry(
        const std::vector<cv::Point2d>& profile,
        const cv::Point2d& entryPoint)
    {
        std::vector<cv::Point2d> result;
        if (profile.empty()) {
            result.push_back(entryPoint);
            return result;
        }

        result.reserve(profile.size() + 1);
        result.push_back(entryPoint);

        for (size_t i = 0; i < profile.size(); ++i) {
            if (profile[i].y > entryPoint.y + 1e-6) {
                result.push_back(profile[i]);
            }
        }

        return result;
    }

    cv::Point2d InterpolateProfileAtY(
        const std::vector<cv::Point2d>& profile,
        double targetY)
    {
        if (profile.empty()) {
            return cv::Point2d(0.0, targetY);
        }

        if (profile.size() == 1) {
            return cv::Point2d(profile.front().x, targetY);
        }

        size_t upperIdx = 1;
        if (targetY <= profile.front().y) {
            upperIdx = 1;
        }
        else if (targetY >= profile.back().y) {
            upperIdx = profile.size() - 1;
        }
        else {
            while (upperIdx < profile.size() && profile[upperIdx].y < targetY) {
                ++upperIdx;
            }
        }

        const cv::Point2d& p0 = profile[upperIdx - 1];
        const cv::Point2d& p1 = profile[upperIdx];
        const double dy = p1.y - p0.y;
        if (std::abs(dy) <= 1e-9) {
            return cv::Point2d((p0.x + p1.x) * 0.5, targetY);
        }

        const double t = (targetY - p0.y) / dy;
        return cv::Point2d(p0.x + t * (p1.x - p0.x), targetY);
    }

    std::vector<cv::Point2d> FitProfileAtGivenY(
        const std::vector<cv::Point2d>& profile,
        const std::vector<double>& targetYs)
    {
        std::vector<cv::Point2d> result;
        result.reserve(targetYs.size());
        for (size_t i = 0; i < targetYs.size(); ++i) {
            result.push_back(InterpolateProfileAtY(profile, targetYs[i]));
        }
        return result;
    }

    cv::Point2d FindBottomSidePoint(
        const std::vector<cv::Point2d>& rawPts,
        bool keepRightMost)
    {
        if (rawPts.empty()) {
            return cv::Point2d();
        }

        double minY = rawPts.front().y;
        double maxY = rawPts.front().y;
        for (size_t i = 1; i < rawPts.size(); ++i) {
            minY = (std::min)(minY, rawPts[i].y);
            maxY = (std::max)(maxY, rawPts[i].y);
        }

        const double bottomBand = (std::max)(3.0, (maxY - minY) * 0.03);
        const double yLimit = maxY - bottomBand;

        cv::Point2d chosen = rawPts.front();
        bool hasChosen = false;
        for (size_t i = 0; i < rawPts.size(); ++i) {
            if (rawPts[i].y < yLimit) {
                continue;
            }

            if (!hasChosen) {
                chosen = rawPts[i];
                hasChosen = true;
                continue;
            }

            if (keepRightMost) {
                if (rawPts[i].x > chosen.x) {
                    chosen = rawPts[i];
                }
            }
            else {
                if (rawPts[i].x < chosen.x) {
                    chosen = rawPts[i];
                }
            }
        }

        if (!hasChosen) {
            for (size_t i = 1; i < rawPts.size(); ++i) {
                if (rawPts[i].y > chosen.y ||
                    (std::abs(rawPts[i].y - chosen.y) <= 1e-6 &&
                        ((keepRightMost && rawPts[i].x > chosen.x) ||
                            (!keepRightMost && rawPts[i].x < chosen.x)))) {
                    chosen = rawPts[i];
                }
            }
        }

        return chosen;
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
    (void)shoeType;

    std::vector<cv::Point2d> maskedPath = FilterByMask(inputPath);
    if (maskedPath.empty()) {
        optimizedPath.PathRight.clear();
        optimizedPath.PathLeft.clear();
        return;
    }

    std::vector<cv::Point2d> rightRaw;
    std::vector<cv::Point2d> leftRaw;
    SplitByCenter(maskedPath, rightRaw, leftRaw);

    if (rightRaw.empty() || leftRaw.empty()) {
        optimizedPath.PathRight.clear();
        optimizedPath.PathLeft.clear();
        return;
    }

    std::vector<cv::Point2d> rightProfile = BuildSideProfileByY(rightRaw, true);
    std::vector<cv::Point2d> leftProfile = BuildSideProfileByY(leftRaw, false);
    if (rightProfile.empty() || leftProfile.empty()) {
        optimizedPath.PathRight.clear();
        optimizedPath.PathLeft.clear();
        return;
    }

    const cv::Point2d entryPoint = FindEntryPointOnProfile(rightProfile, mROI.EntryPointX);
    rightProfile = BuildProfileFromEntry(rightProfile, entryPoint);

    std::vector<cv::Point2d> rightSmoothPts = SampleProfilePoints(rightProfile, 30);
    if (rightSmoothPts.empty()) {
        optimizedPath.PathRight.clear();
        optimizedPath.PathLeft.clear();
        return;
    }

    rightSmoothPts.front() = entryPoint;

    std::vector<double> targetYs;
    targetYs.reserve(rightSmoothPts.size());
    for (size_t i = 0; i < rightSmoothPts.size(); ++i) {
        targetYs.push_back(rightSmoothPts[i].y);
    }

    std::vector<cv::Point2d> leftSmoothPts = FitProfileAtGivenY(leftProfile, targetYs);
    if (leftSmoothPts.size() != rightSmoothPts.size()) {
        optimizedPath.PathRight.clear();
        optimizedPath.PathLeft.clear();
        return;
    }

    if (!rightSmoothPts.empty() && !leftSmoothPts.empty()) {
        rightSmoothPts.back() = FindBottomSidePoint(rightRaw, true);
        leftSmoothPts.back() = FindBottomSidePoint(leftRaw, false);
        leftSmoothPts.back().y = rightSmoothPts.back().y;
    }

    optimizedPath.PathRight = rightSmoothPts;
    optimizedPath.PathLeft = leftSmoothPts;
    for (size_t i = 0; i < optimizedPath.PathRight.size(); ++i) {
        optimizedPath.PathLeft[i].y = optimizedPath.PathRight[i].y;
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

    std::vector<cv::Point2d> result;
    result.reserve(targetYs.size());

    size_t lastSegmentIndex = 0;
    bool hasPrevious = false;
    cv::Point2d previousPoint;

    for (size_t i = 0; i < targetYs.size(); ++i) {
        const double ty = targetYs[i];
        std::vector<cv::Point2d> candidates;
        std::vector<size_t> candidateSegments;

        for (size_t j = 1; j < pts.size(); ++j) {
            const cv::Point2d& p0 = pts[j - 1];
            const cv::Point2d& p1 = pts[j];
            const double y0 = p0.y;
            const double y1 = p1.y;
            const double yMin = (std::min)(y0, y1);
            const double yMax = (std::max)(y0, y1);

            if (ty < yMin - 1e-6 || ty > yMax + 1e-6) {
                continue;
            }

            const double dy = y1 - y0;
            if (std::abs(dy) <= 1e-9) {
                candidates.emplace_back((p0.x + p1.x) * 0.5, ty);
            }
            else {
                const double t = (ty - y0) / dy;
                candidates.emplace_back(p0.x + t * (p1.x - p0.x), ty);
            }
            candidateSegments.push_back(j - 1);
        }

        if (candidates.empty()) {
            cv::Point2d nearest = pts.front();
            double bestDy = std::abs(pts.front().y - ty);
            for (size_t j = 1; j < pts.size(); ++j) {
                const double distY = std::abs(pts[j].y - ty);
                if (distY < bestDy) {
                    bestDy = distY;
                    nearest = pts[j];
                }
            }
            nearest.y = ty;
            candidates.push_back(nearest);
            candidateSegments.push_back(lastSegmentIndex);
        }

        size_t bestIndex = 0;
        double bestScore = std::numeric_limits<double>::max();
        for (size_t j = 0; j < candidates.size(); ++j) {
            double score = 0.0;
            if (hasPrevious) {
                const double dx = candidates[j].x - previousPoint.x;
                const double dy = candidates[j].y - previousPoint.y;
                score += dx * dx + dy * dy;
            }
            else {
                score += candidates[j].x * candidates[j].x;
            }

            if (candidateSegments[j] < lastSegmentIndex) {
                score += 1.0e9;
            }

            if (score < bestScore) {
                bestScore = score;
                bestIndex = j;
            }
        }

        result.push_back(candidates[bestIndex]);
        previousPoint = candidates[bestIndex];
        lastSegmentIndex = candidateSegments[bestIndex];
        hasPrevious = true;
    }

    return result;
}

// ══════════════════════════════════════════════════════════════════════════════
//  其餘函式（FilterByMask、SplitByCenter、PolyFit、PolyEval）
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
