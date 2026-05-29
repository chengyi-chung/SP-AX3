#pragma once
#include <vector>
#include <opencv2/core.hpp>
// ──────────────────────────────────────────────
//  資料結構定義
// ──────────────────────────────────────────────
// Original tool path of image
// Offset: the offset of the tool path
// Path: the tool path
// numClusters: number of clusters in the tool path
struct ToolPath
{
    cv::Point2d            Offset;       // 工具路徑的整體偏移量
    std::vector<cv::Point2d> Path;       // 工具路徑上的點序列
    std::vector<int>       numClusters;  // 每個點所屬的輪廓/叢集編號
};  

struct GluePath
{
    std::vector<cv::Point2d> PathRight;  // 右側膠水路徑（最終輸出）
    std::vector<cv::Point2d> PathLeft;   // 左側膠水路徑（Y與右側完全相同）
};

struct ROIMask
{
    int MaskX;       // ROI left X
    int MaskY;       // ROI top Y
    int MaskWidth;   // ROI width
    int MaskHeight;  // ROI height
    int RefCenterX;  // center X for side split and mirror
    int RefCenterY;  // reserved center Y
    double EntryPointX; // entry X in image pixels; caller converts ini mm
};



//
//  SystemConfig
//


struct SystemConfig  //For AX-3 PLC , YUFA Machine
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

struct SystemConfigA  // For AX-3 PLC, SP Shoe Last Machine
{
    std::string IpAddress;
    int Port;
    int StationID;
    float OffsetValue;
    float EntryPointX;
    int CameraID;
    char MACKey[18];
    char GoldenKey[18];
    char HMI_ID[27];
    char PLC_ID[27];
    int CameraWidth;
    int CameraHeight;
    float TransferFactor;
    int ImageFlip;               // 0, 90, 180, others
    int ImageBinary;
    int CreateToolPath;
    int DispalyToolPath;    // TODO: fix typo → DisplayToolPath
    int DisplayROI;            // TODO: fix typo → DisplayROI
    int BinaryUpper;
    int BinaryLower;
    int Binary;
    int SaveINI;
    int MaskX;                 // ROI Mask X (TopX)
    int MaskY;                 // ROI Mask Y (TopY)
    int MaskWidth;          // ROI Mask Width
    int MaskHeight;         // ROI Mask Height
    int RefCenterX;
    int RefCenterY;
    std::string MachineType;
    // --- Newly Added ---
    int DisplayRefLine;                           // Tool Path
    int TabWork;                                     // ButtonTab: Working: 1 , SystemPara: 2, Modbus TCP: 4
    std::string CameraSerialNumber;     // Camera Serial Number
};

enum class SystemFunctionAddress : int
{
    Grab = 139,
    ImageBinary = 140,
    DisplayROI = 141,
    DisplayRefLine = 142,
    DiplayPath = 143,
    TabStatus = 144
};

struct SystemFunction
{
    static constexpr int StartAddress = 139;
    static constexpr int RegisterCount = 6;

    int Grab = 0;            // Register 139, value = 0 or 1
    int ImageBinary = 0;     // Register 140, reserved, value = 0 or 1
    int DisplayROI = 0;      // Register 141, value = 0 or 1
    int DisplayRefLine = 0;  // Register 142, value = 0 or 1
    int DiplayPath = 0;      // Register 143, reserved, value = 0 or 1
    int TabStatus = 0;       // Register 144: 0=Working, 2=SystemPara, 4=Modbus TCP
};

struct MemStruct_SP   //For SP Shoe Last Machine, IOT
{
    int CreateToolPath; // 保留欄位；Register 157 歸類於 SystemConfigA::CreateToolPath
    int RecipeID;
    int CurrentProduction;
    int Set_temperature0;
    int Temperature0;
    int Set_Temperature1;
    int Temperature1;
    int Set_temperature2;
    int Temperature2;
    int Servo_ALE0;
    int Servo_ALE1;
    int Servo_ALE2;
    int Servo_ALE3;
    int i_ProcessingTimeCount;
    int i_SystemTimeCount; // 新增系統時間計數欄位
    int MachineID; // 新增機器ID欄位
    int MachineModel; // 新增機器類型欄位

    // 替換 bool 為 uint8_t，確保內存對齊一致
    uint8_t Alm_tem_not_reach;
    uint8_t flag_AL_overload;
    uint8_t Alm_airPressureLow;
    uint8_t flag_AL_emergency;
    uint8_t flag_AL_midside_sensor;
    uint8_t Alm_ManualY_GoOut;
    uint8_t MachineStatus; // 新增機器狀態欄位 MachineStatus: false 0: Manul, true 1: Semi/Full
    uint8_t WorkingMode; // 新增工作模式欄位   false 0: Stanby, true 1: In Operation

    // float 放在結構體的最後，避免對齊問題
    float p19;
};

struct PLCData   //For AX-3 PLC, SP Shoe Last Machine
{
    int ShoeSize;
    int RecipeID;
    int ResistiveRuler1;
    int ResistiveRuler2;
    int ResistiveRuler3;
    int ResistiveRuler4;
    std::vector<cv::Point2d> X1;     // Tool path X1: the  tool path from image right
    std::vector<cv::Point2d> X2;     // Tool path X2: the  tool path from image left
    std::vector<cv::Point2d> Y;     // Tool path Y
    //std::vector<int> numClusters;      // 對應 Path 每個點的分群編號（依 contour 分群）
};

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

//struct YUFA
//{
//    // 0 or 1 data type
//    int type;
//    cv::Point2d Position; // Position of the template in the image
//};


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
