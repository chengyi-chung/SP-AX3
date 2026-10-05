#include "../UAX/ShoeInsetMask.h"
#include <iostream>

static void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

int main() {
    cv::Mat binary(160, 180, CV_8UC1, cv::Scalar(255));
    cv::Mat roi = cv::Mat::zeros(binary.size(), CV_8UC1);
    roi(cv::Rect(30, 20, 120, 120)).setTo(255);
    // Shoe continues below ROI; outside-ROI noise must not become the body.
    binary(cv::Rect(65, 40, 50, 120)).setTo(0);
    binary(cv::Rect(0, 0, 25, 160)).setTo(0);
    binary(cv::Rect(40, 60, 3, 3)).setTo(0);
    binary(cv::Rect(85, 80, 3, 3)).setTo(255);
    auto zero = BuildShoeInsetMask(binary, roi, 0);
    auto inset = BuildShoeInsetMask(binary, roi, 5);
    auto deeper = BuildShoeInsetMask(binary, roi, 10);
    require(cv::countNonZero(zero) == 50 * 100, "body selection or hole filling failed");
    require(cv::countNonZero(inset) == 40 * 95, "inset must shrink physical edges by 5 pixels");
    require(inset.at<uchar>(139, 90) == 255, "ROI bottom was incorrectly treated as shoe edge");
    require(cv::countNonZero(inset & ~zero) == 0, "inset expanded outside shoe");
    require(cv::countNonZero(deeper & ~inset) == 0, "larger offset expanded body");
    require(cv::countNonZero(inset & ~roi) == 0, "output leaked beyond ROI");
    require(inset.at<uchar>(61, 41) == 0, "detached noise retained");
    require(inset.at<uchar>(81, 86) == 255, "internal pinhole retained");
    require(cv::countNonZero(BuildShoeInsetMask(binary, roi, 30)) == 0, "oversized inset retained shoe");
    require(cv::countNonZero(BuildShoeInsetMask(cv::Mat(binary.size(), CV_8UC1, cv::Scalar(255)), roi, 5)) == 0, "empty shoe produced path");
    bool rejected = false;
    try { BuildShoeInsetMask(binary, roi, -1); }
    catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "negative inward offset accepted");
    std::cout << "PASS: black shoe, ROI isolation, noise removal, pinhole filling, inward monotonicity, cropped bottom, empty/oversized/negative offsets\n";
}
