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
//  【最終版】OptimizePath
// ══════════════════════════════════════════════════════════════════════════════
void GluePathOptimizer::OptimizePath(const std::vector<cv::Point2d>& inputPath,
    GluePath& optimizedPath,
    int                              shoeType)
{
    // Step 1: 濾除 ROI 外雜點
    std::vector<cv::Point2d> maskedPath = FilterByMask(inputPath);
    if (maskedPath.empty())
    {
        optimizedPath.PathRight.clear();
        optimizedPath.PathLeft.clear();
        return;
    }

    // Step 2: 分割左右原始點
    std::vector<cv::Point2d> rightRaw, leftRaw;
    SplitByCenter(maskedPath, rightRaw, leftRaw);

    // shoeType=2 時左右角色互換
    bool swapSides = (shoeType == 2);
    auto& primaryRaw = swapSides ? leftRaw : rightRaw;
    auto& secondaryRaw = swapSides ? rightRaw : leftRaw;

    if (primaryRaw.empty())
    {
        optimizedPath.PathRight.clear();
        optimizedPath.PathLeft.clear();
        return;
    }

    // Step 3: 先對主要側做擬合 → 決定標準Y序列
    std::vector<cv::Point2d> primarySorted = SortByY(primaryRaw);
    std::vector<cv::Point2d> primarySmooth = FitCurve(primarySorted, 30);

    // 取出標準Y序列（兩側共用）
    std::vector<double> standardYs;
    standardYs.reserve(primarySmooth.size());
    for (const auto& p : primarySmooth)
        standardYs.push_back(p.y);

    // Step 4: 對次要側使用相同Y序列做擬合
    std::vector<cv::Point2d> secondarySmooth;
    if (!secondaryRaw.empty())
    {
        secondarySmooth = FitCurveAtGivenY(secondaryRaw, standardYs);
    }
    else
    {
        // 保底：若次要側無點，複製主要側
        secondarySmooth = primarySmooth;
    }

    // Step 5: 寫回輸出（Y值完全一致）
    if (swapSides)
    {
        optimizedPath.PathRight = secondarySmooth;
        optimizedPath.PathLeft = primarySmooth;
    }
    else
    {
        optimizedPath.PathRight = primarySmooth;
        optimizedPath.PathLeft = secondarySmooth;
    }
}

// ══════════════════════════════════════════════════════════════════════════════
//  Step 1 – FilterByMask
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

// ══════════════════════════════════════════════════════════════════════════════
//  Step 2 – SplitByCenter
// ══════════════════════════════════════════════════════════════════════════════
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

// ══════════════════════════════════════════════════════════════════════════════
//  Step 2b – SortByY
// ══════════════════════════════════════════════════════════════════════════════
std::vector<cv::Point2d>
GluePathOptimizer::SortByY(const std::vector<cv::Point2d>& pts)
{
    std::vector<cv::Point2d> sorted = pts;
    std::sort(sorted.begin(), sorted.end(),
        [](const cv::Point2d& a, const cv::Point2d& b)
        { return a.y < b.y; });
    return sorted;
}

// ══════════════════════════════════════════════════════════════════════════════
//  FitCurve（決定標準Y序列）
// ══════════════════════════════════════════════════════════════════════════════
std::vector<cv::Point2d>
GluePathOptimizer::FitCurve(const std::vector<cv::Point2d>& pts, int maxPoints)
{
    if (pts.size() < 2)
        return pts;

    std::vector<double> ys, xs;
    ys.reserve(pts.size());
    xs.reserve(pts.size());
    for (const auto& p : pts)
    {
        ys.push_back(p.y);
        xs.push_back(p.x);
    }

    const int degree = std::min(3, static_cast<int>(pts.size()) - 1);
    std::vector<double> coeffs = PolyFit(ys, xs, degree);

    const double yStart = ys.front();
    const double yEnd = ys.back();
    const int    nPts = std::min(maxPoints, static_cast<int>(pts.size()));

    std::vector<cv::Point2d> result;
    result.reserve(nPts);

    for (int i = 0; i < nPts; ++i)
    {
        double t = (nPts > 1) ? static_cast<double>(i) / (nPts - 1) : 0.0;
        double y = yStart + t * (yEnd - yStart);
        double x = PolyEval(coeffs, y);
        result.emplace_back(x, y);
    }
    return result;
}

// ══════════════════════════════════════════════════════════════════════════════
//  【核心修正】FitCurveAtGivenY – 使用指定Y序列對另一側擬合
// ══════════════════════════════════════════════════════════════════════════════
std::vector<cv::Point2d>
GluePathOptimizer::FitCurveAtGivenY(const std::vector<cv::Point2d>& pts,
    const std::vector<double>& targetYs)
{
    if (pts.empty() || targetYs.empty())
        return {};

    std::vector<double> ys, xs;
    ys.reserve(pts.size());
    xs.reserve(pts.size());
    for (const auto& p : pts)
    {
        ys.push_back(p.y);
        xs.push_back(p.x);
    }

    const int degree = std::min(3, static_cast<int>(pts.size()) - 1);
    std::vector<double> coeffs = PolyFit(ys, xs, degree);

    std::vector<cv::Point2d> result;
    result.reserve(targetYs.size());

    for (double y : targetYs)
    {
        double x = PolyEval(coeffs, y);
        result.emplace_back(x, y);
    }
    return result;
}

// ══════════════════════════════════════════════════════════════════════════════
//  多項式工具函式
// ══════════════════════════════════════════════════════════════════════════════
std::vector<double>
GluePathOptimizer::PolyFit(const std::vector<double>& y,
    const std::vector<double>& x,
    int  degree)
{
    assert(y.size() == x.size() && !y.empty());

    const int n = static_cast<int>(y.size());
    const int d = degree + 1;

    cv::Mat A(n, d, CV_64F);
    for (int i = 0; i < n; ++i)
    {
        double val = 1.0;
        for (int j = 0; j < d; ++j)
        {
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

double GluePathOptimizer::PolyEval(const std::vector<double>& coeffs, double y)
{
    double result = 0.0;
    double power = 1.0;
    for (double c : coeffs)
    {
        result += c * power;
        power *= y;
    }
    return result;
}