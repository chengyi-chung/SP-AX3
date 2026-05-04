// WorkTab.cpp: 實作檔案
#pragma once
#include "pch.h"
#include "SP.h"
#include "SPDlg.h"
#include "afxdialogex.h"
#include "WorkTab.h"
#include "ImagePro.h"
#include <algorithm>
#include <string>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <tuple>
#include <vector>
#include "UAX\\UAXVision.cpp" // bring UAXVision implementation into this module
//Add pylon header files to MFC project

#include <pylon/PylonIncludes.h>

#include <mutex>  

using namespace Pylon;

//static const uint32_t c_countOfImagesToGrab = 3;

using namespace std;

// Include files to use the pylon API.
#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif


// Namespace for using pylon objects.
using namespace Pylon;

// Namespace for using cout.
using namespace std;

// Number of images to be grabbed.
static const uint32_t c_countOfImagesToGrab = 3;

namespace {
std::string GetCalibrationFilePath()
{
    std::string appPath = GetAppPath();
    if (!appPath.empty() && appPath.back() != '\\' && appPath.back() != '/') {
        appPath += "\\";
    }
    return appPath + "calibration.yml";
}

std::string GetToolDebugExportPath(const std::string& fileName)
{
    std::string appPath = GetAppPath();
    if (!appPath.empty() && appPath.back() != '\\' && appPath.back() != '/') {
        appPath += "\\";
    }
    return appPath + fileName;
}

cv::Mat ApplyConfiguredFlip(const cv::Mat& image, int flipCode)
{
    if (image.empty()) {
        return image;
    }

    if (flipCode == 0 || flipCode == 1 || flipCode == -1) {
        cv::Mat flipped;
        cv::flip(image, flipped, flipCode);
        return flipped;
    }

    return image.clone();
}

// Apply the same flip transformation to a point, given the image size and flip code
// flipCode: 0 = vertical, 1 = horizontal, -1 = both
// The point is expected to be in pixel coordinates relative to the original image
// The flipped point will be returned in the same coordinate system (relative to the original image)
// If the image size is invalid, the original point will be returned unchanged
// Note: This function does not handle rotation, only flipping. Rotation would require a different transformation.
// Example usage:
// cv::Point2d originalPt(100, 50);
// cv::Size imageSize(200, 100);
// int flipCode = 1; // horizontal flip
// cv::Point2d flippedPt = ApplyConfiguredFlipToPoint(originalPt, imageSize, flipCode);
cv::Point2d ApplyConfiguredFlipToPoint(const cv::Point2d& pt, const cv::Size& imageSize, int flipCode)
{
    if (imageSize.width <= 0 || imageSize.height <= 0) {
        return pt;
    }

    cv::Point2d flipped = pt;
    switch (flipCode) {
    case 0:
        flipped.y = (imageSize.height - 1) - pt.y;
        break;
    case 1:
        flipped.x = (imageSize.width - 1) - pt.x;
        break;
    case -1:
        flipped.x = (imageSize.width - 1) - pt.x;
        flipped.y = (imageSize.height - 1) - pt.y;
        break;
    default:
        break;
    }
    return flipped;
}

cv::Point2d RotatePoint180(const cv::Point2d& pt, const cv::Size& imageSize)
{
    if (imageSize.width <= 0 || imageSize.height <= 0) {
        return pt;
    }

    return cv::Point2d(
        (imageSize.width - 1) - pt.x,
        (imageSize.height - 1) - pt.y
    );
}

void ApplyConfiguredFlipToToolPath(ToolPath& toolPath, const cv::Size& imageSize, int flipCode)
{
    if (!(flipCode == 0 || flipCode == 1 || flipCode == -1)) {
        return;
    }

    for (auto& pt : toolPath.Path) {
        pt = ApplyConfiguredFlipToPoint(pt, imageSize, flipCode);
    }
}

void ApplyConfiguredFlipToGluePath(GluePath& gluePath, const cv::Size& imageSize, int flipCode)
{
    if (!(flipCode == 0 || flipCode == 1 || flipCode == -1)) {
        return;
    }

    for (auto& pt : gluePath.PathLeft) {
        pt = ApplyConfiguredFlipToPoint(pt, imageSize, flipCode);
    }

    for (auto& pt : gluePath.PathRight) {
        pt = ApplyConfiguredFlipToPoint(pt, imageSize, flipCode);
    }
}

void RotateGluePath180(GluePath& gluePath, const cv::Size& imageSize)
{
    for (auto& pt : gluePath.PathLeft) {
        pt = RotatePoint180(pt, imageSize);
    }
    for (auto& pt : gluePath.PathRight) {
        pt = RotatePoint180(pt, imageSize);
    }
}

// Sort the glue path points by ascending Y coordinate (top to bottom)
void SortGluePathByAscendingY(GluePath& gluePath)
{
    const size_t count = (std::min)(gluePath.PathRight.size(), gluePath.PathLeft.size());
    if (count == 0) {
        return;
    }

    std::vector<std::tuple<double, cv::Point2d, cv::Point2d>> rows;
    rows.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        const double y = (gluePath.PathRight[i].y + gluePath.PathLeft[i].y) * 0.5;
        rows.emplace_back(y, gluePath.PathRight[i], gluePath.PathLeft[i]);
    }

    std::sort(rows.begin(), rows.end(),
        [](const auto& a, const auto& b) {
            return std::get<0>(a) < std::get<0>(b);
        });

    for (size_t i = 0; i < count; ++i) {
        gluePath.PathRight[i] = std::get<1>(rows[i]);
        gluePath.PathLeft[i] = std::get<2>(rows[i]);
    }
}

bool IsPointInsideRoi(const cv::Point2d& pt, const cv::Rect& roiRect)
{
    return pt.x >= roiRect.x &&
        pt.x < (roiRect.x + roiRect.width) &&
        pt.y >= roiRect.y &&
        pt.y < (roiRect.y + roiRect.height);
}

void FilterGluePathByRoiAndLimit(GluePath& gluePath, const cv::Rect& roiRect, size_t maxPoints)
{
    const size_t count = (std::min)(gluePath.PathRight.size(), gluePath.PathLeft.size());
    std::vector<cv::Point2d> filteredRight;
    std::vector<cv::Point2d> filteredLeft;
    filteredRight.reserve((std::min)(count, maxPoints));
    filteredLeft.reserve((std::min)(count, maxPoints));

    for (size_t i = 0; i < count; ++i) {
        const cv::Point2d& rightPt = gluePath.PathRight[i];
        const cv::Point2d& leftPt = gluePath.PathLeft[i];

        if (!IsPointInsideRoi(rightPt, roiRect) || !IsPointInsideRoi(leftPt, roiRect)) {
            continue;
        }

        filteredRight.push_back(rightPt);
        filteredLeft.push_back(leftPt);

        if (filteredRight.size() >= maxPoints) {
            break;
        }
    }

    gluePath.PathRight = std::move(filteredRight);
    gluePath.PathLeft = std::move(filteredLeft);
}

void ExportGluePathAsToolCSV(const GluePath& gluePath, const std::string& fileName, const char* unit = "")
{
    const size_t count = (std::min)(gluePath.PathRight.size(), gluePath.PathLeft.size());
    std::ofstream out(GetToolDebugExportPath(fileName), std::ios::trunc);
    if (!out.is_open()) {
        return;
    }

    out << "// Auto-generated debug export\n";
    out << "// Format: Tool(X1,Y,X2)\n";
    if (unit && unit[0] != '\0') {
        out << "// Unit: " << unit << "\n";
    }
    out << "Index,X1,Y,X2\n";
    out << std::fixed << std::setprecision(3);

    for (size_t i = 0; i < count; ++i) {
        out << i << ","
            << gluePath.PathRight[i].x << ","
            << gluePath.PathRight[i].y << ","
            << gluePath.PathLeft[i].x << "\n";
    }
}

void ExportToolPathAsCSV(const ToolPath& toolPath, const std::string& fileName)
{
    std::ofstream out(GetToolDebugExportPath(fileName), std::ios::trunc);
    if (!out.is_open()) {
        return;
    }

    out << "// Auto-generated debug export\n";
    out << "// Format: RawToolPath(X,Y,Cluster)\n";
    out << "Index,X,Y,Cluster\n";

    for (size_t i = 0; i < toolPath.Path.size(); ++i) {
        const int cluster =
            (i < toolPath.numClusters.size())
            ? toolPath.numClusters[i]
            : -1;

        out << i << ","
            << toolPath.Path[i].x << ","
            << toolPath.Path[i].y << ","
            << cluster << "\n";
    }
}

class GridLengthInputDialog : public CDialogEx
{
public:
    explicit GridLengthInputDialog(double defaultLengthMm, CWnd* pParent = nullptr)
        : CDialogEx(IDD_DLG_IMAGE_PRO, pParent), m_lengthMm(defaultLengthMm) {}

    double GetLengthMm() const { return m_lengthMm; }

protected:
    BOOL OnInitDialog() override
    {
        CDialogEx::OnInitDialog();
        SetWindowTextW(L"Grid Length");

        CWnd* okButton = GetDlgItem(IDOK);
        CWnd* cancelButton = GetDlgItem(IDCANCEL);
        if (okButton) okButton->SetWindowTextW(L"確定");
        if (cancelButton) cancelButton->SetWindowTextW(L"取消");

        m_staticPrompt.Create(
            L"請輸入單一格點邊長(mm)：",
            WS_CHILD | WS_VISIBLE,
            CRect(16, 20, 180, 40),
            this);

        m_editLength.Create(
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            CRect(16, 48, 140, 68),
            this,
            20001);

        CString text;
        text.Format(L"%.3f", m_lengthMm);
        m_editLength.SetWindowTextW(text);
        m_editLength.SetFocus();
        m_editLength.SetSel(0, -1);
        return FALSE;
    }

    void OnOK() override
    {
        CString text;
        m_editLength.GetWindowTextW(text);
        text.Trim();
        if (text.IsEmpty()) {
            AfxMessageBox(L"請輸入格點邊長(mm)。");
            return;
        }

        wchar_t* endPtr = nullptr;
        const double value = wcstod(text, &endPtr);
        if (endPtr == text.GetString() || value <= 0.0) {
            AfxMessageBox(L"格點邊長必須是大於 0 的數值。");
            return;
        }

        m_lengthMm = value;
        CDialogEx::OnOK();
    }

private:
    double m_lengthMm = 25.0;
    CStatic m_staticPrompt;
    CEdit m_editLength;
};

}


///// OpenCV zoon
// Global variables for mouse callback function
cv::Point startPoint(-1, -1);
cv::Point endPoint(-1, -1);
bool drawingRectangle = false;
cv::Mat imageROI;

void mouseCallback(int event, int x, int y, int flags, void* userdata)
{
    cv::Mat& img = *(cv::Mat*)userdata;

    if (event == cv::EVENT_LBUTTONDOWN)
    {
        startPoint = cv::Point(x, y);
        endPoint = cv::Point(x, y);
        drawingRectangle = true;
    }
    else if (event == cv::EVENT_MOUSEMOVE && drawingRectangle)
    {
        endPoint = cv::Point(x, y);
        img.copyTo(imageROI);
        cv::rectangle(imageROI, startPoint, endPoint, cv::Scalar(255,255,255), 2); 
        cv::imshow("Image", imageROI);
    }
    else if (event == cv::EVENT_LBUTTONUP)
    {
        drawingRectangle = false;
        endPoint = cv::Point(x, y);
        if (startPoint.x > endPoint.x) std::swap(startPoint.x, endPoint.x);
        if (startPoint.y > endPoint.y) std::swap(startPoint.y, endPoint.y);

        cv::Rect roi(startPoint, endPoint);
        if (roi.width > 0 && roi.height > 0)
        {
            imageROI = img(roi);
            cv::imshow("ROI", imageROI);
        }
    }
}

void WorkTab::OnDestroy()
{
    // 在窗口被銷毀前先停止計時器，避免 HWND 已為 NULL 時 KillTimer 觸發斷點
    StopHmiSyncTimer();
    CDialogEx::OnDestroy();
}

cv::Mat showImageAndReturnROI(cv::Mat& m_mat, int screenHeight, int screenWidth)
{
    if (m_mat.empty())
    {
        MessageBox(NULL, _T("No image to display."), _T("Error"), MB_ICONERROR);
        return cv::Mat();
    }

    // Resize the image to fit the screen
    cv::Mat dstImage = m_mat.clone();

    // Resize the image to fit the screen if the image is larger than the screen
    if (m_mat.cols > screenWidth || m_mat.rows > screenHeight)
    {
        double scaleFactor = (std::min)((double)screenWidth / m_mat.cols, (double)screenHeight / m_mat.rows);
        cv::resize(m_mat, dstImage, cv::Size(), scaleFactor, scaleFactor);
    }

    // Create a window and display the image
    cv::namedWindow("Image", cv::WINDOW_NORMAL);
    cv::imshow("Image", dstImage);

    // Set mouse callback function for the window
    cv::setMouseCallback("Image", mouseCallback, (void*)&dstImage);

    // Wait for a key press
    cv::waitKey(0);

    return imageROI;
}


/// OpenCV zoon

// WorkTab 對話方塊

IMPLEMENT_DYNAMIC(WorkTab, CDialogEx)

WorkTab::WorkTab(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TAB_WOK, pParent)
{

}

WorkTab::~WorkTab()
{
    StopHmiSyncTimer();
    PylonTerminate();
}

void WorkTab::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WORK_GRAB, m_Work_Grab);
	DDX_Control(pDX, IDC_WORK_STOP_GRAB, m_Work_StopGrab);
	DDX_Control(pDX, IDC_WORK_TEMP_IMG, m_Work_TempImg);
	DDX_Control(pDX, IDC_WORK_MATCH_TEMP, m_Work_MatchTemp);
	DDX_Control(pDX, IDC_IDC_WORK_TOOL_PATH, m_Work_ToolPath);
	DDX_Control(pDX, IDC_IDC_WORK_LOAD_IMG, m_Work_LoadImg);
	DDX_Control(pDX, IDC_IDC_WORK_SAVE_IMG, m_Work_SaveImg);
	DDX_Control(pDX, IDC_IDC_WORK_GO, m_Work_Go);
	//DDX_Control(pDX, IDC_BTN_CALIBRATION, m_Btn_Calibration);
}
         
BEGIN_MESSAGE_MAP(WorkTab, CDialogEx)
	ON_BN_CLICKED(IDC_WORK_GRAB, &WorkTab::OnBnClickedWorkGrab)
    ON_WM_CTLCOLOR()
    ON_WM_PAINT()
    ON_WM_DESTROY()
    ON_WM_TIMER()
    ON_BN_CLICKED(IDC_WORK_STOP_GRAB, &WorkTab::OnBnClickedWorkStopGrab)
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_SETCURSOR()
    ON_BN_CLICKED(IDC_WORK_TEMP_IMG, &WorkTab::OnBnClickedWorkTempImg)
    ON_BN_CLICKED(IDC_WORK_MATCH_TEMP, &WorkTab::OnBnClickedWorkMatchTemp)
    ON_BN_CLICKED(IDC_IDC_WORK_TOOL_PATH, &WorkTab::OnBnClickedIdcWorkToolPath)
    ON_BN_CLICKED(IDC_IDC_WORK_LOAD_IMG, &WorkTab::OnBnClickedIdcWorkLoadImg)
    ON_BN_CLICKED(IDC_IDC_WORK_SAVE_IMG, &WorkTab::OnBnClickedIdcWorkSaveImg)
    ON_BN_CLICKED(IDC_IDC_WORK_GO, &WorkTab::OnBnClickedIdcWorkGo)
    ON_BN_CLICKED(IDC_CHECK_WORK_CENTER, &WorkTab::OnBnClickedCheckWorkCenter)
    ON_BN_CLICKED(IDC_MFCBTN_WORK_IMG_PROCESS, &WorkTab::OnBnClickedWorkImageProcess) // ← 新增
    ON_BN_CLICKED(IDC_MFCBTN_WORK_IMG_Calibrate, &WorkTab::OnBnClickedMfcbtnWorkImgCalibrate)
    ON_BN_CLICKED(IDC_CHECK_WORK_ROI, &WorkTab::OnBnClickedCheckWorkRoi)
    ON_BN_CLICKED(IDC_MFCBTN_WORK_IMG_FACTOR, &WorkTab::OnBnClickedMfcbtnWorkImgFactor)
END_MESSAGE_MAP()


// WorkTab 訊息處理常式

// OnInitDialog 內初始化按鈕、字型與顏色
BOOL WorkTab::OnInitDialog()
{
	CDialogEx::OnInitDialog();
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    SetIcon(m_hIcon, TRUE);			// 設定大圖示
    SetIcon(m_hIcon, FALSE);		// 設定小圖示

    //Picture Control IDC_PICCTL_DISPLAY
    pWnd = GetDlgItem(IDC_PICCTL_DISPLAY); // 假设你的Picture Control控件的ID是IDC_PICTURE_CONTROL。
    pDC = nullptr;


    // 建立字型 (高度 20, 粗體)
    m_fontBoldBig.CreateFont(
        15,                        // 高度 (字體大小)
        0,                         // 寬度 (0 = 自動)
        0,                         // 角度
        0,                         // 基線角度
        FW_BOLD,                   // 粗體
        FALSE,                     // 斜體
        FALSE,                     // 底線
        0,                         // StrikeOut
        ANSI_CHARSET,              // 字元集
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_SWISS,
        _T("Arial"));    // 字型名稱 (可改 "Arial", "Tahoma" 等)



    //初始化 m_bGrabThread
    m_bGrabThread = false;

	//Change mouse cursor to cross
	HCURSOR hCursor = AfxGetApp()->LoadStandardCursor(IDC_CROSS);
	SetCursor(hCursor);

    //Get Picture Control IDC_PICCTL_DISPLAY 大小
    CRect rect;
    GetDlgItem(IDC_PICCTL_DISPLAY)->GetClientRect(&rect);
    int Width = rect.Width();
    int Height = rect.Height();
   
    m_bGrabThread = false;

    float ret = Add(1.1f, 2.2f);

	flgCenter = false;

    PylonInitialize();

	CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(GetParent()->GetParent());

	imgFlip = pParentWnd->m_SystemPara.ImageFlip;
    m_hmiSyncIntervalMs = 500;
    m_lastSyncedSystemPara = pParentWnd->m_SystemPara;
    m_lastSyncedMemStruct = pParentWnd->m_MemStruct_SP;
    UpdateModbusSyncState(pParentWnd != nullptr && pParentWnd->m_modbusCtx != nullptr);


    // 設定按鈕底色與文字顏色
	//宣告RGB顏色變

	//Grab按鈕
    m_Work_Grab.SetFaceColor(RGB(194, 194, 194));      
    m_Work_Grab.SetTextColor(RGB(0, 0, 0));    //黑色字
	//設定按鈕字型與大小
	m_Work_Grab.SetFont(&m_fontBoldBig);
   
	//Stop Grab按鈕
    m_Work_StopGrab.SetFaceColor(RGB(194, 194, 194));    
    m_Work_StopGrab.SetTextColor(RGB(0, 0, 0));    //黑色字
    m_Work_StopGrab.SetFont(&m_fontBoldBig);
	//Temp Img按鈕
	m_Work_TempImg.SetFaceColor(RGB(194, 194, 194));      
	m_Work_TempImg.SetTextColor(RGB(0, 0, 0));    //黑色字
    m_Work_TempImg.SetFont(&m_fontBoldBig);
    
	//Match Temp按鈕
	m_Work_MatchTemp.SetFaceColor(RGB(194, 194, 194));     
	m_Work_MatchTemp.SetTextColor(RGB(0, 0, 0));    //黑色字
	m_Work_MatchTemp.SetFont(&m_fontBoldBig);

	//Tool Path按鈕
	m_Work_ToolPath.SetFaceColor(RGB(212, 255, 179));      
	m_Work_ToolPath.SetTextColor(RGB(0, 0, 0));    //黑色字
	m_Work_ToolPath.SetFont(&m_fontBoldBig);
    //Go按鈕
    m_Work_Go.SetFaceColor(RGB(212, 255, 179));      // 灰色底
    m_Work_Go.SetTextColor(RGB(0, 0, 0));    //黑色字
	m_Work_Go.SetFont(&m_fontBoldBig);

	//Load Img按鈕
	m_Work_LoadImg.SetFaceColor(RGB(200, 228, 255));      
	m_Work_LoadImg.SetTextColor(RGB(0, 0, 0));    //黑色字
	m_Work_LoadImg.SetFont(&m_fontBoldBig);
	//Save Img按鈕
	m_Work_SaveImg.SetFaceColor(RGB(200, 228, 255));
	m_Work_SaveImg.SetTextColor(RGB(0, 0, 0));    //黑色字
	m_Work_SaveImg.SetFont(&m_fontBoldBig);

	//Calibration按鈕
	//m_Btn_Calibration.SetFaceColor(RGB(255, 212, 253));      
	//m_Btn_Calibration.SetTextColor(RGB(0, 0, 0));    //黑色字
	//m_Btn_Calibration.SetFont(&m_fontBoldBig);

	//Example按鈕
	//設定按鈕字型與大小

	MaskX = pParentWnd->m_SystemPara.MaskX;
	MaskY = pParentWnd->m_SystemPara.MaskY;
	MaskWidth = pParentWnd->m_SystemPara.MaskWidth;
	MaskHeight = pParentWnd->m_SystemPara.MaskHeight;
	referenceX = pParentWnd->m_SystemPara.RefCenterX;
	referenceY = pParentWnd->m_SystemPara.RefCenterY;

    // 嘗試載入既有校正資料；若檔案不存在則維持未校正狀態。
    m_vision.loadCalibrationData(GetCalibrationFilePath());
   

	// 預設check box 為勾選狀態
	CheckDlgButton(IDC_CHECK_WORK_CENTER, BST_CHECKED);
	CheckDlgButton(IDC_CHECK_WORK_ROI, BST_CHECKED);
    m_bROIMode = (IsDlgButtonChecked(IDC_CHECK_WORK_ROI) == BST_CHECKED);
    flgCenter = (IsDlgButtonChecked(IDC_CHECK_WORK_CENTER) == BST_CHECKED);
    return TRUE;

}

HBRUSH WorkTab::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

    // 如果是我們的按鈕
   // if (pWnd->GetDlgCtrlID() == IDC_WORK_GRAB)
    {
        //pDC->SetTextColor(RGB(0, 0, 255));
       // pDC->SetBkColor(RGB(255, 255, 0));

        // 使用自訂字型
        //pDC->SelectObject(&m_fontBoldBig);

       // if (m_brush.GetSafeHandle() == NULL)
       //     m_brush.CreateSolidBrush(RGB(255, 0, 0));

       // return (HBRUSH)m_brush;
    }

    // 否則，回傳預設的筆刷
   return hbr;
}

void WorkTab::StartHmiSyncTimer()
{
    if (!m_hmiSyncEnabled && m_hWnd) {
        SetTimer(kHmiSyncTimerId, m_hmiSyncIntervalMs, nullptr);
        m_hmiSyncEnabled = true;
    }
}

void WorkTab::StopHmiSyncTimer()
{
    // 安全停用：確保視窗尚有有效 HWND 才呼叫 KillTimer
    if (m_hmiSyncEnabled && m_hWnd) {
        KillTimer(kHmiSyncTimerId);
    }
    m_hmiSyncEnabled = false;
}

void WorkTab::UpdateModbusSyncState(bool connected)
{
    if (!m_hWnd) {
        return;
    }

    if (connected) {
        StartHmiSyncTimer();
    }
    else {
        StopHmiSyncTimer();
        m_hasSystemSyncBaseline = false;
        m_hasMemSyncBaseline = false;
    }
}

bool WorkTab::ReadHoldingRegistersBlock(int startAddress, int count, std::vector<uint16_t>& outValues, int stationID)
{
    CSPDlg* pParent = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
    if (!pParent) {
        return false;
    }

    if (!pParent->m_modbusCtx) {
        return false;
    }

    std::lock_guard<std::mutex> lock(pParent->m_modbusMutex);
    modbus_set_slave(pParent->m_modbusCtx, stationID);

    outValues.assign(count, 0);
    return modbus_read_registers(pParent->m_modbusCtx, startAddress, count, outValues.data()) != -1;
}

bool WorkTab::WriteHoldingRegistersBlock(int startAddress, const std::vector<uint16_t>& values, int stationID)
{
    if (values.empty()) {
        return true;
    }

    CSPDlg* pParent = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
    if (!pParent) {
        return false;
    }

    if (!pParent->m_modbusCtx) {
        return false;
    }

    std::lock_guard<std::mutex> lock(pParent->m_modbusMutex);
    modbus_set_slave(pParent->m_modbusCtx, stationID);
    return modbus_write_registers(pParent->m_modbusCtx, startAddress, static_cast<int>(values.size()), values.data()) != -1;
}

void WorkTab::BuildSystemConfigRegisters(const SystemConfigA& src, std::vector<uint16_t>& outValues) const
{
    outValues.assign(18, 0);
    outValues[0] = static_cast<uint16_t>(src.ImageBinary);
    outValues[1] = static_cast<uint16_t>(src.CreateToolPath);
    outValues[2] = static_cast<uint16_t>(src.DispalyToolPath);
    outValues[3] = static_cast<uint16_t>(src.DisplayROI);
    outValues[4] = static_cast<uint16_t>(src.DisplayRefLine);
    outValues[5] = static_cast<uint16_t>(src.TabWork);
    outValues[6] = static_cast<uint16_t>(std::lround(src.OffsetValue));
    outValues[7] = static_cast<uint16_t>(src.BinaryUpper);
    outValues[8] = static_cast<uint16_t>(src.BinaryLower);
    outValues[9] = static_cast<uint16_t>(src.MaskX);
    outValues[10] = static_cast<uint16_t>(src.MaskY);
    outValues[11] = static_cast<uint16_t>(src.MaskWidth);
    outValues[12] = static_cast<uint16_t>(src.MaskHeight);
    outValues[13] = static_cast<uint16_t>(src.StationID);
    outValues[14] = static_cast<uint16_t>(src.CameraID);
    outValues[15] = static_cast<uint16_t>(src.RefCenterX);
    outValues[16] = static_cast<uint16_t>(src.RefCenterY);
    outValues[17] = static_cast<uint16_t>(src.ImageFlip);
}

void WorkTab::ApplySystemConfigRegisters(const std::vector<uint16_t>& values, SystemConfigA& dst) const
{
    if (values.size() < 18) {
        return;
    }

    dst.ImageBinary = values[0];
    dst.CreateToolPath = values[1];
    dst.DispalyToolPath = values[2];
    dst.DisplayROI = values[3];
    dst.DisplayRefLine = values[4];
    dst.TabWork = values[5];
    dst.OffsetValue = static_cast<float>(values[6]);
    dst.BinaryUpper = values[7];
    dst.BinaryLower = values[8];
    dst.MaskX = values[9];
    dst.MaskY = values[10];
    dst.MaskWidth = values[11];
    dst.MaskHeight = values[12];
    dst.StationID = values[13];
    dst.CameraID = values[14];
    dst.RefCenterX = values[15];
    dst.RefCenterY = values[16];
    dst.ImageFlip = static_cast<short>(values[17]);
}

void WorkTab::BuildMemStructRegisters(const MemStruct_SP& src, std::vector<uint16_t>& outValues) const
{
    outValues.assign(27, 0);  // 增加到 27 個寄存器
    outValues[0] = static_cast<uint16_t>(src.CreateToolPath);  // Register 157
    outValues[1] = static_cast<uint16_t>(src.RecipeID);
    outValues[2] = static_cast<uint16_t>(src.CurrentProduction);
    outValues[3] = static_cast<uint16_t>(src.Set_temperature0);
    outValues[4] = static_cast<uint16_t>(src.Temperature0);
    outValues[5] = static_cast<uint16_t>(src.Set_Temperature1);
    outValues[6] = static_cast<uint16_t>(src.Temperature1);
    outValues[7] = static_cast<uint16_t>(src.Set_temperature2);
    outValues[8] = static_cast<uint16_t>(src.Temperature2);
    outValues[9] = static_cast<uint16_t>(src.Servo_ALE0);
    outValues[10] = static_cast<uint16_t>(src.Servo_ALE1);
    outValues[11] = static_cast<uint16_t>(src.Servo_ALE2);
    outValues[12] = static_cast<uint16_t>(src.Servo_ALE3);
    outValues[13] = static_cast<uint16_t>(src.i_ProcessingTimeCount);
    outValues[14] = static_cast<uint16_t>(src.i_SystemTimeCount);
    outValues[15] = static_cast<uint16_t>(src.MachineID);
    outValues[16] = static_cast<uint16_t>(src.MachineModel);
    outValues[17] = src.Alm_tem_not_reach;
    outValues[18] = src.flag_AL_overload;
    outValues[19] = src.Alm_airPressureLow;
    outValues[20] = src.flag_AL_emergency;
    outValues[21] = src.flag_AL_midside_sensor;
    outValues[22] = src.Alm_ManualY_GoOut;
    outValues[23] = src.MachineStatus;
    outValues[24] = src.WorkingMode;

    union FloatBits {
        float f;
        uint32_t u;
    } value{};
    value.f = src.p19;
    outValues[25] = static_cast<uint16_t>((value.u >> 16) & 0xFFFF);
    outValues[26] = static_cast<uint16_t>(value.u & 0xFFFF);
}

void WorkTab::ApplyMemStructRegisters(const std::vector<uint16_t>& values, MemStruct_SP& dst) const
{
    if (values.size() < 27) {  // 增加到 27 個寄存器
        return;
    }

    dst.CreateToolPath = values[0];  // Register 157 的值寫入 CreateToolPath
    dst.RecipeID = values[1];
    dst.CurrentProduction = values[2];
    dst.Set_temperature0 = values[3];
    dst.Temperature0 = values[4];
    dst.Set_Temperature1 = values[5];
    dst.Temperature1 = values[6];
    dst.Set_temperature2 = values[7];
    dst.Temperature2 = values[8];
    dst.Servo_ALE0 = values[9];
    dst.Servo_ALE1 = values[10];
    dst.Servo_ALE2 = values[11];
    dst.Servo_ALE3 = values[12];
    dst.i_ProcessingTimeCount = values[13];
    dst.i_SystemTimeCount = values[14];
    dst.MachineID = values[15];
    dst.MachineModel = values[16];
    dst.Alm_tem_not_reach = static_cast<uint8_t>(values[17]);
    dst.flag_AL_overload = static_cast<uint8_t>(values[18]);
    dst.Alm_airPressureLow = static_cast<uint8_t>(values[19]);
    dst.flag_AL_emergency = static_cast<uint8_t>(values[20]);
    dst.flag_AL_midside_sensor = static_cast<uint8_t>(values[21]);
    dst.Alm_ManualY_GoOut = static_cast<uint8_t>(values[22]);
    dst.MachineStatus = static_cast<uint8_t>(values[23]);
    dst.WorkingMode = static_cast<uint8_t>(values[24]);

    union FloatBits {
        float f;
        uint32_t u;
    } value{};
    value.u = (static_cast<uint32_t>(values[25]) << 16) | values[26];
    dst.p19 = value.f;
}

bool WorkTab::IsSystemConfigEqual(const SystemConfigA& lhs, const SystemConfigA& rhs) const
{
    return lhs.IpAddress == rhs.IpAddress &&
        lhs.Port == rhs.Port &&
        lhs.StationID == rhs.StationID &&
        lhs.OffsetValue == rhs.OffsetValue &&
        lhs.CameraID == rhs.CameraID &&
        lhs.CameraWidth == rhs.CameraWidth &&
        lhs.CameraHeight == rhs.CameraHeight &&
        lhs.TransferFactor == rhs.TransferFactor &&
        lhs.ImageFlip == rhs.ImageFlip &&
        lhs.ImageBinary == rhs.ImageBinary &&
        lhs.CreateToolPath == rhs.CreateToolPath &&
        lhs.DispalyToolPath == rhs.DispalyToolPath &&
        lhs.DisplayROI == rhs.DisplayROI &&
        lhs.BinaryUpper == rhs.BinaryUpper &&
        lhs.BinaryLower == rhs.BinaryLower &&
        lhs.MaskX == rhs.MaskX &&
        lhs.MaskY == rhs.MaskY &&
        lhs.MaskWidth == rhs.MaskWidth &&
        lhs.MaskHeight == rhs.MaskHeight &&
        lhs.RefCenterX == rhs.RefCenterX &&
        lhs.RefCenterY == rhs.RefCenterY &&
        lhs.MachineType == rhs.MachineType &&
        lhs.DisplayRefLine == rhs.DisplayRefLine &&
        lhs.TabWork == rhs.TabWork &&
        lhs.CameraSerialNumber == rhs.CameraSerialNumber;
}

bool WorkTab::IsSystemConfigDisplayDataValid(const SystemConfigA& value) const
{
    if (value.MaskWidth <= 0 || value.MaskHeight <= 0) {
        return false;
    }

    if (value.MaskX < 0 || value.MaskY < 0) {
        return false;
    }

    if (!m_mat.empty()) {
        if (value.MaskX + value.MaskWidth > m_mat.cols) {
            return false;
        }
        if (value.MaskY + value.MaskHeight > m_mat.rows) {
            return false;
        }
        if (value.RefCenterX < 0 || value.RefCenterX >= m_mat.cols) {
            return false;
        }
        if (value.RefCenterY < 0 || value.RefCenterY >= m_mat.rows) {
            return false;
        }
    }

    return true;
}

bool WorkTab::IsMemStructEqual(const MemStruct_SP& lhs, const MemStruct_SP& rhs) const
{
    return lhs.CreateToolPath == rhs.CreateToolPath &&
        lhs.RecipeID == rhs.RecipeID &&
        lhs.CurrentProduction == rhs.CurrentProduction &&
        lhs.Set_temperature0 == rhs.Set_temperature0 &&
        lhs.Temperature0 == rhs.Temperature0 &&
        lhs.Set_Temperature1 == rhs.Set_Temperature1 &&
        lhs.Temperature1 == rhs.Temperature1 &&
        lhs.Set_temperature2 == rhs.Set_temperature2 &&
        lhs.Temperature2 == rhs.Temperature2 &&
        lhs.Servo_ALE0 == rhs.Servo_ALE0 &&
        lhs.Servo_ALE1 == rhs.Servo_ALE1 &&
        lhs.Servo_ALE2 == rhs.Servo_ALE2 &&
        lhs.Servo_ALE3 == rhs.Servo_ALE3 &&
        lhs.i_ProcessingTimeCount == rhs.i_ProcessingTimeCount &&
        lhs.i_SystemTimeCount == rhs.i_SystemTimeCount &&
        lhs.MachineID == rhs.MachineID &&
        lhs.MachineModel == rhs.MachineModel &&
        lhs.Alm_tem_not_reach == rhs.Alm_tem_not_reach &&
        lhs.flag_AL_overload == rhs.flag_AL_overload &&
        lhs.Alm_airPressureLow == rhs.Alm_airPressureLow &&
        lhs.flag_AL_emergency == rhs.flag_AL_emergency &&
        lhs.flag_AL_midside_sensor == rhs.flag_AL_midside_sensor &&
        lhs.Alm_ManualY_GoOut == rhs.Alm_ManualY_GoOut &&
        lhs.MachineStatus == rhs.MachineStatus &&
        lhs.WorkingMode == rhs.WorkingMode &&
        lhs.p19 == rhs.p19;
}

void WorkTab::SyncHmiData(int stationID)
{
    if (m_hmiSyncBusy) {
        return;
    }

    CSPDlg* pParent = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
    if (!pParent || !pParent->m_modbusCtx) {
        UpdateModbusSyncState(false);
        return;
    }

    m_hmiSyncBusy = true;

    constexpr int kSystemConfigStart = 139;
    constexpr int kMemStructStart = 157;   // Assumption: contiguous HMI block after 139~156

    std::vector<uint16_t> regs;
    SystemConfigA remoteSystem = pParent->m_SystemPara;
    if (ReadHoldingRegistersBlock(kSystemConfigStart, 18, regs, stationID)) {
        ApplySystemConfigRegisters(regs, remoteSystem);
        if (IsSystemConfigDisplayDataValid(remoteSystem)) {
            if (!IsSystemConfigEqual(remoteSystem, m_lastSyncedSystemPara)) {
                pParent->m_SystemPara = remoteSystem;
                MaskX = pParent->m_SystemPara.MaskX;
                MaskY = pParent->m_SystemPara.MaskY;
                MaskWidth = pParent->m_SystemPara.MaskWidth;
                MaskHeight = pParent->m_SystemPara.MaskHeight;
                referenceX = pParent->m_SystemPara.RefCenterX;
                referenceY = pParent->m_SystemPara.RefCenterY;
                if (!m_bFactorSelectMode && (m_bROIMode || flgCenter)) {
                    ShowImageOnPictureControl(flgCenter, cv::Scalar(0, 0, 255, 255), 1, CrossStyle::Solid);
                }
            }
            m_lastSyncedSystemPara = remoteSystem;
            m_hasSystemSyncBaseline = true;
        }
    }

    MemStruct_SP remoteMem = pParent->m_MemStruct_SP;
    if (ReadHoldingRegistersBlock(kMemStructStart, 26, regs, stationID)) {
        ApplyMemStructRegisters(regs, remoteMem);
        if (!IsMemStructEqual(remoteMem, m_lastSyncedMemStruct)) {
            pParent->m_MemStruct_SP = remoteMem;
            m_lastSyncedMemStruct = remoteMem;
        }
        m_lastSyncedMemStruct = remoteMem;
        m_hasMemSyncBaseline = true;
    }

    m_hmiSyncBusy = false;
}

void WorkTab::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == kHmiSyncTimerId) {
        CSPDlg* pParent = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
        if (pParent) {
            SyncHmiData(pParent->m_SystemPara.StationID);
        }
        return;
    }

    CDialogEx::OnTimer(nIDEvent);
}


void WorkTab::OnBnClickedWorkGrab()
{
	// TODO: 在此加入控制項告知處理常式程式碼
    // If the grab thread is not running, start it
  
    if (!m_bGrabThread)
	{
		m_bGrabThread = true;
	}
	else
	{
		return;
	}
    //Call the multi-threaded grabber
    AfxBeginThread(GrabThread, this);

    //Display  m_mat with cv::imshow

    //cv::imshow("OpenCV Image", m_mat);
   
}

// Add a multi-treaded grabber with Basler Pylon
UINT WorkTab::GrabThread(LPVOID pParam)
{
    //CMyDialog* pDialog = static_cast<CMyDialog*>(pParam);
    WorkTab* pWorkTab = static_cast<WorkTab*>(pParam);

    // The exit code of the sample application
    int exitCode = 0;

    //PylonInitialize();

    try
    {
        // Create an instant camera object with the camera device found first.
        CInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice());

        // Print the model name of the camera.
        cout << "Using device " << camera.GetDeviceInfo().GetModelName() << endl;

        // The parameter MaxNumBuffer can be used to control the count of buffers
        // allocated for grabbing. The default value of this parameter is 10.
        camera.MaxNumBuffer = 1;

        // Start the grabbing of c_countOfImagesToGrab images.
        // The camera device is parameterized with a default configuration which
        // sets up free-running continuous acquisition.
        // 
        //camera.StartGrabbing(c_countOfImagesToGrab);

        //camera grab continue not stop
        camera.StartGrabbing(GrabStrategy_LatestImageOnly);

        // This smart pointer will receive the grab result data.
        CGrabResultPtr ptrGrabResult;

        // Camera.StopGrabbing() is called automatically by the RetrieveResult() method
        // when c_countOfImagesToGrab images have been retrieved.
        while (camera.IsGrabbing())
        {
            // Wait for an image and then retrieve it. A timeout of 5000 ms is used.
            camera.RetrieveResult(1000, ptrGrabResult, TimeoutHandling_ThrowException);

            // Image grabbed successfully?
            if (ptrGrabResult->GrabSucceeded())
            {
                // if the grab thread is not running, exit the thread
                if (!pWorkTab->m_bGrabThread)
				{
					break;
				}
                // Access the image data.
                //cout << "SizeX: " << ptrGrabResult->GetWidth() << endl;
                //cout << "SizeY: " << ptrGrabResult->GetHeight() << endl;
                //const uint8_t* pImageBuffer = (uint8_t*)ptrGrabResult->GetBuffer();

				//(uint8_t*)ptrGrabResult->GetBuffer() 資料型態是 uint8_t* 傳到 pImageBuffer
                pWorkTab->pImageBuffer = (uint8_t*)ptrGrabResult->GetBuffer();
				// Get pWorkTab->pImageBuffer Height and Width
				pWorkTab->oriImageWidth = ptrGrabResult->GetWidth();
				pWorkTab->oriImageHeight = ptrGrabResult->GetHeight();
                
                //cout << "Gray value of first pixel: " << (uint32_t)pImageBuffer[0] << endl << endl;

                // Create an OpenCV image from the grabbed image data.
                //cv::Mat openCvImage(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(), CV_8UC1, (void*)pImageBuffer);
                //Clone the OpenCV image to m_mat
                //pWorkTab->m_mat = openCvImage.clone();
                
                // 使用 ShowImageOnPictureControl使用下式
				pWorkTab->m_mat = cv::Mat(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(), CV_8UC1, (void*)pWorkTab->pImageBuffer).clone();

                // 根據 imgFlip 的值翻轉影像
				// 0: 垂直翻轉, 1: 水平翻轉, -1: 水平並垂直翻轉, 其他值: 不翻轉
				//flip image pWorkTab->m_mat
                if (pWorkTab->imgFlip == 0)
                {
                    cv::flip(pWorkTab->m_mat, pWorkTab->m_mat, 0);
                }
                else if (pWorkTab->imgFlip == 1)
                {
                    cv::flip(pWorkTab->m_mat, pWorkTab->m_mat, 1);
                }
                else if (pWorkTab->imgFlip == -1)
                {
                    cv::flip(pWorkTab->m_mat, pWorkTab->m_mat, -1);
				}

                

                /*
 #ifdef PYLON_WIN_BUILD 
                // Display the grabbed image.
               // Pylon::DisplayImage(1, ptrGrabResult);
#endif               
                */

				// Display the grabbed image with OnPaint function in Picture Control
				pWorkTab->InvalidateRect(NULL, FALSE);
				//pWorkTab->UpdateWindow();


                //Sleep(50);
            }
            else
            {
                //cout << "Error: " << std::hex << ptrGrabResult->GetErrorCode() << std::dec << " " << ptrGrabResult->GetErrorDescription() << endl;
            }
        }
    }
    catch (const GenericException& e)
    {
        // Error handling.
        cerr << "An exception occurred." << endl
            << e.GetDescription() << endl;
        exitCode = 1;
    }

    // Comment the following two lines to disable waiting on exit.
    cerr << endl << "Press enter to exit." << endl;

   // while (cin.get() != '\n');

    // Releases all pylon resources.
    //PylonTerminate();
	// pImageBuffer clone to m_mat, use pImageBuffer with Height and Width

	// 使用 ShowImageOnPictureCtl() 使用下式
	//pWorkTab->m_mat = cv::Mat(pWorkTab->oriImageHeight, pWorkTab->oriImageWidth, CV_8UC1, (void*)pWorkTab->pImageBuffer).clone();
	
    return exitCode;
}

//Add a button IDC_WORK_STOP
void WorkTab::DrawPicToHDC(cv::Mat cvImg, UINT ID, bool bOnPaint)
{
	// Get the device context of the picture control
	CDC* pDC = GetDlgItem(ID)->GetDC();
	// Create a compatible memory device context
	CDC memDC;
	memDC.CreateCompatibleDC(pDC);
	// Create a bitmap and select it into the memory device context
	CBitmap bitmap;
	bitmap.CreateBitmap(cvImg.cols, cvImg.rows, 1, 24, cvImg.data);
	CBitmap* pOldBitmap = memDC.SelectObject(&bitmap);
	// Copy the bitmap to the picture control
	pDC->BitBlt(0, 0, cvImg.cols, cvImg.rows, &memDC, 0, 0, SRCCOPY);
	// Clean up
	memDC.SelectObject(pOldBitmap);
	GetDlgItem(ID)->ReleaseDC(pDC);
}

void WorkTab::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // 繪製的裝置內容

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // 將圖示置中於用戶端矩形
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // 描繪圖示
        dc.DrawIcon(x, y, m_hIcon);


    }
    else
    {
        CDialogEx::OnPaint();
        //Display m_mat in the dialog IDC_PICCTL_DISPLAY
          
        //DrawPicToHDC(m_mat, IDC_PICCTL_DISPLAY, true);

        //Display m_mat with cv::imshow 

		ShowImageOnPictureControl(flgCenter, cv::Scalar(0, 0, 255, 255), 1, CrossStyle::Solid);


		//以下只在 debug 模式下執行
#ifdef _DEBUG


        // 首先将数字转换为std::wstring
        std::wstring x_pos = std::to_wstring(m_MousePos.x);
        std::wstring y_pos = std::to_wstring(m_MousePos.y);

        // 构建完整的字符串
        std::wstring wstr = L"Mouse Cursor Position: (" + x_pos + L", " + y_pos + L")";
        // 将 std::wstring 转换为 CString
        m_strMousePos = CString(wstr.c_str());

		//Draw text on the dialog IDC_PICCTL_DISPLAY
		CClientDC dc(this);
		CRect rect;
		GetDlgItem(IDC_PICCTL_DISPLAY)->GetClientRect(&rect);
		dc.SetTextColor(RGB(255, 0, 0));
		dc.SetBkMode(TRANSPARENT);
		dc.TextOutW(m_MousePos.x, m_MousePos.y, m_strMousePos);
#endif
	
		//ShowImageOnPictureCtl();    
    }
}

//Create a function to convert gray scale cv:mat to CImage
// mat::cv::Mat is the input image, gray scale
// CImg::CImage is the output image
//ImageWidth: Picture Control width
//ImageHeight: Picture Control height
void WorkTab::MatConvertCimg(cv::Mat mat, CImage* CImg, int Width, int Height)
{
    // cv::Mat mat Scale resize to Width and Height
    cv::resize(mat, mat, cv::Size(Width, Height));

    // Create the CImage object using the cv::Mat's columns and rows and a bit depth of 8
    CImg->Create(mat.cols, mat.rows, 8);
    // Get the pixel data of the CImage object
    BYTE* pucImage = (BYTE*)CImg->GetBits();
    // Get the pitch of the CImage object
    int iPitch = CImg->GetPitch();

    // Copy the data from the cv::Mat object to the CImage object
    for (int i = 0; i < mat.rows; i++)
    {
        memcpy(pucImage + i * iPitch, mat.ptr<BYTE>(i), mat.cols);
    }
}

// 实际上在Picture Control上显示图像的函数实现。
/*
void WorkTab::ShowImageOnPictureControl()
{
    if (m_mat.empty()) return;

    CRect rect;
    //CWnd* pWnd = GetDlgItem(IDC_PICTURE_CONTROL); // 假设你的Picture Control控件的ID是IDC_PICTURE_CONTROL。
    pWnd->GetClientRect(&rect);
    cv::Mat resizedImage;
    cv::resize(m_mat, resizedImage, cv::Size(rect.Width(), rect.Height())); // 对图像进行缩放。

    cv::Mat imageToShow;
    cv::cvtColor(resizedImage, imageToShow, cv::COLOR_BGR2BGRA); // 转换颜色空间以适应MFC应用程序。

    BITMAPINFO bitmapInfo;
    memset(&bitmapInfo, 0, sizeof(bitmapInfo));
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biWidth = imageToShow.cols;
    bitmapInfo.bmiHeader.biHeight = -imageToShow.rows; // 注意这里的负号，它将图像翻转。
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    //CDC* pDC = pWnd->GetDC();

    ::StretchDIBits(
        pDC->GetSafeHdc(),
        0, 0, rect.Width(), rect.Height(),
        0, 0, imageToShow.cols, imageToShow.rows,
        imageToShow.data,
        &bitmapInfo,
        DIB_RGB_COLORS,
        SRCCOPY
    );

    //ReleaseDC(pDC);
}

*/




void WorkTab::ShowImageOnPictureControlWithCImage()
{
    // 創建一個 CImage
    CImage image;

    if (!m_mat.empty())
    {
        //cv::imshow("OpenCV Image", m_mat);
        cv::Mat m_mat_clone;


        //Get IDC_PICCTL_DISPLAY : Picture Control width and height
        CRect rect;
        GetDlgItem(IDC_PICCTL_DISPLAY)->GetClientRect(&rect);
        int Width = rect.Width();
        int Height = rect.Height();

        int originalWidth = m_mat.cols;
        int originalHeight = m_mat.rows;

        //計算適合的比例
        double scale = min((double)Width / originalWidth, (double)Height / originalHeight);
        Width = originalWidth * scale;
        Height = originalHeight * scale;

        //改變 m_mat 大小 , Width and Height, scale resiz
       //cv::resize(m_mat, m_mat_clone, cv::Size(Width, Height), cv::INTER_AREA);
        m_mat_clone = m_mat.clone();
        cv::resize(m_mat_clone, m_mat_clone, cv::Size(Width, Height));

        cv::imshow("OpenCV Image", m_mat);
        return;

        //m_mat convert to CImage
        MatConvertCimg(m_mat, &image, Width, Height);

        //MatConvertCimg(m_mat, &image,);

        // 獲取 Picture Control 的 DC
        CDC* pDC = GetDlgItem(IDC_PICCTL_DISPLAY)->GetDC();
        // 創建一個兼容的內存 DC
        CDC memDC;
        memDC.CreateCompatibleDC(pDC);
        // 創建一個位圖並選擇到內存 DC
        CBitmap bitmap;
        bitmap.CreateCompatibleBitmap(pDC, image.GetWidth(), image.GetHeight());
        CBitmap* pOldBitmap = memDC.SelectObject(&bitmap);
        // 複製位圖到 Picture Control
        image.Draw(memDC.GetSafeHdc(), CRect(0, 0, image.GetWidth(), image.GetHeight()));
        pDC->BitBlt(0, 0, image.GetWidth(), image.GetHeight(), &memDC, 0, 0, SRCCOPY);

        // 清理
        memDC.SelectObject(pOldBitmap);
        GetDlgItem(IDC_PICCTL_DISPLAY)->ReleaseDC(pDC);

        //Sleep(50);

    }
}

//Resize the image
//pImageBuffer: Original Image data pointer
//originalWidth: Original Image width
//originalHeight: Original Image height
//pResizedBuffer: Resized Image data pointer
//targetWidth: Resized Image width
//targetHeight: Resized Image height
void WorkTab::ResizeGrayImage(uint8_t* pImageBuffer, int originalWidth, int originalHeight, uint8_t*& pResizedBuffer, int targetWidth, int targetHeight)
{
    // 為調整大小後的影像分配記憶體
    pResizedBuffer = new uint8_t[targetWidth * targetHeight]; // 灰階格式，假設每像素 1 字節

    float x_ratio = float(originalWidth) / float(targetWidth);
    float y_ratio = float(originalHeight) / float(targetHeight);

    for (int i = 0; i < targetHeight; i++) {
        for (int j = 0; j < targetWidth; j++) {
            int px = int(j * x_ratio);
            int py = int(i * y_ratio);
            pResizedBuffer[i * targetWidth + j] = pImageBuffer[py * originalWidth + px];
        }
    }
}



//Display the image in the rect of Picture Control
//pImage: Resized Image data pointer
//width: Resized Image width
//height: Resized Image height
//pictureControl: MFC Picture Control
void WorkTab::DisplayGrayImageInControl(uint8_t* pImage, int width, int height, CStatic& pictureControl)
{
    // Create a bitmap with the grayscale image data
    CBitmap bitmap;
    if (!bitmap.CreateBitmap(width, height, 1, 8, pImage)) {
        AfxMessageBox(_T("Failed to create bitmap."));
        return;
    }

    // Using CClientDC for safer handling of the DC for pictureControl
    CClientDC controlDC(&pictureControl);
    if (!controlDC) {
        AfxMessageBox(_T("Failed to get device context for picture control."));
        return;
    }

    // Create a memory DC compatible with the control's DC and select the bitmap into it
    CDC memDC;
    if (!memDC.CreateCompatibleDC(&controlDC)) {
        AfxMessageBox(_T("Failed to create memory device context."));
        return;
    }
    CBitmap* pOldBitmap = memDC.SelectObject(&bitmap);

    // Calculate the size and position for the image within the control, maintaining aspect ratio
    CRect rect;
    pictureControl.GetClientRect(&rect);
    int controlWidth = rect.Width();
    int controlHeight = rect.Height();
    double imageAspectRatio = static_cast<double>(width) / height;
    double controlAspectRatio = static_cast<double>(controlWidth) / controlHeight;

    int imageDisplayWidth, imageDisplayHeight, x, y;

    // Fit the image into the control based on the aspect ratio
    if (imageAspectRatio > controlAspectRatio) {
        imageDisplayWidth = controlWidth;
        imageDisplayHeight = static_cast<int>(controlWidth / imageAspectRatio);
        x = 0;
        y = (controlHeight - imageDisplayHeight) / 2; // Center vertically
    }
    else {
        imageDisplayHeight = controlHeight;
        imageDisplayWidth = static_cast<int>(controlHeight * imageAspectRatio);
        x = (controlWidth - imageDisplayWidth) / 2; // Center horizontally
        y = 0;
    }

    // Draw the resized image in the control
    controlDC.StretchBlt(x, y, imageDisplayWidth, imageDisplayHeight, &memDC, 0, 0, width, height, SRCCOPY);

    // Clean up
    memDC.SelectObject(pOldBitmap);  // Restore the original bitmap

}

// pImageBuffer, Zoom All  在Picture Control上直接显示图像的函數。
void WorkTab::ShowImageOnPictureCtl()
{
	// if pImageBuffer is empty, return
	if (pImageBuffer == NULL) return;
	
	int targetWidth = 640;
	int targetHeight = 480;

	// Get the Picture Control IDC_PICCTL_DISPLAY wide : targetWidth, height : targetHeight
	CRect rect;
	GetDlgItem(IDC_PICCTL_DISPLAY)->GetClientRect(&rect);
	targetWidth = rect.Width();
	targetHeight = rect.Height();

    ResizeGrayImage(pImageBuffer, oriImageWidth, oriImageHeight, pResizedImage, targetWidth, targetHeight);

    // Display the image in the Picture Control
    DisplayGrayImageInControl(pResizedImage, targetWidth, targetHeight, m_PicCtl_Display);  

}

void WorkTab::ShowImageOnPictureControl(bool flgCenter, cv::Scalar crossColor, int lineThickness, CrossStyle style)
{
    const cv::Mat& sourceMat = !m_factorPreviewMat.empty() ? m_factorPreviewMat : m_mat;
    if (sourceMat.empty() || pWnd == nullptr || !::IsWindow(pWnd->GetSafeHwnd())) return;

    CRect rect;
    pWnd->GetClientRect(&rect);
    if (rect.Width() <= 0 || rect.Height() <= 0) return;

    cv::Mat resizedImage;
    cv::resize(sourceMat, resizedImage, cv::Size(rect.Width(), rect.Height()));

    cv::Mat imageToShow;
    if (resizedImage.channels() == 1) {
        cv::cvtColor(resizedImage, imageToShow, cv::COLOR_GRAY2BGRA);
    }
    else if (resizedImage.channels() == 3) {
        cv::cvtColor(resizedImage, imageToShow, cv::COLOR_BGR2BGRA);
    }
    else if (resizedImage.channels() == 4) {
        imageToShow = resizedImage.clone();
    }
    else {
        return;
    }



	//取得 ROI checkbox 狀態，決定是否繪製 Mask 矩形

	if (m_bROIMode && m_factorPreviewMat.empty())
    {
        // --- 新增：繪製 Mask 矩形 ---
     // Calculate scale factors
     // Draw the rectangle if MaskWidth and MaskHeight are greater than 0
        if (MaskWidth > 0 && MaskHeight > 0) {
            double scaleX = static_cast<double>(rect.Width()) / sourceMat.cols;
            double scaleY = static_cast<double>(rect.Height()) / sourceMat.rows;
           

            //double scaleX = static_cast<double>(imageToShow.cols) / rect.Width();
            //double scaleY = static_cast<double>(imageToShow.rows) / rect.Height();

            int x = static_cast<int>(std::lround(MaskX * scaleX));
            int y = static_cast<int>(std::lround(MaskY * scaleY));
            int w = static_cast<int>(std::lround(MaskWidth * scaleX));
            int h = static_cast<int>(std::lround(MaskHeight * scaleY));

            x = (std::max)(0, (std::min)(x, imageToShow.cols - 1));
            y = (std::max)(0, (std::min)(y, imageToShow.rows - 1));
            w = (std::max)(0, (std::min)(w, imageToShow.cols - x));
            h = (std::max)(0, (std::min)(h, imageToShow.rows - y));

            if (w > 0 && h > 0) {
                cv::rectangle(imageToShow, cv::Rect(x, y, w, h), cv::Scalar(0, 255, 0, 255), 1);
            }
        }
        // --- End ---
    }
    
	// Draw center cross
    //
    if (flgCenter)
    {

        double scaleX = static_cast<double>(rect.Width()) / sourceMat.cols;
        double scaleY = static_cast<double>(rect.Height()) / sourceMat.rows;
        int centerX = static_cast<int>(std::lround(referenceX * scaleX));
        int centerY = static_cast<int>(std::lround(referenceY * scaleY));
        centerX = (std::max)(0, (std::min)(centerX, imageToShow.cols - 1));
        centerY = (std::max)(0, (std::min)(centerY, imageToShow.rows - 1));

        auto drawDashedLine = [&](cv::Point start, cv::Point end, int dashLength)
            {
                double totalLength = cv::norm(end - start);
                cv::Point2f dir = (end - start) / static_cast<float>(totalLength);

                for (double d = 0; d < totalLength; d += dashLength * 2)
                {
                    cv::Point2f p1f = cv::Point2f(start) + dir * static_cast<float>(d);
                    cv::Point2f p2f = cv::Point2f(start) + dir * static_cast<float>((std::min)(d + dashLength, totalLength));

                    cv::line(imageToShow, cv::Point(cvRound(p1f.x), cvRound(p1f.y)),
                        cv::Point(cvRound(p2f.x), cvRound(p2f.y)),
                        crossColor, lineThickness);
                }
            };

		// Draw horizontal and vertical lines
        if (style == CrossStyle::Solid)
        {
            cv::line(imageToShow, cv::Point(0, centerY),
                cv::Point(imageToShow.cols - 1, centerY),
                crossColor, lineThickness);

            cv::line(imageToShow, cv::Point(centerX, 0),
                cv::Point(centerX, imageToShow.rows - 1),
                crossColor, lineThickness);
        }
        else if (style == CrossStyle::Dashed)
        {
            int dashLength = 10;
            drawDashedLine(cv::Point(0, centerY),
                cv::Point(imageToShow.cols - 1, centerY), dashLength);
            drawDashedLine(cv::Point(centerX, 0),
                cv::Point(centerX, imageToShow.rows - 1), dashLength);
        }
    }

    if (m_bFactorSelectMode && (m_bDrawingROI || m_bROIConfirmed)) {
        CRect roiRect(m_ROIStart, m_ROICurrent);
        roiRect.NormalizeRect();
        const int maxX = imageToShow.cols - 1;
        const int maxY = imageToShow.rows - 1;
        const int left = (std::max)(0, (std::min)(static_cast<int>(roiRect.left), maxX));
        const int top = (std::max)(0, (std::min)(static_cast<int>(roiRect.top), maxY));
        const int right = (std::max)(left + 1, (std::min)(static_cast<int>(roiRect.right), imageToShow.cols));
        const int bottom = (std::max)(top + 1, (std::min)(static_cast<int>(roiRect.bottom), imageToShow.rows));
        roiRect.left = left;
        roiRect.top = top;
        roiRect.right = right;
        roiRect.bottom = bottom;

        cv::rectangle(
            imageToShow,
            cv::Rect(roiRect.left, roiRect.top, roiRect.Width(), roiRect.Height()),
            cv::Scalar(0, 255, 255, 255),
            2);
    }

    if (m_factorPreviewMat.empty()) {
        const double scaleX = static_cast<double>(rect.Width()) / sourceMat.cols;
        const double scaleY = static_cast<double>(rect.Height()) / sourceMat.rows;

        auto toDisplayPoint = [&](const cv::Point2d& pt) {
            const int x = (std::max)(0, (std::min)(cvRound(pt.x * scaleX), imageToShow.cols - 1));
            const int y = (std::max)(0, (std::min)(cvRound(pt.y * scaleY), imageToShow.rows - 1));
            return cv::Point(x, y);
            };

        for (const auto& pt : toolPath.Path) {
            cv::circle(imageToShow, toDisplayPoint(pt), 2, cv::Scalar(0, 255, 255, 255), cv::FILLED);
        }

        for (const auto& pt : m_OptimizedGluePath.PathLeft) {
            cv::circle(imageToShow, toDisplayPoint(pt), 4, cv::Scalar(0, 0, 255, 255), cv::FILLED);
        }

        for (const auto& pt : m_OptimizedGluePath.PathRight) {
            cv::circle(imageToShow, toDisplayPoint(pt), 4, cv::Scalar(0, 255, 0, 255), cv::FILLED);
        }
    }

    BITMAPINFO bitmapInfo;
    memset(&bitmapInfo, 0, sizeof(bitmapInfo));
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biWidth = imageToShow.cols;
    bitmapInfo.bmiHeader.biHeight = -imageToShow.rows;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    CClientDC controlDC(pWnd);
    ::StretchDIBits(
        controlDC.GetSafeHdc(),
        0, 0, rect.Width(), rect.Height(),
        0, 0, imageToShow.cols, imageToShow.rows,
        imageToShow.data,
        &bitmapInfo,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

void WorkTab::OnBnClickedWorkStopGrab()
{
    // TODO: 在此加入控制項告知處理常式

    // If the grab thread is running, stop it
    if (m_bGrabThread)
	{
		m_bGrabThread = false;
	}
}




void WorkTab::OnOK()
{
    // TODO: 在此加入特定的程式碼和 (或) 呼叫基底類別
    StopHmiSyncTimer();
    CDialogEx::OnOK();
}


void WorkTab::OnCancel()
{
    // TODO: 在此加入特定的程式碼和 (或) 呼叫基底類別
    StopHmiSyncTimer();
    CDialogEx::OnCancel();
}

BOOL WorkTab::PreTranslateMessage(MSG* pMsg)
{
    if (m_bFactorSelectMode && !m_factorPreviewMat.empty()) {
        CRect picRectScreen;
        if (GetDlgItem(IDC_PICCTL_DISPLAY) != nullptr) {
            GetDlgItem(IDC_PICCTL_DISPLAY)->GetWindowRect(&picRectScreen);
        }

        if (!picRectScreen.IsRectEmpty() && picRectScreen.PtInRect(pMsg->pt)) {
            const int localX = static_cast<int>(pMsg->pt.x - picRectScreen.left);
            const int localY = static_cast<int>(pMsg->pt.y - picRectScreen.top);
            const int maxLocalX = picRectScreen.Width() - 1;
            const int maxLocalY = picRectScreen.Height() - 1;
            const CPoint localPoint(
                (std::max)(0, (std::min)(localX, maxLocalX)),
                (std::max)(0, (std::min)(localY, maxLocalY)));

            switch (pMsg->message) {
            case WM_LBUTTONDOWN:
                m_bDrawingROI = true;
                m_bROIConfirmed = false;
                m_ROIStart = localPoint;
                m_ROICurrent = localPoint;
                ShowImageOnPictureControl(flgCenter, cv::Scalar(0, 0, 255, 255), 1, CrossStyle::Solid);
                return TRUE;

            case WM_MOUSEMOVE:
                if (m_bDrawingROI) {
                    m_ROICurrent = localPoint;
                    ShowImageOnPictureControl(flgCenter, cv::Scalar(0, 0, 255, 255), 1, CrossStyle::Solid);
                    return TRUE;
                }
                break;

            case WM_LBUTTONUP:
                if (m_bDrawingROI) {
                    m_bDrawingROI = false;
                    m_ROICurrent = localPoint;
                    m_bROIConfirmed = true;

                    const double scaleX = static_cast<double>(m_factorPreviewMat.cols) / picRectScreen.Width();
                    const double scaleY = static_cast<double>(m_factorPreviewMat.rows) / picRectScreen.Height();
                    CRect roiRect(m_ROIStart, m_ROICurrent);
                    roiRect.NormalizeRect();
                    if (roiRect.Width() <= 1 || roiRect.Height() <= 1) {
                        AfxMessageBox(L"框選區域太小。");
                        m_bROIConfirmed = false;
                        ShowImageOnPictureControl(flgCenter, cv::Scalar(0, 0, 255, 255), 1, CrossStyle::Solid);
                        return TRUE;
                    }

                    cv::Rect imageRoi(
                        cvRound(roiRect.left * scaleX),
                        cvRound(roiRect.top * scaleY),
                        (std::max)(1, cvRound(roiRect.Width() * scaleX)),
                        (std::max)(1, cvRound(roiRect.Height() * scaleY)));
                    imageRoi &= cv::Rect(0, 0, m_factorPreviewMat.cols, m_factorPreviewMat.rows);

                    int minCol = m_factorBoardSize.width;
                    int maxCol = -1;
                    int minRow = m_factorBoardSize.height;
                    int maxRow = -1;

                    for (int row = 0; row < m_factorBoardSize.height; ++row) {
                        for (int col = 0; col < m_factorBoardSize.width; ++col) {
                            const cv::Point2f& pt = m_factorCorners[row * m_factorBoardSize.width + col];
                            if (imageRoi.contains(cv::Point(cvRound(pt.x), cvRound(pt.y)))) {
                                minCol = (std::min)(minCol, col);
                                maxCol = (std::max)(maxCol, col);
                                minRow = (std::min)(minRow, row);
                                maxRow = (std::max)(maxRow, row);
                            }
                        }
                    }

                    if (maxCol < 0 || maxRow < 0) {
                        AfxMessageBox(L"框選區域內沒有偵測到有效角點。");
                        return TRUE;
                    }

                    const int colCells = maxCol - minCol;
                    const int rowCells = maxRow - minRow;
                    if (colCells <= 0 && rowCells <= 0) {
                        AfxMessageBox(L"框選區域至少需要跨越 1 格以上，才能自動計算格數。");
                        return TRUE;
                    }

                    std::vector<double> pixelSteps;
                    for (int row = minRow; row <= maxRow; ++row) {
                        for (int col = minCol; col < maxCol; ++col) {
                            const cv::Point2f& p1 = m_factorCorners[row * m_factorBoardSize.width + col];
                            const cv::Point2f& p2 = m_factorCorners[row * m_factorBoardSize.width + col + 1];
                            pixelSteps.push_back(cv::norm(p2 - p1));
                        }
                    }
                    for (int row = minRow; row < maxRow; ++row) {
                        for (int col = minCol; col <= maxCol; ++col) {
                            const cv::Point2f& p1 = m_factorCorners[row * m_factorBoardSize.width + col];
                            const cv::Point2f& p2 = m_factorCorners[(row + 1) * m_factorBoardSize.width + col];
                            pixelSteps.push_back(cv::norm(p2 - p1));
                        }
                    }

                    if (pixelSteps.empty()) {
                        AfxMessageBox(L"無法從所選區域計算角點間距。");
                        return TRUE;
                    }

                    double alongColsDx = 0.0, alongColsDy = 0.0;
                    double alongRowsDx = 0.0, alongRowsDy = 0.0;
                    int alongColsCount = 0, alongRowsCount = 0;
                    for (int row = minRow; row <= maxRow; ++row) {
                        for (int col = minCol; col < maxCol; ++col) {
                            const cv::Point2f& p1 = m_factorCorners[row * m_factorBoardSize.width + col];
                            const cv::Point2f& p2 = m_factorCorners[row * m_factorBoardSize.width + col + 1];
                            alongColsDx += std::abs(p2.x - p1.x);
                            alongColsDy += std::abs(p2.y - p1.y);
                            ++alongColsCount;
                        }
                    }
                    for (int row = minRow; row < maxRow; ++row) {
                        for (int col = minCol; col <= maxCol; ++col) {
                            const cv::Point2f& p1 = m_factorCorners[row * m_factorBoardSize.width + col];
                            const cv::Point2f& p2 = m_factorCorners[(row + 1) * m_factorBoardSize.width + col];
                            alongRowsDx += std::abs(p2.x - p1.x);
                            alongRowsDy += std::abs(p2.y - p1.y);
                            ++alongRowsCount;
                        }
                    }

                    const bool alongColsLooksHorizontal =
                        alongColsCount > 0 && (alongColsDx / alongColsCount) >= (alongColsDy / alongColsCount);

                    int horizontalCells = colCells;
                    int verticalCells = rowCells;
                    if (!alongColsLooksHorizontal) {
                        horizontalCells = rowCells;
                        verticalCells = colCells;
                    }

                    CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
                    if (!pParentWnd) {
                        AfxMessageBox(L"無法取得父視窗指標。");
                        return TRUE;
                    }

                    const double gridLengthPx = std::accumulate(pixelSteps.begin(), pixelSteps.end(), 0.0) / static_cast<double>(pixelSteps.size());
                    const double factorMmPerPixel = m_pendingGridLengthMm / gridLengthPx;
                    pParentWnd->m_SystemPara.TransferFactor = static_cast<float>(factorMmPerPixel);

                    std::string appPath = GetAppPath();
                    std::string configPath = appPath;
                    if (!configPath.empty() && configPath.back() != '\\' && configPath.back() != '/') {
                        configPath += "\\";
                    }
                    configPath += "SystemConfig.ini";
                    WriteConfigToFile_SP(configPath, pParentWnd->m_SystemPara);

                    m_bFactorSelectMode = false;
                    m_factorPreviewMat.release();
                    m_factorCorners.clear();
                    m_bROIConfirmed = false;

                    ShowImageOnPictureControl(flgCenter, cv::Scalar(0, 0, 255, 255), 1, CrossStyle::Solid);

                    CString msg;
                    msg.Format(L"Factor 計算完成。\n橫向格數: %d\n縱向格數: %d\n單格長度: %.3f mm\n平均單格像素: %.3f px\nTransferFactor: %.6f mm/pixel\nINI已更新: %s",
                        horizontalCells,
                        verticalCells,
                        m_pendingGridLengthMm,
                        gridLengthPx,
                        factorMmPerPixel,
                        CString(configPath.c_str()));
                    AfxMessageBox(msg);
                    return TRUE;
                }
                break;
            }
        }
    }

    return CDialogEx::PreTranslateMessage(pMsg);
}


void WorkTab::OnMouseMove(UINT nFlags, CPoint point)
{
    m_MousePos = point;

    if (m_bFactorSelectMode && m_bDrawingROI) {
        CRect picRect;
        GetDlgItem(IDC_PICCTL_DISPLAY)->GetWindowRect(&picRect);
        ScreenToClient(&picRect);
        if (picRect.PtInRect(point)) {
            const int localX = static_cast<int>(point.x - picRect.left);
            const int localY = static_cast<int>(point.y - picRect.top);
            const int maxLocalX = picRect.Width() - 1;
            const int maxLocalY = picRect.Height() - 1;
            m_ROICurrent = CPoint(
                (std::max)(0, (std::min)(localX, maxLocalX)),
                (std::max)(0, (std::min)(localY, maxLocalY)));
            ShowImageOnPictureControl(flgCenter, cv::Scalar(0, 0, 255, 255), 1, CrossStyle::Solid);
        }
    }

    CDialogEx::OnMouseMove(nFlags, point);
}

void WorkTab::OnLButtonDown(UINT nFlags, CPoint point)
{
    if (m_bFactorSelectMode && !m_factorPreviewMat.empty()) {
        CRect picRect;
        GetDlgItem(IDC_PICCTL_DISPLAY)->GetWindowRect(&picRect);
        ScreenToClient(&picRect);
        if (picRect.PtInRect(point)) {
            m_bDrawingROI = true;
            m_bROIConfirmed = false;
            m_ROIStart = CPoint(point.x - picRect.left, point.y - picRect.top);
            m_ROICurrent = m_ROIStart;
            ShowImageOnPictureControl(flgCenter, cv::Scalar(0, 0, 255, 255), 1, CrossStyle::Solid);
            return;
        }
    }

    CDialogEx::OnLButtonDown(nFlags, point);
}

void WorkTab::OnLButtonUp(UINT nFlags, CPoint point)
{
    if (m_bFactorSelectMode && m_bDrawingROI && !m_factorPreviewMat.empty()) {
        CRect picRect;
        GetDlgItem(IDC_PICCTL_DISPLAY)->GetWindowRect(&picRect);
        ScreenToClient(&picRect);
        m_bDrawingROI = false;

        const int localX = static_cast<int>(point.x - picRect.left);
        const int localY = static_cast<int>(point.y - picRect.top);
        const int maxLocalX = picRect.Width() - 1;
        const int maxLocalY = picRect.Height() - 1;
        CPoint endPoint(
            (std::max)(0, (std::min)(localX, maxLocalX)),
            (std::max)(0, (std::min)(localY, maxLocalY)));
        m_ROICurrent = endPoint;
        m_bROIConfirmed = true;

        CRect roiRect(m_ROIStart, m_ROICurrent);
        roiRect.NormalizeRect();
        if (roiRect.Width() <= 1 || roiRect.Height() <= 1) {
            AfxMessageBox(L"框選區域太小。");
            m_bROIConfirmed = false;
            ShowImageOnPictureControl(flgCenter, cv::Scalar(0, 0, 255, 255), 1, CrossStyle::Solid);
            return;
        }

        const double scaleX = static_cast<double>(m_factorPreviewMat.cols) / picRect.Width();
        const double scaleY = static_cast<double>(m_factorPreviewMat.rows) / picRect.Height();
        cv::Rect imageRoi(
            cvRound(roiRect.left * scaleX),
            cvRound(roiRect.top * scaleY),
            (std::max)(1, cvRound(roiRect.Width() * scaleX)),
            (std::max)(1, cvRound(roiRect.Height() * scaleY)));
        imageRoi &= cv::Rect(0, 0, m_factorPreviewMat.cols, m_factorPreviewMat.rows);

        int minCol = m_factorBoardSize.width;
        int maxCol = -1;
        int minRow = m_factorBoardSize.height;
        int maxRow = -1;

        for (int row = 0; row < m_factorBoardSize.height; ++row) {
            for (int col = 0; col < m_factorBoardSize.width; ++col) {
                const cv::Point2f& pt = m_factorCorners[row * m_factorBoardSize.width + col];
                if (imageRoi.contains(cv::Point(cvRound(pt.x), cvRound(pt.y)))) {
                    minCol = (std::min)(minCol, col);
                    maxCol = (std::max)(maxCol, col);
                    minRow = (std::min)(minRow, row);
                    maxRow = (std::max)(maxRow, row);
                }
            }
        }

        if (maxCol < 0 || maxRow < 0) {
            AfxMessageBox(L"框選區域內沒有偵測到有效角點。");
            return;
        }

        const int horizontalCells = maxCol - minCol;
        const int verticalCells = maxRow - minRow;
        if (horizontalCells <= 0 && verticalCells <= 0) {
            AfxMessageBox(L"框選區域至少需要跨越 1 格以上，才能自動計算格數。");
            return;
        }

        std::vector<double> pixelSteps;
        for (int row = minRow; row <= maxRow; ++row) {
            for (int col = minCol; col < maxCol; ++col) {
                const cv::Point2f& p1 = m_factorCorners[row * m_factorBoardSize.width + col];
                const cv::Point2f& p2 = m_factorCorners[row * m_factorBoardSize.width + col + 1];
                pixelSteps.push_back(cv::norm(p2 - p1));
            }
        }
        for (int row = minRow; row < maxRow; ++row) {
            for (int col = minCol; col <= maxCol; ++col) {
                const cv::Point2f& p1 = m_factorCorners[row * m_factorBoardSize.width + col];
                const cv::Point2f& p2 = m_factorCorners[(row + 1) * m_factorBoardSize.width + col];
                pixelSteps.push_back(cv::norm(p2 - p1));
            }
        }

        if (pixelSteps.empty()) {
            AfxMessageBox(L"無法從所選區域計算角點間距。");
            return;
        }

        CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
        if (!pParentWnd) {
            AfxMessageBox(L"無法取得父視窗指標。");
            return;
        }

        const double gridLengthPx = std::accumulate(pixelSteps.begin(), pixelSteps.end(), 0.0) / static_cast<double>(pixelSteps.size());
        const double factorMmPerPixel = m_pendingGridLengthMm / gridLengthPx;
        pParentWnd->m_SystemPara.TransferFactor = static_cast<float>(factorMmPerPixel);

        std::string appPath = GetAppPath();
        std::string configPath = appPath;
        if (!configPath.empty() && configPath.back() != '\\' && configPath.back() != '/') {
            configPath += "\\";
        }
        configPath += "SystemConfig.ini";
        WriteConfigToFile_SP(configPath, pParentWnd->m_SystemPara);

        m_bFactorSelectMode = false;
        m_factorPreviewMat.release();
        m_factorCorners.clear();
        m_bROIConfirmed = false;

        ShowImageOnPictureControl(flgCenter, cv::Scalar(0, 0, 255, 255), 1, CrossStyle::Solid);

        CString msg;
        msg.Format(L"Factor 計算完成。\n橫向格數: %d\n縱向格數: %d\n單格長度: %.3f mm\n平均單格像素: %.3f px\nTransferFactor: %.6f mm/pixel\nINI已更新: %s",
            horizontalCells,
            verticalCells,
            m_pendingGridLengthMm,
            gridLengthPx,
            factorMmPerPixel,
            CString(configPath.c_str()));
        AfxMessageBox(msg);
        return;
    }

    CDialogEx::OnLButtonUp(nFlags, point);
}

BOOL WorkTab::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_CROSS));
	return TRUE;
}

//Create Template Image for Matching
void WorkTab::OnBnClickedWorkTempImg()
{
    // TODO: 在此加入控制項告知處理常式程式碼
    //get the display size of monitor
    // DisplayWidth : size of width of monitor
    // DisplayHeight : size of height of monitor
    //GetDisplaySize(DisplayWidth, DisplayHeight);
    // Load the image
   
    if (m_mat.empty()) 
    {
        AfxMessageBox(_T("No image to display."));
        return;
    }

    //Get image size of m_mat and display on messagebox
    //CString str;
    //str.Format(_T("Image Size: %d x %d"), m_mat.cols, m_mat.rows);
    //AfxMessageBox(str);
    
   
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);

    cv::Mat roi_image = showImageAndReturnROI(m_mat, screenHeight, screenWidth);



    if (!roi_image.empty()) 
    {
        m_matTemp= roi_image.clone();
        //cv::imshow("Selected ROI", roi_image);
        //cv::waitKey(0);
    }
	else
    {
		AfxMessageBox(_T("No ROI selected."));
	}

}


void WorkTab::OnBnClickedWorkMatchTemp()
{
    // TODO: 在此加入控制項告知處理常式
	//m_mat: Source Image
	//m_matTemp: Template Image

	//Load the Source Image
	if (m_mat.empty())
	{
		AfxMessageBox(_T("No image to match."));
		return;
	}

	// Load the template image
	if (m_matTemp.empty())
	{
		AfxMessageBox(_T("No template image to match."));
		return;
	}

    // Rotate the template image
    //double angle = 30.0; // Rotation angle in degrees
    //cv::Point2f center(templateImg.cols / 2.0, templateImg.rows / 2.0);
    //cv::Mat rotMat = cv::getRotationMatrix2D(center, angle, 1.0);
    //cv::Mat rotatedTemplate;
    //cv::warpAffine(templateImg, rotatedTemplate, rotMat, templateImg.size());
	

	// Match the template
	//ImageSrc: Source Image m_mat
	//ImageTemp: Template Image m_matTemp
	//ImageDst: Result Image result
	//match_method: Matching method
	//Location: ImageLocation
    cv::Mat ImageSrc = m_mat;
	cv::Mat ImageTemp = m_matTemp.clone();
	cv::Mat ImageDst;
	int match_method = cv::TM_CCOEFF_NORMED;
	ImageLocation Location;

	int ret = MatchTemplate(ImageSrc, ImageTemp, ImageDst, match_method, Location);
		if (ret == 0)
		{
			AfxMessageBox(_T("Template not found."));
			return;
		}

		// Display the matched image withthe Location of the template to display rotated rectangle

		cv::Mat source = ImageSrc.clone();
		cv::Point2f vertices[4];

        //Freom  Location to get center and degree to rotate ractangle
		cv::Point2f center;
		cv::Rect rotatedRect;
		center.x = Location.Position.x;
		center.y = Location.Position.y;
		int degree = Location.Angle;
		rotatedRect = Location.Rect;

		//Convert cv::Rect rotatedRect to cv::Point2f vertices[4]
		vertices[0] = cv::Point(rotatedRect.x, rotatedRect.y);
		vertices[1] = cv::Point(rotatedRect.x + rotatedRect.width, rotatedRect.y);
		vertices[2] = cv::Point(rotatedRect.x + rotatedRect.width, rotatedRect.y + rotatedRect.height);
		vertices[3] = cv::Point(rotatedRect.x, rotatedRect.y + rotatedRect.height);

		
		
		for (int i = 0; i < 4; i++)
		{
			cv::line(source, vertices[i], vertices[(i + 1) % 4], cv::Scalar(0, 255, 0), 2);
		}

    // Display the result
    cv::imshow("Matched Image", source);
    cv::waitKey(0);

    //return 0;


}


void WorkTab::OnBnClickedIdcWorkToolPath() 
{
    // 1. 影像檢查
    if (m_mat.empty()) {
        AfxMessageBox(_T("Source image is empty."));
        return;
    }

    // 2. 獲取父視窗參數
    CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
    if (!pParentWnd) return;

    // 3. ROI 範圍安全檢查 (修正 X/Y 與 Width/Height 的對應)
    if (MaskX < 0 || MaskY < 0 ||
        MaskX + MaskWidth > m_mat.cols ||
        MaskY + MaskHeight > m_mat.rows) {
        AfxMessageBox(_T("ROI exceeds image dimensions."));
        return;
    }

    // 4. 計算單一 inward offset，單位為 pixel
    double OffsetValue = pParentWnd->m_SystemPara.OffsetValue;
    double factor = pParentWnd->m_SystemPara.TransferFactor;

    // TransferFactor 的單位是 mm/pixel，因此 OffsetValue(mm) / factor(mm/pixel) = pixel。
    const double offsetPixel = (factor > 0.0) ? (OffsetValue / factor) : 0.0;

    // 5. 準備 Mask
    cv::Mat mask = cv::Mat::zeros(m_mat.size(), CV_8UC1);
    cv::Rect roiRect(MaskX, MaskY, MaskWidth, MaskHeight);
    mask(roiRect) = cv::Scalar(255);

    // 6. 提取原始工具路徑前，先做影像校正
	cv::Mat correctedImage = m_mat;  // 預設為原始影像，如果校正失敗或未校正，則繼續使用原始影像提取路徑
	cv::Mat pathSourceImage = m_mat; // 用於路徑提取的影像，通常是校正後的影像，但如果校正失敗則回退為原始影像
    cv::Mat pathMask = mask.clone();
    if (!m_vision.isCalibrated()) {
        m_vision.loadCalibrationData(GetCalibrationFilePath());
    }

    bool usedCalibrationCorrection = false;
    if (m_vision.isCalibrated()) {
        try {
            // m_mat 在取像後可能已依 imgFlip 翻轉；校正參數通常是以原始相機方向建立。
            // 因此在原始方向做去畸變與路徑提取，最後再把點座標翻回目前顯示座標。
			cv::Mat rawOrientationImage = ApplyConfiguredFlip(m_mat, imgFlip); // 把 m_mat 翻回校正參數對應的原始相機方向，這樣去畸變後的影像才會和校正參數對齊。
			cv::Mat undistortedRaw = m_vision.undistortImage(rawOrientationImage); // 去畸變後的影像仍然是原始相機方向，這樣提取路徑才會和校正參數對齊。
            if (!undistortedRaw.empty()) {
				correctedImage = ApplyConfiguredFlip(undistortedRaw, imgFlip); // 把去畸變後的影像翻回目前顯示的方向，這樣 correctedImage 就是校正後且符合目前顯示方向的影像，可以直接用來顯示。
                pathSourceImage = undistortedRaw;

				//ApplyConfiguredFlip() 會對 mask 做同樣的翻轉，確保 pathMask 與 pathSourceImage 在同一個座標系統下對齊。
                pathMask = ApplyConfiguredFlip(mask, imgFlip);
                usedCalibrationCorrection = true;
            }
        }
        catch (const cv::Exception&) {
            correctedImage = m_mat;
            pathSourceImage = m_mat;
            pathMask = mask.clone();
        }
    }

    // 7. 提取工具路徑 (直接操作成員變數)
	// correctedImage: 顯示用影像，原點為左上角，單位為 pixel
	// pathSourceImage / imgClone: 實際拿來取路徑的影像，原點為左上角，單位為 pixel
    this->toolPath.Path.clear();
    cv::Mat imgClone = pathSourceImage.clone();
	cv::flip(imgClone, imgClone, -1); // 提取路徑前先將輸入影像旋轉 180 度。

	pParentWnd->m_SystemPara.BinaryUpper = 255;
	pParentWnd->m_SystemPara.BinaryLower = 210; 


    GetToolPath_CurvatureOptimized_Mask(
        imgClone,
        pathMask,
        offsetPixel,
        this->toolPath,
        0.0008,
        pParentWnd->m_SystemPara.BinaryUpper,
        pParentWnd->m_SystemPara.BinaryLower,
        false);

	// 此時 toolPath 中的點原本是基於 pathSourceImage 左上角的 pixel 座標。
	// 若校正流程有做方向翻轉，則再把點翻回 correctedImage 的顯示方向。
	// 在進入 OptimizeGluePath(...) 前，toolPath 仍然是影像左上角為原點、單位為 pixel。

    /*
     if (usedCalibrationCorrection) {
		// 把 toolPath 中的點座標翻回 pathSourceImage 的顯示方向，這樣後續優化與繪圖就不需要再考慮翻轉了。
        ApplyConfiguredFlipToToolPath(this->toolPath, pathSourceImage.size(), imgFlip);
    }

    if (this->toolPath.Path.empty()) {
        AfxMessageBox(_T("No path detected in ROI."));
        return;
    }
    */
   

    // 8. 膠路同步優化 (核心步驟)
	// 直接使用成員變數 MaskX/Y/Width/Height 和 referenceX/Y
    ROIMask roiOpt = {};
    roiOpt.MaskX = MaskX; roiOpt.MaskY = MaskY;
    roiOpt.MaskWidth = MaskWidth; roiOpt.MaskHeight = MaskHeight;
    roiOpt.RefCenterX = this->referenceX;
    roiOpt.RefCenterY = this->referenceY;

    // 直接傳入成員變數，避免重複拷貝
    // 注意：OutputPath 建議定義為類別成員，以便後續繪圖或傳送給 PLC
    GluePath finalPath;

	// 分割成左、右兩條路徑優化，輸出的 finalPath 仍以影像左上角為原點、單位為 pixel。
    OptimizeGluePath(this->toolPath.Path, roiOpt, finalPath, 2);

    // 校正後的 toolPath 已經透過 ApplyConfiguredFlipToToolPath(...)
    // 翻回 correctedImage 的顯示方向。
    // imgFlip 的目的只是讓拍照/顯示方向符合直覺參考，這裡不要再額外旋轉 180 度，
    // 否則 finalPath 會和 correctedImage 使用不同的視覺方向。

	// 最後再依 Y 座標排序，確保路徑點是從上到下的順序，這對於後續的機器人運動規劃很重要。
    SortGluePathByAscendingY(finalPath);

    // 只保留 ROI 內的有效路徑點，並限制最多輸出 30 筆。
    // 30 是上限，若有效點數小於 30 則直接接受。
    FilterGluePathByRoiAndLimit(finalPath, roiRect, 30);

    // 9. 儲存或顯示結果
	// m_OptimizedGluePath：原點為 correctedImage 左上角，單位為 pixel
    this->m_OptimizedGluePath = finalPath; // 假設你有一個成員變數儲存最終結果


    // 10. 重建顯示/輸出用座標
    // 依序產生：
    ConvertToMachineCoordinates();

#ifdef _DEBUG
    //刪除CSV檔
     DeleteFile(_T("tool_raw_path.csv"));
     DeleteFile(_T("tool_final_glue_path.csv"));
     DeleteFile(_T("tool_optimized_glue_path.csv"));
     DeleteFile(_T("tool_machine_glue_path.csv"));
	 DeleteFile(_T("tool_machine_glue_path_mm.csv"));
	 DeleteFile(_T("tool_HMIGluePath_temp.csv"));
	 DeleteFile(_T("tool_HMIGluePath.csv"));

	 ExportToolPathAsCSV(this->toolPath, "tool_raw_path.csv");  // 原始工具路徑：原點為影像左上角，單位為 pixel
     ExportGluePathAsToolCSV(finalPath, "tool_final_glue_path.csv", "pixel");
     ExportGluePathAsToolCSV(this->m_OptimizedGluePath, "tool_optimized_glue_path.csv", "pixel");
     ExportGluePathAsToolCSV(this->m_machineGluePath, "tool_machine_glue_path.csv", "pixel");
     ExportGluePathAsToolCSV(this->m_machineGluePath_mm, "tool_machine_glue_path_mm.csv", "mm");
     ExportGluePathAsToolCSV(this->m_HMIGluePath_temp, "tool_HMIGluePath_temp.csv", "mm x10");
     ExportGluePathAsToolCSV(this->m_HMIGluePath, "tool_HMIGluePath.csv", "integer display coordinate");

    // 僅 Debug 模式顯示 OpenCV 視窗，方便開發階段檢查路徑正確性。
    // 此視窗只用來比對「計算出的路徑」和「取路徑時使用的影像(imgClone)」之相對位置，
    // 不涉及 ConvertToMachineCoordinates() 的 HMI 定義轉換。
    cv::Mat displayImg = imgClone.clone();
    if (displayImg.channels() == 1) {
        cv::cvtColor(displayImg, displayImg, cv::COLOR_GRAY2BGR);
    }
    else if (displayImg.channels() == 4) {
        cv::cvtColor(displayImg, displayImg, cv::COLOR_BGRA2BGR);
    }

    ToolPath displayToolPath = this->toolPath;
    GluePath displayPath = finalPath;

    /*
     if (usedCalibrationCorrection) {
        // toolPath / finalPath 目前都是 correctedImage 的顯示座標系；
        // Debug 視窗要疊在 imgClone(pathSourceImage 座標系) 上，因此翻回取路徑時的影像方向。
        ApplyConfiguredFlipToToolPath(displayToolPath, correctedImage.size(), imgFlip);

        // finalPath 目前是 correctedImage 的顯示座標系；
        // Debug 視窗要疊在 imgClone(pathSourceImage 座標系) 上，因此翻回取路徑時的影像方向。
        ApplyConfiguredFlipToGluePath(displayPath, correctedImage.size(), imgFlip);
    }
    */
   

    // 原始 toolPath：黃色，先確認抓路徑本身和 imgClone 是否對齊
    for (const auto& pt : displayToolPath.Path) {
        cv::circle(displayImg,
            cv::Point(cvRound(pt.x), cvRound(pt.y)),
            2, cv::Scalar(0, 255, 255), cv::FILLED);
    }

    // 建議：把 cv::Point2d 轉成整數座標再畫，避免 OpenCV 警告
    for (const auto& pt : displayPath.PathLeft) {
        cv::circle(displayImg,
            cv::Point(cvRound(pt.x), cvRound(pt.y)),
            4, cv::Scalar(0, 0, 255), cv::FILLED);
    }

     //可選：畫右側路徑（綠色）做對照
     for (const auto& pt : displayPath.PathRight) {
         cv::circle(displayImg, cv::Point(cvRound(pt.x), cvRound(pt.y)), 
                    4, cv::Scalar(0, 255, 0), cv::FILLED);
     }

    // Debug 視窗以實際設備觀察方向顯示，顯示前將整張疊圖旋轉 180 度。
    //cv::flip(displayImg, displayImg, -1);

    cv::imshow("Optimized Glue Path (Debug only)", displayImg);
    cv::waitKey(0);
#endif


    // 觸發重繪或更新 UI
    Invalidate(FALSE);
}


void WorkTab::OnBnClickedIdcWorkLoadImg()
{
    // TODO: 在此加入控制項告知處理常式
    // 載入新影像前先清除上一張圖的所有路徑資料，避免舊資料殘留。
    this->toolPath.Path.clear();
    this->m_OptimizedGluePath.PathLeft.clear();
    this->m_OptimizedGluePath.PathRight.clear();
    this->m_machineGluePath.PathLeft.clear();
    this->m_machineGluePath.PathRight.clear();
    this->m_machineGluePath_mm.PathLeft.clear();
    this->m_machineGluePath_mm.PathRight.clear();
    this->m_HMIGluePath_temp.PathLeft.clear();
    this->m_HMIGluePath_temp.PathRight.clear();
    this->m_HMIGluePath.PathLeft.clear();
    this->m_HMIGluePath.PathRight.clear();

	//Add Dialog Box to load image
	CString strFilter = _T("Image Files (*.bmp;*.jpg;*.jpeg;*.png;*.tif;*.tiff)|*.bmp;*.jpg;*.jpeg;*.png;*.tif;*.tiff|All Files (*.*)|*.*||");
	CFileDialog dlg(TRUE, NULL, NULL, OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY, strFilter, this);
	if (dlg.DoModal() == IDOK)
	{
		CString strPath = dlg.GetPathName();
		// Convert CString to std::string
		std::string strPathA = CT2A(strPath);
		// Load the image
		m_mat = cv::imread(strPathA, cv::IMREAD_GRAYSCALE);
		// Display the image
		//ShowImageOnPictureControl();
        // 紅色實線
        ShowImageOnPictureControl(false, cv::Scalar(0, 0, 255, 255), 2, CrossStyle::Solid);

	}
}


void WorkTab::OnBnClickedIdcWorkSaveImg()
{
    // TODO: 在此加入控制項告知處理常式
	//Add Dialog Box to save image
	CString strFilter = _T("Image Files (*.bmp;*.jpg;*.jpeg;*.png;*.tif;*.tiff)|*.bmp;*.jpg;*.jpeg;*.png;*.tif;*.tiff|All Files (*.*)|*.*||");
	CFileDialog dlg(FALSE, NULL, NULL, OFN_PATHMUSTEXIST | OFN_HIDEREADONLY, strFilter, this);
	if (dlg.DoModal() == IDOK)
	{
		CString strPath = dlg.GetPathName();
       
		// Convert CString to std::string
		std::string strPathA = CT2A(strPath);
		// Save the image
		cv::imwrite(strPathA, m_mat);
	}
}

//Get Tools Path from image
void WorkTab::GetToolPathData(cv::Mat& ImgSrc, cv::Point2d Offset, ToolPath& toolpath)
{
	//Call the function to get the tool path from UAX
	//ImgSrc: Source Image
	//Offset: Offset of the tool path
	//toolpath: Tool Path
	toolpath.Path.clear();

    //Offset value is pixel
  	GetToolPath(ImgSrc, Offset, toolpath);

}

void WorkTab::OnBnClickedIdcWorkGo()
{
    // 1. 取得全域資源與父視窗指標
    CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
    if (!pParentWnd) {
        AfxMessageBox(_T("無法獲取父視窗資源。"));
        return;
    }

    // 2. 數據合法性檢查：確保已經過路徑優化且點位不為空
    if (m_OptimizedGluePath.PathLeft.empty() || m_OptimizedGluePath.PathRight.empty()) {
        AfxMessageBox(_T("無效的路徑資料，請先執行路徑生成。"));
        return;
    }

    // 3. 每次送出前都依最新的 m_OptimizedGluePath 重建衍生路徑：
    //    m_OptimizedGluePath -> m_machineGluePath -> m_machineGluePath_mm
    //    -> m_HMIGluePath_temp -> m_HMIGluePath
    // HMI 最終要接收的是 m_HMIGluePath.PathRight / m_HMIGluePath.PathLeft，
    // 因此不能只靠 size 判斷是否重建，否則內容變了但筆數相同時會送出舊資料。
    ConvertToMachineCoordinates();

    if (m_machineGluePath.PathLeft.empty() || m_machineGluePath.PathRight.empty() ||
        m_HMIGluePath.PathLeft.empty() || m_HMIGluePath.PathRight.empty()) {
        AfxMessageBox(_T("路徑座標轉換失敗，無法產生 Machine/HMI 路徑。"));
        return;
    }

    /*
     //增加測試功能：將優化後的膠路點位寫入 PLC (AX-3 系列) 的 Modbus TCP 寄存器中
 //讀、寫 Address 145, 146,147
    uint16_t test_data[3] = { 0 };

    int rc = modbus_read_registers(pParentWnd->m_modbusCtx, 145, 3, test_data);

    if (rc == -1) {
        CString err;
        err.Format(_T("讀取失敗！錯誤: %S"), modbus_strerror(errno));
        MessageBox(err);
        return;
    }

    CString msg;
    msg.Format(_T("讀取成功！\n數據: %u, %u, %u"), test_data[0], test_data[1], test_data[2]);

    MessageBox(msg);
	return; // 測試完成後直接返回，正式使用時請移除這行
    */

    // 4. 定義 HMI 暫存器位址 (對應 HMI 內部配置)
    constexpr int kAxisStartX1 = 14; // 右手 X 軸路徑起始位址 (D14~D43)
    constexpr int kAxisStartY = 44; // 共有 Y 軸路徑起始位址 (D44~D73)
    constexpr int kAxisStartX2 = 74; // 左手 X 軸路徑起始位址 (D74~D103)
    constexpr int kAxisCount = 30; // HMI 陣列預留長度
    // 5. 計算實際寫入點數 (取 HMI 長度與路徑資料長度的最小值，防止陣列越界)
    const size_t pointCount = (std::min)(
        static_cast<size_t>(kAxisCount),
        (std::min)(m_HMIGluePath.PathRight.size(), m_HMIGluePath.PathLeft.size()));

    if (pointCount == 0) {
        AfxMessageBox(_T("優化後的點位數為 0。"));
        return;
    }

    // 6. 準備 Modbus 寫入緩衝區 (uint16_t 格式)
    std::vector<uint16_t> x1Regs(kAxisCount, 0);
    std::vector<uint16_t> yRegs(kAxisCount, 0);
    std::vector<uint16_t> x2Regs(kAxisCount, 0);

    // 內部 Lambda：處理座標轉換、四捨五入與 16-bit 數值限幅
    auto toReg = [](double v) -> uint16_t {
        long val = lround(v); // 四捨五入為長整數
        if (val < 0) val = 0;
        if (val > 65535) val = 65535; // 確保不超出 uint16 範圍
        return static_cast<uint16_t>(val);
        };

  
    /*
    // 6. 資料轉換：將視覺座標轉換為 PLC 寄存器格式
    for (size_t i = 0; i < pointCount; ++i) {
        x1Regs[i] = toReg(m_OptimizedGluePath.PathRight[i].x);
        yRegs[i] = toReg(m_OptimizedGluePath.PathRight[i].y);
        x2Regs[i] = toReg(m_OptimizedGluePath.PathLeft[i].x);
    }
    */
    // 7. 資料轉換：將最終要送給 HMI 的路徑資料轉成寄存器格式。
    //    左側路徑 -> X1 / Y
    //    右側路徑 -> X2
    //    依 HMI Data 定義，左右 X 軸送出前都轉為正值。
    //    這裡使用的資料來源是 m_HMIGluePath.PathLeft / m_HMIGluePath.PathRight。
    for (size_t i = 0; i < pointCount; ++i) 
    {
        x1Regs[i] = toReg(std::abs(m_HMIGluePath.PathLeft[i].x));
        yRegs[i] = toReg(m_HMIGluePath.PathLeft[i].y);
        x2Regs[i] = toReg(std::abs(m_HMIGluePath.PathRight[i].x));
    }

    // 8. Modbus 連線檢查與自動重連邏輯
    const int stationID = 1;
    if (!pParentWnd->m_modbusCtx) {
        bool ok = pParentWnd->InitModbusWithRetry(
            pParentWnd->m_SystemPara.IpAddress,
            pParentWnd->Port,
            stationID,
            3,    // 重試次數
            1000  // 超時 (ms)
        );
        if (!ok) {
            AfxMessageBox(_T("Modbus TCP 連線失敗，請檢查網路設定。"));
            return;
        }
    }

    // 9. 執行執行緒安全寫入操作
    {
        // 鎖定 Mutex，避免多執行緒同時競爭同一 Modbus 句柄 (Handle)
        std::lock_guard<std::mutex> lock(pParentWnd->m_modbusMutex);
        modbus_set_slave(pParentWnd->m_modbusCtx, stationID);

        // 分別寫入 X1, Y, X2 三組路徑陣列到 PLC
        // 寫入 X1 軸
        if (modbus_write_registers(pParentWnd->m_modbusCtx, kAxisStartX1, kAxisCount, x1Regs.data()) == -1) {
            CString err;
            err.Format(_T("寫入 X1 路徑失敗: %S"), modbus_strerror(errno));
            AfxMessageBox(err);
            return;
        }

        // 寫入 Y 軸
        if (modbus_write_registers(pParentWnd->m_modbusCtx, kAxisStartY, kAxisCount, yRegs.data()) == -1) {
            CString err;
            err.Format(_T("寫入 Y 路徑失敗: %S"), modbus_strerror(errno));
            AfxMessageBox(err);
            return;
        }

        // 寫入 X2 軸
        if (modbus_write_registers(pParentWnd->m_modbusCtx, kAxisStartX2, kAxisCount, x2Regs.data()) == -1) {
            CString err;
            err.Format(_T("寫入 X2 路徑失敗: %S"), modbus_strerror(errno));
            AfxMessageBox(err);
            return;
        }
    } // 離開 Scope 自動釋放 Lock

    // 10. 完成提示
    CString doneMsg;
    doneMsg.Format(_T("HMI 路徑已成功透過 Modbus TCP 傳送。點數=%u (最大限制 %d)。"), static_cast<unsigned>(pointCount), kAxisCount);
    AfxMessageBox(doneMsg);
}

void WorkTab::ToolPathTransform32(ToolPath ToolPapath_Ori, uint16_t* m_ToolPathData)
{
    if (!m_ToolPathData || ToolPapath_Ori.Path.empty()) return;

    // 三點對應（像素 → 世界）
    //float imagePts[] = { 1097, 1063, 1373, 1063, 1371, 945 };
    //float worldPts[] = { 34.79f, 205.19f, 187.19f, 205.19f, 187.19f, 141.79f };

	// imagePts (1035, 844) → worldPts (-0.01,67.59)
	// imagePts (1311, 1247) → worldPts (150.79, 288.83)
	// imagePts (1511, 963) → worldPts (259.71, 134.03)
	//float imagePts[] = { 1035.0f, 844.0f, 1311.0f, 1247.0f, 1511.0f, 963.0f };
	//float worldPts[] = { -0.01f, 67.59f, 150.79f, 288.83f, 259.71f, 134.03f };

    float imagePts[] = { 1146.0f, 1087.0f, 1526.0f, 1086.0f, 1425.0f, 904.0f };
    float worldPts[] = { 59.44f, 203.0f, 266.44f, 203.0f, 266.44f, 103.68f };


    constexpr float scaleFactor = 100.0f; // mm → 整數

    // 計算仿射矩陣
    cv::Mat AffineMatrix;
    InitTransformer(imagePts, worldPts, 3, AffineMatrix);

    for (size_t i = 0; i < ToolPapath_Ori.Path.size(); ++i)
    {
        float x_mm = 0.0f, y_mm = 0.0f;
        PixelToWorld(ToolPapath_Ori.Path[i].x, ToolPapath_Ori.Path[i].y, x_mm, y_mm, AffineMatrix);

        // 放大並取整
        int32_t x_int = static_cast<int32_t>(std::lround(x_mm * scaleFactor));
        int32_t y_int = static_cast<int32_t>(std::lround(y_mm * scaleFactor));

        // 維持二補數(負數可正確拆分)
        uint32_t x_u = static_cast<uint32_t>(x_int);
        uint32_t y_u = static_cast<uint32_t>(y_int);

        size_t base = i * 4;
        m_ToolPathData[base + 0] = static_cast<uint16_t>(x_u & 0xFFFFu); // X low
        m_ToolPathData[base + 1] = static_cast<uint16_t>((x_u >> 16) & 0xFFFFu); // X high
        m_ToolPathData[base + 2] = static_cast<uint16_t>(y_u & 0xFFFFu);  // Y low
        m_ToolPathData[base + 3] = static_cast<uint16_t>((y_u >> 16) & 0xFFFFu);  //Y hight
    }
}


/*
void WorkTab::ToolPathTransform32A(ToolPath ToolPapath_Ori, uint16_t* m_ToolPathData, float z_Machining, float z_Retract)
{
    if (!m_ToolPathData || ToolPapath_Ori.Path.empty()) return;

    ToolPath ToolPapath_Temp = ToolPapath_Ori;

    float toolPathTemp[20000]; //暫存陣列

    // 三點對應（像素 → 世界）
    //float imagePts[] = { 1097, 1063, 1373, 1063, 1371, 945 };
    //float worldPts[] = { 34.79f, 205.19f, 187.19f, 205.19f, 187.19f, 141.79f };

    // imagePts (1035, 844) → worldPts (-0.01,67.59)
    // imagePts (1311, 1247) → worldPts (150.79, 288.83)
    // imagePts (1511, 963) → worldPts (259.71, 134.03)
    float imagePts[] = { 1035.0f, 844.0f, 1311.0f, 1247.0f, 1511.0f, 963.0f };
    float worldPts[] = { -0.01f, 67.59f, 150.79f, 288.83f, 259.71f, 134.03f };

    constexpr float scaleFactor = 100.0f; // mm → 整數

    // 計算仿射矩陣
    cv::Mat AffineMatrix;
    InitTransformer(imagePts, worldPts, 3, AffineMatrix);

    for (size_t i = 0; i < ToolPapath_Ori.Path.size(); ++i)
    {
        float x_mm = 0.0f, y_mm = 0.0f;
        PixelToWorld(ToolPapath_Ori.Path[i].x, ToolPapath_Ori.Path[i].y, x_mm, y_mm, AffineMatrix);

        // 放大並取整
        int32_t x_int = static_cast<int32_t>(std::lround(x_mm * scaleFactor));
        int32_t y_int = static_cast<int32_t>(std::lround(y_mm * scaleFactor));

        // 維持二補數(負數可正確拆分)
        uint32_t x_u = static_cast<uint32_t>(x_int);
        uint32_t y_u = static_cast<uint32_t>(y_int);

        size_t base = i * 4;
        m_ToolPathData[base + 0] = static_cast<uint16_t>(x_u & 0xFFFFu); // X low
        m_ToolPathData[base + 1] = static_cast<uint16_t>((x_u >> 16) & 0xFFFFu); // X high
        m_ToolPathData[base + 2] = static_cast<uint16_t>(y_u & 0xFFFFu);  // Y low
        m_ToolPathData[base + 3] = static_cast<uint16_t>((y_u >> 16) & 0xFFFFu);  //Y hight
    }
}
*/

inline void AppendPointSafe(uint16_t* data, size_t& idx, size_t capacity,
    int32_t x, int32_t y, int32_t z) {
    if (idx + 6 > capacity) {
        throw std::runtime_error("Output buffer overflow in AppendPointSafe");
    }
    // 處理負值：添加偏移，使其正（假設最小值-100mm，scale後-10000，偏移+10000）
    constexpr int32_t offset = 10000;  // 根據實際範圍調整
    uint32_t x_u = static_cast<uint32_t>(x + offset);
    uint32_t y_u = static_cast<uint32_t>(y + offset);
    uint32_t z_u = static_cast<uint32_t>(z + offset);  // z可能負？
    data[idx++] = static_cast<uint16_t>(x_u & 0xFFFF);
    data[idx++] = static_cast<uint16_t>((x_u >> 16) & 0xFFFF);
    data[idx++] = static_cast<uint16_t>(y_u & 0xFFFF);
    data[idx++] = static_cast<uint16_t>((y_u >> 16) & 0xFFFF);
    data[idx++] = static_cast<uint16_t>(z_u & 0xFFFF);
    data[idx++] = static_cast<uint16_t>((z_u >> 16) & 0xFFFF);
}



void WorkTab::ToolPathTransform32A(ToolPath pathOri, uint16_t* outData, size_t outCapacity, float z_Machining, float zRetract) {
    if (!outData || pathOri.Path.empty() || pathOri.Path.size() != pathOri.numClusters.size()) {
        throw std::invalid_argument("Invalid input in ToolPathTransform32A");
        return;
    }

    // 定義縮放因子：將 mm 轉換為整數 (x100)
    float scaleFactor = 100.0f;

    // 定義三點對應的像素點和世界座標點（用於仿射轉換）
    static float imagePts[] = { 1035, 844, 1311, 1247, 1511, 963 };  // 像素點座標
    static float worldPts[] = { -0.01f, 67.59f, 150.79f, 288.83f, 259.71f, 134.03f };  // 對應世界座標 (mm)

    // 靜態初始化仿射矩陣：僅計算一次，提高效率
    static cv::Mat affine = []() {
        cv::Mat mat;
        InitTransformer(imagePts, worldPts, 3, mat);  // 計算仿射轉換矩陣
        return mat;
        }();

    // 預計算所有點的世界座標：避免在迴圈中重複計算，提高效率
    // 變更: 儲存縮放後的 int32_t 座標，而不是轉換後的 float
    std::vector<std::pair<int32_t, int32_t>> worldCoords(pathOri.Path.size());
    for (size_t i = 0; i < pathOri.Path.size(); ++i) {
        float x_mm = 0.0f;
        float y_mm = 0.0f;
        PixelToWorld(static_cast<float>(pathOri.Path[i].x),
            static_cast<float>(pathOri.Path[i].y),
            x_mm,
            y_mm,
            affine);

        // 放大並取整
        int32_t x_int = static_cast<int32_t>(std::lround(x_mm * scaleFactor));
        int32_t y_int = static_cast<int32_t>(std::lround(y_mm * scaleFactor));

        // 變更: 直接儲存 int32_t
        worldCoords[i] = { x_int, y_int };
    }

    // 計算分群變換次數：用於預估輸出數據大小
    size_t numClustersChanges = 0;
    for (size_t i = 1; i < pathOri.Path.size(); ++i) {
        if (pathOri.numClusters[i] != pathOri.numClusters[i - 1]) ++numClustersChanges;
    }
    size_t totalPoints = pathOri.Path.size() + numClustersChanges;
    if (totalPoints * 6 > outCapacity) {
        throw std::runtime_error("Insufficient output capacity");
    }

    size_t idx = 0;
    for (size_t i = 0; i < pathOri.Path.size(); ++i) {
        if (i > 0 && pathOri.numClusters[i] != pathOri.numClusters[i - 1]) {
            auto& prev = worldCoords[i - 1];
            auto& curr = worldCoords[i];
            int32_t mx_int = (prev.first + curr.first) / 2;
            int32_t my_int = (prev.second + curr.second) / 2;
            int32_t zRet_int = static_cast<int32_t>(std::lround(zRetract * scaleFactor));
            AppendPointSafe(outData, idx, outCapacity, mx_int, my_int, zRet_int);
        }
        auto& curr = worldCoords[i];
        int32_t x_int = curr.first;
        int32_t y_int = curr.second;
        int32_t zWork_int = static_cast<int32_t>(std::lround(z_Machining * scaleFactor));
        AppendPointSafe(outData, idx, outCapacity, x_int, y_int, zWork_int);
    }

    // --- 新增 Debug 輸出檢查（改用 OutputDebugStringA）---
#ifdef _DEBUG
    {
        std::string debugOutput;
        debugOutput.reserve(8192);  // 預留足夠空間避免頻繁重新配置

        debugOutput += "\n-------------------------------------------------------------------------------------------------------------------------------------------\n";
        debugOutput += "\n--- ToolPathTransform32A(ToolPath ToolPath_Ori, float z_Machining, float zRetract)  ---\n";

        // 每個點由 6 個 uint16_t 組成
        size_t numPoints = idx / 6;

        char buffer[256];

        for (size_t i = 0; i < numPoints; ++i) {
            size_t offset = i * 6;

            // 重新組合 X、Y、Z (little-endian)
            uint32_t x_u32 = (static_cast<uint32_t>(m_ToolPathDataA[offset + 1]) << 16) |
                static_cast<uint32_t>(m_ToolPathDataA[offset + 0]);
            int32_t x_int32 = static_cast<int32_t>(x_u32);

            uint32_t y_u32 = (static_cast<uint32_t>(m_ToolPathDataA[offset + 3]) << 16) |
                static_cast<uint32_t>(m_ToolPathDataA[offset + 2]);
            int32_t y_int32 = static_cast<int32_t>(y_u32);

            uint32_t z_u32 = (static_cast<uint32_t>(m_ToolPathDataA[offset + 5]) << 16) |
                static_cast<uint32_t>(m_ToolPathDataA[offset + 4]);
            int32_t z_int32 = static_cast<int32_t>(z_u32);

            // 還原成 mm（除以 100）
            float x_mm = static_cast<float>(x_int32) / scaleFactor;
            float y_mm = static_cast<float>(y_int32) / scaleFactor;
            float z_mm = static_cast<float>(z_int32) / scaleFactor;

            // 使用 snprintf 格式化（比 std::stringstream 更快且不會有 locale 問題）
            int len = snprintf(buffer, sizeof(buffer),
                "Point %zu: X=%.3f mm (%d), Y=%.3f mm (%d), Z=%.3f mm (%d)\n",
                i, x_mm, x_int32, y_mm, y_int32, z_mm, z_int32);

            if (len > 0) {
                debugOutput.append(buffer, static_cast<size_t>(len));
            }
        }

        debugOutput += "-----------------------   ToolPathTransform32A()     END -----------------------------------------\n";

        // 一次性輸出，避免 OutputDebugStringA 被呼叫太多次（效能較好）
        OutputDebugStringA(debugOutput.c_str());
    }
#endif
}

void WorkTab::ToolPathTransform(ToolPath& toolpath, uint16_t* m_ToolPathData)
{
    //Convert toolPath to m_ToolPathData[20000]
    //toolPath: Tool Path
    //m_ToolPathData: Tool Path Data Array
    //toolPath.Path : Path of the tool
    //Convert toolPath.Path to m_ToolPathData[20000]
    int sizeOfToolPath = toolpath.Path.size();
    for (int i = 0; i < sizeOfToolPath; i++)
    {
        m_ToolPathData[i] = toolpath.Path[i].x;
        m_ToolPathData[i + 1] = toolpath.Path[i].y;
    }
}

//Send Tool Path Data to PLC with Modbus TCP
//int* m_ToolPathData: Tool Path Data Array
void WorkTab::SendToolPathData(uint16_t *m_ToolPathData, int sizeOfArray, int stationID)
{
    const int maxBatchSize = 100;
    const int maxModbusBatchSize = MODBUS_MAX_WRITE_REGISTERS; // 123

    CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
    if (pParentWnd == nullptr) {
        AfxMessageBox(_T("Parent window is NULL."));
        return;
    }

    // Ensure modbus ctx available; try to init if not
    if (!pParentWnd->m_modbusCtx) {
        bool ok = pParentWnd->InitModbusWithRetry(pParentWnd->m_SystemPara.IpAddress, pParentWnd->Port, stationID, 3, 1000);
        if (!ok) {
            AfxMessageBox(_T("Failed to initialize Modbus connection."));
            return;
        }
    }

    // Use parent ctx under mutex
    {
        std::lock_guard<std::mutex> lock(pParentWnd->m_modbusMutex);

        // optional: set slave id on context (harmless if already set)
        modbus_set_slave(pParentWnd->m_modbusCtx, stationID);

        // Write a control bit if needed (kept from original)
        modbus_write_bit(pParentWnd->m_modbusCtx, 0, TRUE);

        int index = 0;
        while (index < sizeOfArray) 
        {
            int batchSize = (sizeOfArray - index > maxBatchSize) ? maxBatchSize : (sizeOfArray - index);
            batchSize = (std::min)(batchSize, maxModbusBatchSize);

            int rc = modbus_write_registers(pParentWnd->m_modbusCtx, index, batchSize, &m_ToolPathData[index]);
            if (rc == -1) 
            {
                CString err;
                err.Format(_T("Failed to write registers at %d: %S"), index, modbus_strerror(errno));
                AfxMessageBox(err);
                return;
            }
            index += batchSize;
        }
    } // unlock here

    // 不關閉或釋放 pParentWnd->m_modbusCtx（由主視窗管理）
}

void WorkTab::SendToolPathDataA(uint16_t* m_ToolPathData, int sizeOfArray, int stationID)
{
    const int maxBatchSize = 100;
    const int maxModbusBatchSize = MODBUS_MAX_WRITE_REGISTERS; // 123
    const int regsPerPointPerAxis = 2;
    const int maxBatchPoints = maxModbusBatchSize / regsPerPointPerAxis;

    CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
    if (pParentWnd == NULL) {
        AfxMessageBox(_T("Parent window is NULL."));
        return;
    }

    if (!pParentWnd->m_modbusCtx) {
        bool ok = pParentWnd->InitModbusWithRetry(pParentWnd->m_SystemPara.IpAddress, pParentWnd->Port, stationID, 3, 1000);
        if (!ok) {
            AfxMessageBox(_T("Failed to initialize Modbus connection."));
            return;
        }
    }

    {
        std::lock_guard<std::mutex> lock(pParentWnd->m_modbusMutex);
        modbus_set_slave(pParentWnd->m_modbusCtx, stationID);

        // write total count to PLC address 40026 (preserve original intent)
        int rc = modbus_write_register(pParentWnd->m_modbusCtx, 40026, sizeOfArray);
        if (rc == -1) {
            CString err;
            err.Format(_T("Failed to write total count: %S"), modbus_strerror(errno));
            AfxMessageBox(err);
            return;
        }

        int index = 0;
        while (index < sizeOfArray)
        {
            int remainingPoints = sizeOfArray - index;
            // 每批最多傳輸 maxBatchPoints 點
            int batchPoints = (std::min)(maxBatchPoints, remainingPoints);

            // 單軸傳輸的寄存器數量 (每個點 2 個寄存器)
            const int batchRegsPerAxis = batchPoints * regsPerPointPerAxis;

            // 預先分配空間，用於存放分軸後的數據
            std::vector<uint16_t> xRegs(batchRegsPerAxis);
            std::vector<uint16_t> yRegs(batchRegsPerAxis);
            std::vector<uint16_t> zRegs(batchRegsPerAxis);

            // 填入本批次資料：從原始陣列中將 X, Y, Z 分離
            for (int i = 0; i < batchPoints; ++i)
            {
                size_t base = (index + i) * 6;
                int regIndex = i * 2;

                xRegs[regIndex + 0] = m_ToolPathDataA[base + 0]; // X 低位
                xRegs[regIndex + 1] = m_ToolPathDataA[base + 1]; // X 高位

                yRegs[regIndex + 0] = m_ToolPathDataA[base + 2]; // Y 低位
                yRegs[regIndex + 1] = m_ToolPathDataA[base + 3]; // Y 高位

                zRegs[regIndex + 0] = m_ToolPathDataA[base + 4]; // Z 低位
                zRegs[regIndex + 1] = m_ToolPathDataA[base + 5]; // Z 高位
            }

            // 計算當前批次的寫入偏移量 (每個座標佔 2 個寄存器)
            int writeOffset = index * 2;

            // 寫入 X (0 + writeOffset)
            if (modbus_write_registers(pParentWnd->m_modbusCtx, 0 + writeOffset, batchRegsPerAxis, xRegs.data()) == -1) {
                CString err;
                err.Format(_T("Failed to write X block at %d: %S"), writeOffset, modbus_strerror(errno));
                AfxMessageBox(err);
                return;
            }

            // 寫入 Y (2 + writeOffset)
            if (modbus_write_registers(pParentWnd->m_modbusCtx, 2 + writeOffset, batchRegsPerAxis, yRegs.data()) == -1) {
                CString err;
                err.Format(_T("Failed to write Y block at %d: %S"), writeOffset, modbus_strerror(errno));
                AfxMessageBox(err);
                return;
            }

            // 寫入 Z (4 + writeOffset)
            if (modbus_write_registers(pParentWnd->m_modbusCtx, 4 + writeOffset, batchRegsPerAxis, zRegs.data()) == -1) {
                CString err;
                err.Format(_T("Failed to write Z block at %d: %S"), writeOffset, modbus_strerror(errno));
                AfxMessageBox(err);
                return;
            }

            index += batchPoints;
        }
    } // unlock
}

void WorkTab::OnBnClickedCheckWorkCenter()
{
    // TODO: 在此加入控制項告知處理常式程式碼
	// assign value of IDC_CHECK_WORK_CENTER to flgCenter
	CButton* pCheckBox = (CButton*)GetDlgItem(IDC_CHECK_WORK_CENTER);
    if (pCheckBox->GetCheck() == BST_CHECKED)
    {
        flgCenter = true;
    }
    else
    {
        flgCenter = false;
	}
	//觸發重繪
	Invalidate();
    if (!m_mat.empty()) {
        ShowImageOnPictureControl(flgCenter, cv::Scalar(0, 0, 255, 255), 1, CrossStyle::Solid);
    }
    if (pWnd && ::IsWindow(pWnd->GetSafeHwnd())) {
        pWnd->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
    }

}

void WorkTab::OnBnClickedWorkImageProcess()
{
    if (m_mat.empty()) {
        AfxMessageBox(_T("目前沒有可顯示的影像。"));
        return;
    }

    ImagePro dlg(this);
    dlg.SetImage(m_mat);
    dlg.DoModal();

}

void WorkTab::OnBnClickedMfcbtnWorkImgCalibrate()
{
   // 開啟檔案對話框取得校正影像
#ifdef _WIN32
    try {
        std::vector<std::string> files = m_vision.selectCalibrationFiles();
        if (files.empty())
        {
            AfxMessageBox(L"未選取任何影像");
            return;
        }

        if (MaskWidth > 0 && MaskHeight > 0) {
            m_vision.setCalibrationROI(cv::Rect(MaskX, MaskY, MaskWidth, MaskHeight));
        }
        else {
            m_vision.clearCalibrationROI();
        }

        // 依需求調整棋盤內部角點數與邊長
        // m_vision.setCalibrationPattern(cv::Size(9, 6), 25.0f); // 可改成由 UI/成員設定
        double rms = m_vision.calibrate(files);
        if (rms < 0.0)
        {
            CString message(m_vision.getLastCalibrationMessage().c_str());
            if (message.IsEmpty()) {
                message = L"校正失敗，無法找到角點";
            }
            AfxMessageBox(message);
            return;
        }

        cv::Mat preview = m_vision.getCalibrationPreviewImage();
        if (!preview.empty())
        {
            cv::imshow("Calibration Grid Preview", preview);
            cv::waitKey(1);
        }

        // 儲存校正結果
        const std::string outFile = GetCalibrationFilePath();
        bool saved = m_vision.saveCalibrationData(outFile);
        if (!saved)
        {
            AfxMessageBox(L"校正資料儲存失敗");
            return;
        }

        std::wstring outFileW(outFile.begin(), outFile.end());
        CString msg;
        msg.Format(L"校正完成，RMS=%.3f px\n儲存於: %s", rms, outFileW.c_str());
        AfxMessageBox(msg);
    }
    catch (const cv::Exception& e) {
        CString message;
        message.Format(L"OpenCV 校正失敗:\n%S", e.what());
        AfxMessageBox(message);
    }
    catch (const std::exception& e) {
        CString message;
        message.Format(L"校正失敗:\n%S", e.what());
        AfxMessageBox(message);
    }
#else
    AfxMessageBox(L"此功能僅支援 Windows 平台");
#endif
}

// 新增：讀取 Holding Registers
bool WorkTab::ReadModbusRegisters(int startAddress, int numRegisters, std::vector<uint16_t>& outRegs, int stationID)
{
    CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(AfxGetMainWnd());
    if (!pParentWnd) {
        AfxMessageBox(_T("Parent window is NULL."));
        return false;
    }
    if (!pParentWnd->m_modbusCtx) {
        bool ok = pParentWnd->InitModbusWithRetry(pParentWnd->m_SystemPara.IpAddress,
            pParentWnd->Port, stationID, 3, 1000);
        if (!ok) {
            AfxMessageBox(_T("Failed to initialize Modbus connection."));
            return false;
        }
    }

    // 這裡假設呼叫端已鎖定 m_modbusMutex
    modbus_set_slave(pParentWnd->m_modbusCtx, stationID);
    outRegs.resize(numRegisters);
    int rc = modbus_read_registers(pParentWnd->m_modbusCtx, startAddress, numRegisters, outRegs.data());
    if (rc == -1) {
        CString err;
        err.Format(_T("Failed to read registers at %d: %S"), startAddress, modbus_strerror(errno));
        AfxMessageBox(err);
        return false;
    }
    return true;
}

//HMI Test Read Holding Registers
void WorkTab::HMIReadHoldingRegistersTest(int stationID)
{
	//read 3 registers from address 145
    const int startAddress = 145;
    const int numRegisters = 3;
    std::vector<uint16_t> regs;
    CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
    if (!pParentWnd) {
        AfxMessageBox(_T("Parent window is NULL."));
        return;
    }
    // Ensure modbus ctx available; try to init if not
    if (!pParentWnd->m_modbusCtx) {
        bool ok = pParentWnd->InitModbusWithRetry(pParentWnd->m_SystemPara.IpAddress,
            pParentWnd->Port, stationID, 3, 1000);
        if (!ok) {
            AfxMessageBox(_T("Failed to initialize Modbus connection."));
            return;
        }
    }
    // Use parent ctx under mutex
    {
        std::lock_guard<std::mutex> lock(pParentWnd->m_modbusMutex);
        if (ReadModbusRegisters(startAddress, numRegisters, regs, stationID)) {
            CString msg;
            msg.Format(_T("Read Holding Registers from %d:\nReg[%d]=%d\nReg[%d]=%d\nReg[%d]=%d"),
                startAddress,
                startAddress, regs[0],
                startAddress + 1, regs[1],
                startAddress + 2, regs[2]);
            AfxMessageBox(msg);
        }
	} // unlock here
 
}

// ============================================================================
// 在 WorkTab.cpp 中實作
// ============================================================================

bool WorkTab::ReadSystemParaBatch_139_to_156(std::vector<uint16_t>& outValues, int stationID)
{
    CSPDlg* pParent = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
    if (!pParent) {
        AfxMessageBox(_T("無法取得父視窗指標"));
        return false;
    }

    if (!pParent->m_modbusCtx) {
        bool ok = pParent->InitModbusWithRetry(
            pParent->m_SystemPara.IpAddress,
            pParent->Port,
            stationID,
            3,
            1000);
        if (!ok) {
            AfxMessageBox(_T("Modbus 初始化失敗"));
            return false;
        }
    }

    std::lock_guard<std::mutex> lock(pParent->m_modbusMutex);
    modbus_set_slave(pParent->m_modbusCtx, stationID);

    const int START_ADDR = 139;
    const int COUNT = 18;

    outValues.resize(COUNT);
    if (modbus_read_registers(pParent->m_modbusCtx, START_ADDR, COUNT, outValues.data()) == -1) {
        CString err;
        err.Format(_T("批量讀取 139~156 失敗：%S"), modbus_strerror(errno));
        AfxMessageBox(err);
        return false;
    }

    return true;
}

bool WorkTab::WriteSystemParaBatch_139_to_156(const std::vector<uint16_t>& inValues, int stationID)
{
    if (inValues.size() != 18) {
        AfxMessageBox(_T("寫入資料必須正好 18 個值 (139~156)"));
        return false;
    }

    CSPDlg* pParent = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
    if (!pParent) return false;

    if (!pParent->m_modbusCtx) {
        if (!pParent->InitModbusWithRetry(pParent->m_SystemPara.IpAddress, pParent->Port, stationID, 3, 1000)) return false;
    }

    std::lock_guard<std::mutex> lock(pParent->m_modbusMutex);
    modbus_set_slave(pParent->m_modbusCtx, stationID);

    const int START_ADDR = 139;
    const int COUNT = 18;

    if (modbus_write_registers(pParent->m_modbusCtx, START_ADDR, COUNT, inValues.data()) == -1) {
        CString err;
        err.Format(_T("批量寫入 139~156 失敗：%S"), modbus_strerror(errno));
        AfxMessageBox(err);
        return false;
    }

    return true;
}


bool WorkTab::SyncReadAndUpdateSystemPara(int stationID)
{
    std::vector<uint16_t> values;
    if (!ReadSystemParaBatch_139_to_156(values, stationID)) {
        return false;
    }

    CSPDlg* pParent = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
    if (!pParent) return false;

    ApplySystemConfigRegisters(values, pParent->m_SystemPara);
    m_lastSyncedSystemPara = pParent->m_SystemPara;

    // 觸發 UI 更新（很重要！）
    Invalidate(FALSE);
    // 或呼叫 UpdateData(FALSE); 如果有對應的 DDX 控制項

    return true;
}

bool WorkTab::SyncWriteFromSystemPara(int stationID)
{
    CSPDlg* pParent = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
    if (!pParent) return false;

    std::vector<uint16_t> values;
    BuildSystemConfigRegisters(pParent->m_SystemPara, values);

    return WriteSystemParaBatch_139_to_156(values, stationID);
}

bool WorkTab::SyncReadAndUpdateMemStruct(int stationID)
{
    constexpr int kMemStructStart = 157;
    std::vector<uint16_t> values;
    if (!ReadHoldingRegistersBlock(kMemStructStart, 27, values, stationID)) {  // 增加到 27 個寄存器
        return false;
    }

    CSPDlg* pParent = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
    if (!pParent) {
        return false;
    }

    ApplyMemStructRegisters(values, pParent->m_MemStruct_SP);
    m_lastSyncedMemStruct = pParent->m_MemStruct_SP;
    return true;
}

bool WorkTab::SyncWriteFromMemStruct(int stationID)
{
    constexpr int kMemStructStart = 157;
    CSPDlg* pParent = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
    if (!pParent) {
        return false;
    }

    std::vector<uint16_t> values;
    BuildMemStructRegisters(pParent->m_MemStruct_SP, values);
    if (!WriteHoldingRegistersBlock(kMemStructStart, values, stationID)) {
        return false;
    }

    m_lastSyncedMemStruct = pParent->m_MemStruct_SP;
    return true;
}

void WorkTab::OnBnClickedCheckWorkRoi()
{
    // TODO: 在此加入控制項告知處理常式程式碼
	CButton* pCheckBox = (CButton*)GetDlgItem(IDC_CHECK_WORK_ROI);
    	if (pCheckBox) 
        {
            		m_bROIMode = (pCheckBox->GetCheck() == BST_CHECKED);
        }
        Invalidate(); // 觸發重繪
        if (!m_mat.empty()) {
            ShowImageOnPictureControl(flgCenter, cv::Scalar(0, 0, 255, 255), 1, CrossStyle::Solid);
        }
        if (pWnd && ::IsWindow(pWnd->GetSafeHwnd())) {
            pWnd->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
        }
}

// 完整座標轉換流程
// 1. m_OptimizedGluePath：原點為影像左上角，單位為 pixel
// 2. m_machineGluePath：以 referenceX/referenceY 為機械原點重構座標，X 向右為正、Y 向下為正，單位為 pixel
// 3. m_machineGluePath_mm：將 machine pixel 乘上 TransferFactor(mm/pixel)，單位為 mm
// 4. m_HMIGluePath_temp：將 machine mm 乘上 10，單位為 mm x10
// 5. m_HMIGluePath：將 HMI temp 直接取整數後得到最終 HMI 顯示座標
void WorkTab::ConvertToMachineCoordinates()
{
    CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(GetParent()->GetParent());

    // 每次重建前先清空舊資料，避免路徑重複累加。
    m_machineGluePath.PathRight.clear();
    m_machineGluePath.PathLeft.clear();
    m_machineGluePath_mm.PathRight.clear();
    m_machineGluePath_mm.PathLeft.clear();
    m_HMIGluePath_temp.PathRight.clear();
    m_HMIGluePath_temp.PathLeft.clear();
    m_HMIGluePath.PathRight.clear();
    m_HMIGluePath.PathLeft.clear();

    // TransferFactor 的單位是 mm/pixel。
    // 若系統參數尚未設定，先退回 1.0，避免除錯期間因 0 值造成異常。
    const double transferFactor =
        (pParentWnd && pParentWnd->m_SystemPara.TransferFactor > 0.0f)
        ? static_cast<double>(pParentWnd->m_SystemPara.TransferFactor)
        : 1.0;

    constexpr double HMI_TEMP_SCALE = 10.0;

    // 右側膠路：
    // optimized pixel -> machine pixel(X右正、Y下正) -> machine mm -> HMI temp(mm x10) -> HMI display(integer)
    for (const auto& pt : m_OptimizedGluePath.PathRight) {
        // 1. 以 referenceX/referenceY 當作機械原點，X 向右為正、Y 向下為正，只重構座標值。
        cv::Point2d machinePt;
        machinePt.x = pt.x - referenceX;
        machinePt.y = pt.y - referenceY;
        m_machineGluePath.PathRight.push_back(machinePt);

        // 2. 將 machine pixel 乘上 mm/pixel 係數，得到機械座標(mm)。
        cv::Point2d machinePtMm;
        machinePtMm.x = machinePt.x * transferFactor;
        machinePtMm.y = machinePt.y * transferFactor;
        m_machineGluePath_mm.PathRight.push_back(machinePtMm);

        // 3. 將機械 mm 座標乘上 10，得到 HMI 暫存座標。
        cv::Point2d hmiTemp;
        hmiTemp.x = machinePtMm.x * HMI_TEMP_SCALE;
        hmiTemp.y = machinePtMm.y * HMI_TEMP_SCALE;
        m_HMIGluePath_temp.PathRight.push_back(hmiTemp);

        // 4. 直接取整數，得到最終 HMI 顯示座標。
        cv::Point2d hmiPt;
        hmiPt.x = std::lround(hmiTemp.x);
        hmiPt.y = std::lround(hmiTemp.y);
        m_HMIGluePath.PathRight.push_back(hmiPt);
    }

    // 左側膠路與右側使用完全相同的轉換流程。
    for (const auto& pt : m_OptimizedGluePath.PathLeft) {
        // 1. optimized pixel -> machine pixel (X右正、Y下正)
        cv::Point2d machinePt;
        machinePt.x = pt.x - referenceX;
        machinePt.y = pt.y - referenceY;
        m_machineGluePath.PathLeft.push_back(machinePt);

        // 2. machine pixel -> machine mm
        cv::Point2d machinePtMm;
        machinePtMm.x = machinePt.x * transferFactor;
        machinePtMm.y = machinePt.y * transferFactor;
        m_machineGluePath_mm.PathLeft.push_back(machinePtMm);

        // 3. machine mm -> HMI temp(mm x10)
        cv::Point2d hmiTemp;
        hmiTemp.x = machinePtMm.x * HMI_TEMP_SCALE;
        hmiTemp.y = machinePtMm.y * HMI_TEMP_SCALE;
        m_HMIGluePath_temp.PathLeft.push_back(hmiTemp);

        // 4. HMI temp -> HMI display(integer)
        cv::Point2d hmiPt;
        hmiPt.x = std::lround(hmiTemp.x);
        hmiPt.y = std::lround(hmiTemp.y);
        m_HMIGluePath.PathLeft.push_back(hmiPt);
    }
}

void WorkTab::OnBnClickedMfcbtnWorkImgFactor()
{
    if (m_mat.empty()) {
        AfxMessageBox(L"請先載入影像。");
        return;
    }

    CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
    if (!pParentWnd) {
        AfxMessageBox(L"無法取得父視窗指標。");
        return;
    }

    if (!m_vision.isCalibrated() && !m_vision.loadCalibrationData(GetCalibrationFilePath())) {
        AfxMessageBox(L"無法載入校正參數檔 calibration.yml。");
        return;
    }

    GridLengthInputDialog lengthDlg(25.0, this);
    if (lengthDlg.DoModal() != IDOK) {
        return;
    }

    CWaitCursor waitCursor;

    cv::Mat rawOrientationImage = ApplyConfiguredFlip(m_mat, imgFlip);
    cv::Mat undistortedRaw;
    try {
        undistortedRaw = m_vision.undistortImage(rawOrientationImage);
    }
    catch (const cv::Exception& e) {
        CString msg;
        msg.Format(L"影像校正失敗：\n%S", e.what());
        AfxMessageBox(msg);
        return;
    }

    if (undistortedRaw.empty()) {
        AfxMessageBox(L"校正後影像為空，無法計算 Factor。");
        return;
    }

    cv::Mat gray;
    if (undistortedRaw.channels() == 1) {
        gray = undistortedRaw;
    }
    else {
        cv::cvtColor(undistortedRaw, gray, cv::COLOR_BGR2GRAY);
    }

    std::vector<cv::Point2f> corners;
    cv::Size boardSize = m_vision.getCalibrationBoardSize();
    std::vector<cv::Size> candidates = {
        boardSize,
        cv::Size(9, 6), cv::Size(8, 6), cv::Size(7, 6),
        cv::Size(11, 8), cv::Size(10, 7), cv::Size(10, 8),
        cv::Size(6, 4), cv::Size(5, 4)
    };
    candidates.erase(std::remove_if(candidates.begin(), candidates.end(),
        [](const cv::Size& s) { return s.width <= 0 || s.height <= 0; }), candidates.end());
    candidates.erase(std::unique(candidates.begin(), candidates.end(),
        [](const cv::Size& a, const cv::Size& b) { return a.width == b.width && a.height == b.height; }),
        candidates.end());

    bool found = false;
    for (const auto& candidate : candidates) {
        corners.clear();
        found = cv::findChessboardCornersSB(
            gray,
            candidate,
            corners,
            cv::CALIB_CB_EXHAUSTIVE | cv::CALIB_CB_ACCURACY);

        if (!found) {
            found = cv::findChessboardCorners(
                gray,
                candidate,
                corners,
                cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE);
            if (found) {
                cv::cornerSubPix(
                    gray,
                    corners,
                    cv::Size(11, 11),
                    cv::Size(-1, -1),
                    cv::TermCriteria(cv::TermCriteria::EPS | cv::TermCriteria::COUNT, 30, 0.1));
            }
        }

        if (found && !corners.empty()) {
            boardSize = candidate;
            m_vision.setCalibrationPattern(candidate, m_vision.getCalibrationSquareSize());
            break;
        }
    }

    if (!found || corners.empty()) {
        AfxMessageBox(L"無法在校正後影像中偵測到棋盤格角點。請確認校正板格點數是否正確，或重新拍攝更清晰的格點影像。");
        return;
    }

    cv::Mat preview;
    if (undistortedRaw.channels() == 1) {
        cv::cvtColor(undistortedRaw, preview, cv::COLOR_GRAY2BGRA);
    }
    else if (undistortedRaw.channels() == 3) {
        cv::cvtColor(undistortedRaw, preview, cv::COLOR_BGR2BGRA);
    }
    else if (undistortedRaw.channels() == 4) {
        preview = undistortedRaw.clone();
    }
    else {
        AfxMessageBox(L"不支援的影像格式。");
        return;
    }

    cv::drawChessboardCorners(preview, boardSize, corners, found);

    m_factorPreviewMat = preview;
    m_factorCorners = corners;
    m_factorBoardSize = boardSize;
    m_pendingGridLengthMm = lengthDlg.GetLengthMm();
    m_bFactorSelectMode = true;
    m_bDrawingROI = false;
    m_bROIConfirmed = false;

    Invalidate(FALSE);
    ShowImageOnPictureControl(flgCenter, cv::Scalar(0, 0, 255, 255), 1, CrossStyle::Solid);
    if (pWnd && ::IsWindow(pWnd->GetSafeHwnd())) {
        pWnd->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
    }
    waitCursor.Restore();
    AfxMessageBox(L"已切換到校正後預覽圖。請直接在主畫面的影像區拖拉框選多個格點，放開滑鼠後會自動計算 Factor。");
}
