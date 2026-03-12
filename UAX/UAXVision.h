#pragma once
#ifndef UAXVISION_H
#define UAXVISION_H

#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <vector>
#include <string>
#include <iostream>
// 注意：<windows.h> 已移至 UAXVision.cpp，不在此處引入

class UAXVision {
private:
    // 相機內部參數與畸變係數
    cv::Mat cameraMatrix;
    cv::Mat distCoeffs;

    // [修正 3] 輔助函式：判斷校正參數是否已設定（非預設值）
    bool isCalibrated() const;

public:
    // 建構子
    UAXVision();

    // [修正 5] 僅在 Windows 平台宣告 Win32 檔案對話框
#ifdef _WIN32
    // 開啟 Windows 檔案選取對話框 (支援多選)
    std::vector<std::string> selectCalibrationFiles();
#endif

    // 1. 相機校正 (Camera Calibration)
    // imagePaths : 用於校正的影像路徑陣列
    // boardSize  : 棋盤格「內部角點」數量（格數 - 1）
    //              例如 10×7 格的棋盤應傳入 cv::Size(9, 6)
    // squareSize : 棋盤格單一格子實際邊長（單位：毫米，例如 25.0f）
    // 回傳值     : RMS 重投影誤差（像素）；回傳 -1.0 表示校正失敗
    //              建議閾值：RMS < 1.0 px 為良好校正結果
    double calibrate(const std::vector<std::string>& imagePaths,
        cv::Size boardSize,
        float squareSize);

    // 影像去畸變 (Undistort)
    // [修正 3] 若校正參數尚未設定，會印出警告訊息
    cv::Mat undistortImage(const cv::Mat& inputImage);

    // 2. 特徵匹配 (Feature Matching)
    // img1, img2 : 欲匹配的兩張影像（灰階或 BGR 均可，內部自動轉換）
    // [修正 4] 彩色影像會自動轉換為灰階後再進行特徵偵測
    cv::Mat matchFeatures(const cv::Mat& img1, const cv::Mat& img2);

    // 取得相機參數矩陣（供外部存取）
    cv::Mat getCameraMatrix() const;
    cv::Mat getDistCoeffs() const;

    // --- 儲存與讀取校正資料 ---
    // 將當前的相機矩陣與畸變係數存成檔案（支援 .xml 或 .yml）
    bool saveCalibrationData(const std::string& filename) const;
    // 從檔案讀取相機矩陣與畸變係數
    bool loadCalibrationData(const std::string& filename);
};

#endif // UAXVISION_H