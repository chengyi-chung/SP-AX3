// windows.h is included only here to avoid polluting other translation units
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#endif

#include "UAXVision.h"
#include <algorithm>
#include <iostream>

// Constructor (init)
UAXVision::UAXVision() {
    calibrationBoardSize = cv::Size(9, 6);
    calibrationSquareSize = 25.0f;
    calibrationROI = cv::Rect();
    lastCalibrationMessage = "Calibration not started.";
    // Initialize with identity / zeros
    cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
    distCoeffs = cv::Mat::zeros(4, 1, CV_64F);   // 魚眼模型使用 4 個畸變係數

    imageSize = cv::Size(0, 0);
}

// Check if calibration is actually set (not default)
bool UAXVision::isCalibrated() const {
    // In fisheye model fx/fy should be >1 and distCoeffs not all zero
    if (cameraMatrix.at<double>(0, 0) <= 1.0001 &&
        cameraMatrix.at<double>(1, 1) <= 1.0001 &&
        cv::countNonZero(distCoeffs) == 0) {
        return false;
    }
    return true;
}

void UAXVision::setCalibrationPattern(const cv::Size& boardSize, float squareSize)
{
    calibrationBoardSize = boardSize;
    calibrationSquareSize = squareSize;
}

cv::Size UAXVision::getCalibrationBoardSize() const
{
    return calibrationBoardSize;
}

float UAXVision::getCalibrationSquareSize() const
{
    return calibrationSquareSize;
}

void UAXVision::setCalibrationROI(const cv::Rect& roi)
{
    calibrationROI = roi;
}

void UAXVision::clearCalibrationROI()
{
    calibrationROI = cv::Rect();
}

cv::Rect UAXVision::getCalibrationROI() const
{
    return calibrationROI;
}

#ifdef _WIN32
// Open a Windows file dialog (multi-select supported)
std::vector<std::string> UAXVision::selectCalibrationFiles()
{
    std::vector<std::string> filePaths;
    char fileBuffer[MAX_PATH * 100] = { 0 };

    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = fileBuffer;
    ofn.nMaxFile = static_cast<DWORD>(sizeof(fileBuffer));
    ofn.lpstrFilter = "Image Files\0*.jpg;*.jpeg;*.png;*.bmp\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = "Select calibration images (multi-select supported)";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

    if (GetOpenFileNameA(&ofn))
    {
        char* ptr = ofn.lpstrFile;
        std::string directory = ptr;
        ptr += directory.length() + 1;

        if (*ptr == '\0')
        {
            filePaths.push_back(directory);
        }
        else
        {
            while (*ptr != '\0')
            {
                std::string entryName = ptr;
                filePaths.push_back(directory + "\\" + entryName);
                ptr += entryName.length() + 1;
            }
        }
    }
    else
    {
        std::cout << "File selection canceled or failed." << std::endl;
    }

    return filePaths;
}
#endif // _WIN32

// Fisheye camera calibration (fisheye model)
double UAXVision::calibrate(const std::vector<std::string>& imagePaths)
{
    try {
        std::vector<cv::Size> candidates = {
            calibrationBoardSize,
            cv::Size(9, 6), cv::Size(8, 6), cv::Size(7, 6),
            cv::Size(11, 8), cv::Size(10, 7), cv::Size(10, 8),
            cv::Size(6, 4), cv::Size(5, 4)
        };

        candidates.erase(std::remove_if(candidates.begin(), candidates.end(),
            [](const cv::Size& s) { return s.width <= 0 || s.height <= 0; }), candidates.end());
        candidates.erase(std::unique(candidates.begin(), candidates.end(),
            [](const cv::Size& a, const cv::Size& b) { return a.width == b.width && a.height == b.height; }),
            candidates.end());

        std::string lastFailureMessage = "Unable to detect a valid calibration grid in at least 3 images.";
        for (const auto& candidate : candidates) {
            double rms = calibrate(imagePaths, candidate, calibrationSquareSize);
            if (rms >= 0.0) {
                setCalibrationPattern(candidate, calibrationSquareSize);
                return rms;
            }

            if (!lastCalibrationMessage.empty()) {
                lastFailureMessage = lastCalibrationMessage;
            }
        }

        lastCalibrationMessage = lastFailureMessage;
        return -1.0;
    }
    catch (const cv::Exception& e) {
        lastCalibrationMessage = std::string("OpenCV calibration failed: ") + e.what();
        calibrationPreviewImage.release();
        return -1.0;
    }
}

double UAXVision::calibrate(const std::vector<std::string>& imagePaths,
    cv::Size boardSize,
    float squareSize)
{
    try {
        setCalibrationPattern(boardSize, squareSize);
        lastCalibrationMessage = "Calibration started.";

        std::vector<std::vector<cv::Point3f>> objectPoints;
        std::vector<std::vector<cv::Point2f>> imagePoints;
        std::vector<cv::Mat> previewTiles;
        calibrationPreviewImage.release();

        // Generate ideal 3D chessboard points (Z=0)
        std::vector<cv::Point3f> obj;
        for (int i = 0; i < boardSize.height; ++i) {
            for (int j = 0; j < boardSize.width; ++j) {
                obj.emplace_back(j * squareSize, i * squareSize, 0.0f);
            }
        }

        cv::Size currentSize(0, 0);

        for (const auto& path : imagePaths)
        {
            cv::Mat img = cv::imread(path, cv::IMREAD_GRAYSCALE);
            cv::Mat color = cv::imread(path, cv::IMREAD_COLOR);
            if (img.empty()) {
                std::cerr << "Cannot read image: " << path << " -> skipped\n";
                lastCalibrationMessage = "Cannot read one or more calibration images.";
                continue;
            }
            if (color.empty()) {
                cv::cvtColor(img, color, cv::COLOR_GRAY2BGR);
            }

            // Check size consistency
            if (currentSize.area() > 0 && img.size() != currentSize) {
                std::cerr << "Inconsistent size in " << path
                    << " (expected " << currentSize << ", got " << img.size() << ") -> skipped\n";
                lastCalibrationMessage = "Calibration images must all have the same size.";
                continue;
            }
            if (currentSize.area() == 0) {
                currentSize = img.size();
            }

            cv::Rect roi = calibrationROI & cv::Rect(0, 0, img.cols, img.rows);
            cv::Mat searchImg = img;
            if (roi.width > 0 && roi.height > 0) {
                searchImg = img(roi);
                cv::rectangle(color, roi, cv::Scalar(255, 255, 0), 2);
            }

            try {
                std::vector<cv::Point2f> corners;
                bool found = cv::findChessboardCornersSB(
                    searchImg, boardSize, corners,
                    cv::CALIB_CB_EXHAUSTIVE |
                    cv::CALIB_CB_ACCURACY |
                    cv::CALIB_CB_NORMALIZE_IMAGE);

                if (!found) {
                    found = cv::findChessboardCorners(
                        searchImg, boardSize, corners,
                        cv::CALIB_CB_ADAPTIVE_THRESH |
                        cv::CALIB_CB_NORMALIZE_IMAGE);
                }

                if (found)
                {
                    cv::cornerSubPix(
                        searchImg, corners,
                        cv::Size(11, 11), cv::Size(-1, -1),
                        cv::TermCriteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 40, 0.001));

                    if (roi.width > 0 && roi.height > 0) {
                        for (auto& pt : corners) {
                            pt.x += static_cast<float>(roi.x);
                            pt.y += static_cast<float>(roi.y);
                        }
                    }

                    imagePoints.push_back(corners);
                    objectPoints.push_back(obj);

                    cv::drawChessboardCorners(color, boardSize, corners, true);
                    cv::putText(color, "FOUND", cv::Point(20, 40),
                        cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);
                    previewTiles.push_back(color);
                }
                else
                {
                    cv::putText(color, "NOT FOUND", cv::Point(20, 40),
                        cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 255), 2);
                    previewTiles.push_back(color);
                    std::cerr << "Chessboard not found in: " << path << " -> skipped\n";
                }
            }
            catch (const cv::Exception& e) {
                cv::putText(color, "OPENCV ERROR", cv::Point(20, 40),
                    cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255), 2);
                previewTiles.push_back(color);
                std::cerr << "OpenCV error in calibration image " << path << ": " << e.what() << "\n";
            }
        }

        if (imagePoints.size() < 3) {
            std::cerr << "Error: Not enough valid images (need at least 3)\n";
            if (calibrationROI.width > 0 && calibrationROI.height > 0) {
                lastCalibrationMessage = "Not enough valid calibration images inside the selected ROI. Need at least 3 images with detected corners.";
            }
            else {
                lastCalibrationMessage = "Not enough valid calibration images. Need at least 3 images with detected corners.";
            }
            return -1.0;
        }

        std::vector<cv::Mat> rvecs, tvecs;

        // Reset intrinsics and distortion
        cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
        distCoeffs = cv::Mat::zeros(4, 1, CV_64F);

        int baseFlags = cv::fisheye::CALIB_RECOMPUTE_EXTRINSIC |
            cv::fisheye::CALIB_FIX_SKEW;
        int strictFlags = baseFlags | cv::fisheye::CALIB_CHECK_COND;

        double rms = -1.0;
        bool usedRelaxedCalibration = false;
        try {
            rms = cv::fisheye::calibrate(
                objectPoints, imagePoints, currentSize,
                cameraMatrix, distCoeffs, rvecs, tvecs,
                strictFlags,
                cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 100, 1e-6));
        }
        catch (const cv::Exception& e) {
            const std::string errorText = e.what();
            if (errorText.find("CALIB_CHECK_COND") == std::string::npos &&
                errorText.find("ill-conditioned") == std::string::npos) {
                throw;
            }

            cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
            distCoeffs = cv::Mat::zeros(4, 1, CV_64F);
            rvecs.clear();
            tvecs.clear();

            rms = cv::fisheye::calibrate(
                objectPoints, imagePoints, currentSize,
                cameraMatrix, distCoeffs, rvecs, tvecs,
                baseFlags,
                cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 100, 1e-6));
            usedRelaxedCalibration = true;
        }

        imageSize = currentSize;
        if (!previewTiles.empty()) {
            size_t previewCount = std::min<size_t>(previewTiles.size(), 4);
            int cellWidth = 0;
            int cellHeight = 0;
            for (size_t i = 0; i < previewCount; ++i) {
                cellWidth = std::max(cellWidth, previewTiles[i].cols);
                cellHeight = std::max(cellHeight, previewTiles[i].rows);
            }

            int columns = previewCount > 1 ? 2 : 1;
            int rows = static_cast<int>((previewCount + columns - 1) / columns);
            calibrationPreviewImage = cv::Mat::zeros(rows * cellHeight, columns * cellWidth, CV_8UC3);

            for (size_t i = 0; i < previewCount; ++i) {
                cv::Mat resized;
                if (previewTiles[i].cols != cellWidth || previewTiles[i].rows != cellHeight) {
                    cv::resize(previewTiles[i], resized, cv::Size(cellWidth, cellHeight));
                }
                else {
                    resized = previewTiles[i];
                }

                int row = static_cast<int>(i) / columns;
                int col = static_cast<int>(i) % columns;
                resized.copyTo(calibrationPreviewImage(cv::Rect(col * cellWidth, row * cellHeight, cellWidth, cellHeight)));
            }
        }

        std::cout << "Fisheye calibration completed.\n"
            << "RMS reprojection error: " << rms << " pixels\n";
        if (rms > 0.8) {
            std::cout << "  [Warning] RMS > 0.8 -> consider more images or better poses\n";
        }
        lastCalibrationMessage = "Calibration completed successfully.";
        if (usedRelaxedCalibration) {
            lastCalibrationMessage += " Condition check was relaxed because the image set is ill-conditioned; add more varied board poses for a more stable result.";
        }

        return rms;
    }
    catch (const cv::Exception& e) {
        lastCalibrationMessage = std::string("OpenCV calibration failed: ") + e.what();
        if (std::string(e.what()).find("InitExtrinsics") != std::string::npos) {
            lastCalibrationMessage +=
                " The detected chessboard points are likely inconsistent for this board size or the calibration images do not provide enough pose variation.";
        }
        calibrationPreviewImage.release();
        return -1.0;
    }
}

cv::Mat UAXVision::getCalibrationPreviewImage() const
{
    return calibrationPreviewImage.clone();
}

std::string UAXVision::getLastCalibrationMessage() const
{
    return lastCalibrationMessage;
}

// 去畸變（支援 balance 參數控制黑邊與視場取捨）
cv::Mat UAXVision::undistortImage(const cv::Mat& inputImage, double balance) const
{
    if (!isCalibrated()) {
        std::cerr << "[Warning] undistortImage called before calibration. "
            << "Result will be incorrect.\n";
        return inputImage.clone();
    }

    if (inputImage.empty()) {
        return cv::Mat();
    }

    cv::Mat newK;
    cv::fisheye::estimateNewCameraMatrixForUndistortRectify(
        cameraMatrix, distCoeffs, inputImage.size(),
        cv::Matx33d::eye(), newK, balance);

    cv::Mat output;
    cv::fisheye::undistortImage(inputImage, output, cameraMatrix, distCoeffs, newK);

    return output;
}

// Feature matching (basic ORB/BF)
cv::Mat UAXVision::matchFeatures(const cv::Mat& img1, const cv::Mat& img2)
{
    auto toGray = [](const cv::Mat& img) -> cv::Mat {
        if (img.channels() == 3) {
            cv::Mat gray; cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY); return gray;
        }
        if (img.channels() == 4) {
            cv::Mat gray; cv::cvtColor(img, gray, cv::COLOR_BGRA2GRAY); return gray;
        }
        return img;
        };

    cv::Mat gray1 = toGray(img1);
    cv::Mat gray2 = toGray(img2);

    cv::Ptr<cv::ORB> detector = cv::ORB::create(2000); // 增加一點特徵點數量

    std::vector<cv::KeyPoint> kp1, kp2;
    cv::Mat desc1, desc2;

    detector->detectAndCompute(gray1, cv::noArray(), kp1, desc1);
    detector->detectAndCompute(gray2, cv::noArray(), kp2, desc2);

    if (desc1.empty() || desc2.empty()) {
        std::cerr << "Failed to extract descriptors.\n";
        return cv::Mat();
    }

    cv::BFMatcher matcher(cv::NORM_HAMMING, true);
    std::vector<cv::DMatch> matches;
    matcher.match(desc1, desc2, matches);

    std::sort(matches.begin(), matches.end(),
        [](const cv::DMatch& a, const cv::DMatch& b) { return a.distance < b.distance; });

    int n = std::min(60, (int)matches.size());
    std::vector<cv::DMatch> good(matches.begin(), matches.begin() + n);

    cv::Mat vis;
    cv::drawMatches(gray1, kp1, gray2, kp2, good, vis,
        cv::Scalar::all(-1), cv::Scalar::all(-1),
        std::vector<char>(), cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

    return vis;
}

// Getters
cv::Mat UAXVision::getCameraMatrix() const { return cameraMatrix.clone(); }
cv::Mat UAXVision::getDistCoeffs()   const { return distCoeffs.clone(); }
cv::Size UAXVision::getImageSize()   const { return imageSize; }

// Save calibration data (包含 imageSize)
bool UAXVision::saveCalibrationData(const std::string& filename) const
{
    cv::FileStorage fs(filename, cv::FileStorage::WRITE);
    if (!fs.isOpened()) {
        std::cerr << "Cannot open file for writing: " << filename << "\n";
        return false;
    }

    fs << "cameraMatrix" << cameraMatrix;
    fs << "distCoeffs" << distCoeffs;
    fs << "imageWidth" << imageSize.width;
    fs << "imageHeight" << imageSize.height;

    fs.release();
    std::cout << "Calibration saved to: " << filename << "\n";
    return true;
}

// Load calibration data
bool UAXVision::loadCalibrationData(const std::string& filename)
{
    cv::FileStorage fs(filename, cv::FileStorage::READ);
    if (!fs.isOpened()) {
        std::cerr << "Cannot open file for reading: " << filename << "\n";
        return false;
    }

    fs["cameraMatrix"] >> cameraMatrix;
    fs["distCoeffs"] >> distCoeffs;

    int w = 0, h = 0;
    fs["imageWidth"] >> w;
    fs["imageHeight"] >> h;
    if (w > 0 && h > 0) {
        imageSize = cv::Size(w, h);
    }

    fs.release();

    if (cameraMatrix.empty() || distCoeffs.empty() || distCoeffs.total() != 4) {
        std::cerr << "Invalid calibration data loaded from " << filename << "\n";
        return false;
    }

    std::cout << "Calibration loaded from: " << filename << "\n";
    return true;
}
