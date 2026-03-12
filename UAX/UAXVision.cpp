// windows.h is included only here to avoid polluting other translation units
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#endif

#include "UAXVision.h"
#include <algorithm>

// Constructor (init)
UAXVision::UAXVision() {
    // Initialize with identity / zeros
    cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
    distCoeffs = cv::Mat::zeros(8, 1, CV_64F);
}

// [修正 3] 判斷校正參數是否已被實際設定（非建構子預設值）
bool UAXVision::isCalibrated() const {
    // 若 fx（cameraMatrix[0,0]）仍為預設的 1.0 且 distCoeffs 全為零，
    // 視為尚未校正
    if (cameraMatrix.at<double>(0, 0) == 1.0 &&
        cameraMatrix.at<double>(1, 1) == 1.0 &&
        cv::countNonZero(distCoeffs) == 0) {
        return false;
    }
    return true;
}

// ─────────────────────────────────────────────
// [修正 5] 僅在 Windows 平台編譯 Win32 對話框
// ─────────────────────────────────────────────
#ifdef _WIN32
// Open a Windows file dialog (supports multi-select)
std::vector<std::string> UAXVision::selectCalibrationFiles()
{
    std::vector<std::string> filePaths;

    // Use a dedicated buffer name to avoid clashing with std::string variables
    char fileBuffer[MAX_PATH * 100] = { 0 };

    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = fileBuffer;
    ofn.nMaxFile = static_cast<DWORD>(sizeof(fileBuffer));
    ofn.lpstrFilter = "Image Files\0*.jpg;*.jpeg;*.png;*.bmp\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = "Select images (multi-select supported)";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

    if (GetOpenFileNameA(&ofn))
    {
        char* ptr = ofn.lpstrFile;
        std::string directory = ptr;
        ptr += directory.length() + 1;

        if (*ptr == '\0')
        {
            // Single selection: lpstrFile already contains the full path
            filePaths.push_back(directory);
        }
        else
        {
            // Multiple selection: first segment is directory, following are file names
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
        std::cout << "Selection canceled or an error occurred." << std::endl;
    }

    return filePaths;
}
#endif // _WIN32

// ─────────────────────────────────────────────
// Camera calibration
// [修正 1] 文件說明 boardSize 為「內部角點」數量
// [修正 2] 回傳 RMS 誤差（double），失敗時回傳 -1.0
// [修正 6] 偵測影像尺寸不一致並跳過問題影像
// ─────────────────────────────────────────────
double UAXVision::calibrate(const std::vector<std::string>& imagePaths,
    cv::Size boardSize,
    float squareSize) {

    std::vector<std::vector<cv::Point3f>> objectPoints;
    std::vector<std::vector<cv::Point2f>> imagePoints;

    // Generate ideal chessboard 3D coordinates (Z=0 plane)
    std::vector<cv::Point3f> obj;
    for (int i = 0; i < boardSize.height; i++) {
        for (int j = 0; j < boardSize.width; j++) {
            obj.push_back(cv::Point3f(j * squareSize, i * squareSize, 0.0f));
        }
    }

    cv::Size imageSize;

    for (const auto& path : imagePaths) {
        cv::Mat img = cv::imread(path, cv::IMREAD_GRAYSCALE);
        if (img.empty()) {
            std::cerr << "Warning: cannot read image " << path << ", skip." << std::endl;
            continue;
        }

        // Check if image size matches the first successful image
        if (imageSize.width > 0 && imageSize.height > 0 && (img.size() != imageSize)) {
            std::cerr << "Warning: inconsistent image size in " << path
                << " (expected " << imageSize.width << "x" << imageSize.height
                << ", got " << img.cols << "x" << img.rows << "), skip." << std::endl;
            continue;
        }
        if (imageSize.width == 0 && imageSize.height == 0) {
            imageSize = img.size();
        }

        std::vector<cv::Point2f> corners;

        bool found = cv::findChessboardCorners(
            img, boardSize, corners,
            cv::CALIB_CB_ADAPTIVE_THRESH |
            cv::CALIB_CB_NORMALIZE_IMAGE |
            cv::CALIB_CB_FAST_CHECK);

        if (found) {
            cv::cornerSubPix(
                img, corners,
                cv::Size(11, 11), cv::Size(-1, -1),
                cv::TermCriteria(
                    cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER,
                    30, 0.001));

            imagePoints.push_back(corners);
            objectPoints.push_back(obj);
        }
        else {
            std::cerr << "Warning: cannot find chessboard corners in " << path << ", skip." << std::endl;
        }
    }

    if (imagePoints.empty()) {
        std::cerr << "Error: no chessboard corners found in any image." << std::endl;
        return -1.0;
    }

    std::vector<cv::Mat> rvecs, tvecs;
    double rms = cv::calibrateCamera(
        objectPoints, imagePoints, imageSize,
        cameraMatrix, distCoeffs, rvecs, tvecs);

    std::cout << "Calibration done. RMS reprojection error: " << rms << " px";
    if (rms > 1.0)
        std::cout << "  *** Warning: RMS > 1.0, consider recapturing images ***";
    std::cout << std::endl;

    return rms;
}

// ─────────────────────────────────────────────
// Undistort image
// [修正 3] 若校正參數尚未設定則印出警告
// ─────────────────────────────────────────────
cv::Mat UAXVision::undistortImage(const cv::Mat& inputImage) {
    // [修正 3] 防呆：尚未校正時給出明確警告
    if (!isCalibrated()) {
        std::cerr << "Warning: undistortImage() called before calibration. "
            << "Results will be incorrect. "
            << "Call calibrate() or loadCalibrationData() first." << std::endl;
    }

    cv::Mat outputImage;
    cv::undistort(inputImage, outputImage, cameraMatrix, distCoeffs);
    return outputImage;
}

// ─────────────────────────────────────────────
// Feature matching using ORB + BFMatcher
// [修正 4] 自動將彩色影像轉換為灰階
// ─────────────────────────────────────────────
cv::Mat UAXVision::matchFeatures(const cv::Mat& img1, const cv::Mat& img2) {

    // [修正 4] 自動轉灰階 lambda
    auto toGray = [](const cv::Mat& img) -> cv::Mat {
        if (img.channels() == 3) {
            cv::Mat gray;
            cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
            return gray;
        }
        if (img.channels() == 4) {
            cv::Mat gray;
            cv::cvtColor(img, gray, cv::COLOR_BGRA2GRAY);
            return gray;
        }
        return img; // already single channel
        };

    cv::Mat gray1 = toGray(img1);
    cv::Mat gray2 = toGray(img2);

    cv::Ptr<cv::ORB> detector = cv::ORB::create();

    std::vector<cv::KeyPoint> keypoints1, keypoints2;
    cv::Mat descriptors1, descriptors2;

    detector->detectAndCompute(gray1, cv::noArray(), keypoints1, descriptors1);
    detector->detectAndCompute(gray2, cv::noArray(), keypoints2, descriptors2);

    if (descriptors1.empty() || descriptors2.empty()) {
        std::cerr << "Error: failed to extract features from image." << std::endl;
        return cv::Mat();
    }

    // crossCheck=true keeps only mutual best matches
    cv::BFMatcher matcher(cv::NORM_HAMMING, true);
    std::vector<cv::DMatch> matches;
    matcher.match(descriptors1, descriptors2, matches);

    // Sort by distance and take top 50 matches
    std::sort(matches.begin(), matches.end(),
        [](const cv::DMatch& a, const cv::DMatch& b) {
            return a.distance < b.distance;
        });

    const int numGoodMatches = std::min(50, static_cast<int>(matches.size()));
    std::vector<cv::DMatch> goodMatches(matches.begin(),
        matches.begin() + numGoodMatches);

    cv::Mat imgMatches;
    cv::drawMatches(
        gray1, keypoints1,
        gray2, keypoints2,
        goodMatches, imgMatches,
        cv::Scalar::all(-1), cv::Scalar::all(-1),
        std::vector<char>(),
        cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

    return imgMatches;
}

// Getters
cv::Mat UAXVision::getCameraMatrix() const {
    return cameraMatrix.clone();
}

cv::Mat UAXVision::getDistCoeffs() const {
    return distCoeffs.clone();
}

// Save calibration data to .xml / .yml
bool UAXVision::saveCalibrationData(const std::string& filename) const {
    cv::FileStorage fs(filename, cv::FileStorage::WRITE);
    if (!fs.isOpened()) {
        std::cerr << "Error: cannot open file " << filename << " for write." << std::endl;
        return false;
    }

    fs << "cameraMatrix" << cameraMatrix;
    fs << "distCoeffs" << distCoeffs;
    fs.release();

    std::cout << "Calibration data saved to " << filename << std::endl;
    return true;
}

// Load calibration data from .xml / .yml
bool UAXVision::loadCalibrationData(const std::string& filename) {
    cv::FileStorage fs(filename, cv::FileStorage::READ);
    if (!fs.isOpened()) {
        std::cerr << "Error: cannot open file " << filename << " for read." << std::endl;
        return false;
    }

    fs["cameraMatrix"] >> cameraMatrix;
    fs["distCoeffs"] >> distCoeffs;
    fs.release();

    if (cameraMatrix.empty() || distCoeffs.empty()) {
        std::cerr << "Error: failed to load calibration data from " << filename << "." << std::endl;
        return false;
    }

    std::cout << "Calibration data loaded from " << filename << std::endl;
    return true;
}