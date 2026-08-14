// windows.h is included only here to avoid polluting other translation units
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#endif

#include "UAXVision.h"
#include <algorithm>
#include <cmath>
#include <iostream>

// Constructor (init)
UAXVision::UAXVision() {
    calibrationBoardSize = cv::Size(5, 7);
    calibrationSquareSize = 25.0f;
    calibrationROI = cv::Rect();
    lastCalibrationMessage = "Calibration not started.";
    // Initialize with identity / zeros
    cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
    distCoeffs = cv::Mat::zeros(4, 1, CV_64F);   // 魚眼模型使用 4 個畸變係數
    homographyMatrix.release();
    calibrationPixelsPerSquare = 0.0;
    calibrationMmPerPixel = 0.0;

    imageSize = cv::Size(0, 0);
}

// Check if calibration is actually set (not default)
bool UAXVision::isCalibrated() const {
    return !homographyMatrix.empty() && homographyMatrix.rows == 3 &&
        homographyMatrix.cols == 3 && cv::checkRange(homographyMatrix) &&
        imageSize.width > 0 && imageSize.height > 0 &&
        calibrationPixelsPerSquare > 0.0 && calibrationMmPerPixel > 0.0;
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
    ofn.lpstrTitle = "Select one full-ROI chessboard image";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;

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

// Single-image planar calibration. A chessboard covering the working ROI is
// rectified to an ideal, equally spaced grid using a homography.
double UAXVision::calibrate(const std::vector<std::string>& imagePaths)
{
    try {
        if (imagePaths.size() != 1) {
            lastCalibrationMessage = "Select exactly one chessboard image that covers the working ROI.";
            return -1.0;
        }

        std::vector<cv::Size> candidates = { calibrationBoardSize };

        candidates.erase(std::remove_if(candidates.begin(), candidates.end(),
            [](const cv::Size& s) { return s.width <= 0 || s.height <= 0; }), candidates.end());
        candidates.erase(std::unique(candidates.begin(), candidates.end(),
            [](const cv::Size& a, const cv::Size& b) { return a.width == b.width && a.height == b.height; }),
            candidates.end());

        std::string lastFailureMessage = "Unable to detect a complete chessboard in the selected ROI.";
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
        if (imagePaths.size() != 1 || boardSize.width < 2 || boardSize.height < 2 || squareSize <= 0.0f) {
            lastCalibrationMessage = "Homography calibration requires one image, a valid board size, and a positive square size.";
            return -1.0;
        }

        setCalibrationPattern(boardSize, squareSize);
        lastCalibrationMessage = "Homography calibration started.";
        calibrationPreviewImage.release();
        cv::Mat gray = cv::imread(imagePaths.front(), cv::IMREAD_GRAYSCALE);
        cv::Mat color = cv::imread(imagePaths.front(), cv::IMREAD_COLOR);
        if (gray.empty()) {
            lastCalibrationMessage = "Cannot read the calibration image.";
            return -1.0;
        }
        if (color.empty()) cv::cvtColor(gray, color, cv::COLOR_GRAY2BGR);

        cv::Rect imageRect(0, 0, gray.cols, gray.rows);
        cv::Rect roi = calibrationROI & imageRect;
        if (roi.width <= 0 || roi.height <= 0) roi = imageRect;
        cv::Mat search = gray(roi);

        std::vector<cv::Point2f> sourceCorners;
        bool found = cv::findChessboardCornersSB(search, boardSize, sourceCorners,
            cv::CALIB_CB_EXHAUSTIVE | cv::CALIB_CB_ACCURACY | cv::CALIB_CB_NORMALIZE_IMAGE);
        if (!found) {
            found = cv::findChessboardCorners(search, boardSize, sourceCorners,
                cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE);
            if (found) {
                cv::cornerSubPix(search, sourceCorners, cv::Size(11, 11), cv::Size(-1, -1),
                    cv::TermCriteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 40, 0.001));
            }
        }
        if (!found || sourceCorners.size() != static_cast<size_t>(boardSize.area())) {
            lastCalibrationMessage = "The complete chessboard was not detected inside the ROI.";
            return -1.0;
        }
        for (auto& p : sourceCorners) {
            p.x += static_cast<float>(roi.x);
            p.y += static_cast<float>(roi.y);
        }

        std::vector<double> spacings;
        for (int row = 0; row < boardSize.height; ++row)
            for (int col = 0; col + 1 < boardSize.width; ++col)
                spacings.push_back(cv::norm(sourceCorners[row * boardSize.width + col + 1] -
                    sourceCorners[row * boardSize.width + col]));
        for (int row = 0; row + 1 < boardSize.height; ++row)
            for (int col = 0; col < boardSize.width; ++col)
                spacings.push_back(cv::norm(sourceCorners[(row + 1) * boardSize.width + col] -
                    sourceCorners[row * boardSize.width + col]));
        if (spacings.empty()) {
            lastCalibrationMessage = "Unable to measure chessboard spacing.";
            return -1.0;
        }
        std::nth_element(spacings.begin(), spacings.begin() + spacings.size() / 2, spacings.end());
        const double pixelsPerSquare = spacings[spacings.size() / 2];
        if (!std::isfinite(pixelsPerSquare) || pixelsPerSquare < 5.0) {
            lastCalibrationMessage = "Chessboard squares are too small or invalid.";
            return -1.0;
        }

        cv::Point2f sourceCenter(0, 0);
        for (const auto& p : sourceCorners) sourceCenter += p;
        sourceCenter *= 1.0f / static_cast<float>(sourceCorners.size());
        // Preserve the chessboard orientation seen in the grabbed image. OpenCV's
        // board width is the first corner-index direction; for a portrait board
        // that direction can be vertical. Mapping it unconditionally to +X rotates
        // the corrected image by 90 degrees.
        cv::Point2f sourceColumnAxis(0, 0);
        cv::Point2f sourceRowAxis(0, 0);
        for (int row = 0; row < boardSize.height; ++row) {
            sourceColumnAxis += sourceCorners[row * boardSize.width + boardSize.width - 1] -
                sourceCorners[row * boardSize.width];
        }
        for (int col = 0; col < boardSize.width; ++col) {
            sourceRowAxis += sourceCorners[(boardSize.height - 1) * boardSize.width + col] -
                sourceCorners[col];
        }

        cv::Point2f idealColumnAxis;
        cv::Point2f idealRowAxis;
        if (std::abs(sourceColumnAxis.x) >= std::abs(sourceColumnAxis.y)) {
            idealColumnAxis = cv::Point2f(sourceColumnAxis.x >= 0.0f ? 1.0f : -1.0f, 0.0f);
            idealRowAxis = cv::Point2f(0.0f, sourceRowAxis.y >= 0.0f ? 1.0f : -1.0f);
        }
        else {
            idealColumnAxis = cv::Point2f(0.0f, sourceColumnAxis.y >= 0.0f ? 1.0f : -1.0f);
            idealRowAxis = cv::Point2f(sourceRowAxis.x >= 0.0f ? 1.0f : -1.0f, 0.0f);
        }

        std::vector<cv::Point2f> idealCorners;
        idealCorners.reserve(sourceCorners.size());
        const float centerCol = static_cast<float>(boardSize.width - 1) * 0.5f;
        const float centerRow = static_cast<float>(boardSize.height - 1) * 0.5f;
        for (int row = 0; row < boardSize.height; ++row) {
            for (int col = 0; col < boardSize.width; ++col) {
                const float colOffset = static_cast<float>((col - centerCol) * pixelsPerSquare);
                const float rowOffset = static_cast<float>((row - centerRow) * pixelsPerSquare);
                idealCorners.emplace_back(sourceCenter +
                    idealColumnAxis * colOffset + idealRowAxis * rowOffset);
            }
        }

        cv::Mat inlierMask;
        cv::Mat candidateH = cv::findHomography(sourceCorners, idealCorners, cv::RANSAC, 2.0, inlierMask);
        if (candidateH.empty() || !cv::checkRange(candidateH) || std::abs(cv::determinant(candidateH)) < 1e-12) {
            lastCalibrationMessage = "Unable to compute a stable homography.";
            return -1.0;
        }

        std::vector<cv::Point2f> projected;
        cv::perspectiveTransform(sourceCorners, projected, candidateH);
        double squaredError = 0.0;
        for (size_t i = 0; i < projected.size(); ++i) {
            const double error = cv::norm(projected[i] - idealCorners[i]);
            squaredError += error * error;
        }
        const double rms = std::sqrt(squaredError / projected.size());
        if (!std::isfinite(rms) || rms > 3.0) {
            lastCalibrationMessage = "Homography reprojection error is too high.";
            return -1.0;
        }

        homographyMatrix = candidateH.clone();
        imageSize = gray.size();
        calibrationPixelsPerSquare = pixelsPerSquare;
        calibrationMmPerPixel = static_cast<double>(squareSize) / pixelsPerSquare;
        cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
        distCoeffs = cv::Mat::zeros(4, 1, CV_64F);

        cv::warpPerspective(color, calibrationPreviewImage, homographyMatrix, imageSize,
            cv::INTER_LINEAR, cv::BORDER_CONSTANT);
        cv::drawChessboardCorners(calibrationPreviewImage, boardSize, idealCorners, true);
        cv::rectangle(calibrationPreviewImage, roi, cv::Scalar(255, 255, 0), 2);
        cv::putText(calibrationPreviewImage, "HOMOGRAPHY OK", cv::Point(20, 40),
            cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);
        lastCalibrationMessage = "Single-image homography calibration completed successfully.";
        return rms;
    }
    catch (const cv::Exception& e) {
        lastCalibrationMessage = std::string("OpenCV homography calibration failed: ") + e.what();
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

// Keep the historical API name for callers; planar correction now uses the
// saved homography and preserves the original canvas size.
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

    cv::Mat output;
    if (inputImage.size() != imageSize) {
        std::cerr << "[Warning] Homography image size mismatch.\n";
        return cv::Mat();
    }
    cv::warpPerspective(inputImage, output, homographyMatrix, imageSize,
        cv::INTER_LINEAR, cv::BORDER_CONSTANT);

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
double UAXVision::getCalibrationMmPerPixel() const { return calibrationMmPerPixel; }
cv::Point2d UAXVision::transformPoint(const cv::Point2d& point) const
{
    if (!isCalibrated()) return point;
    std::vector<cv::Point2f> source{ cv::Point2f(static_cast<float>(point.x), static_cast<float>(point.y)) };
    std::vector<cv::Point2f> destination;
    cv::perspectiveTransform(source, destination, homographyMatrix);
    return destination.empty() ? point : cv::Point2d(destination.front());
}

// Save calibration data (包含 imageSize)
bool UAXVision::saveCalibrationData(const std::string& filename) const
{
    cv::FileStorage fs(filename, cv::FileStorage::WRITE);
    if (!fs.isOpened()) {
        std::cerr << "Cannot open file for writing: " << filename << "\n";
        return false;
    }

    if (!isCalibrated()) return false;
    fs << "model" << "homography_v1";
    fs << "homographyMatrix" << homographyMatrix;
    fs << "imageWidth" << imageSize.width;
    fs << "imageHeight" << imageSize.height;
    fs << "boardWidth" << calibrationBoardSize.width;
    fs << "boardHeight" << calibrationBoardSize.height;
    fs << "squareSizeMm" << calibrationSquareSize;
    fs << "pixelsPerSquare" << calibrationPixelsPerSquare;
    fs << "mmPerPixel" << calibrationMmPerPixel;
    fs << "roiX" << calibrationROI.x;
    fs << "roiY" << calibrationROI.y;
    fs << "roiWidth" << calibrationROI.width;
    fs << "roiHeight" << calibrationROI.height;

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

    std::string model;
    fs["model"] >> model;
    if (model != "homography_v1") {
        std::cerr << "Unsupported legacy calibration model in " << filename << "\n";
        fs.release();
        homographyMatrix.release();
        return false;
    }
    fs["homographyMatrix"] >> homographyMatrix;

    int w = 0, h = 0;
    fs["imageWidth"] >> w;
    fs["imageHeight"] >> h;
    if (w > 0 && h > 0) {
        imageSize = cv::Size(w, h);
    }
    int boardWidth = 0, boardHeight = 0;
    fs["boardWidth"] >> boardWidth;
    fs["boardHeight"] >> boardHeight;
    if (boardWidth > 1 && boardHeight > 1) calibrationBoardSize = cv::Size(boardWidth, boardHeight);
    fs["squareSizeMm"] >> calibrationSquareSize;
    fs["pixelsPerSquare"] >> calibrationPixelsPerSquare;
    fs["mmPerPixel"] >> calibrationMmPerPixel;
    fs["roiX"] >> calibrationROI.x;
    fs["roiY"] >> calibrationROI.y;
    fs["roiWidth"] >> calibrationROI.width;
    fs["roiHeight"] >> calibrationROI.height;

    fs.release();

    if (!isCalibrated()) {
        std::cerr << "Invalid calibration data loaded from " << filename << "\n";
        return false;
    }

    std::cout << "Calibration loaded from: " << filename << "\n";
    return true;
}
