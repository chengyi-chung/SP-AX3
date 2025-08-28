#ifdef UAX_EXPORTS
#define UAX_API __declspec(dllexport)
#else
#define UAX_API __declspec(dllimport)
#endif

#pragma once
#include <atlimage.h>
//#include <opencv2/core/core.hpp>
//#include <opencv2/highgui/highgui.hpp>
//#include <opencv2/imgproc/imgproc.hpp>
#include <vector>


#include "opencv2/core.hpp"


#include "opencv2/calib3d.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/features2d.hpp"
//#include "opencv2/xfeatures2d.hpp"

//SQLite3
#include <sqlite3.h>


//#include <boost/property_tree/ptree.hpp>
//#include <boost/property_tree/ini_parser.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>

using namespace cv;
using namespace std;

//using namespace cv::xfeatures2d;
using std::cout;
using std::endl;

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

extern "C" UAX_API int MatchTemplateFLANN(cv::Mat& src, cv::Mat& templ, cv::Mat& dst, int match_method, ImageLocation& Location, cv::Point2d Offset);

//Lens calibration function with  Mutiple images for huge FOV
// Use the images in the folder to calibrate the lens
// // src: the input image
// // templ: the template image
// // dst: the output image
// // match_method: the method to match the template
// // Location: the output location of the template in the image
extern "C" UAX_API int LensCalibration(cv::Mat& src, cv::Mat& templ, cv::Mat& dst, int match_method, ImageLocation& Location, cv::Point2d Offset);

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




//***********************************************************************
//     SQLite Database functions
//***********************************************************************
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

//Get Application Path
extern "C" UAX_API std::string GetAppPath();


struct SystemConfig
{
    std::string IpAddress;
    int Port;
    int StationID;
    float OffsetX;
    float OffsetY;
    int CameraID;
    char MACKey[17];    // 128 bit key
	char GoldenKey[17]; // 128 bit key
    int CameraWidth;
    int CameraHeight;
    float TransferFactor;
    int ImageFlip;
    std::string MachineType;
    int JogVelocity;
    int AutoVelocity;
    int DecAcceleration;
    int IncAcceleration;
    float Pitch;
};

// Write system configuration to ini file
extern "C" UAX_API void WriteConfigToFile(const std::string& filename,  SystemConfig &SysConfig);
// Read System Configuration from ini file
extern "C" UAX_API int ReadSystemConfig(const std::string& filename, SystemConfig &SysConfig);
// Initialize the system configuration ini file
//extern "C" UAX_API void InitialConfig(const std::string& filename, const SystemConfig& SysConfig);



