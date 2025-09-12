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
//units of Offset is pixel
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

void InitTransformer(float* imagePts, float* worldPts, int count)
{
	std::vector<cv::Point2f> img, world;
    // 在檔案開頭（全域區域）宣告 affineMatrix 變數
    cv::Mat affineMatrix;
	for (int i = 0; i < count; ++i) {
		img.emplace_back(imagePts[i * 2], imagePts[i * 2 + 1]);
		world.emplace_back(worldPts[i * 2], worldPts[i * 2 + 1]);
	}
	affineMatrix = cv::estimateAffine2D(img, world);
}

//Transform pixel to real world coordinate
// x, y: the pixel coordinate
// outX, outY: the real world coordinate
bool TransformPixel(float x, float y, float* outX, float* outY) 
{
	cv::Mat affineMatrix;
	if (affineMatrix.empty()) return false;

	cv::Mat pt = (cv::Mat_<double>(3, 1) << x, y, 1.0);
	cv::Mat result = affineMatrix * pt;

	*outX = static_cast<float>(result.at<double>(0, 0));
	*outY = static_cast<float>(result.at<double>(1, 0));
	return true;
}

//Transform image pixel to real world coordinate
//With 3 points to calculate the affine matrix :  InitTransformer、TransformPixel 
// x_pixel: the x coordinate of the pixel
// y_pixel: the y coordinate of the pixel
// &x_mm: the x coordinate of the real world
// &y_mm: the y coordinate of the real world
// imagePts: 指向影像座標點陣列的指標（長度至少6，3點）
// worldPts: 指向對應世界座標點陣列的指標（長度至少6，3點）
void PixelToWorld(float x_pixel, float y_pixel, float& x_mm, float& y_mm, float* imagePts, float* worldPts)
{
	if (!imagePts || !worldPts) {
		std::cerr << "Invalid input points.\n";
		return;
	}

	InitTransformer(imagePts, worldPts, 3);

	if (TransformPixel(x_pixel, y_pixel, &x_mm, &y_mm)) {
		std::cout << "World Coord: (" << x_mm << ", " << y_mm << ") mm\n";
	}
	else {
		std::cerr << "Transform failed.\n";
	}
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
	file << "[Machine]\n";
	file << "MachineType=" << SysConfig.MachineType << "\n";
	file << "JogVelocity=" << SysConfig.JogVelocity << "\n";
	file << "AutoVelocity=" << SysConfig.AutoVelocity << "\n";
	file << "DecAcceleration=" << SysConfig.DecAcceleration << "\n";
	file << "IncAcceleration=" << SysConfig.IncAcceleration << "\n";
	file << "Pitch=" << std::fixed << std::setprecision(2) << SysConfig.Pitch << "\n";
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
		InitialConfig(filename, SysConfig);
		return -1; // Indicate default configuration was used
	}

	std::string line;
	while (std::getline(file, line)) {
		// Skip empty lines or lines without '='
		if (line.empty() || line.find('=') == std::string::npos) {
			continue;
		}

		try {
			if (line.find("IpAddress=") == 0) {
				SysConfig.IpAddress = line.length() > 10 ? line.substr(10) : "";
			}
			else if (line.find("Port=") == 0) {
				SysConfig.Port = line.length() > 5 ? std::stoi(line.substr(5)) : 0;
			}
			else if (line.find("StationID=") == 0) {
				SysConfig.StationID = line.length() > 10 ? std::stoi(line.substr(10)) : 0;
			}
			else if (line.find("OffsetX=") == 0) {
				SysConfig.OffsetX = line.length() > 8 ? std::stof(line.substr(8)) : 0.000f;
			}
			else if (line.find("OffsetY=") == 0) {
				SysConfig.OffsetY = line.length() > 8 ? std::stof(line.substr(8)) : 0.000f;
			}
			else if (line.find("CameraID=") == 0) {
				SysConfig.CameraID = line.length() > 9 ? std::stoi(line.substr(9)) : 0;
			}
			else if (line.find("ImageFlip=") == 0) {
				SysConfig.ImageFlip = line.length() > 10 ? std::stoi(line.substr(10)) : 0;
			}
			else if (line.find("MACKey=") == 0) {
				std::string key = line.length() > 7 ? line.substr(7) : "";
				strncpy_s(SysConfig.MACKey, sizeof(SysConfig.MACKey), key.c_str(), _TRUNCATE);
			}	
			else if (line.find("GoldenKey=") == 0) {
				std::string key = line.length() > 10 ? line.substr(10) : "";
				strncpy_s(SysConfig.GoldenKey, sizeof(SysConfig.GoldenKey), key.c_str(), _TRUNCATE);
			}
			else if (line.find("CameraWidth=") == 0) {
				SysConfig.CameraWidth = line.length() > 12 ? std::stoi(line.substr(12)) : 0;
			}
			else if (line.find("CameraHeight=") == 0) {
				SysConfig.CameraHeight = line.length() > 13 ? std::stoi(line.substr(13)) : 0;
			}
			else if (line.find("TransferFactor=") == 0) {
				SysConfig.TransferFactor = line.length() > 15 ? std::stof(line.substr(15)) : 0.0000f;
			}
			else if (line.find("CenterX=") == 0) {
				SysConfig.CenterX = line.length() > 8 ? std::stof(line.substr(8)) : 0.000f;
			}
			else if (line.find("CenterY=") == 0) {
				SysConfig.CenterY = line.length() > 8 ? std::stof(line.substr(8)) : 0.000f;
			}
			else if (line.find("MachineType=") == 0) {
				SysConfig.MachineType = line.length() > 12 ? line.substr(12) : "";
			}
			else if (line.find("JogVelocity=") == 0) {
				SysConfig.JogVelocity = line.length() > 12 ? std::stoi(line.substr(12)) : 0;
			}
			else if (line.find("AutoVelocity=") == 0) {
				SysConfig.AutoVelocity = line.length() > 13 ? std::stoi(line.substr(13)) : 0;
			}
			else if (line.find("DecAcceleration=") == 0) {
				SysConfig.DecAcceleration = line.length() > 16 ? std::stoi(line.substr(16)) : 0;
			}
			else if (line.find("IncAcceleration=") == 0) {
				SysConfig.IncAcceleration = line.length() > 16 ? std::stoi(line.substr(16)) : 0;
			}
			else if (line.find("Pitch=") == 0) {
				SysConfig.Pitch = line.length() > 6 ? std::stof(line.substr(6)) : 0.000f;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Error parsing config line: " << line << ", Error: " << e.what() << std::endl;
			// Continue processing other lines
		}
	}

	return 0; // Indicate successful read
}

// 更新系統配置到 INI 檔案
void UpdateSystemConfig(const std::string& filename,  SystemConfig &SysConfig)
{
    // 將 const SystemConfig* 轉為 SystemConfig*，以符合 WriteConfigToFile 的參數型別
    WriteConfigToFile(filename, SysConfig);
}
