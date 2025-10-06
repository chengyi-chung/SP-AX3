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
#include <iphlpapi.h>
#include <stdio.h>

//Add UAX.h
#include "UAX.h"


// T MAcro
#define _T(x) L ## x

#pragma comment(lib, "iphlpapi.lib")  // Link with iphlpapi.lib
#pragma comment(lib, "ws2_32.lib")  // Link with ws2_32.lib : Winsock2 Library for Windows Sockets programming 
//定義  PIP_ADAPTER_ADDRESSES



//
#include <iostream>


using namespace std;
using namespace cv;



byte key[16] = { 0x2b, 0x7e, 0x15, 0x16,
			0x28, 0xae, 0xd2, 0xa6,
			0xab, 0xf7, 0x15, 0x88,
			0x09, 0xcf, 0x4f, 0x3c };

byte plain[4 * 4] = { 0x32, 0x88, 0x31, 0xe0,
				0x43, 0x5a, 0x31, 0x37,
				0xf6, 0x30, 0x98, 0x07,
				0xa8, 0x8d, 0xa2, 0x34 };


float Add(float a, float b)
{
	return a + b;
}

/////////////////////////////

#include <windows.h> // 用於取得螢幕解析度（Windows 專用）

void ShowZoomedImage(const std::string& windowName, const cv::Mat& image)
{
	// 取得螢幕解析度
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	// 計算縮放比例
	double scaleX = static_cast<double>(screenWidth) / image.cols;
	double scaleY = static_cast<double>(screenHeight) / image.rows;
	double scale = min(scaleX, scaleY); // 保持比例不失真

	// 縮放影像
	cv::Mat resized;
	cv::resize(image, resized, cv::Size(), scale, scale, cv::INTER_LINEAR);

	// 顯示影像
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

	cv::Mat thresh;
	cv::threshold(gray, thresh, 128, 255, cv::THRESH_BINARY);

	std::vector<std::vector<cv::Point>> contours;
	std::vector<cv::Vec4i> hierarchy;
	cv::findContours(thresh, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

	cv::Mat outputImage = ImgSrc.clone();
	cv::drawContours(outputImage, contours, -1, cv::Scalar(0, 255, 0), 2);

	toolpath.Offset = Offset;
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

// Get Tool Path
// Use Erosiong find the tool path
// ImgSrc: the input image
// Offset: the offse value of the tool path(Pixel)
// ToolPath: the output tool path
// With mask image to limit the area of tool path
void GetToolPathWithMask(const cv::Mat& ImgSrc, const cv::Mat& Mask, double offsetDistance, ToolPath& toolpath)
{
	// Input validation
	if (ImgSrc.empty() || Mask.empty())
	{
		throw std::invalid_argument("Input image or mask is empty.");
	}
	if (ImgSrc.size() != Mask.size())
	{
		throw std::invalid_argument("Image and mask sizes do not match.");
	}

	// Apply mask to source image
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

	// Perform erosion for offset
	cv::Mat result = maskedImage.clone();
	int numPixelsToErode = static_cast<int>(offsetDistance);
	cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));

	for (int i = 0; i < numPixelsToErode; ++i)
	{
		cv::erode(result, result, kernel);
	}

	// Convert to grayscale if needed
	cv::Mat gray;
	if (result.channels() != 1)
	{
		cv::cvtColor(result, gray, cv::COLOR_BGR2GRAY);
	}
	else
	{
		gray = result;
	}

	// Apply threshold
	cv::Mat thresh;
	cv::threshold(gray, thresh, 200, 255, cv::THRESH_BINARY);

	// Find contours in masked region
	std::vector<std::vector<cv::Point>> contours;
	std::vector<cv::Vec4i> hierarchy;
	cv::findContours(thresh, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

	// Smooth contours
	std::vector<std::vector<cv::Point2d>> smoothedContours;
	for (const auto& contour : contours)
	{
		// Convert contour to Point2d for smoothing
		std::vector<cv::Point2d> points;
		for (const auto& point : contour)
		{
			points.push_back(cv::Point2d(point.x, point.y));
		}

		// Apply Gaussian smoothing to the points
		std::vector<cv::Point2d> smoothedPoints;
		int smoothingSize = 5; // Kernel size for smoothing (must be odd)
		double sigma = 1.5; // Standard deviation for Gaussian kernel
		if (points.size() >= smoothingSize)
		{
			std::vector<double> xCoords(points.size()), yCoords(points.size());
			for (size_t i = 0; i < points.size(); ++i)
			{
				xCoords[i] = points[i].x;
				yCoords[i] = points[i].y;
			}

			// Create Gaussian kernel
			cv::Mat kernel1D = cv::getGaussianKernel(smoothingSize, sigma, CV_64F);
			cv::Mat xMat(1, xCoords.size(), CV_64F, xCoords.data());
			cv::Mat yMat(1, yCoords.size(), CV_64F, yCoords.data());
			cv::Mat smoothedX, smoothedY;

			// Apply convolution for smoothing
			cv::filter2D(xMat, smoothedX, -1, kernel1D, cv::Point(-1, -1), 0, cv::BORDER_REFLECT);
			cv::filter2D(yMat, smoothedY, -1, kernel1D, cv::Point(-1, -1), 0, cv::BORDER_REFLECT);

			// Store smoothed points
			for (size_t i = 0; i < points.size(); ++i)
			{
				smoothedPoints.push_back(cv::Point2d(smoothedX.at<double>(0, i), smoothedY.at<double>(0, i)));
			}
		}
		else
		{
			smoothedPoints = points; // Skip smoothing if contour is too small
		}

		smoothedContours.push_back(smoothedPoints);
	}

	// Store results in toolpath
	toolpath.Offset = cv::Point2d(offsetDistance, offsetDistance);
	int indexPath = 0;
	for (const auto& contour : smoothedContours)
	{
		for (const auto& point : contour)
		{
			toolpath.Path.push_back(point);
			indexPath++;
		}
	}

	// Print tool path data to a file in debug mode
#ifdef _DEBUG
	std::ofstream outFile("C:\\Temp\\ToolPathData.txt");
	if (outFile.is_open())
	{
		for (const auto& point : toolpath.Path)
		{
			outFile << point.x << ", " << point.y << std::endl;
		}
		outFile.close();
	}
#endif

	// Draw smoothed contours on original image
	cv::Mat outputImage = ImgSrc.clone();
	std::vector<std::vector<cv::Point>> contoursToDraw;
	for (const auto& smoothedContour : smoothedContours)
	{
		std::vector<cv::Point> contourInt;
		for (const auto& point : smoothedContour)
		{
			contourInt.push_back(cv::Point(static_cast<int>(point.x), static_cast<int>(point.y)));
		}
		contoursToDraw.push_back(contourInt);
	}
	cv::drawContours(outputImage, contoursToDraw, -1, cv::Scalar(0, 255, 0), 2);

	// Display result
	ShowZoomedImage("Smoothed Tool Path", outputImage);
	cv::waitKey(0);
	cv::destroyAllWindows();
}



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
// use mouse to select the area
void CreateTemplate(cv::Mat & src, cv::Mat & templ, cv::Rect & rect)
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

//Coordinate Transformation Tools
// Init the transformer with 3 points
// imagePts: the pixel coordinate of the 3 points
// worldPts: the real world coordinate of the 3 points
// count: the number of the points
// cv::Mat & affineMatrix: the affine matrix of the transformation form pixel to world
void InitTransformer(float* imagePts, float* worldPts, int count, cv::Mat& affineMatrix)
{
	std::vector<cv::Point2f> img, world;
    // 在檔案開頭（全域區域）宣告 affineMatrix 變數
    //cv::Mat affineMatrix;
	for (int i = 0; i < count; ++i) {
		img.emplace_back(imagePts[i * 2], imagePts[i * 2 + 1]);
		world.emplace_back(worldPts[i * 2], worldPts[i * 2 + 1]);
	}
	affineMatrix = cv::estimateAffine2D(img, world);
}



// 建立 3×3 仿射矩陣（齊次表示）
void InitTransformer(const float* imagePts, const float* worldPts, int count, cv::Mat& affineMatrix)
{
	std::vector<cv::Point2f> img, world;
	img.reserve(count);
	world.reserve(count);

	for (int i = 0; i < count; ++i) {
		img.emplace_back(imagePts[i * 2], imagePts[i * 2 + 1]);
		world.emplace_back(worldPts[i * 2], worldPts[i * 2 + 1]);
	}

	cv::Mat affine2x3 = cv::estimateAffine2D(img, world); // 2×3
	if (affine2x3.empty()) return;

	// 擴展成 3×3
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
// 將像素轉世界座標
/*
//Transform image pixel to real world coordinate
//With 3 points to calculate the affine matrix :  InitTransformer、TransformPixel
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
// 將像素轉世界座標
inline void PixelToWorld(float x_pixel, float y_pixel, float& x_mm, float& y_mm, const cv::Mat& affineMatrix)
{
	if (affineMatrix.empty()) return;

	// 使用齊次座標做轉換
	cv::Mat pt = (cv::Mat_<double>(3, 1) << x_pixel, y_pixel, 1.0);
	cv::Mat result = affineMatrix * pt;

	x_mm = static_cast<float>(result.at<double>(0, 0));
	y_mm = static_cast<float>(result.at<double>(1, 0));
}



/*  以上 InitTransformer、TransformPixel 使用範例
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

// 函數：將 Double Word 拆分為 High Word 和 Low Word
void splitDoubleWord(uint32_t doubleWord, uint16_t& highWord, uint16_t& lowWord)
{
	highWord = (doubleWord >> 16) & 0xFFFF; // 提取高 16 位
	lowWord = doubleWord & 0xFFFF;          // 提取低 16 位
}

/*
uint16_t tab_reg[2];
uint32_t value = 0x12345678;
splitDoubleWord(value, tab_reg[0], tab_reg[1]); // 拆分為 High Word 和 Low Word
modbus_write_registers(ctx, 0, 2, tab_reg);     // 寫入 PLC
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
void WriteConfigToFile(const std::string& filename, SystemConfig &SysConfig)
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
	
	// 新增 Mask 區段
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

// Initialize system configuration file
void InitialConfig(const std::string& filename, SystemConfig &SysConfig)
{
	
	WriteConfigToFile(filename, SysConfig);
}

// Read system configuration from INI file; initialize if file doesn't exist
int ReadSystemConfig(const std::string& filename, SystemConfig &SysConfig)
{
	std::ifstream file(filename);
	if (!file.is_open()) {
		// If file doesn't exist, initialize with default configuration
		// 設定預設的 mask 值
		SysConfig.MaskX = 0;
		SysConfig.MaskY = 0;
		SysConfig.MaskWidth = 0;
		SysConfig.MaskHeight = 0;
		
		InitialConfig(filename, SysConfig);
		return -1; // Indicate default configuration was used
	}

	auto trim = [](std::string &s) {
		const char* ws = " \t\r\n";
		size_t start = s.find_first_not_of(ws);
		if (start == std::string::npos) { s.clear(); return; }
		size_t end = s.find_last_not_of(ws);
		s = s.substr(start, end - start + 1);
	};

	// 初始化 mask 預設值
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
			// 新增 mask 參數讀取
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

// 更新系統配置到 INI 檔案
void UpdateSystemConfig(const std::string& filename,  SystemConfig &SysConfig)
{
    // 將 const SystemConfig* 轉為 SystemConfig*，以符合 WriteConfigToFile 的參數型別
    WriteConfigToFile(filename, SysConfig);
}



// UModbus thread safety
std::mutex plc_mutex;

void SafeModbusRead(/*...*/) 
{
    std::lock_guard<std::mutex> lock(plc_mutex);
    // 呼叫 UModbus 讀取函式
}

void SafeModbusWrite(/*...*/) {
    std::lock_guard<std::mutex> lock(plc_mutex);
    // 呼叫 UModbus 寫入函式
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


