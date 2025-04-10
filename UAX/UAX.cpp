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
void  GetToolPath(cv::Mat& ImgSrc, cv::Point2d Offset, ToolPath& toolpath)
{
	// Validate input image
	if (ImgSrc.empty()) {
		throw std::invalid_argument("Input image is empty.");
	}

	cv::Mat result = ImgSrc.clone(); // Clone the input image

	// Calculate erosion iterations based on Offset
	int numPixelsToErode = static_cast<int>(Offset.x + Offset.y);

	// Create erosion kernel
	cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));

	// Apply erosion iteratively
	for (int i = 0; i < numPixelsToErode; ++i) {
		cv::erode(result, result, kernel);
	}

	// Check if result is already grayscale (1 channel)
	cv::Mat gray;
	if (result.channels() != 1) {
		// Convert the image to grayscale if it is not already grayscale
		cv::cvtColor(result, gray, cv::COLOR_BGR2GRAY);
	}
	else {
		// If the image is already grayscale, just use it as is
		gray = result;
	}

	// Apply binary thresholding
	cv::Mat thresh;
	cv::threshold(gray, thresh, 128, 255, cv::THRESH_BINARY);

	// Find contours to derive the tool path
	std::vector<std::vector<cv::Point>> contours; // Contours detected
	std::vector<cv::Vec4i> hierarchy;            // Contour hierarchy
	cv::findContours(thresh, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

	// 在原始影像上繪製輪廓
	cv::Mat outputImage = ImgSrc.clone();
	cv::drawContours(outputImage, contours, -1, Scalar(0, 255, 0), 2);

	// Populate the toolpath
	toolpath.Offset = Offset;
	//toolpath.Path.clear();
	for (const auto& contour : contours)
	{
		for (const auto& point : contour) {
			toolpath.Path.push_back(cv::Point2d(point));
		}
	}
	
	// Draw contours on the original image for visualization
	cv::drawContours(ImgSrc, contours, -1, cv::Scalar(0, 255, 0), 2);

	// Display the images
	cv::imshow("Input Image", ImgSrc);

	//cv::imshow("Eroded Result", result);
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

/*
//feature match template with FLANN
// cv::Mat& ImageSrc: Source image
// cv::Mat& ImageTemp: template image
// cv::Mat& ImageDst: output image
// match_method: method to match the template
// Location: output location of the template in the image
// Offset: output offset of the template in the image
int MatchTemplateFLANN(cv::Mat& ImageSrc, cv::Mat& ImageTemp, cv::Mat& ImageDst, int match_method, ImageLocation& Location, cv::Point2d Offset)
{
	//-- Step 1: Detect the keypoints using SURF Detector, compute the descriptors
	int minHessian = 400;
	// Detect ORB keypoints and descriptors in both images
	Ptr<SURF> detector = cv::SURF::create();
	std::vector<cv::KeyPoint> keypointsSrc, keypointsTemp;
	cv::Mat descriptorsSrc, descriptorsTemp;

	orb->detectAndCompute(ImageSrc, cv::noArray(), keypointsSrc, descriptorsSrc);
	orb->detectAndCompute(ImageTemp, cv::noArray(), keypointsTemp, descriptorsTemp);

	// Match descriptors using FLANN
	cv::FlannBasedMatcher matcher;
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
	Location.Position = cv;
}
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
	sql = "INSERT INTO TestTable (ID, Name) VALUES (1, 'Test');";
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


//Add a class with Pylon to get the camera information, Grab the image and save the image


