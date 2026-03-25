#pragma once
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

class UAXVision {
public:
    UAXVision();

    bool isCalibrated() const;
    void setCalibrationPattern(const cv::Size& boardSize, float squareSize);
    cv::Size getCalibrationBoardSize() const;
    float getCalibrationSquareSize() const;

#ifdef _WIN32
    std::vector<std::string> selectCalibrationFiles();
#endif

    double calibrate(const std::vector<std::string>& imagePaths);
    double calibrate(const std::vector<std::string>& imagePaths,
        cv::Size boardSize,
        float squareSize);
    cv::Mat getCalibrationPreviewImage() const;

    cv::Mat undistortImage(const cv::Mat& inputImage,
        double balance = 0.8) const;

    cv::Mat matchFeatures(const cv::Mat& img1, const cv::Mat& img2);

    cv::Mat getCameraMatrix() const;
    cv::Mat getDistCoeffs() const;
    cv::Size getImageSize() const;

    bool saveCalibrationData(const std::string& filename) const;
    bool loadCalibrationData(const std::string& filename);

private:
    cv::Size calibrationBoardSize;
    float calibrationSquareSize;
    cv::Mat cameraMatrix;
    cv::Mat distCoeffs;
    cv::Size imageSize;
    cv::Mat calibrationPreviewImage;
};
