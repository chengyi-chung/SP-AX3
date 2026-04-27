#pragma once
#include "pch.h"
#include <iostream>
#include <string>
#include <stdio.h>       
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <opencv2/opencv.hpp>
#include <atlimage.h>
#include <WinSock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <opencv2/flann.hpp>
#include <cmath>
#include <cstring>
#include <algorithm>

#include "GluePathOptimizer.h"

//Add UAX.h
#include "UAX.h"
#include "UAXTypes.h"

// T MAcro
#define _T(x) L ## x

#pragma comment(lib, "iphlpapi.lib")  // Link with iphlpapi.lib
#pragma comment(lib, "ws2_32.lib")  // Link with ws2_32.lib : Winsock2 Library for Windows Sockets programming 
// Define PIP_ADAPTER_ADDRESSES


using namespace std;
using namespace cv;
//using namespace CryptoPP;


// Use unsigned char for byte-like data to avoid type conflicts.
// This keeps the legacy AES-related data declarations simple.
unsigned char key[16] = { 0x2b, 0x7e, 0x15, 0x16,
			0x28, 0xae, 0xd2, 0xa6,
			0xab, 0xf7, 0x15, 0x88,
			0x09, 0xcf, 0x4f, 0x3c };

unsigned char plain[4 * 4] = { 0x32, 0x88, 0x31, 0xe0,
				0x43, 0x5a, 0x31, 0x37,
				0xf6, 0x30, 0x98, 0x07,
				0xa8, 0x8d, 0xa2, 0x34 };


float Add(float a, float b)
{
	return a + b;
}

/////////////////////////////

#include <windows.h> // For screen resolution on Windows

void ShowZoomedImage(const std::string& windowName, const cv::Mat& image)
{
	// Get screen resolution
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	// Compute zoom scale
	double scaleX = static_cast<double>(screenWidth) / image.cols;
	double scaleY = static_cast<double>(screenHeight) / image.rows;
	double scale = min(scaleX, scaleY); // Keep the whole image visible

	// Resize image for display
	cv::Mat resized;
	cv::resize(image, resized, cv::Size(), scale, scale, cv::INTER_LINEAR);

	// Show image
	cv::imshow(windowName, resized);

}



////////////////////////////

//Find the area of image
//Use findContours to find the area of image
// cv::Mat& src: the input image
//ContourArea is a struct to store the area and perimeter of the contour
void FindArea(cv::Mat& src, ContourArea& contourarea)
{
	// Convert the image to grayscale
	cv::Mat gray;
	cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);

	// Threshold the image
	cv::Mat binary;
	cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

	// Find contours
	std::vector<std::vector<cv::Point>> contours;
	std::vector<cv::Vec4i> hierarchy;
	cv::findContours(binary, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

	// Calculate the area and perimeter of the contour
	double area = 0.0;
	double perimeter = 0.0;
	for (size_t i = 0; i < contours.size(); i++)
	{
		area += cv::contourArea(contours[i]);
		perimeter += cv::arcLength(contours[i], true);
	}

	// Store the area and perimeter in the struct
	contourarea.Area = area;
	contourarea.Perimeter = perimeter;
}

// Get Tool Path
// Use Erosiong find the tool path
// ImgSrc: the input image
// Offset: the offse value of the tool path
// ToolPath: the output tool path
// units of Offset is pixel
void  GetToolPath(cv::Mat& ImgSrc, cv::Point2d Offset, ToolPath& toolpath)
{
	if (ImgSrc.empty())
	{
		throw std::invalid_argument("Input image is empty.");
	}

	cv::Mat result = ImgSrc.clone();
	int numPixelsToErode = static_cast<int>(Offset.x + Offset.y);
	cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));

	for (int i = 0; i < numPixelsToErode; ++i)
	{
		cv::erode(result, result, kernel);
	}

	cv::Mat gray;
	if (result.channels() != 1)
	{
		cv::cvtColor(result, gray, cv::COLOR_BGR2GRAY);
	}
	else
	{
		gray = result;
	}

	//Image Processing
	cv::Mat thresh;
	cv::threshold(gray, thresh, 128, 255, cv::THRESH_BINARY);

	std::vector<std::vector<cv::Point>> contours;
	std::vector<cv::Vec4i> hierarchy;
	cv::findContours(thresh, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

	cv::Mat outputImage = ImgSrc.clone();
	cv::drawContours(outputImage, contours, -1, cv::Scalar(0, 255, 0), 2);

	toolpath.Offset = Offset;
	int numCounts = static_cast<int>(contours.size());
	for (const auto& contour : contours)
	{
		for (const auto& point : contour)
		{
			toolpath.Path.push_back(cv::Point2d(point));
		}
	}

	cv::drawContours(ImgSrc, contours, -1, cv::Scalar(0, 255, 0), 2);

	ShowZoomedImage("Input Image", ImgSrc);
	cv::waitKey(0);
	cv::destroyAllWindows();
}

void GetToolPath_Optimized(cv::Mat& ImgSrc, cv::Point2d Offset, ToolPath& toolpath)
{
	if (ImgSrc.empty()) {
		throw std::invalid_argument("Input image is empty.");
	}

	// 1. Preprocess by eroding the source image inward.
	// The erosion count is derived from Offset.x + Offset.y in pixel units.
	cv::Mat processed;
	int numPixelsToErode = static_cast<int>(Offset.x + Offset.y);

	if (numPixelsToErode > 0) {
		cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
		cv::erode(ImgSrc, processed, kernel, cv::Point(-1, -1), numPixelsToErode);
	}
	else {
		processed = ImgSrc.clone();
	}

	// 2. Convert to grayscale if needed.
	cv::Mat gray;
	if (processed.channels() == 3) {
		cv::cvtColor(processed, gray, cv::COLOR_BGR2GRAY);
	}
	else {
		gray = processed;
	}

	// 3. Threshold and extract outer contours.
	cv::threshold(gray, gray, 128, 255, cv::THRESH_BINARY);

	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(gray, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

	// 4. Reserve memory before flattening all contour points into toolpath.Path.
	size_t totalPoints = 0;
	for (const auto& c : contours) totalPoints += c.size();

	toolpath.Path.clear();
	toolpath.Path.reserve(totalPoints);
	toolpath.Offset = Offset;

	for (const auto& contour : contours) {
		for (const auto& point : contour) {
			toolpath.Path.emplace_back(static_cast<double>(point.x), static_cast<double>(point.y));
		}
	}

	// 5. Draw contour result back to the source image for display/debug.
	cv::drawContours(ImgSrc, contours, -1, cv::Scalar(0, 255, 0), 2);
}

void GetToolPath_CurvatureOptimized(cv::Mat& ImgSrc, cv::Point2d Offset, ToolPath& toolpath, double epsilonFactor)
{
	if (ImgSrc.empty()) throw std::invalid_argument("Input image is empty.");

	// 1. Erode inward according to the requested pixel offset.
	cv::Mat processed;
	int numPixelsToErode = static_cast<int>(Offset.x + Offset.y);
	if (numPixelsToErode > 0) {
		cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
		cv::erode(ImgSrc, processed, kernel, cv::Point(-1, -1), numPixelsToErode);
	}
	else {
		processed = ImgSrc.clone();
	}

	// 2. Convert to grayscale and binarize.
	cv::Mat gray;
	if (processed.channels() == 3) cv::cvtColor(processed, gray, cv::COLOR_BGR2GRAY);
	else gray = processed;

	cv::threshold(gray, gray, 128, 255, cv::THRESH_BINARY);

	// 3. Extract contours using TC89_L1 to preserve shape with fewer points.
	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(gray, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_TC89_L1);

	toolpath.Path.clear();
	toolpath.Offset = Offset;

	// 4. Reduce contour points with Douglas-Peucker.
	for (const auto& contour : contours)
	{
		std::vector<cv::Point> simplifiedContour;

		// epsilon is proportional to contour arc length.
		double arcLen = cv::arcLength(contour, true);
		double epsilon = epsilonFactor * arcLen;

		cv::approxPolyDP(contour, simplifiedContour, epsilon, true);

		// Append simplified points to toolpath.
		for (const auto& point : simplifiedContour)
		{
			toolpath.Path.emplace_back(static_cast<double>(point.x), static_cast<double>(point.y));
		}
	}

	// Draw original contours and reduced points for debug display.
	cv::drawContours(ImgSrc, contours, -1, cv::Scalar(0, 0, 255), 1);
	for (const auto& p : toolpath.Path) {
		cv::circle(ImgSrc, cv::Point(p.x, p.y), 2, cv::Scalar(0, 255, 0), -1);
	}

	ShowZoomedImage("Reduced Points Result", ImgSrc);
}


// ====================== Shared Preprocess Helper ======================
void PreprocessImage(const cv::Mat& ImgSrc,
	cv::Mat& gray,
	cv::Point2d Offset,
	const cv::Mat& Mask = cv::Mat())
{
	// 1. Erosion
	cv::Mat processed;
	int numPixelsToErode = static_cast<int>(Offset.x + Offset.y);
	if (numPixelsToErode > 0) {
		cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
		cv::erode(ImgSrc, processed, kernel, cv::Point(-1, -1), numPixelsToErode);
	}
	else {
		processed = ImgSrc.clone();
	}

	// 2. Convert to grayscale.
	if (processed.channels() == 3)
		cv::cvtColor(processed, gray, cv::COLOR_BGR2GRAY);
	else
		gray = processed;

	// 3. Apply binary mask if provided.
	if (!Mask.empty()) {
		cv::Mat maskGray;
		if (Mask.channels() == 3)
			cv::cvtColor(Mask, maskGray, cv::COLOR_BGR2GRAY);
		else
			maskGray = Mask;

		// Ensure mask is binary.
		cv::threshold(maskGray, maskGray, 1, 255, cv::THRESH_BINARY);

		// Resize mask if its size does not match the source image.
		if (maskGray.size() != gray.size()) {
			std::cout << "[WARN] Mask size mismatch, resizing..." << std::endl;
			cv::resize(maskGray, maskGray, gray.size(), 0, 0, cv::INTER_NEAREST);
		}

		cv::bitwise_and(gray, maskGray, gray);

		int nz = cv::countNonZero(gray);
		std::cout << "[DEBUG] After Mask, non-zero pixels: " << nz << std::endl;
	}

	// 4. Binarize with OTSU for stable thresholding.
	cv::threshold(gray, gray, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

	std::cout << "[DEBUG] After threshold, non-zero pixels: " << cv::countNonZero(gray) << std::endl;
}



// ====================== Mode 1: Mask + Curvature Optimized Path ======================
void GetToolPath_CurvatureOptimized_Mask(
	cv::Mat& ImgSrc,
	const cv::Mat& Mask,              // Input mask
	double offsetPixel,
	ToolPath& toolpath,
	double epsilonFactor,
	int binaryUpper,
	int binaryLower,
	bool enableCurvatureOptimization)
{
	if (ImgSrc.empty()) throw std::invalid_argument("Input image is empty.");

	// 1. Erode the source image by the requested inward offset in pixels
	cv::Mat processed;
	int numPixelsToErode = static_cast<int>(std::lround(offsetPixel));
	if (numPixelsToErode > 0) {
		cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
		cv::erode(ImgSrc, processed, kernel, cv::Point(-1, -1), numPixelsToErode);
	}
	else {
		processed = ImgSrc.clone();
	}

	// 2. Convert to grayscale.
	cv::Mat gray;
	if (processed.channels() == 3) cv::cvtColor(processed, gray, cv::COLOR_BGR2GRAY);
	else gray = processed;

	// 3. Apply binary mask before extracting contours.
	if (!Mask.empty()) {
		cv::Mat maskGray;
		if (Mask.channels() == 3)
			cv::cvtColor(Mask, maskGray, cv::COLOR_BGR2GRAY);
		else
			maskGray = Mask;

		cv::threshold(maskGray, maskGray, 1, 255, cv::THRESH_BINARY);

		if (maskGray.size() != gray.size()) {
			cv::resize(maskGray, maskGray, gray.size(), 0, 0, cv::INTER_NEAREST);
		}

		cv::bitwise_and(gray, maskGray, gray);

		std::cout << "[DEBUG] After Mask, non-zero pixels: " << cv::countNonZero(gray) << std::endl;
	}

	// 4. Binarize with configured lower/upper bounds.
	int lowerBound = (std::max)(0, (std::min)(255, binaryLower));
	int upperBound = (std::max)(0, (std::min)(255, binaryUpper));
	if (lowerBound > upperBound) {
		std::swap(lowerBound, upperBound);
	}
	cv::inRange(gray, cv::Scalar(lowerBound), cv::Scalar(upperBound), gray);

	// 5. Extract outer contours.
	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(gray, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_TC89_L1);

	toolpath.Path.clear();
	toolpath.Offset = cv::Point2d(offsetPixel, 0.0);

	// 6. Either simplify contours or keep original contour points.
	for (const auto& contour : contours)
	{
		if (contour.size() < 3) continue;

		std::vector<cv::Point> finalContour;

		if (enableCurvatureOptimization)
		{
			// Douglas-Peucker simplification.
			double arcLen = cv::arcLength(contour, true);
			double epsilon = epsilonFactor * arcLen;

			cv::approxPolyDP(contour, finalContour, epsilon, true);

			std::cout << "[INFO] Simplified contour from " << contour.size()
				<< " to " << finalContour.size() << " points (epsilon=" << epsilon << ")" << std::endl;
		}
		else
		{
			// Keep original contour points without simplification.
			finalContour = contour;

			std::cout << "[INFO] Using original contour (" << contour.size() << " points) - simplification disabled" << std::endl;
		}

		// Convert contour points to cv::Point2d and append to toolpath.
		for (const auto& point : finalContour)
		{
			toolpath.Path.emplace_back(static_cast<double>(point.x), static_cast<double>(point.y));
		}
	}

	//以下只在 debug 模式下執行
#ifdef _DEBUG
	
	// 7. Draw contours and sampled path points for debug display
	cv::drawContours(ImgSrc, contours, -1, cv::Scalar(0, 0, 255), 1); // contour outline

	cv::Scalar drawColor = enableCurvatureOptimization ? cv::Scalar(0, 255, 0) : cv::Scalar(255, 255, 0); // green=simplified, cyan=original
	for (const auto& p : toolpath.Path) {
		cv::circle(ImgSrc, cv::Point(static_cast<int>(p.x), static_cast<int>(p.y)), 2, drawColor, -1);
	}

	std::cout << "[INFO] GetToolPath_CurvatureOptimized_Mask: Generated " << toolpath.Path.size() << " points "
		<< (enableCurvatureOptimization ? "(simplified)" : "(original)") << std::endl;

	cv::Mat image = ImgSrc.clone();
	//cv::flip(image, image, -1);
	ShowZoomedImage("Masked & " + std::string(enableCurvatureOptimization ? "Reduced" : "Original") + " Points Result", image);
#endif


}


// ====================== Mode 2: Symmetric Path Generation ======================
void GetToolPath_SymmetricOnly(cv::Mat& ImgSrc, cv::Point2d Offset, ToolPath& toolpath, double epsilonFactor)
{
	if (ImgSrc.empty()) return;

	// 1. Erode inward and threshold the image.
	cv::Mat processed, gray;
	int numPixelsToErode = static_cast<int>(Offset.x + Offset.y);
	cv::erode(ImgSrc, processed, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3)), cv::Point(-1, -1), numPixelsToErode);

	if (processed.channels() == 3) cv::cvtColor(processed, gray, cv::COLOR_BGR2GRAY);
	else gray = processed;
	cv::threshold(gray, gray, 128, 255, cv::THRESH_BINARY);

	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(gray, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_TC89_L1);

	toolpath.Path.clear();
	double centerX = gray.cols / 2.0;

	for (const auto& contour : contours) {
		std::vector<cv::Point> simplified;
		double epsilon = epsilonFactor * cv::arcLength(contour, true);
		cv::approxPolyDP(contour, simplified, epsilon, true);

		// Collect left-side arc points and center-line control points.
		std::vector<cv::Point2d> leftArc;
		cv::Point2d topCP(centerX, 1e9), bottomCP(centerX, -1e9);
		bool hasCenter = false;

		// Scan simplified contour points around the symmetry center line.
		for (const auto& p : simplified) {
			if (p.x <= centerX + 1.0) { // include center line points
				cv::Point2d p2d(p.x, p.y);

				// Track top/bottom center points near the middle line.
				if (std::abs(p.x - centerX) < 2.0) {
					hasCenter = true;
					if (p.y < topCP.y) topCP = cv::Point2d(centerX, p.y);
					if (p.y > bottomCP.y) bottomCP = cv::Point2d(centerX, p.y);
				}

				if (p.x < centerX) {
					leftArc.push_back(p2d);
				}
			}
		}

		// Sort by Y so the path runs from top to bottom.
		std::sort(leftArc.begin(), leftArc.end(), [](const cv::Point2d& a, const cv::Point2d& b) {
			return a.y < b.y;
			});

		// Compose path: top center -> left arc -> bottom center -> mirrored right arc.
		if (hasCenter) toolpath.Path.push_back(topCP);

		for (const auto& p : leftArc) toolpath.Path.push_back(p);

		if (hasCenter && topCP != bottomCP) toolpath.Path.push_back(bottomCP);

		// Mirror the left arc to generate the right side.
		for (int i = (int)leftArc.size() - 1; i >= 0; --i) {
			toolpath.Path.push_back(cv::Point2d(2 * centerX - leftArc[i].x, leftArc[i].y));
		}
	}

	// 4. Draw the generated symmetric path for debug display.
	cv::Mat drawImg = ImgSrc.clone();
	for (size_t i = 0; i < toolpath.Path.size(); ++i) {
		cv::circle(drawImg, toolpath.Path[i], 3, cv::Scalar(0, 255, 0), -1);
		if (i > 0) {
			cv::line(drawImg, toolpath.Path[i - 1], toolpath.Path[i], cv::Scalar(255, 0, 0), 1);
		}
	}

	ShowZoomedImage("Symmetric Result", drawImg);
}



// Compute curvature from three adjacent points.
static double ComputeCurvature(
	const cv::Point2d& p1,
	const cv::Point2d& p2,
	const cv::Point2d& p3)
{
	double area = std::abs(
		(p1.x * (p2.y - p3.y) +
			p2.x * (p3.y - p1.y) +
			p3.x * (p1.y - p2.y)) / 2.0);
	double len1 = cv::norm(p1 - p2);
	double len2 = cv::norm(p2 - p3);
	double len3 = cv::norm(p1 - p3);
	if (len1 * len2 * len3 == 0) return 0.0;
	return (4.0 * area) / (len1 * len2 * len3);
}

// Reduce points based on local turning angle / curvature.
static std::vector<cv::Point2d> ReducePointsByCurvature(
	const std::vector<cv::Point2d>& points,
	double curvatureThreshold,
	int minDistancePixels = 1)
{
	// Basic guard.
	if (points.size() < 3) return points;

	std::vector<cv::Point2d> reduced;
	reduced.reserve(points.size());
	reduced.push_back(points.front());

	auto getDistance = [](const cv::Point2d& p1, const cv::Point2d& p2) {
		return std::hypot(p1.x - p2.x, p1.y - p2.y);
		};

	auto computeCurvature = [](const cv::Point2d& prev,
		const cv::Point2d& curr,
		const cv::Point2d& next) {
			// Use turning angle as a simple curvature measure.
			cv::Point2d v1 = curr - prev;
			cv::Point2d v2 = next - curr;
			double mag1 = std::hypot(v1.x, v1.y);
			double mag2 = std::hypot(v2.x, v2.y);

			if (mag1 < 1e-8 || mag2 < 1e-8) return 0.0; // avoid zero-length segments

			double dot = v1.x * v2.x + v1.y * v2.y;
			double cosTheta = dot / (mag1 * mag2);
			cosTheta = std::clamp(cosTheta, -1.0, 1.0);

			double angle = std::acos(cosTheta); // radians
			return angle;
		};

	// Keep points if curvature is large enough or spacing exceeds the minimum distance.
	for (size_t i = 1; i + 1 < points.size(); ++i)
	{
		const cv::Point2d& prev = reduced.back();
		const cv::Point2d& curr = points[i];
		const cv::Point2d& next = points[i + 1];

		double dist = getDistance(prev, curr);
		double curvature = computeCurvature(prev, curr, next);

		// Preserve corners or points that are far enough away from the last accepted point.
		if (curvature >= curvatureThreshold || dist >= minDistancePixels)
			reduced.push_back(curr);
	}

	// Always keep the last point.
	if (reduced.back() != points.back())
		reduced.push_back(points.back());

	return reduced;
}






// Simple moving-average smoothing.
static std::vector<cv::Point2d> SmoothPoints(
	const std::vector<cv::Point2d>& points,
	int windowSize)
{
	std::vector<cv::Point2d> result;
	if (points.empty()) return result;
	int halfWin = windowSize / 2;

	for (size_t i = 0; i < points.size(); ++i)
	{
		double sumX = 0.0, sumY = 0.0;
		int count = 0;
		for (int j = -halfWin; j <= halfWin; ++j)
		{
			int idx = static_cast<int>(i) + j;
			if (idx >= 0 && idx < (int)points.size())
			{
				sumX += points[idx].x;
				sumY += points[idx].y;
				count++;
			}
		}
		result.push_back(cv::Point2d(sumX / count, sumY / count));
	}
	return result;
}


// 2025 /06/12  BEGIN


// Cluster points with KD-tree radius search.
int ClusterKDTree(const Point2D* input, int inputSize, double radius,
	Cluster** outputClusters, int* clusterCount) {
	if (!input || inputSize <= 0 || !outputClusters || !clusterCount) return -1;

	std::vector<cv::Point2d> points;
	for (int i = 0; i < inputSize; ++i)
		points.emplace_back(input[i].x, input[i].y);

	cv::Mat data(inputSize, 2, CV_32F);
	for (int i = 0; i < inputSize; ++i) {
		data.at<float>(i, 0) = static_cast<float>(points[i].x);
		data.at<float>(i, 1) = static_cast<float>(points[i].y);
	}

	cv::flann::Index kdtree(data, cv::flann::KDTreeIndexParams(1));
	std::vector<bool> visited(inputSize, false);
	std::vector<std::vector<cv::Point2d>> clusters;

	for (int i = 0; i < inputSize; ++i) {
		if (visited[i]) continue;
		std::vector<cv::Point2d> cluster;
		std::vector<int> queue = { i };
		visited[i] = true;

		while (!queue.empty()) {
			int idx = queue.back(); queue.pop_back();
			cluster.push_back(points[idx]);

			std::vector<int> indices;
			std::vector<float> dists;
			kdtree.radiusSearch(data.row(idx), indices, dists, radius * radius, inputSize);

			for (int j : indices) {
				if (!visited[j]) {
					visited[j] = true;
					queue.push_back(j);
				}
			}
		}

		clusters.push_back(cluster);
	}

	*clusterCount = static_cast<int>(clusters.size());
	*outputClusters = new Cluster[*clusterCount];

	for (int i = 0; i < *clusterCount; ++i) {
		int n = static_cast<int>(clusters[i].size());
		(*outputClusters)[i].points = new Point2D[n];
		(*outputClusters)[i].count = n;
		for (int j = 0; j < n; ++j) {
			(*outputClusters)[i].points[j].x = clusters[i][j].x;
			(*outputClusters)[i].points[j].y = clusters[i][j].y;
		}
	}

	return 0;
}

// Simple smoothing helper (current implementation uses uniform weights).
int SmoothPath(const Point2D* input, int inputSize, int windowSize, Point2D* output) {
	if (!input || inputSize <= 0 || windowSize < 3 || !output) return -1;
	if (windowSize % 2 == 0) windowSize++; // enforce odd window size

	int half = windowSize / 2;
	std::vector<double> coeff(windowSize);
	double sum = 0;
	for (int i = -half; i <= half; ++i) {
		coeff[i + half] = 1.0; // uniform weight
		sum += coeff[i + half];
	}

	for (int i = 0; i < inputSize; ++i) {
		double sx = 0, sy = 0;
		for (int j = -half; j <= half; ++j) {
			int idx = std::min<int>(std::max<int>(i + j, 0), inputSize - 1);
			sx += input[idx].x * coeff[j + half];
			sy += input[idx].y * coeff[j + half];
		}
		output[i].x = sx / sum;
		output[i].y = sy / sum;
	}

	return 0;
}

// Lightweight B-spline-like interpolation placeholder.
int FitBSpline(const Point2D* input, int inputSize, int degree,
	Point2D* output, int* outputSize) {
	if (!input || inputSize < 2 || !output || !outputSize) return -1;

	int segments = inputSize - 1;
	int samplesPerSegment = 10;
	*outputSize = segments * samplesPerSegment;

	for (int i = 0; i < segments; ++i) {
		for (int j = 0; j < samplesPerSegment; ++j) {
			double t = static_cast<double>(j) / samplesPerSegment;
			output[i * samplesPerSegment + j].x =
				(1 - t) * input[i].x + t * input[i + 1].x;
			output[i * samplesPerSegment + j].y =
				(1 - t) * input[i].y + t * input[i + 1].y;
		}
	}

	return 0;
}

// Release cluster memory allocated by ClusterKDTree.
void FreeClusters(Cluster* clusters, int clusterCount) {
	if (!clusters || clusterCount <= 0) return;
	for (int i = 0; i < clusterCount; ++i)
		delete[] clusters[i].points;
	delete[] clusters;
}

// 2025  END

// Get Tool Path
// Use Erosiong find the tool path
// ImgSrc: the input image
// Offset: the offse value of the tool path(Pixel)
// ToolPath: the output tool path
// With mask image to limit the area of tool path
void GetToolPathWithMask(const cv::Mat& ImgSrc, const cv::Mat& Mask, double offsetDistance, ToolPath& toolpath)
{
    // Validate input image and mask.
    if (ImgSrc.empty() || Mask.empty())
    {
        throw std::invalid_argument("Input image or mask is empty.");
    }
    if (ImgSrc.size() != Mask.size())
    {
        throw std::invalid_argument("Image and mask sizes do not match.");
    }

    // Apply mask to limit the effective image area.
    cv::Mat maskedImage;
    if (ImgSrc.channels() == 3)
    {
        maskedImage = cv::Mat::zeros(ImgSrc.size(), ImgSrc.type());
        ImgSrc.copyTo(maskedImage, Mask);
    }
    else
    {
        maskedImage = ImgSrc.clone();
        maskedImage.setTo(0, Mask == 0);
    }

    // Erode inward by the requested offset distance in pixels.
    cv::Mat result = maskedImage.clone();
    int numPixelsToErode = static_cast<int>(offsetDistance);
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));

    for (int i = 0; i < numPixelsToErode; ++i)
    {
        cv::erode(result, result, kernel);
    }

    // Convert to grayscale and binarize.
    cv::Mat gray;
    if (result.channels() != 1)
    {
        cv::cvtColor(result, gray, cv::COLOR_BGR2GRAY);
    }
    else
    {
        gray = result;
    }

    cv::Mat thresh;
    cv::threshold(gray, thresh, 200, 255, cv::THRESH_BINARY);

    // Find contours from the thresholded result.
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(thresh, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // Smooth each contour before curvature-based point reduction.
    std::vector<std::vector<cv::Point2d>> smoothedContours;
    int numContours = static_cast<int>(contours.size());

    for (const auto& contour : contours)
    {
        std::vector<cv::Point2d> points;
        points.reserve(contour.size());
        for (const auto& point : contour)
        {
            points.emplace_back(point.x, point.y);
        }

        int smoothingSize = 7;
        double sigma = 2.5;

        std::vector<cv::Point2d> smoothedPoints;
        if (points.size() >= static_cast<size_t>(smoothingSize))
        {
            std::vector<double> xCoords(points.size()), yCoords(points.size());
            for (size_t i = 0; i < points.size(); ++i)
            {
                xCoords[i] = points[i].x;
                yCoords[i] = points[i].y;
            }

            cv::Mat kernel1D = cv::getGaussianKernel(smoothingSize, sigma, CV_64F);
            cv::Mat xMat(1, static_cast<int>(xCoords.size()), CV_64F, xCoords.data());
            cv::Mat yMat(1, static_cast<int>(yCoords.size()), CV_64F, yCoords.data());
            cv::Mat smoothedX, smoothedY;

            cv::filter2D(xMat, smoothedX, -1, kernel1D, cv::Point(-1, -1), 0, cv::BORDER_REFLECT);
            cv::filter2D(yMat, smoothedY, -1, kernel1D, cv::Point(-1, -1), 0, cv::BORDER_REFLECT);

            smoothedPoints.reserve(points.size());
            for (size_t i = 0; i < points.size(); ++i)
            {
                smoothedPoints.emplace_back(smoothedX.at<double>(0, static_cast<int>(i)),
                                            smoothedY.at<double>(0, static_cast<int>(i)));
            }
        }
        else
        {
            smoothedPoints = points;
        }

        smoothedContours.push_back(std::move(smoothedPoints));
    }

    // Reduce points after smoothing.
    double curvatureThreshold = 0.1;
    std::vector<std::vector<cv::Point2d>> finalContours;
    finalContours.reserve(smoothedContours.size());
    for (const auto& contour : smoothedContours)
    {
        finalContours.push_back(ReducePointsByCurvature(contour, curvatureThreshold));
    }

    // Flatten contours into toolpath and record cluster indices.
    toolpath.Offset = cv::Point2d(offsetDistance, offsetDistance);
    toolpath.Path.clear();
    toolpath.numClusters.clear();

    for (size_t cIdx = 0; cIdx < finalContours.size(); ++cIdx)
    {
        const auto& contour = finalContours[cIdx];
        for (const auto& point : contour)
        {
            toolpath.Path.push_back(point);
            toolpath.numClusters.push_back(static_cast<int>(cIdx)); // contour index for each point
        }
    }

	int pathSize = toolpath.Path.size();
	int clusterCount = toolpath.numClusters.size();


#ifdef _DEBUG
{
    const size_t total = toolpath.Path.size();
    const size_t totalClusters = toolpath.numClusters.size();

    // Verify point count matches cluster count.
    {
        std::ostringstream head;
        head << "[ToolPath] points=" << total
             << ", clusters=" << totalClusters
             << (total == totalClusters ? " (OK)" : " (MISMATCH)") << "\n";
        OutputDebugStringA(head.str().c_str());
    }

    // Dump points in chunks so large paths do not flood one debug string at once.
    const size_t chunk = 512;
    for (size_t start = 0; start < total; start += chunk)
    {
        std::ostringstream oss;
        size_t end = std::min<size_t>(total, start + chunk);
        for (size_t i = start; i < end; ++i)
        {
            const auto& p = toolpath.Path[i];
            int cid = (i < totalClusters) ? toolpath.numClusters[i] : -1;
            oss << i << ": (" << p.x << ", " << p.y << "), cluster=" << cid << "\n";
        }
        OutputDebugStringA(oss.str().c_str());
    }
}
#endif

    // Draw reduced points and connecting segments for debug display.
    cv::Mat outputImage = ImgSrc.clone();
    std::vector<std::vector<cv::Point>> contoursToDraw;

    for (const auto& finalContour : finalContours)
    {
        std::vector<cv::Point> contourInt;
        contourInt.reserve(finalContour.size());
        for (size_t i = 0; i < finalContour.size(); ++i)
        {
            cv::Point p(static_cast<int>(finalContour[i].x), static_cast<int>(finalContour[i].y));
            contourInt.push_back(p);

            cv::circle(outputImage, p, 8, cv::Scalar(0, 255, 0), cv::FILLED);

            if (i < finalContour.size() - 1)
            {
                cv::Point nextP(static_cast<int>(finalContour[i + 1].x), static_cast<int>(finalContour[i + 1].y));
                cv::line(outputImage, p, nextP, cv::Scalar(0, 255, 255), 2, cv::LINE_AA);
            }
        }
        contoursToDraw.push_back(std::move(contourInt));
    }

    cv::drawContours(outputImage, contoursToDraw, -1, cv::Scalar(255, 0, 0), 1);

    ShowZoomedImage("Curvature Reduced Points with Lines", outputImage);
    cv::waitKey(0);
    cv::destroyAllWindows();
}

// Enhanced version with curvature-based point reduction and smoothing
// std::vector<cv::Point2d> points;
// std::vector<cv::Point2d> pointsOut;
void ReduceAndSmoothPoints(const std::vector<cv::Point2d>& points, std::vector<cv::Point2d>& pointsOut,
	                                                   double curvatureThreshold, int smoothingWindowSize)
{
	// Step 1: Reduce points based on curvature
	auto reducedPoints = ReducePointsByCurvature(points, curvatureThreshold);
	// Step 2: Smooth the reduced points
	pointsOut = SmoothPoints(reducedPoints, smoothingWindowSize);
}


/*
// Legacy experimental version of GetToolPathWithMask().
// Kept for reference only.
void GetToolPathWithMask(const cv::Mat& ImgSrc, const cv::Mat& Mask, double offsetDistance, ToolPath& toolpath)
{
	if (ImgSrc.empty() || Mask.empty())
		throw std::invalid_argument("Input image or mask is empty.");
	if (ImgSrc.size() != Mask.size())
		throw std::invalid_argument("Image and mask sizes do not match.");

	// Apply mask
	cv::Mat maskedImage;
	if (ImgSrc.channels() == 3)
	{
		maskedImage = cv::Mat::zeros(ImgSrc.size(), ImgSrc.type());
		ImgSrc.copyTo(maskedImage, Mask);
	}
	else
	{
		maskedImage = ImgSrc.clone();
		maskedImage.setTo(0, Mask == 0);
	}

	// Erode
	cv::Mat result = maskedImage.clone();
	int numPixelsToErode = static_cast<int>(offsetDistance);
	cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
	for (int i = 0; i < numPixelsToErode; ++i)
	{
		cv::erode(result, result, kernel);
	}

	// Convert to grayscale and threshold
	cv::Mat gray;
	if (result.channels() != 1)
		cv::cvtColor(result, gray, cv::COLOR_BGR2GRAY);
	else
		gray = result;

	cv::Mat thresh;
	cv::threshold(gray, thresh, 200, 255, cv::THRESH_BINARY);

	// Find contours
	std::vector<std::vector<cv::Point>> contours;
	std::vector<cv::Vec4i> hierarchy;
	cv::findContours(thresh, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

	// Reduce and smooth each contour
	std::vector<std::vector<cv::Point2d>> optimizedContours;
	for (const auto& contour : contours)
	{
		// Convert to double precision points
		std::vector<cv::Point2d> points;
		for (const auto& p : contour)
			points.push_back(cv::Point2d(p.x, p.y));

		// Reduce points
		auto reduced = ReducePointsByCurvature(points, 0.01);

		// Smooth points
		auto smoothed = SmoothPoints(reduced, 5);

		optimizedContours.push_back(smoothed);
	}

	// Build output toolpath
	toolpath.Offset = cv::Point2d(offsetDistance, offsetDistance);
	for (const auto& contour : optimizedContours)
		for (const auto& point : contour)
			toolpath.Path.push_back(point);

	// Draw debug result
	cv::Mat outputImage = ImgSrc.clone();
	std::vector<std::vector<cv::Point>> contoursToDraw;
	for (const auto& optContour : optimizedContours)
	{
		std::vector<cv::Point> contourInt;
		for (const auto& point : optContour)
			contourInt.push_back(cv::Point(static_cast<int>(point.x), static_cast<int>(point.y)));
		contoursToDraw.push_back(contourInt);
	}
	cv::drawContours(outputImage, contoursToDraw, -1, cv::Scalar(0, 255, 0), 2);

	ShowZoomedImage("Optimized Tool Path", outputImage);
	cv::waitKey(0);
	cv::destroyAllWindows();
}


*/

//Convert contour to tool path
// cv::Mat& src: the input image
// ToolPath: the output tool path
void ContourToToolPath(cv::Mat& src, ToolPath& toolpath)
{
	// Convert the image to grayscale
	cv::Mat gray;
	cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);

	// Apply a binary threshold to the image
	cv::Mat binary;
	cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

	// Find contours
	std::vector<std::vector<cv::Point>> contours;
	std::vector<cv::Vec4i> hierarchy;
	cv::findContours(binary, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

	// Convert the contour to a tool path
	std::vector<cv::Point2d> path;
	for (size_t i = 0; i < contours.size(); i++)
	{
		for (size_t j = 0; j < contours[i].size(); j++)
		{
			path.push_back(contours[i][j]);
		}
	}

	// Store the tool path in the struct
	toolpath.Path = path;
}

// Function to resize the image to fit the screen
// inputImage: the input image
// screenWidth: the width of the screen
// screenHeight: the height of the screen
// return: the resized image
cv::Mat fitImageToScreen(const cv::Mat& inputImage, int screenWidth, int screenHeight, ImageScaleFactor& scalefactor)
{
	int imageWidth = inputImage.cols;
	int imageHeight = inputImage.rows;

	// Calculate the scaling factors
	double scaleX = static_cast<double>(screenWidth) / imageWidth;
	double scaleY = static_cast<double>(screenHeight) / imageHeight;

	// Choose the minimum scaling factor to fit both width and height
	double scaleFactor = min(scaleX, scaleY);

	// Resize the image
	cv::Mat resizedImage;
	cv::resize(inputImage, resizedImage, cv::Size(), scaleFactor, scaleFactor);

	// Set the scale factors
	scalefactor.X = static_cast<float>(scaleFactor);
	scalefactor.Y = static_cast<float>(scaleFactor);

	return resizedImage;
}

bool drawing = false; // Flag to check if the user is currently drawing
cv::Point startPoint; // Starting point of the rectangle

void mouseCallback(int event, int x, int y, int, void* userdata)
{
	auto data = static_cast<std::pair<cv::Rect*, cv::Mat*>*>(userdata);
	cv::Rect* rect = data->first;
	cv::Mat* src = data->second;

	if (event == cv::EVENT_LBUTTONDOWN)
	{
		// Set the starting point and initialize drawing flag
		startPoint = cv::Point(x, y);
		drawing = true;
	}
	else if (event == cv::EVENT_MOUSEMOVE && drawing)
	{
		// Update the rectangle dimensions while dragging the mouse
		cv::Mat img = src->clone();
		*rect = cv::Rect(startPoint, cv::Point(x, y));
		//With red color
		cv::rectangle(img, *rect, cv::Scalar(255, 255, 255), 1);

		//cv::rectangle(img, *rect, cv::Scalar(255, 255, 255), 2); // White color
		cv::imshow("Select Area", img);
	}
	else if (event == cv::EVENT_LBUTTONUP)
	{
		// Finalize the rectangle dimensions when mouse button is released
		drawing = false;
		*rect = cv::Rect(startPoint, cv::Point(x, y));
		cv::Mat img = src->clone();
		cv::rectangle(img, *rect, cv::Scalar(255, 255, 255), 2); // White color
		cv::imshow("Select Area", img);
	}
}

// Display the image with image scale factor imgscl
// use mouse to select the area
// src: input image
// templ: output template
// rect: output rectangle
// display the image and select the area
void CreateTemplate(cv::Mat& src, cv::Mat& templ, cv::Rect& rect)
{
	// check if the image is empty
	if (src.empty())
	{
		//std::cerr << "Error: Image is empty!" << std::endl;
		MessageBox(NULL, _T("Error: Image is empty!"), _T("Error"), MB_OK);
		return;
	}

	// Display the image
	cv::imshow("Select Area", src);

	// Set the callback function for mouse events
	cv::setMouseCallback("Select Area", mouseCallback, new std::pair<cv::Rect*, cv::Mat*>(&rect, &src));

	// Wait for the user to select the area and press ESC to exit
	while (true)
	{
		int key = cv::waitKey(0);
		if (key == 27) // ESC
		{
			break;
		}
	}

	// Destroy the window
	cv::destroyWindow("Select Area");

	// Crop the template image
	templ = src(rect);
}

//feature match template 
// cv::Mat& ImageSrc: Source image
// cv::Mat& ImageTemp: template image
// cv::Mat& ImageDst: output image
// match_method: method to match the template
// Location: output location of the template in the image
int MatchTemplate(cv::Mat& ImageSrc, cv::Mat& ImageTemp, cv::Mat& ImageDst, int match_method, ImageLocation& Location)
{
	// Detect ORB keypoints and descriptors in both images
	cv::Ptr<cv::ORB> orb = cv::ORB::create();
	std::vector<cv::KeyPoint> keypointsSrc, keypointsTemp;
	cv::Mat descriptorsSrc, descriptorsTemp;

	orb->detectAndCompute(ImageSrc, cv::noArray(), keypointsSrc, descriptorsSrc);
	orb->detectAndCompute(ImageTemp, cv::noArray(), keypointsTemp, descriptorsTemp);

	// Match descriptors using BFMatcher
	cv::BFMatcher matcher(cv::NORM_HAMMING, true);
	std::vector<cv::DMatch> matches;
	matcher.match(descriptorsTemp, descriptorsSrc, matches);

	if (matches.empty())
	{
		std::cerr << "No matches found!" << std::endl;
		return -1;
	}

	// Extract location of good matches
	std::vector<cv::Point2f> pointsTemp, pointsSrc;
	for (size_t i = 0; i < matches.size(); i++)
	{
		pointsTemp.push_back(keypointsTemp[matches[i].queryIdx].pt);
		pointsSrc.push_back(keypointsSrc[matches[i].trainIdx].pt);
	}

	// Find homography
	cv::Mat H = cv::findHomography(pointsTemp, pointsSrc, cv::RANSAC);

	// Get the corners from the template image
	std::vector<cv::Point2f> cornersTemp(4);
	cornersTemp[0] = cv::Point2f(0, 0);
	cornersTemp[1] = cv::Point2f(static_cast<float>(ImageTemp.cols), 0);
	cornersTemp[2] = cv::Point2f(static_cast<float>(ImageTemp.cols), static_cast<float>(ImageTemp.rows));
	cornersTemp[3] = cv::Point2f(0, static_cast<float>(ImageTemp.rows));

	// Transform the corners to the source image
	std::vector<cv::Point2f> cornersSrc(4);
	cv::perspectiveTransform(cornersTemp, cornersSrc, H);

	// Calculate the bounding box
	cv::Rect boundingBox = cv::boundingRect(cornersSrc);
	Location.Rect = boundingBox;

	// Calculate the center position
	Location.Position = cv::Point2d(boundingBox.x + boundingBox.width / 2.0, boundingBox.y + boundingBox.height / 2.0);

	// Calculate the angle
	double angle = atan2(cornersSrc[1].y - cornersSrc[0].y, cornersSrc[1].x - cornersSrc[0].x) * 180.0 / CV_PI;
	Location.Angle = static_cast<float>(angle);

	return 0; // Return success
}

std::string GetAppPath()
{
	wchar_t path[MAX_PATH];
	GetModuleFileNameW(NULL, path, MAX_PATH);

	// Convert wchar_t array to std::wstring, then to std::string (UTF-8 encoding)
	std::wstring wfullPath = path;
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, wfullPath.c_str(), (int)wfullPath.size(), NULL, 0, NULL, NULL);
	std::string fullPath(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, wfullPath.c_str(), (int)wfullPath.size(), &fullPath[0], size_needed, NULL, NULL);

	// Find the last backslash to remove the filename, keeping only the directory path
	size_t lastSlash = fullPath.find_last_of("\\");

	if (lastSlash != std::string::npos)
	{
		return fullPath.substr(0, lastSlash);
	}

	return fullPath;
}

//System Tools
//Get mac address
void GetMacAddress(char* macAddress)
{
	//Get the MAC address of the computer, Get the first if exit
	IP_ADAPTER_ADDRESSES* pAddresses = NULL;
	ULONG outBufLen = 0;
	DWORD dwRetVal = 0;
	// Call GetAdaptersAddresses to get the size needed
	dwRetVal = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, pAddresses, &outBufLen);
	if (dwRetVal == ERROR_BUFFER_OVERFLOW)
	{
		pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
		if (pAddresses == NULL)
		{
			//std::cerr << "Error: Memory allocation failed for IP_ADAPTER_ADDRESSES struct" << std::endl;
			MessageBox(NULL, _T("Error: Memory allocation failed for IP_ADAPTER_ADDRESSES struct"), _T("Error"), MB_OK);
			return;
		}
	}
	dwRetVal = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, pAddresses, &outBufLen);
	if (dwRetVal == NO_ERROR)
	{
		IP_ADAPTER_ADDRESSES* pCurrAddresses = pAddresses;
		while (pCurrAddresses)
		{
			if (pCurrAddresses->PhysicalAddressLength != 0)
			{
				// Format the MAC address
				sprintf_s(macAddress, 18, "%02X-%02X-%02X-%02X-%02X-%02X",
					pCurrAddresses->PhysicalAddress[0],
					pCurrAddresses->PhysicalAddress[1],
					pCurrAddresses->PhysicalAddress[2],
					pCurrAddresses->PhysicalAddress[3],
					pCurrAddresses->PhysicalAddress[4],
					pCurrAddresses->PhysicalAddress[5]);
				break; // Get the first MAC address and exit
			}
			pCurrAddresses = pCurrAddresses->Next;
		}
	}
	else
	{
		//std::cerr << "Error: GetAdaptersAddresses failed with error: " << dwRetVal << std::endl;
		MessageBox(NULL, _T("Error: GetAdaptersAddresses failed"), _T("Error"), MB_OK);
	}
	if (pAddresses)
	{
		free(pAddresses);
	}
	return;

}

// Encrypt function
//Use AES encryption
// input: the input data
// output: the output data
void Encrypt(unsigned char* input, unsigned char* output, unsigned char* key)
{
	
}

//Coordinate Transformation Tools
// Init the transformer with 3 points
// imagePts: the pixel coordinate of the 3 points
// worldPts: the real world coordinate of the 3 points
// count: the number of the points
// cv::Mat & affineMatrix: the affine matrix of the transformation form pixel to world
void InitTransformer(float* imagePts, float* worldPts, int count, cv::Mat& affineMatrix)
{
	std::vector<cv::Point2f> img, world;
	// Build corresponding pixel/world point sets, then estimate affine transform.
	for (int i = 0; i < count; ++i) {
		img.emplace_back(imagePts[i * 2], imagePts[i * 2 + 1]);
		world.emplace_back(worldPts[i * 2], worldPts[i * 2 + 1]);
	}
	affineMatrix = cv::estimateAffine2D(img, world);
}



// Build a 3x3 homogeneous affine matrix from the estimated 2x3 transform.
void InitTransformer(const float* imagePts, const float* worldPts, int count, cv::Mat& affineMatrix)
{
	std::vector<cv::Point2f> img, world;
	img.reserve(count);
	world.reserve(count);

	for (int i = 0; i < count; ++i) {
		img.emplace_back(imagePts[i * 2], imagePts[i * 2 + 1]);
		world.emplace_back(worldPts[i * 2], worldPts[i * 2 + 1]);
	}

	cv::Mat affine2x3 = cv::estimateAffine2D(img, world); // 2x3 affine matrix
	if (affine2x3.empty()) return;

	// Expand to 3x3 homogeneous form for convenient matrix multiplication.
	affineMatrix = cv::Mat::eye(3, 3, CV_64F);
	affine2x3.copyTo(affineMatrix(cv::Rect(0, 0, 3, 2)));
}

// Transform pixel to real world coordinate
// x, y: the pixel coordinate
// outX, outY: the real world coordinate
// cv::Mat & affineMatrix: the affine matrix of the transformation form pixel to world
/*
bool TransformPixel(float x, float y, float* outX, float* outY, cv::Mat affineMatrix)
{
	//cv::Mat affineMatrix;
	if (affineMatrix.empty()) return false;

	cv::Mat pt = (cv::Mat_<double>(3, 1) << x, y, 1.0);
	cv::Mat result = affineMatrix * pt;

	*outX = static_cast<float>(result.at<double>(0, 0));
	*outY = static_cast<float>(result.at<double>(1, 0));
	return true;
}
*/

// Transform pixel to real world coordinate
// x, y: the pixel coordinate
// outX, outY: the real world coordinate
// cv::Mat & affineMatrix: the affine matrix of the transformation form pixel to world
/*
//Transform image pixel to real world coordinate
// With 3 points to calculate the affine matrix: InitTransformer + PixelToWorld
// x_pixel: the x coordinate of the pixel
// y_pixel: the y coordinate of the pixel
// &x_mm: the x coordinate of the real world
// &y_mm: the y coordinate of the real world
// cv::Mat & affineMatrix: the affine matrix of the transformation form pixel to world
void PixelToWorld(float x_pixel, float y_pixel, float& x_mm, float& y_mm, cv::Mat affineMatrix)
{
	//cv::Mat affineMatrix;
	if (affineMatrix.empty()) return;

	cv::Mat pt = (cv::Mat_<double>(3, 1) << x_pixel, y_pixel, 1.0);
	cv::Mat result = affineMatrix * pt;

	x_mm = static_cast<float>(result.at<double>(0, 0));
	y_mm = static_cast<float>(result.at<double>(1, 0));

}
*/

// Transform pixel to real world coordinate
// x, y: the pixel coordinate
// outX, outY: the real world coordinate
// cv::Mat & affineMatrix: the affine matrix of the transformation form pixel to world
// Convert one image pixel coordinate to world/mm coordinate.
inline void PixelToWorld(float x_pixel, float y_pixel, float& x_mm, float& y_mm, const cv::Mat& affineMatrix)
{
	if (affineMatrix.empty()) return;

	// Homogeneous coordinate transform.
	cv::Mat pt = (cv::Mat_<double>(3, 1) << x_pixel, y_pixel, 1.0);
	cv::Mat result = affineMatrix * pt;

	x_mm = static_cast<float>(result.at<double>(0, 0));
	y_mm = static_cast<float>(result.at<double>(1, 0));
}



/*  Example usage of InitTransformer / PixelToWorld
int main()
{
	float imagePts[] = {1097,1063, 1373,1063, 1371,945};
	float worldPts[] = {34.79f,205.19f, 187.19f,205.19f, 187.19f,141.79f};

	InitTransformer(imagePts, worldPts, 3);

	float x_mm, y_mm;
	if (TransformPixel(1200, 1000, &x_mm, &y_mm)) {
		std::cout << "World Coord: (" << x_mm << ", " << y_mm << ") mm\n";
	} else {
		std::cerr << "Transform failed.\n";
	}
}
*/



//Data Tools

// Double Word split to Hight Word and Low Word
// DW2W(int32 dw, int16* hw, int16* lw)

// Split a 32-bit value into high word and low word.
void splitDoubleWord(uint32_t doubleWord, uint16_t& highWord, uint16_t& lowWord)
{
	highWord = (doubleWord >> 16) & 0xFFFF; // upper 16 bits
	lowWord = doubleWord & 0xFFFF;          // lower 16 bits
}

/*
uint16_t tab_reg[2];
uint32_t value = 0x12345678;
splitDoubleWord(value, tab_reg[0], tab_reg[1]); // high word / low word
modbus_write_registers(ctx, 0, 2, tab_reg);     // write to PLC
*/



//Create a database with sqlite3
//void CreateDatabase()
int CreateDatabase(sqlite3* db, const char* db_name)
{
	// Open the database
	int rc = sqlite3_open(db_name, &db);
	if (rc)
	{
		//std::cerr << "Error: Can't open database: " << sqlite3_errmsg(db) << std::endl;
		MessageBox(NULL, _T("Error: Can't open database"), _T("Error"), MB_OK);
		return -1;
	}

	// Create a table
	const char* sql = "CREATE TABLE IF NOT EXISTS TestTable (ID INT PRIMARY KEY NOT NULL, Name TEXT NOT NULL);";
	rc = sqlite3_exec(db, sql, NULL, 0, NULL);
	if (rc != SQLITE_OK)
	{
		//std::cerr << "Error: Can't create table: " << sqlite3_errmsg(db) << std::endl;
		MessageBox(NULL, _T("Error: Can't create table"), _T("Error"), MB_OK);
		return -1;
	}

	// Insert data into the table
	sql = "INSERT INTO MachineTable (ID, Name) VALUES (1, 'Test');";
	rc = sqlite3_exec(db, sql, NULL, 0, NULL);
	if (rc != SQLITE_OK)
	{
		//std::cerr << "Error: Can't insert data: " << sqlite3_errmsg(db) << std::endl;
		MessageBox(NULL, _T("Error: Can't insert data"), _T("Error"), MB_OK);
		return -1;
	}

	// Close the database
	sqlite3_close(db);

	return 0; // Return success
}

//Insert a single data record into the database
int InsertSingleData(sqlite3* db, const char* db_name, const char* table_name, const char* data)
{
	// Open the database
	int rc = sqlite3_open(db_name, &db);
	if (rc)
	{
		//std::cerr << "Error: Can't open database: " << sqlite3_errmsg(db) << std::endl;
		MessageBox(NULL, _T("Error: Can't open database"), _T("Error"), MB_OK);
		return -1;
	}

	// Insert data into the table
	std::string sql = "INSERT INTO " + std::string(table_name) + " VALUES (" + std::string(data) + ");";
	rc = sqlite3_exec(db, sql.c_str(), NULL, 0, NULL);
	if (rc != SQLITE_OK)
	{
		//std::cerr << "Error: Can't insert data: " << sqlite3_errmsg(db) << std::endl;
		MessageBox(NULL, _T("Error: Can't insert data"), _T("Error"), MB_OK);
		return -1;
	}

	// Close the database
	sqlite3_close(db);

	return 0; // Return success
}

//Insert multiple data records into the database
int InsertMassData(sqlite3* db, const char* db_name, const char* table_name, const char* data, int n)
{
	// Open the database
	int rc = sqlite3_open(db_name, &db);
	if (rc)
	{
		//std::cerr << "Error: Can't open database: " << sqlite3_errmsg(db) << std::endl;
		MessageBox(NULL, _T("Error: Can't open database"), _T("Error"), MB_OK);
		return -1;
	}

	// Insert data into the table
	std::string sql = "INSERT INTO " + std::string(table_name) + " VALUES ";
	for (int i = 0; i < n; i++)
	{
		sql += "(" + std::string(data) + ")";
		if (i < n - 1)
		{
			sql += ", ";
		}
	}
	rc = sqlite3_exec(db, sql.c_str(), NULL, 0, NULL);
	if (rc != SQLITE_OK)
	{
		//std::cerr << "Error: Can't insert data: " << sqlite3_errmsg(db) << std::endl;
		MessageBox(NULL, _T("Error: Can't insert data"), _T("Error"), MB_OK);
		return -1;
	}

	// Close the database
	sqlite3_close(db);

	return 0; // Return success
}

//Query data from the database
int QueryData(sqlite3* db, const char* db_name, const char* table_name, const char* data)
{
	// Open the database
	int rc = sqlite3_open(db_name, &db);
	if (rc)
	{
		//std::cerr << "Error: Can't open database: " << sqlite3_errmsg(db) << std::endl;
		MessageBox(NULL, _T("Error: Can't open database"), _T("Error"), MB_OK);
		return -1;
	}

	// Query data from the table
	std::string sql = "SELECT * FROM " + std::string(table_name) + " WHERE " + std::string(data) + ";";
	rc = sqlite3_exec(db, sql.c_str(), NULL, 0, NULL);
	if (rc != SQLITE_OK)
	{
		//std::cerr << "Error: Can't query data: " << sqlite3_errmsg(db) << std::endl;
		MessageBox(NULL, _T("Error: Can't query data"), _T("Error"), MB_OK);
		return -1;
	}

	// Close the database
	sqlite3_close(db);

	return 0; // Return success
}

//Update data in the database
int UpdateData(sqlite3* db, const char* db_name, const char* table_name, const char* data)
{
	// Open the database
	int rc = sqlite3_open(db_name, &db);
	if (rc)
	{
		//std::cerr << "Error: Can't open database: " << sqlite3_errmsg(db) << std::endl;
		MessageBox(NULL, _T("Error: Can't open database"), _T("Error"), MB_OK);
		return -1;
	}

	// Update data in the table
	std::string sql = "UPDATE " + std::string(table_name) + " SET " + std::string(data) + ";";
	rc = sqlite3_exec(db, sql.c_str(), NULL, 0, NULL);
	if (rc != SQLITE_OK)
	{
		//std::cerr << "Error: Can't update data: " << sqlite3_errmsg(db) << std::endl;
		MessageBox(NULL, _T("Error: Can't update data"), _T("Error"), MB_OK);
		return -1;
	}

	// Close the database
	sqlite3_close(db);

	return 0; // Return success
}

//Delete data from the database
int DeleteData(sqlite3* db, const char* db_name, const char* table_name, const char* data)
{
	// Open the database
	int rc = sqlite3_open(db_name, &db);
	if (rc)
	{
		//std::cerr << "Error: Can't open database: " << sqlite3_errmsg(db) << std::endl;
		MessageBox(NULL, _T("Error: Can't open database"), _T("Error"), MB_OK);
		return -1;
	}

	// Delete data from the table
	std::string sql = "DELETE FROM " + std::string(table_name) + " WHERE " + std::string(data) + ";";
	rc = sqlite3_exec(db, sql.c_str(), NULL, 0, NULL);
	if (rc != SQLITE_OK)
	{
		//std::cerr << "Error: Can't delete data: " << sqlite3_errmsg(db) << std::endl;
		MessageBox(NULL, _T("Error: Can't delete data"), _T("Error"), MB_OK);
		return -1;
	}

	// Close the database
	sqlite3_close(db);

	return 0; // Return success
}

//Close the database
int CloseDatabase(sqlite3* db)
{
	// Close the database
	sqlite3_close(db);

	return 0; // Return success
}


//System Configuration ini
//Create a new system configuration ini file
// Write system configuration to ini file
// Read System Configuration from ini file
//Update system configuration value from ini file



// Helper function to write configuration to file
void WriteConfigToFile(const std::string& filename, SystemConfig& SysConfig)
{
	std::ofstream file(filename);
	if (!file.is_open()) {
		std::cerr << "Error: Unable to open file " << filename << " for writing!" << std::endl;
		throw std::runtime_error("Unable to open configuration file for writing");
	}

	// Write system configuration data with consistent formatting
	file << "[ModbusTCP]\n";
	file << "IpAddress=" << SysConfig.IpAddress << "\n";
	file << "Port=" << SysConfig.Port << "\n";
	file << "StationID=" << SysConfig.StationID << "\n";

	file << "[ToolPath]\n";
	file << "OffsetX=" << std::fixed << std::setprecision(4) << SysConfig.OffsetX << "\n";
	file << "OffsetY=" << std::fixed << std::setprecision(4) << SysConfig.OffsetY << "\n";

	file << "[Camera]\n";
	file << "CameraID=" << SysConfig.CameraID << "\n";
	file << "MACKey=" << SysConfig.MACKey << "\n";
	file << "GoldenKey=" << SysConfig.GoldenKey << "\n";
	file << "CameraWidth=" << SysConfig.CameraWidth << "\n";
	file << "CameraHeight=" << SysConfig.CameraHeight << "\n";
	file << "TransferFactor=" << std::fixed << std::setprecision(4) << SysConfig.TransferFactor << "\n";
	file << "ImageFlip=" << SysConfig.ImageFlip << "\n";
	file << "CenterX=" << std::fixed << std::setprecision(2) << SysConfig.CenterX << "\n";
	file << "CenterY=" << std::fixed << std::setprecision(2) << SysConfig.CenterY << "\n";

	// Write mask section
	file << "[Mask]\n";
	file << "MaskX=" << SysConfig.MaskX << "\n";
	file << "MaskY=" << SysConfig.MaskY << "\n";
	file << "MaskWidth=" << SysConfig.MaskWidth << "\n";
	file << "MaskHeight=" << SysConfig.MaskHeight << "\n";

	file << "[Machine]\n";
	file << "MachineType=" << SysConfig.MachineType << "\n";
	file << "JogVelocity=" << SysConfig.JogVelocity << "\n";
	file << "AutoVelocity=" << SysConfig.AutoVelocity << "\n";
	file << "DecAcceleration=" << SysConfig.DecAcceleration << "\n";
	file << "IncAcceleration=" << SysConfig.IncAcceleration << "\n";
	file << "Pitch=" << std::fixed << std::setprecision(2) << SysConfig.Pitch << "\n";
	file << "Z1=" << std::fixed << std::setprecision(2) << SysConfig.Z1 << "\n";
	file << "Z2=" << std::fixed << std::setprecision(2) << SysConfig.Z2 << "\n";
	file << "Z3=" << std::fixed << std::setprecision(2) << SysConfig.Z3 << "\n";
	file << "Z4=" << std::fixed << std::setprecision(2) << SysConfig.Z4 << "\n";
	file << "Z5=" << std::fixed << std::setprecision(2) << SysConfig.Z5 << "\n";

	file.close();
}
// Helper function to write configuration to file
void WriteConfigToFile_SP(const std::string& filename, const SystemConfigA& SysConfig)
{
	std::ofstream file(filename);
	if (!file.is_open()) {
		throw std::runtime_error("Unable to open configuration file for writing");
	}

	file << "[ModbusTCP]\n";
	file << "IpAddress=" << SysConfig.IpAddress << "\n";
	file << "Port=" << SysConfig.Port << "\n";
	file << "StationID=" << SysConfig.StationID << "\n";

	file << "[ToolPath]\n";
	file << std::fixed << std::setprecision(4);
	file << "OffsetValue=" << SysConfig.OffsetValue << "\n";
	file << "TransferFactor=" << SysConfig.TransferFactor << "\n";

	file << "[Camera]\n";
	file << "CameraID=" << SysConfig.CameraID << "\n";
	file << "MACKey=" << SysConfig.MACKey << "\n";
	file << "GoldenKey=" << SysConfig.GoldenKey << "\n";
	file << "HMI_ID=" << SysConfig.HMI_ID << "\n";
	file << "PLC_ID=" << SysConfig.PLC_ID << "\n";
	file << "CameraWidth=" << SysConfig.CameraWidth << "\n";
	file << "CameraHeight=" << SysConfig.CameraHeight << "\n";
	file << "ImageFlip=" << SysConfig.ImageFlip << "\n";
	file << "ImageBinary=" << SysConfig.ImageBinary << "\n";
	file << "CameraSerialNumber=" << SysConfig.CameraSerialNumber << "\n";  // NEW

	file << "[ROI]\n";
	file << "RefCenterX=" << SysConfig.RefCenterX << "\n";
	file << "RefCenterY=" << SysConfig.RefCenterY << "\n";
	file << "DispayROI=" << SysConfig.DisplayROI << "\n";

	file << "[Mask]\n";
	file << "MaskX=" << SysConfig.MaskX << "\n";
	file << "MaskY=" << SysConfig.MaskY << "\n";
	file << "MaskWidth=" << SysConfig.MaskWidth << "\n";
	file << "MaskHeight=" << SysConfig.MaskHeight << "\n";

	file << "[Binary]\n";
	file << "BinaryUpper=" << SysConfig.BinaryUpper << "\n";
	file << "BinaryLower=" << SysConfig.BinaryLower << "\n";

	file << "[Tool]\n";
	file << "CreateToolPath=" << SysConfig.CreateToolPath << "\n";
	file << "DispalyToolPath=" << SysConfig.DispalyToolPath << "\n";
	file << "DisplayRefLine=" << SysConfig.DisplayRefLine << "\n";  // NEW

	file << "[UI]\n";                                                  // NEW section
	file << "TabWork=" << SysConfig.TabWork << "\n";                   // NEW

	file << "[Machine]\n";
	file << "MachineType=" << SysConfig.MachineType << "\n";
}

// Initialize system configuration file
void InitialConfig(const std::string& filename, SystemConfig& SysConfig)
{

	WriteConfigToFile(filename, SysConfig);
}

// Initialize system configuration file
//void InitialConfigA(const std::string& filename, SystemConfigA& SysConfig)
void InitialConfigA(const std::string& filename, SystemConfigA& SysConfig)
{
	// Fill default values for a new configuration file.

	SysConfig.IpAddress = "192.168.1.10";
	SysConfig.Port = 502;
	SysConfig.StationID = 1;
	SysConfig.TransferFactor = 1.0f;
	SysConfig.MachineType = "AX-3";
	SysConfig.TabWork = 1;
	SysConfig.MaskX = 400;
	SysConfig.MaskY = 200;
	SysConfig.MaskWidth = 650;
	SysConfig.MaskHeight = 870;	
	SysConfig.OffsetValue = 10.0f;
	SysConfig.ImageFlip = 2;   
	SysConfig.RefCenterX = 695.0f;
	SysConfig.RefCenterY = 194.0f;
	SysConfig.HMI_ID[0] = '\0';
	SysConfig.PLC_ID[0] = '\0';

	WriteConfigToFile_SP(filename, SysConfig);
}

// Read system configuration from INI file; initialize if file doesn't exist
int ReadSystemConfig(const std::string& filename, SystemConfig& SysConfig)
{
	std::ifstream file(filename);
	if (!file.is_open()) {
		// If file doesn't exist, initialize with default configuration
		// Initialize default mask values.
		SysConfig.MaskX = 0;
		SysConfig.MaskY = 0;
		SysConfig.MaskWidth = 0;
		SysConfig.MaskHeight = 0;

		InitialConfig(filename, SysConfig);
		return -1; // Indicate default configuration was used
	}

	//const unsigned char* key;
	//GetMACAddress((char*)SysConfig.MACKey);

	auto trim = [](std::string& s) {
		const char* ws = " \t\r\n";
		size_t start = s.find_first_not_of(ws);
		if (start == std::string::npos) { s.clear(); return; }
		size_t end = s.find_last_not_of(ws);
		s = s.substr(start, end - start + 1);
		};

	// Reset mask values before parsing the file.
	SysConfig.MaskX = 0;
	SysConfig.MaskY = 0;
	SysConfig.MaskWidth = 0;
	SysConfig.MaskHeight = 0;

	std::string line;
	while (std::getline(file, line)) {
		// Skip empty lines or lines without '='
		if (line.empty() || line.find('=') == std::string::npos) {
			continue;
		}

		try
		{
			// split key and value
			size_t pos = line.find('=');
			std::string key = line.substr(0, pos);
			std::string val = line.substr(pos + 1);
			trim(key);
			trim(val);

			if (key == "IpAddress") {
				SysConfig.IpAddress = val;
			}
			else if (key == "Port") {
				SysConfig.Port = val.empty() ? 0 : std::stoi(val);
			}
			else if (key == "StationID") {
				SysConfig.StationID = val.empty() ? 0 : std::stoi(val);
			}
			else if (key == "OffsetX") {
				SysConfig.OffsetX = val.empty() ? 0.0f : std::stof(val);
			}
			else if (key == "OffsetY") {
				SysConfig.OffsetY = val.empty() ? 0.0f : std::stof(val);
			}
			else if (key == "CameraID") {
				SysConfig.CameraID = val.empty() ? 0 : std::stoi(val);
			}
			else if (key == "ImageFlip") {
				SysConfig.ImageFlip = val.empty() ? 0 : std::stoi(val);
			}
			else if (key == "MACKey") {
				strncpy_s(SysConfig.MACKey, sizeof(SysConfig.MACKey), val.c_str(), _TRUNCATE);
			}
			else if (key == "GoldenKey") {
				strncpy_s(SysConfig.GoldenKey, sizeof(SysConfig.GoldenKey), val.c_str(), _TRUNCATE);
			}
			else if (key == "CameraWidth") {
				SysConfig.CameraWidth = val.empty() ? 0 : std::stoi(val);
			}
			else if (key == "CameraHeight") {
				SysConfig.CameraHeight = val.empty() ? 0 : std::stoi(val);
			}
			else if (key == "TransferFactor") {
				SysConfig.TransferFactor = val.empty() ? 0.0f : std::stof(val);
			}
			else if (key == "CenterX") {
				SysConfig.CenterX = val.empty() ? 0.0f : std::stof(val);
			}
			else if (key == "CenterY") {
				SysConfig.CenterY = val.empty() ? 0.0f : std::stof(val);
			}
			// Read mask parameters.
			else if (key == "MaskX") {
				SysConfig.MaskX = val.empty() ? 0 : std::stoi(val);
			}
			else if (key == "MaskY") {
				SysConfig.MaskY = val.empty() ? 0 : std::stoi(val);
			}
			else if (key == "MaskWidth") {
				SysConfig.MaskWidth = val.empty() ? 0 : std::stoi(val);
			}
			else if (key == "MaskHeight") {
				SysConfig.MaskHeight = val.empty() ? 0 : std::stoi(val);
			}
			else if (key == "MachineType") {
				SysConfig.MachineType = val;
			}
			else if (key == "JogVelocity") {
				SysConfig.JogVelocity = val.empty() ? 0 : std::stoi(val);
			}
			else if (key == "AutoVelocity") {
				SysConfig.AutoVelocity = val.empty() ? 0 : std::stoi(val);
			}
			else if (key == "DecAcceleration") {
				SysConfig.DecAcceleration = val.empty() ? 0 : std::stoi(val);
			}
			else if (key == "IncAcceleration") {
				SysConfig.IncAcceleration = val.empty() ? 0 : std::stoi(val);
			}
			else if (key == "Pitch") {
				SysConfig.Pitch = val.empty() ? 0.0f : std::stof(val);
			}
			else if (key == "Z1") {
				SysConfig.Z1 = val.empty() ? 0.0f : std::stof(val);
			}
			else if (key == "Z2") {
				SysConfig.Z2 = val.empty() ? 0.0f : std::stof(val);
			}
			else if (key == "Z3") {
				SysConfig.Z3 = val.empty() ? 0.0f : std::stof(val);
			}
			else if (key == "Z4") {
				SysConfig.Z4 = val.empty() ? 0.0f : std::stof(val);
			}
			else if (key == "Z5") {
				SysConfig.Z5 = val.empty() ? 0.0f : std::stof(val);
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "Error parsing config line: " << line << ", Error: " << e.what() << std::endl;
			// Continue processing other lines
		}
	}

	file.close();
	return 0; // Indicate successful read
}

// Read system configuration from INI file; initialize if file doesn't exist
int ReadSystemConfig_SP(const std::string& filename, SystemConfigA& SysConfig)
{
	std::ifstream file(filename);
	if (!file.is_open()) {
		InitialConfigA(filename, SysConfig);
		return -1;
	}

	auto trim = [](std::string& s) {
		const char* ws = " \t\r\n";
		size_t start = s.find_first_not_of(ws);
		if (start == std::string::npos) { s.clear(); return; }
		size_t end = s.find_last_not_of(ws);
		s = s.substr(start, end - start + 1);
		};

	std::string line;

	while (std::getline(file, line))
	{
		if (line.empty() || line.find('=') == std::string::npos)
			continue;

		try
		{
			size_t pos = line.find('=');
			std::string key = line.substr(0, pos);
			std::string val = line.substr(pos + 1);
			trim(key);
			trim(val);

			if (key == "IpAddress")        SysConfig.IpAddress = val;
			else if (key == "Port")              SysConfig.Port = std::stoi(val);
			else if (key == "StationID")         SysConfig.StationID = std::stoi(val);
			else if (key == "OffsetValue")       SysConfig.OffsetValue = std::stof(val);
			else if (key == "TransferFactor")    SysConfig.TransferFactor = std::stof(val);

			else if (key == "CameraID")          SysConfig.CameraID = std::stoi(val);
			else if (key == "MACKey")            strncpy_s(SysConfig.MACKey, val.c_str(), _TRUNCATE);
			else if (key == "GoldenKey")         strncpy_s(SysConfig.GoldenKey, val.c_str(), _TRUNCATE);
			else if (key == "HMI_ID")            strncpy_s(SysConfig.HMI_ID, val.c_str(), _TRUNCATE);
			else if (key == "PLC_ID")            strncpy_s(SysConfig.PLC_ID, val.c_str(), _TRUNCATE);
			else if (key == "CameraWidth")       SysConfig.CameraWidth = std::stoi(val);
			else if (key == "CameraHeight")      SysConfig.CameraHeight = std::stoi(val);
			else if (key == "ImageFlip")         SysConfig.ImageFlip = std::stoi(val);
			else if (key == "ImageBinary")       SysConfig.ImageBinary = std::stoi(val);
			else if (key == "CameraSerialNumber") SysConfig.CameraSerialNumber = val;  // NEW

			else if (key == "RefCenterX")        SysConfig.RefCenterX = std::stoi(val);
			else if (key == "RefCenterY")        SysConfig.RefCenterY = std::stoi(val);
			else if (key == "DisplayROI")        SysConfig.DisplayROI = std::stoi(val);

			else if (key == "MaskX")             SysConfig.MaskX = std::stoi(val);
			else if (key == "MaskY")             SysConfig.MaskY = std::stoi(val);
			else if (key == "MaskWidth")         SysConfig.MaskWidth = std::stoi(val);
			else if (key == "MaskHeight")        SysConfig.MaskHeight = std::stoi(val);

			else if (key == "BinaryUpper")       SysConfig.BinaryUpper = std::stoi(val);
			else if (key == "BinaryLower")       SysConfig.BinaryLower = std::stoi(val);

			else if (key == "CreateToolPath")    SysConfig.CreateToolPath = std::stoi(val);
			else if (key == "DispalyToolPath")   SysConfig.DispalyToolPath = std::stoi(val);
			else if (key == "DisplayRefLine")    SysConfig.DisplayRefLine = std::stoi(val);  // NEW

			else if (key == "TabWork")           SysConfig.TabWork = std::stoi(val);  // NEW

			else if (key == "MachineType")       SysConfig.MachineType = val;
		}
		catch (...) {
			std::cerr << "Config parse error: " << line << std::endl;
		}
	}

	return 0;
}

// Update configuration file contents on disk.
void UpdateSystemConfig(const std::string& filename, SystemConfig& SysConfig)
{
	// Reuse the common writer to persist the latest settings.
	WriteConfigToFile(filename, SysConfig);
}



// UModbus thread safety
std::mutex plc_mutex;

void SafeModbusRead(/*...*/)
{
	std::lock_guard<std::mutex> lock(plc_mutex);
	// Placeholder for thread-safe UModbus read wrapper.
}

void SafeModbusWrite(/*...*/) {
	std::lock_guard<std::mutex> lock(plc_mutex);
	// Placeholder for thread-safe UModbus write wrapper.
}

int SafeModbusReadRegisters(modbus_t* ctx, int addr, int nb, uint16_t* dest)
{
	std::lock_guard<std::mutex> lock(plc_mutex);
	return modbus_read_registers(ctx, addr, nb, dest);
}
int SafeModbusWriteRegisters(modbus_t* ctx, int addr, int nb, const uint16_t* data)
{
	std::lock_guard<std::mutex> lock(plc_mutex);
	return modbus_write_registers(ctx, addr, nb, data);
}
int SafeModbusWriteRegister(modbus_t* ctx, int addr, uint16_t value)
{
	std::lock_guard<std::mutex> lock(plc_mutex);
	return modbus_write_register(ctx, addr, value);
}
int SafeModbusReadBits(modbus_t* ctx, int addr, int nb, uint8_t* dest)
{
	std::lock_guard<std::mutex> lock(plc_mutex);
	return modbus_read_bits(ctx, addr, nb, dest);
}
int SafeModbusWriteBit(modbus_t* ctx, int addr, int status)
{
	std::lock_guard<std::mutex> lock(plc_mutex);
	return modbus_write_bit(ctx, addr, status);
}

void GetMACAddress(unsigned char* macAddress)
{
	//Get the MAC address of the computer, Get the first if exit
	IP_ADAPTER_ADDRESSES* pAddresses = NULL;
	ULONG outBufLen = 0;
	DWORD dwRetVal = 0;
	// Call GetAdaptersAddresses to get the size needed
	dwRetVal = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, pAddresses, &outBufLen);
	if (dwRetVal == ERROR_BUFFER_OVERFLOW)
	{
		pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
		if (pAddresses == NULL)
		{
			//std::cerr << "Error: Memory allocation failed for IP_ADAPTER_ADDRESSES struct" << std::endl;
			MessageBox(NULL, _T("Error: Memory allocation failed for IP_ADAPTER_ADDRESSES struct"), _T("Error"), MB_OK);
			return;
		}
	}
	dwRetVal = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, pAddresses, &outBufLen);
	if (dwRetVal == NO_ERROR)
	{
		IP_ADAPTER_ADDRESSES* pCurrAddresses = pAddresses;
		while (pCurrAddresses)
		{
			if (pCurrAddresses->PhysicalAddressLength != 0)
			{
				// Copy the MAC address
				memcpy(macAddress, pCurrAddresses->PhysicalAddress, pCurrAddresses->PhysicalAddressLength);
				break; // Get the first MAC address and exit
			}
			pCurrAddresses = pCurrAddresses->Next;
		}
	}
	else
	{
		//std::cerr << "Error: GetAdaptersAddresses failed with error: " << dwRetVal << std::endl;
		MessageBox(NULL, _T("Error: GetAdaptersAddresses failed"), _T("Error"), MB_OK);
	}
	if (pAddresses)
	{
		free(pAddresses);
	}
	return;
}



/**
 * @brief GluePathOptimizer DLL wrapper entry.
 */
void OptimizeGluePath(
	const std::vector<cv::Point2d>& inputPath,
	const ROIMask& roi,
	GluePath& optimizedPath,
	int shoeType)
{
	try {
		if (inputPath.empty()) return;

		// 1. Create optimizer with ROI/mask information.
		GluePathOptimizer optimizer(roi);

		// 2. Run path optimization.
		optimizer.OptimizePath(inputPath, optimizedPath, shoeType);

		// 3. Optional debug output
#ifdef _DEBUG
		std::string msg = "[UAX_DLL] Optimized Path Points: Left=" +
			std::to_string(optimizedPath.PathLeft.size()) +
			", Right=" + std::to_string(optimizedPath.PathRight.size()) + "\n";
		OutputDebugStringA(msg.c_str());
#endif
	}
	catch (const std::exception& e) {
		std::cerr << "OptimizeGluePath Error: " << e.what() << std::endl;
	}
}
