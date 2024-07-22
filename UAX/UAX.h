#ifdef UAX_EXPORTS
#define UAX_API __declspec(dllexport)
#else
#define UAX_API __declspec(dllimport)
#endif

#pragma once
#include <atlimage.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <vector>

#include <sqlite3.h>

using namespace cv;
using namespace std;



//add a AES encrypt function 
// 128位密钥


//word w[4 * (Nr + 1)];

//C dll for AX-3 PLC 
//Add two float numbers
//a: the first number
//b: the second number
//return: the sum of the two numbers
extern "C" UAX_API  float Add(float a, float b);

//extern "C" UAX_API float Add(float a, float b);

extern "C" UAX_API void Encrypt(byte * input, byte * output, byte * key);

extern "C" UAX_API void Decrypt(byte * input, byte * output, byte * key);

//Modbus TCP/IP

// Machine Vision functions
//OpenCV function for UAX

// struct fo scale factor of image
struct ImageScaleFactor
{
	float X;
	float Y;
};

//Struct for contoure area and perimeter
struct ContourArea
{
	double Area;
	double Perimeter;
};


struct ImageLocation
{
	cv::Point2d Position; // Position of the template in the image
	cv::Rect Rect;           // Rectangle of the template in the image
	float Angle;			     // Angle of the template in the image 
};

struct ToolPath
{
	cv::Point2d Offset; // Offset of the tool path
	std::vector<cv::Point2d> Path; // Tool path
};

struct YUFA
{
    // 0 or 1 data type
	int type;


	cv::Point2d Position; // Position of the template in the image
};

// Function to resize the image to fit the screen
// inputImage: the input image
// screenWidth: the width of the screen
// screenHeight: the height of the screen
// Scale factor of image: ImageScaleFactor
// return: the resized image
extern "C" UAX_API cv::Mat fitImageToScreen(const cv::Mat& inputImage, int screenWidth, int screenHeight, ImageScaleFactor& scalefactor);
//extern "C" UAX_API cv::Mat fitImageToScreen(const cv::Mat& inputImage, int screenWidth, int screenHeight, );

// Display the image with image scale factor imgscl
extern "C" UAX_API void DisplayImage(cv::Mat & src, cv::String window_name, float imgscl);

//Create a template image
extern "C" UAX_API void CreateTemplate(cv::Mat & src, cv::Mat & templ, cv::Rect & rect);
//extern "C" UAX_API void DisplayImage(cv::Mat & src, cv::String window_name);

//match template
// Get the position of the template in the image
extern "C" UAX_API int MatchTemplate(cv::Mat& src, cv::Mat& templ, cv::Mat& dst, int match_method, ImageLocation &Location);

// Get Tool Path
// Use Erosiong find the tool path
// ImgSrc: the input image
// Offset: the offse value of the tool path
// ToolPath: the output tool path
extern "C" UAX_API void GetToolPath(cv::Mat& ImgSrc, cv::Point2d Offset, ToolPath& toolpath);


//Find the area of image
//Use findContours to find the area of image
// cv::Mat& src: the input image
//ContourArea is a struct to store the area and perimeter of the contour
extern "C" UAX_API void FindArea(cv::Mat& src, ContourArea& contourarea);

//Convert contour to tool path
//
extern "C" UAX_API void ContourToToolPath(cv::Mat& src, ToolPath& toolpath);





//SQlite Database
//Create a database
extern "C" UAX_API int CreateDatabase(sqlite3* db, const char* db_name);

//Insert a single data record into the database
extern "C" UAX_API int InsertSingleData(sqlite3* db, const char* db_name, const char* table_name, const char* data);

//Insert multiple data records into the database
extern "C" UAX_API int InsertMassData(sqlite3* db, const char* db_name, const char* table_name, const char* data, int n);

//Query data from the database
extern "C" UAX_API int QueryData(sqlite3* db, const char* db_name, const char* table_name, const char* data);

//Update data in the database
extern "C" UAX_API int UpdateData(sqlite3* db, const char* db_name, const char* table_name, const char* data);

//Delete data from the database
extern "C" UAX_API int DeleteData(sqlite3* db, const char* db_name, const char* table_name, const char* data);

//Close the database
extern "C" UAX_API int CloseDatabase(sqlite3* db);


