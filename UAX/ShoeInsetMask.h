#pragma once
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

// Input follows the operator preview: black shoe, white background.
// Output is an internal white shoe mask for contour extraction, not a preview.
inline cv::Mat BuildShoeInsetMask(const cv::Mat& binary, const cv::Mat& roi,
    double insetPixels)
{
    if (binary.type() != CV_8UC1 || roi.type() != CV_8UC1 ||
        binary.size() != roi.size() || !std::isfinite(insetPixels) || insetPixels < 0.0) {
        throw std::invalid_argument("Invalid shoe mask, ROI or inward offset.");
    }

    cv::Mat shoe;
    cv::compare(binary, 0, shoe, cv::CMP_EQ);
    cv::bitwise_and(shoe, roi, shoe); // Exclude outside pixels before component selection.
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(shoe, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    cv::Mat body = cv::Mat::zeros(binary.size(), CV_8UC1);
    if (contours.empty()) return body;
    const auto largest = std::max_element(contours.begin(), contours.end(),
        [](const auto& a, const auto& b) {
            return std::abs(cv::contourArea(a)) < std::abs(cv::contourArea(b));
        });
    if (cv::contourArea(*largest) == 0.0) return body;
    // One shoe per ROI: discard detached black speckles and fill internal white pinholes.
    cv::drawContours(body, contours, static_cast<int>(largest - contours.begin()),
        cv::Scalar(255), cv::FILLED);
    cv::bitwise_and(body, roi, body);

    if (insetPixels >= (std::max)(binary.cols, binary.rows)) return cv::Mat::zeros(binary.size(), CV_8UC1);
    const int iterations = static_cast<int>(std::lround(insetPixels));
    if (iterations > 0) {
        // ROI is a clipping window, not a physical shoe edge. Neutral foreground
        // outside it prevents an artificial inset along the cropped bottom edge.
        body.setTo(255, roi == 0);
        cv::erode(body, body, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3)),
            cv::Point(-1, -1), iterations);
        cv::bitwise_and(body, roi, body);
    }
    return body;
}
