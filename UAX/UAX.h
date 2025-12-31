#ifdef UAX_EXPORTS
#define UAX_API __declspec(dllexport)
#else
#define UAX_API __declspec(dllimport)
#endif

#pragma once
#include <cstdint>
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

#include "modbus.h"

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

// System Security Tools
// AES Encrypt and Decrypt function 

extern "C" UAX_API void Encrypt(unsigned char* input, unsigned char* output, unsigned char* key);

extern "C" UAX_API void Decrypt(unsigned char* input, unsigned char* output, unsigned char* key);

//Get MAC address function
//xtern "C" UAX_API void GetMACAddress(unsigned char* macAddress);
extern "C" UAX_API void GetMacAddress(char* macAddress);








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

//Original tool path of image
// Offset: the offset of the tool path
// Path: the tool path
// numClusters: number of clusters in the tool path
struct ToolPath
{
    cv::Point2d Offset;                // Offset of the tool path
    std::vector<cv::Point2d> Path;     // Tool path
    std::vector<int> numClusters;      // 對應 Path 每個點的分群編號（依 contour 分群）
};

struct YUFA
{
    // 0 or 1 data type
    int type;
    cv::Point2d Position; // Position of the template in the image
};


// 基本點結構
typedef struct {
    double x;
    double y;
} Point2D;

// 分群結果結構
typedef struct {
    Point2D* points;
    int count;
} Cluster;


//
// OpenCV function for UAX
//

// Function to resize the image to fit the screen
// inputImage: the input image
// screenWidth: the width of the screen
// screenHeight: the height of the screen
// Scale factor of image: ImageScaleFactor
// return: the resized image
extern "C" UAX_API cv::Mat fitImageToScreen(const cv::Mat& inputImage, int screenWidth, int screenHeight, ImageScaleFactor& scalefactor);
//extern "C" UAX_API cv::Mat fitImageToScreen(const cv::Mat& inputImage, int screenWidth, int screenHeight, );

//Convert Image to subpixel
//extern "C" UAX_API void ImageToSubPixel(const cv::Mat& src, cv::Mat &dstImg);

// Display the image with image scale factor imgscl
extern "C" UAX_API void DisplayImage(cv::Mat& src, cv::String window_name, float imgscl);

//Create a template image
extern "C" UAX_API void CreateTemplate(cv::Mat& src, cv::Mat& templ, cv::Rect& rect);
//extern "C" UAX_API void DisplayImage(cv::Mat & src, cv::String window_name);

//match template
// Get the position of the template in the image
extern "C" UAX_API int MatchTemplate(cv::Mat& src, cv::Mat& templ, cv::Mat& dst, int match_method, ImageLocation& Location);

extern "C" UAX_API int MatchTemplateFLANN(cv::Mat& src, cv::Mat& templ, cv::Mat& dst, int match_method, ImageLocation& Location, cv::Point2d Offset);

// 分群主函式（KD-Tree）
extern "C" UAX_API int ClusterKDTree(const Point2D* input, int inputSize, double radius,
    Cluster** outputClusters, int* clusterCount);

// 平滑處理（Savitzky-Golay）
extern "C" UAX_API int SmoothPath(const Point2D* input, int inputSize, int windowSize,
    Point2D* output);

// B-spline 擬合
extern "C" UAX_API  int FitBSpline(const Point2D* input, int inputSize, int degree,
    Point2D* output, int* outputSize);

// 釋放記憶體
extern "C" UAX_API void FreeClusters(Cluster* clusters, int clusterCount);




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
// Offset: the offse value of the tool path(Pixel)
// ToolPath: the output tool path
extern "C" UAX_API void GetToolPath(cv::Mat& ImgSrc, cv::Point2d Offset, ToolPath& toolpath);
// Get Tool Path
// Use Erosiong find the tool path
// ImgSrc: the input image
// Offset: the offse value of the tool path(Pixel)
// ToolPath: the output tool path
// With mask image to limit the area of tool path
//extern "C" UAX_API void GetToolPathWithMask(cv::Mat& ImgSrc, cv::Mat& Mask, cv::Point2d Offset, ToolPath& toolpath);
extern "C" UAX_API void GetToolPathWithMask(const cv::Mat& ImgSrc, const cv::Mat& Mask, double offsetDistance, ToolPath& toolpath);
//extern "C" UAX_API void GetToolPathWithMask(cv::Mat& ImgSrc, cv::Mat& Mask, cv::Point2d Offset, ToolPath& toolpath);


//Find the area of image
//Use findContours to find the area of image
// cv::Mat& src: the input image
//ContourArea is a struct to store the area and perimeter of the contour
extern "C" UAX_API void FindArea(cv::Mat& src, ContourArea& contourarea);

//Convert contour to tool path
//
extern "C" UAX_API void ContourToToolPath(cv::Mat& src, ToolPath& toolpath);






//Convert Coordinate to real world coordinate with




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




//Data Tools

// Double Word split to Hight Word and Low Word
// DW2W(int32 dw, int16* hw, int16* lw)

// 函數：將 Double Word 拆分為 High Word 和 Low Word
extern "C" UAX_API void splitDoubleWord(uint32_t doubleWord, uint16_t& highWord, uint16_t& lowWord);

//Transform image pixel to real world coordinate
//With 3 points to calculate the affine matrix :  InitTransformer、TransformPixel 
// x_pixel: the x coordinate of the pixel
// y_pixel: the y coordinate of the pixel
// &x_mm: the x coordinate of the real world
// &y_mm: the y coordinate of the real world
// imagePts: 指向影像座標點陣列的指標（長度至少6，3點）
// worldPts: 指向對應世界座標點陣列的指標（長度至少6，3點）
//cv::Mat & affineMatrix: the affine matrix of the transformation form pixel to world

extern "C" UAX_API void InitTransformer(float* imagePts, float* worldPts, int numPoints, cv::Mat& affineMatrix);
//extern "C" UAX_API void PixelToWorld(float x_pixel, float y_pixel, float& x_mm, float& y_mm, float* imagePts, float* worldPts);
//extern "C" UAX_API void PixelToWorld(float x_pixel, float y_pixel, float& x_mm, float& y_mm, cv::Mat affineMatrix);
extern "C" UAX_API void PixelToWorld(float x_pixel, float y_pixel, float& x_mm, float& y_mm, const cv::Mat& affineMatrix);
//extern "C" UAX_API bool TransformPixel(float x, float y, float* outX, float* outY, cv::Mat affineMatrix);



//
//  SystemConfig
//


struct SystemConfig
{
    std::string IpAddress;
    int Port;
    int StationID;
    float OffsetX;
    float OffsetY;
    int CameraID;
	char MACKey[18];    // MAC Address as key
    char GoldenKey[18]; // 128 bit key
    int CameraWidth;
    int CameraHeight;
    float TransferFactor;
    int ImageFlip;
    float CenterX;
    float CenterY;
    std::string MachineType;
    int JogVelocity;
    int AutoVelocity;
    int DecAcceleration;
    int IncAcceleration;
    float Pitch;
    float Z1;
    float Z2;
    float Z3;
    float Z4;
    float Z5;
    int MaskX;
    int MaskY;
    int MaskWidth;
    int MaskHeight;
};

// Write system configuration to ini file
extern "C" UAX_API void WriteConfigToFile(const std::string& filename, SystemConfig& SysConfig);
// Read System Configuration from ini file
extern "C" UAX_API int ReadSystemConfig(const std::string& filename, SystemConfig& SysConfig);
// Initialize the system configuration ini file
//extern "C" UAX_API void InitialConfig(const std::string& filename, const SystemConfig& SysConfig);

//Get Application Path
extern "C" UAX_API std::string GetAppPath();



//Get mac address
//extern "C" UAX_API void GetMacAddress(char* macAddress);

//
// 裝 thread-safe 函式
//

extern "C" UAX_API int SafeModbusReadRegisters(modbus_t* ctx, int addr, int nb, uint16_t* dest);
extern "C" UAX_API int SafeModbusWriteRegisters(modbus_t* ctx, int addr, int nb, const uint16_t* data);
extern "C" UAX_API int SafeModbusWriteRegister(modbus_t* ctx, int addr, uint16_t value);
extern "C" UAX_API int SafeModbusReadBits(modbus_t* ctx, int addr, int nb, uint8_t* dest);
extern "C" UAX_API int SafeModbusWriteBit(modbus_t* ctx, int addr, int status);
//extern "C" UAX_API void foo(unsigned char* data, int len);
// 或

//void foo(std::uint8_t* data, int len);
