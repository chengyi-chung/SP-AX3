// WorkTab.cpp: 實作檔案
#pragma once
#include "pch.h"
#include "SP.h"
#include "SPDlg.h"
#include "afxdialogex.h"
#include "WorkTab.h"
#include <string>
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
        double scaleFactor = std::min((double)screenWidth / m_mat.cols, (double)screenHeight / m_mat.rows);
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
    ON_BN_CLICKED(IDC_WORK_STOP_GRAB, &WorkTab::OnBnClickedWorkStopGrab)
    ON_WM_MOUSEMOVE()
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
    pDC = pWnd->GetDC();


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
    return 0;

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

// pImageBuffer, Zoom All  在Picture Control上直接显示图像的函数。
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
    if (m_mat.empty()) return;

    CRect rect;
    pWnd->GetClientRect(&rect);

    cv::Mat resizedImage;
    cv::resize(m_mat, resizedImage, cv::Size(rect.Width(), rect.Height()));

    cv::Mat imageToShow;
    cv::cvtColor(resizedImage, imageToShow, cv::COLOR_BGR2BGRA);



	//取得 ROI checkbox 狀態，決定是否繪製 Mask 矩形

	if (IsDlgButtonChecked(IDC_CHECK_WORK_ROI))
    {
        // --- 新增：繪製 Mask 矩形 ---
     // Calculate scale factors
     // Draw the rectangle if MaskWidth and MaskHeight are greater than 0
        if (MaskWidth > 0 && MaskHeight > 0) {
            double scaleX = static_cast<double>(rect.Width()) / m_mat.cols;
            double scaleY = static_cast<double>(rect.Height()) / m_mat.rows;
           

            //double scaleX = static_cast<double>(imageToShow.cols) / rect.Width();
            //double scaleY = static_cast<double>(imageToShow.rows) / rect.Height();

            int x = static_cast<int>(MaskX * scaleX);
            int y = static_cast<int>(MaskY * scaleY);
            int w = static_cast<int>(MaskWidth * scaleX);
            int h = static_cast<int>(MaskHeight * scaleY);
            cv::rectangle(imageToShow, cv::Rect(x, y, w, h), cv::Scalar(0, 255, 0, 255), 1);
        }
        // --- End ---
    }
    
	// Draw center cross
    //
    if (flgCenter)
    {

        int centerX = imageToShow.cols / 2;
        //int centerY = imageToShow.rows / 2;
        int centerY = referenceY; // *static_cast<double>(rect.Height()) / m_mat.rows; // 使用 referenceY 計算 centerY

        auto drawDashedLine = [&](cv::Point start, cv::Point end, int dashLength)
            {
                double totalLength = cv::norm(end - start);
                cv::Point2f dir = (end - start) / static_cast<float>(totalLength);

                for (double d = 0; d < totalLength; d += dashLength * 2)
                {
                    cv::Point2f p1f = cv::Point2f(start) + dir * static_cast<float>(d);
                    cv::Point2f p2f = cv::Point2f(start) + dir * static_cast<float>(std::min(d + dashLength, totalLength));

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

    BITMAPINFO bitmapInfo;
    memset(&bitmapInfo, 0, sizeof(bitmapInfo));
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biWidth = imageToShow.cols;
    bitmapInfo.bmiHeader.biHeight = -imageToShow.rows;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    ::StretchDIBits(
        pDC->GetSafeHdc(),
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

    CDialogEx::OnOK();
}


void WorkTab::OnCancel()
{
    // TODO: 在此加入特定的程式碼和 (或) 呼叫基底類別

    CDialogEx::OnCancel();
}


void WorkTab::OnMouseMove(UINT nFlags, CPoint point)
{
    // TODO: 在此加入您的訊息處理常式程式碼和 (或) 呼叫預設值

   //Set Mouse Cursor Position
	//CString str;
	//str.Format(_T("Mouse Cursor Position: (%d, %d)"), point.x, point.y);
	m_MousePos = point;

	//Invalidate();

   
    CDialogEx::OnMouseMove(nFlags, point);
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

    // 4. 計算 Offset (Pixel)
    double degree = 45.0;
    double theta = degree * CV_PI / 180.0;
    double OffsetValue = pParentWnd->m_SystemPara.OffsetValue;
    double factor = pParentWnd->m_SystemPara.TransferFactor;

    // 轉為像素單位
    cv::Point2d offsetPx(
        (OffsetValue * std::cos(theta)) / factor,
        (OffsetValue * std::sin(theta)) / factor
    );

    // 5. 準備 Mask
    cv::Mat mask = cv::Mat::zeros(m_mat.size(), CV_8UC1);
    cv::Rect roiRect(MaskX, MaskY, MaskWidth, MaskHeight);
    mask(roiRect) = cv::Scalar(255);

    // 6. 提取原始工具路徑 (直接操作成員變數)
	//m_mat: 原始影像
    this->toolPath.Path.clear();
    cv::Mat imgClone = m_mat.clone();
    GetToolPath_CurvatureOptimized_Mask(imgClone, mask, offsetPx, this->toolPath, 0.0008,false);

    if (this->toolPath.Path.empty()) {
        AfxMessageBox(_T("No path detected in ROI."));
        return;
    }

    // 7. 膠路同步優化 (核心步驟)
    ROIMask roiOpt = {};
    roiOpt.MaskX = MaskX; roiOpt.MaskY = MaskY;
    roiOpt.MaskWidth = MaskWidth; roiOpt.MaskHeight = MaskHeight;
    roiOpt.RefCenterX = this->referenceX;
    roiOpt.RefCenterY = this->referenceY;

    // 直接傳入成員變數，避免重複拷貝
    // 注意：OutputPath 建議定義為類別成員，以便後續繪圖或傳送給 PLC
    GluePath finalPath;
    OptimizeGluePath(this->toolPath.Path, roiOpt, finalPath, 2);

    // 8. 儲存或顯示結果
    this->m_OptimizedGluePath = finalPath; // 假設你有一個成員變數儲存最終結果

    // 9. 座標轉換將 m_OptimizedGluePath 轉換到 m_machineGluePath
    Point2D OriginalMachineCoord;
	OriginalMachineCoord.x = referenceX;  
	OriginalMachineCoord.y = referenceY;

    //函數將優化的膠水路徑轉換為機器座標系，通過從每個點的座標中減去給定的機器座標來實現。
	//這樣做的目的是將路徑點從圖像座標系轉換到機器座標系，確保機器能夠正確地理解和執行路徑。
	//轉換後的座標將存儲在 m_machineGluePath 中，這樣你就可以使用這些座標來控制機器的運動。
    ConvertToMachineCoordinates(OriginalMachineCoord);

#ifdef _DEBUG
    // 僅 Debug 模式顯示 OpenCV 視窗，方便開發階段檢查路徑正確性
    cv::Mat displayImg = m_mat.clone();

    // 建議：把 cv::Point2d 轉成整數座標再畫，避免 OpenCV 警告
    for (const auto& pt : finalPath.PathLeft) {
        cv::circle(displayImg,
            cv::Point(cvRound(pt.x), cvRound(pt.y)),
            2, cv::Scalar(0, 0, 255), cv::FILLED);
    }

     //可選：畫右側路徑（綠色）做對照
     for (const auto& pt : finalPath.PathRight) {
         cv::circle(displayImg, cv::Point(cvRound(pt.x), cvRound(pt.y)), 
                    2, cv::Scalar(0, 255, 0), cv::FILLED);
     }

    cv::imshow("Optimized Glue Path (Debug only)", displayImg);
    cv::waitKey(0);
#endif

    // 觸發重繪或更新 UI
    Invalidate(FALSE);
}


void WorkTab::OnBnClickedIdcWorkLoadImg()
{
    // TODO: 在此加入控制項告知處理常式
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

    // 3. 定義 PLC 暫存器位址 (對應 AX-3 系列 PLC 內部配置)
    constexpr int kAxisStartX1 = 14; // 右手 X 軸路徑起始位址 (D14~D43)
    constexpr int kAxisStartY = 44; // 共有 Y 軸路徑起始位址 (D44~D73)
    constexpr int kAxisStartX2 = 74; // 左手 X 軸路徑起始位址 (D74~D103)
    constexpr int kAxisCount = 30; // PLC 陣列預留長度

    // 4. 計算實際寫入點數 (取 PLC 長度與路徑資料長度的最小值，防止陣列越界)
    const size_t pointCount = std::min({
        static_cast<size_t>(kAxisCount),
        m_OptimizedGluePath.PathRight.size(),
        m_OptimizedGluePath.PathLeft.size()
        });

    if (pointCount == 0) {
        AfxMessageBox(_T("優化後的點位數為 0。"));
        return;
    }

    // 5. 準備 Modbus 寫入緩衝區 (uint16_t 格式)
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
    // 6. 資料轉換：將膠水機械座標轉換為 PLC 寄存器格式
    for (size_t i = 0; i < pointCount; ++i) 
    {
        x1Regs[i] = toReg(m_machineGluePath.PathRight[i].x);
        yRegs[i] = toReg(m_machineGluePath.PathRight[i].y);
        x2Regs[i] = toReg(m_machineGluePath.PathLeft[i].x);
    }

    // 7. Modbus 連線檢查與自動重連邏輯
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

    // 8. 執行執行緒安全寫入操作
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

    // 9. 完成提示
    CString doneMsg;
    doneMsg.Format(_T("路徑已成功傳送至 PLC。點數=%u (最大限制 %d)。"), static_cast<unsigned>(pointCount), kAxisCount);
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
    uint32_t xu = static_cast<uint32_t>(x + offset);
    uint32_t yu = static_cast<uint32_t>(y + offset);
    uint32_t zu = static_cast<uint32_t>(z + offset);  // z可能負？
    data[idx++] = static_cast<uint16_t>(xu & 0xFFFF);
    data[idx++] = static_cast<uint16_t>((xu >> 16) & 0xFFFF);
    data[idx++] = static_cast<uint16_t>(yu & 0xFFFF);
    data[idx++] = static_cast<uint16_t>((yu >> 16) & 0xFFFF);
    data[idx++] = static_cast<uint16_t>(zu & 0xFFFF);
    data[idx++] = static_cast<uint16_t>((zu >> 16) & 0xFFFF);
}



void WorkTab::ToolPathTransform32A(ToolPath pathOri, uint16_t* outData, size_t outCapacity, float z_Machining, float zRetract) {
    if (!outData || pathOri.Path.empty() || pathOri.Path.size() != pathOri.numClusters.size()) {
        throw std::invalid_argument("Invalid input in ToolPathTransform32A");
        return;
    }
	float scaleFactor = 100.0f; // mm → 整數
     static float imagePts[] = { 1035, 844, 1311, 1247, 1511, 963 };
     static float worldPts[] = { -0.01f, 67.59f, 150.79f, 288.83f, 259.71f, 134.03f };

    // 靜態初始化仿射矩陣，只計算一次
    static cv::Mat affine = []() {
        cv::Mat mat;
        InitTransformer(imagePts, worldPts, 3, mat);
        return mat;
        }();

    // 預計算世界座標
    std::vector<std::pair<float, float>> worldCoords(pathOri.Path.size());
    for (size_t i = 0; i < pathOri.Path.size(); ++i) {
        PixelToWorld(pathOri.Path[i].x, pathOri.Path[i].y, worldCoords[i].first, worldCoords[i].second, affine);
    }

    // 估計總點數：原始點 + 簇變更數
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
            int32_t mx_int = static_cast<int32_t>(std::lround((prev.first + curr.first) / 2 * scaleFactor));
            int32_t my_int = static_cast<int32_t>(std::lround((prev.second + curr.second) / 2 * scaleFactor));
            int32_t zRet_int = static_cast<int32_t>(std::lround(zRetract * scaleFactor));
            AppendPointSafe(outData, idx, outCapacity, mx_int, my_int, zRet_int);
        }
        auto& curr = worldCoords[i];
        int32_t x_int = static_cast<int32_t>(std::lround(curr.first * scaleFactor));
        int32_t y_int = static_cast<int32_t>(std::lround(curr.second * scaleFactor));
        int32_t zWork_int = static_cast<int32_t>(std::lround(z_Machining * scaleFactor));
        AppendPointSafe(outData, idx, outCapacity, x_int, y_int, zWork_int);
    }
}

// 內聯函數：安全附加一個點到緩衝區
// 將 int32_t 的 X, Y, Z 拆分成低/高 16 位 uint16_t，並處理負數（維持二補數）
inline void AppendPointSafeA(std::vector<uint16_t>& buffer,  // 輸出：數據緩衝區
    size_t& idx,                    // 輸入/輸出：當前索引
    int32_t x, int32_t y, int32_t z) {  // 輸入：點的 X, Y, Z 值 (已縮放)
    if (idx + 6 > buffer.size())
        throw std::runtime_error("AppendPointSafe: buffer overflow");

    const uint16_t* px = reinterpret_cast<const uint16_t*>(&x);
    const uint16_t* py = reinterpret_cast<const uint16_t*>(&y);
    const uint16_t* pz = reinterpret_cast<const uint16_t*>(&z);

    buffer[idx++] = px[0];  buffer[idx++] = px[1];
    buffer[idx++] = py[0];  buffer[idx++] = py[1];
    buffer[idx++] = pz[0];  buffer[idx++] = pz[1];
}


// 類別 WorkTab 的成員函數
// 此函數將原始工具路徑轉換為機器可讀的 uint16_t 數據格式
// 並在分群變換時插入中間點以處理 Z 軸的 retract 操作
void WorkTab::ToolPathTransform32B(ToolPath ToolPath_Ori,      // 輸入：原始工具路徑結構
                                                                   float z_Machining,          // 輸入：加工時的 Z 值 (mm)
                                                                    float zRetract) {           // 輸入：退刀時的 Z 值 (mm)
    // 輸入驗證：檢查路徑是否為空，或 Path 和 numClusters 大小是否匹配
    if (ToolPath_Ori.Path.empty() ||
        ToolPath_Ori.Path.size() != ToolPath_Ori.numClusters.size()) {
        throw std::invalid_argument("Invalid input in ToolPathTransform32A");
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
    std::vector<std::pair<int32_t, int32_t>> worldCoords(ToolPath_Ori.Path.size());
    for (size_t i = 0; i < ToolPath_Ori.Path.size(); ++i)
    {
        float x_mm = 0.0f, y_mm = 0.0f;
        PixelToWorld(ToolPath_Ori.Path[i].x, ToolPath_Ori.Path[i].y, x_mm, y_mm, affine);

        // 放大並取整
        int32_t x_int = static_cast<int32_t>(std::lround(x_mm * scaleFactor));
        int32_t y_int = static_cast<int32_t>(std::lround(y_mm * scaleFactor));

        // 變更: 直接儲存 int32_t
        worldCoords[i] = { x_int, y_int };
    }

    // 計算分群變換次數：用於預估輸出數據大小
    size_t numClustersChanges = 0;
    for (size_t i = 1; i < ToolPath_Ori.Path.size(); ++i) {
        if (ToolPath_Ori.numClusters[i] != ToolPath_Ori.numClusters[i - 1]) ++numClustersChanges;
    }
    size_t totalPoints = ToolPath_Ori.Path.size() + numClustersChanges;
	int outCapacity = static_cast<int>(m_ToolPathDataA.size());
    if (totalPoints * 6 > outCapacity) {
        throw std::runtime_error("Insufficient output capacity");
    }

    size_t idx = 0;
    for (size_t i = 0; i < ToolPath_Ori.Path.size(); ++i) {
        if (i > 0 && ToolPath_Ori.numClusters[i] != ToolPath_Ori.numClusters[i - 1]) {
            auto& prev = worldCoords[i - 1];
            auto& curr = worldCoords[i];
            int32_t mx_int = static_cast<int32_t>(std::lround((prev.first + curr.first) / 2 * scaleFactor));
            int32_t my_int = static_cast<int32_t>(std::lround((prev.second + curr.second) / 2 * scaleFactor));
            int32_t zRet_int = static_cast<int32_t>(std::lround(zRetract * scaleFactor));
            AppendPointSafeA(m_ToolPathDataA, idx, mx_int, my_int, zRet_int);
        }
        auto& curr = worldCoords[i];
        int32_t x_int = curr.first;
        int32_t y_int = curr.second;
        int32_t zWork_int = static_cast<int32_t>(std::lround(z_Machining * scaleFactor));
        AppendPointSafeA(m_ToolPathDataA, idx, x_int, y_int, zWork_int);
    }

    // --- 新增 Debug 輸出檢查（改用 OutputDebugStringA）---
#ifdef _DEBUG
    {
        std::string debugOutput;
        debugOutput.reserve(8192);  // 預留足夠空間避免頻繁重新配置

        debugOutput += "\n-------------------------------------------------------------------------------------------------------------------------------------------\n";
        debugOutput += "\n--- ToolPathTransform32B(ToolPath ToolPath_Ori, float z_Machining, float zRetract)  ---\n";

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

        debugOutput += "-----------------------   ToolPathTransform32B()     END -----------------------------------------\n";

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
            batchSize = std::min(batchSize, maxModbusBatchSize);

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

        uint16_t index = 0;
        while (index < sizeOfArray)
        {
            int batchSize = (sizeOfArray - index > maxBatchSize) ? maxBatchSize : (sizeOfArray - index);
            batchSize = std::min(batchSize, maxModbusBatchSize);

            // original logic wrote X block and Y block separately; here preserve addresses
            int startAddressX = 0;
            int startAddressY = 10000;

            // write X block
            rc = modbus_write_registers(pParentWnd->m_modbusCtx, startAddressX + index, batchSize, &m_ToolPathData[index]);
            if (rc == -1) {
                CString err;
                err.Format(_T("Failed to write X registers at %d: %S"), index, modbus_strerror(errno));
                AfxMessageBox(err);
                return;
            }

            // write Y block (offset by 1 if original layout interleaved)
            // Keep existing behavior: write starting from index+1 to represent odd index layout
            rc = modbus_write_registers(pParentWnd->m_modbusCtx, startAddressY + index, batchSize, &m_ToolPathData[index + 1]);
            if (rc == -1) {
                CString err;
                err.Format(_T("Failed to write Y registers at %d: %S"), index, modbus_strerror(errno));
                AfxMessageBox(err);
                return;
            }

            index += batchSize;
        }
    } // unlock
}

// 將工具路徑資料 m_ToolPathData 寫入 PLC，使用 Modbus 通訊
void WorkTab::SendToolPathData32(uint16_t* m_ToolPathData, int sizeOfArray, int stationID)
{
    const int maxBatchSize = 100; // 每次最多寫入 100 個暫存器（Modbus 限制）

    // 取得主視窗指標（用於存取 Modbus 相關設定）
    CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
    if (!pParentWnd) {
        AfxMessageBox(_T("Parent window is NULL."));
        return;
    }

    // 若尚未建立 Modbus 連線，則嘗試初始化
    if (!pParentWnd->m_modbusCtx) {
        bool ok = pParentWnd->InitModbusWithRetry(pParentWnd->m_SystemPara.IpAddress, pParentWnd->Port, stationID, 3, 1000);
        if (!ok) {
            AfxMessageBox(_T("Failed to initialize Modbus connection."));
            return;
        }
    }

    // 加鎖，確保 Modbus 操作執行緒安全
    std::lock_guard<std::mutex> lock(pParentWnd->m_modbusMutex);
    modbus_set_slave(pParentWnd->m_modbusCtx, stationID); // 設定目標站號

    // 每個點佔 4 個暫存器（X低、X高、Y低、Y高）
    int totalPoints = sizeOfArray / 4;

    // 將總點數寫入 PLC 的 40026 暫存器
    if (modbus_write_register(pParentWnd->m_modbusCtx, 40026, totalPoints) == -1) {
        CString err;
        err.Format(_T("Failed to write total points: %S"), modbus_strerror(errno));
        AfxMessageBox(err);
        return;
    }

    // 開始分批寫入資料
    int pointIndex = 0;
    while (pointIndex < totalPoints)
    {
        // 計算剩餘點數與本批次要處理的點數（最多 50 點）
        int remainingPoints = totalPoints - pointIndex;
        int batchPoints = std::min(maxBatchSize / 2, remainingPoints); // 每批最多 50 點
        int batchSize = batchPoints * 2; // 每個座標佔 2 個寄存器（低位、高位）

        // 建立 X/Y 暫存器資料陣列
        std::vector<uint16_t> xRegs(batchSize);
        std::vector<uint16_t> yRegs(batchSize);

        // 從 m_ToolPathData 中擷取 X/Y 座標資料
        for (int i = 0; i < batchPoints; ++i)
        {
            size_t base = (pointIndex + i) * 4;
            xRegs[i * 2] = m_ToolPathData[base + 0]; // X 低位
            xRegs[i * 2 + 1] = m_ToolPathData[base + 1]; // X 高位
            yRegs[i * 2] = m_ToolPathData[base + 2]; // Y 低位
            yRegs[i * 2 + 1] = m_ToolPathData[base + 3]; // Y 高位
        }

        // 計算寫入暫存器的起始位置（每個座標佔 2 個暫存器）
        int writeIndex = pointIndex * 2;

        // 寫入 X 座標資料到暫存器區段 0–19999
        if (modbus_write_registers(pParentWnd->m_modbusCtx, writeIndex, batchSize, xRegs.data()) == -1) {
            CString err;
            err.Format(_T("Failed to write X block at %d: %S"), writeIndex, modbus_strerror(errno));
            AfxMessageBox(err);
            return;
        }

        // 寫入 Y 座標資料到暫存器區段 20000–39999
        if (modbus_write_registers(pParentWnd->m_modbusCtx, 20000 + writeIndex, batchSize, yRegs.data()) == -1) {
            CString err;
            err.Format(_T("Failed to write Y block at %d: %S"), 20000 + writeIndex, modbus_strerror(errno));
            AfxMessageBox(err);
            return;
        }

        // 移動到下一批點資料
        pointIndex += batchPoints;
    }
}


void WorkTab::SendToolPathData32A(const std::vector<uint16_t>& m_ToolPathDataA, int sizeOfArray, int stationID)
{
    // sizeOfArray 必須是 6 的倍數（每點 6 個 uint16）
    if (sizeOfArray <= 0 || sizeOfArray % 6 != 0) {
        AfxMessageBox(_T("Tool path data size must be a multiple of 6 (XLo,XHi,YLo,YHi,ZLo,ZHi)."));
        return;
    }

    const int maxBatchRegs = 100; // Modbus 一次最多寫 100~125 個暫存器
    const int maxBatchPoints = maxBatchRegs / 6; // 每批最多傳輸 16 點 (96 個寄存器)

    // 獲取父視窗指標（假設 CYUFADlg 存在）
    CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
    if (!pParentWnd) {
        AfxMessageBox(_T("Parent window is NULL."));
        return;
    }

    // 初始化 Modbus（如尚未連線）
    if (!pParentWnd->m_modbusCtx) {
        bool ok = pParentWnd->InitModbusWithRetry(pParentWnd->m_SystemPara.IpAddress,
            pParentWnd->Port, stationID, 3, 1000);
        if (!ok) {
            AfxMessageBox(_T("Failed to initialize Modbus connection."));
            return;
        }
    }

    // 鎖定 Modbus 資源並設置從機 ID
    std::lock_guard<std::mutex> lock(pParentWnd->m_modbusMutex);
    modbus_set_slave(pParentWnd->m_modbusCtx, stationID);

    int totalPoints = sizeOfArray / 6;

    // 寫入總點數到 60016 (假設這是 PLC 用於讀取的長度暫存器)
    if (modbus_write_register(pParentWnd->m_modbusCtx, 60016, static_cast<uint16_t>(totalPoints)) == -1) {
        CString err;
        err.Format(_T("Failed to write total points: %S"), modbus_strerror(errno));
        AfxMessageBox(err);
        return;
    }

    int pointIndex = 0;
    while (pointIndex < totalPoints)
    {
        int remainingPoints = totalPoints - pointIndex;
        // 每批最多傳輸 maxBatchPoints 點
        int batchPoints = std::min(maxBatchPoints, remainingPoints);

        // 單軸傳輸的寄存器數量 (每個點 2 個寄存器)
        const int batchRegsPerAxis = batchPoints * 2;

        // 預先分配空間，用於存放分軸後的數據
        std::vector<uint16_t> xRegs(batchRegsPerAxis);
        std::vector<uint16_t> yRegs(batchRegsPerAxis);
        std::vector<uint16_t> zRegs(batchRegsPerAxis);

        // 填入本批次資料：從原始陣列中將 X, Y, Z 分離
        for (int i = 0; i < batchPoints; ++i)
        {
            size_t base = (pointIndex + i) * 6;
            int regIndex = i * 2;

            xRegs[regIndex + 0] = m_ToolPathDataA[base + 0]; // X 低位
            xRegs[regIndex + 1] = m_ToolPathDataA[base + 1]; // X 高位

            yRegs[regIndex + 0] = m_ToolPathDataA[base + 2]; // Y 低位
            yRegs[regIndex + 1] = m_ToolPathDataA[base + 3]; // Y 高位

            zRegs[regIndex + 0] = m_ToolPathDataA[base + 4]; // Z 低位
            zRegs[regIndex + 1] = m_ToolPathDataA[base + 5]; // Z 高位
        }

        // 計算當前批次的寫入偏移量 (每個點佔 2 個寄存器)
        int writeOffset = pointIndex * 2;

        // 寫入 X (0 + writeOffset)
        if (modbus_write_registers(pParentWnd->m_modbusCtx, 0 + writeOffset, batchRegsPerAxis, xRegs.data()) == -1) {
            CString err;
            err.Format(_T("Failed to write X block at %d: %S"), writeOffset, modbus_strerror(errno));
            AfxMessageBox(err);
            return;
        }

        // 寫入 Y (20000 + writeOffset)
        if (modbus_write_registers(pParentWnd->m_modbusCtx, 20000 + writeOffset, batchRegsPerAxis, yRegs.data()) == -1) {
            CString err;
            err.Format(_T("Failed to write Y block at %d: %S"), 20000 + writeOffset, modbus_strerror(errno));
            AfxMessageBox(err);
            return;
        }

        // 寫入 Z (40000 + writeOffset)
        if (modbus_write_registers(pParentWnd->m_modbusCtx, 40000 + writeOffset, batchRegsPerAxis, zRegs.data()) == -1) {
            CString err;
            err.Format(_T("Failed to write Z block at %d: %S"), 40000 + writeOffset, modbus_strerror(errno));
            AfxMessageBox(err);
            return;
        }

        pointIndex += batchPoints;
    }

    // 可選：傳送完成後發送一個通知訊號給 PLC
    // if (modbus_write_register(pParentWnd->m_modbusCtx, 40027, 1) == -1) { /* 錯誤處理 */ }
}




BOOL WorkTab::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN)
    {
        int bitAddress = -1;
        int bitValue = 1;
        int nID = -1;

        switch (pMsg->wParam)
        {
        case VK_LEFT:  // 左方向鍵，模擬 X- 按鈕
            bitAddress = 4;
            nID = IDC_BTN_JOG_X_MINUS;
            break;
        case VK_RIGHT:  // 右方向鍵，模擬 X+ 按鈕
            bitAddress = 3;
            nID = IDC_BTN_JOG_X_PLUS;
            break;
        case VK_UP:  // 上方向鍵，模擬 Y+ 按鈕
            bitAddress = 5;
            nID = IDC_BTN_JOG_Y_PLUS;
            break;
        case VK_DOWN:  // 下方向鍵，模擬 Y- 按鈕
            bitAddress = 6;
            nID = IDC_BTN_JOG_Y_MINUS;
            break;
        }

        if (bitAddress != -1)
        {
            //ClearDiscrete3000(0, 8);
            //Discrete3000Change(1, bitAddress, bitValue, nID);
            return TRUE;  // 處理完鍵盤事件，不再繼續傳遞訊息
        }
    }
    else if (pMsg->message == WM_KEYUP)
    {
        int bitAddress = -1;
        int bitValue = 0;
        int nID = -1;

        switch (pMsg->wParam)
        {
        case VK_LEFT:  // 左方向鍵釋放，模擬 X- 按鈕釋放
            bitAddress = 4;
            nID = IDC_BTN_JOG_X_MINUS;
            break;
        case VK_RIGHT:  // 右方向鍵釋放，模擬 X+ 按鈕釋放
            bitAddress = 3;
            nID = IDC_BTN_JOG_X_PLUS;
            break;
        case VK_UP:  // 上方向鍵釋放，模擬 Y+ 按鈕釋放
            bitAddress = 5;
            nID = IDC_BTN_JOG_Y_PLUS;
            break;
        case VK_DOWN:  // 下方向鍵釋放，模擬 Y- 按鈕釋放
            bitAddress = 6;
            nID = IDC_BTN_JOG_Y_MINUS;
            break;
        }

        if (bitAddress != -1)
        {
            //ClearDiscrete3000(0, 8);
            // 註釋掉 Discrete3000Change 以避免在釋放時更新狀態
            // Discrete3000Change(1, bitAddress, bitValue, nID);
            return TRUE;  // 處理完鍵盤事件，不再繼續傳遞訊息
        }
    }

    return CDialogEx::PreTranslateMessage(pMsg);
}

// 假設 InitTransformer 的正確宣告如下：
// void InitTransformer(const std::vector<cv::Point2f>& imagePts, const std::vector<cv::Point2f>& worldPts, cv::Mat& affineMatrix);


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

}

void WorkTab::OnBnClickedWorkImageProcess()
{


}

void WorkTab::OnBnClickedMfcbtnWorkImgCalibrate()
{
   // 開啟檔案對話框取得校正影像
#ifdef _WIN32
    std::vector<std::string> files = m_vision.selectCalibrationFiles();
    if (files.empty())
    {
        AfxMessageBox(L"未選取任何影像");
        return;
    }

    // 依需求調整棋盤內部角點數與邊長
    const cv::Size boardSize(9, 6); // 9x6 內部角點
    const float squareSize = 25.0f; // 毫米

    double rms = m_vision.calibrate(files, boardSize, squareSize);
    if (rms < 0.0)
    {
        AfxMessageBox(L"校正失敗，無法找到角點");
        return;
    }

    // 儲存校正結果
    const std::string outFile = "calibration.yml";
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
    CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(AfxGetMainWnd());
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

    // 注意：以下假設 m_SystemPara 的成員是 int / long / float 等
    // 如果是 float 或其他類型，需要自行轉換（例如除以 100.0f 等）

    pParent->m_SystemPara.ImageBinary = values[0];   // 139
    pParent->m_SystemPara.CreateToolPath = values[1];   // 140
    pParent->m_SystemPara.DispalyToolPath = values[2];   // 141 (DiplayToolPath)
    pParent->m_SystemPara.DisplayROI = values[3];   // 142
    pParent->m_SystemPara.DisplayRefLine = values[4];   // 143
    pParent->m_SystemPara.TabWork = values[5];   // 144  (1=Work,2=Sys,4=Modbus)

    pParent->m_SystemPara.OffsetValue = static_cast<double>(values[6]);  // 145 可考慮單位轉換
    pParent->m_SystemPara.BinaryUpper = values[7];   // 146
    pParent->m_SystemPara.BinaryLower = values[8];   // 147

    pParent->m_SystemPara.MaskX = values[9];   // 148 TopX
    pParent->m_SystemPara.MaskY = values[10];  // 149 TopY
    pParent->m_SystemPara.MaskWidth = values[11];  // 150
    pParent->m_SystemPara.MaskHeight = values[12];  // 151

    // 153 IPAddress 通常不是單一 uint16，建議另外處理或只存部分
    // pParent->m_SystemPara.IPAddress  = ??? values[14];

    pParent->m_SystemPara.RefCenterX = values[15];  // 154
    pParent->m_SystemPara.RefCenterY = values[16];  // 155
    pParent->m_SystemPara.ImageFlip = values[17];  // 156

    // 觸發 UI 更新（很重要！）
    Invalidate(FALSE);
    // 或呼叫 UpdateData(FALSE); 如果有對應的 DDX 控制項

    return true;
}

bool WorkTab::SyncWriteFromSystemPara(int stationID)
{
    std::vector<uint16_t> values(18);

    CSPDlg* pParent = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
    if (!pParent) return false;

    values[0] = static_cast<uint16_t>(pParent->m_SystemPara.ImageBinary);
    values[1] = static_cast<uint16_t>(pParent->m_SystemPara.CreateToolPath);
    values[2] = static_cast<uint16_t>(pParent->m_SystemPara.DispalyToolPath);
    values[3] = static_cast<uint16_t>(pParent->m_SystemPara.DisplayROI);
    values[4] = static_cast<uint16_t>(pParent->m_SystemPara.DisplayRefLine);
    values[5] = static_cast<uint16_t>(pParent->m_SystemPara.TabWork);

    values[6] = static_cast<uint16_t>(std::lround(pParent->m_SystemPara.OffsetValue)); // 注意單位！
    values[7] = static_cast<uint16_t>(pParent->m_SystemPara.BinaryUpper);
    values[8] = static_cast<uint16_t>(pParent->m_SystemPara.BinaryLower);

    values[9] = static_cast<uint16_t>(pParent->m_SystemPara.MaskX);
    values[10] = static_cast<uint16_t>(pParent->m_SystemPara.MaskY);
    values[11] = static_cast<uint16_t>(pParent->m_SystemPara.MaskWidth);
    values[12] = static_cast<uint16_t>(pParent->m_SystemPara.MaskHeight);

    // values[13] = SerialNumber (152) 如果有對應欄位
    // values[14] = IPAddress (153) ← 通常不適合單 uint16，建議另外處理

    values[15] = static_cast<uint16_t>(pParent->m_SystemPara.RefCenterX);
    values[16] = static_cast<uint16_t>(pParent->m_SystemPara.RefCenterY);
    values[17] = static_cast<uint16_t>(pParent->m_SystemPara.ImageFlip);

    return WriteSystemParaBatch_139_to_156(values, stationID);
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

}

// 將 finalPath 中的每一點 轉換成 機械座標系
// MachineCoord 是機械座標系的原點在圖像座標系中的位置
// 轉換後的機械座標會存入 m_machineGluePath 中
// 轉換公式：機械座標 = 圖像座標 - MachineCoord
/*
void WorkTab::ConvertToMachineCoordinates(const Point2D& MachineCoord)
{
    m_machineGluePath.PathRight.clear();
    m_machineGluePath.PathLeft.clear();

    // 右側
    for (const auto& pt : m_OptimizedGluePath.PathRight)
    {
        cv::Point2d machinePt;
        machinePt.x = pt.x - MachineCoord.x;
        machinePt.y = pt.y - MachineCoord.y;
        m_machineGluePath.PathRight.push_back(machinePt);
    }

    // 左側（獨立處理）
    for (const auto& pt : m_OptimizedGluePath.PathLeft)
    {
        cv::Point2d machinePt;
        machinePt.x = pt.x - MachineCoord.x;
        machinePt.y = pt.y - MachineCoord.y;
        m_machineGluePath.PathLeft.push_back(machinePt);
    }
}
*/


// 完整座標轉換：相機座標 → 機械座標 → HMI座標
// 流程：
//   1. m_OptimizedGluePath - MachineCoord  → m_machineGluePath (機械原點歸零)
//   2. m_machineGluePath + (400, 16)        → m_HMIGluePath_temp (映射至HMI原點)
//   3. m_HMIGluePath_temp / (3, 5)          → m_HMIGluePath (縮放至HMI顯示)

void WorkTab::ConvertToMachineCoordinates(const Point2D& MachineCoord)
{
    m_machineGluePath.PathRight.clear();
    m_machineGluePath.PathLeft.clear();
    m_HMIGluePath.PathRight.clear();
    m_HMIGluePath.PathLeft.clear();

    constexpr double HMI_ORIGIN_X = 400.0;
    constexpr double HMI_ORIGIN_Y = 16.0;
    constexpr double HMI_SCALE_X = 3.0;
    constexpr double HMI_SCALE_Y = 5.0;

    // 右側
    for (const auto& pt : m_OptimizedGluePath.PathRight)
    {
        // Step 1: 相機座標 → 機械座標
        cv::Point2d machinePt;
        machinePt.x = pt.x - MachineCoord.x;
        machinePt.y = pt.y - MachineCoord.y;
        m_machineGluePath.PathRight.push_back(machinePt);

        // Step 2: 機械座標 → HMI暫存（映射至HMI原點）
        cv::Point2d hmiTemp;
        hmiTemp.x = machinePt.x + HMI_ORIGIN_X;
        hmiTemp.y = machinePt.y + HMI_ORIGIN_Y;

        // Step 3: 縮放至HMI顯示座標
        cv::Point2d hmiPt;
        hmiPt.x = hmiTemp.x / HMI_SCALE_X;
        hmiPt.y = hmiTemp.y / HMI_SCALE_Y;
        m_HMIGluePath.PathRight.push_back(hmiPt);
    }

    // 左側
    for (const auto& pt : m_OptimizedGluePath.PathLeft)
    {
        // Step 1: 相機座標 → 機械座標
        cv::Point2d machinePt;
        machinePt.x = pt.x - MachineCoord.x;
        machinePt.y = pt.y - MachineCoord.y;
        m_machineGluePath.PathLeft.push_back(machinePt);

        // Step 2: 機械座標 → HMI暫存（映射至HMI原點）
        cv::Point2d hmiTemp;
        hmiTemp.x = machinePt.x + HMI_ORIGIN_X;
        hmiTemp.y = machinePt.y + HMI_ORIGIN_Y;

        // Step 3: 縮放至HMI顯示座標
        cv::Point2d hmiPt;
        hmiPt.x = hmiTemp.x / HMI_SCALE_X;
        hmiPt.y = hmiTemp.y / HMI_SCALE_Y;
        m_HMIGluePath.PathLeft.push_back(hmiPt);
    }
}