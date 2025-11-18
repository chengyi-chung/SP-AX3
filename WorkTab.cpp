// WorkTab.cpp: 實作檔案
#pragma once
#include "pch.h"
#include "YUFA.h"
#include "YUFADlg.h"
#include "afxdialogex.h"
#include "WorkTab.h"
#include <string>
//Add pylon header files to MFC project

#include <pylon/PylonIncludes.h>

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

    CYUFADlg* pParentWnd = dynamic_cast<CYUFADlg*>(GetParent()->GetParent());

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
                // 0: 垂直翻轉, 1: 水平翻轉, -1: 水平並垂直翻轉
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
    
    if (flgCenter)
    {
        int centerX = imageToShow.cols / 2;
        int centerY = imageToShow.rows / 2;

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
    // TODO: 在此加入控制項告知處理常式

	//Get the tool path from the image m_mat
	//m_mat: Source Image m_mat
	//Offset: Offset of the tool path
	//toolpath: Tool Path
	cv::Mat ImgSrc = m_mat.clone();
	cv::Point2d Offset;
	//ToolPath toolpath;
	//Get m_ToolPathData from Parrent Window
	//CYUFADlg* pParentWnd = (CYUFADlg*)GetParent();
    CYUFADlg* pParentWnd = dynamic_cast<CYUFADlg*>(GetParent()->GetParent());

    Offset.x = pParentWnd->m_SystemPara.OffsetX;
	Offset.y = pParentWnd->m_SystemPara.OffsetY;

    //convert Offset mm to pixel by TransferFactor
    Offset.x = Offset.x / pParentWnd->m_SystemPara.TransferFactor;
    Offset.y = Offset.y / pParentWnd->m_SystemPara.TransferFactor;

	//Call the function to get the tool path from UAX
	//toolpath: Tool Path pixel
	//ImgSrc: Source Image
	//Offset: Offset of the tool path, mm to pixel before call GetToolPathData

    //直接把結果放到成員變數 toolPath，避免用區域變數後沒指派回來
    this->toolPath.Path.clear();
    //GetToolPathData(ImgSrc, Offset, this->toolPath);

	//Get Mask of the tool path from the image m_mat
	//ImgSrc: Source Image m_mat
	//Offset: Offset of the tool path
	//toolpath: Tool Path
	//ImgSrc: Source Image
	//Offset: Offset of the tool path, mm to pixel before call GetToolPathWithMask
	//直接把結果放到成員變數 toolPath，避免用區域變數後沒指派回來

	//cv::Rect roi(MaskX, MaskY, MaskWidth, MaskHeight);
	//cv::Mat ROI_Mask = m_mat(roi);
	//GetToolPathWithMask(ImgSrc, ROI_Mask, Offset, this->toolPath);

	// 檢查 m_mat 是否為空
if (m_mat.empty()) {
    AfxMessageBox(_T("Source image is empty."));
    return;
}

// 檢查 ROI 是否在影像範圍內
if (MaskX < 0 || MaskY < 0 ||
    MaskWidth <= 0 || MaskHeight <= 0 ||
    MaskX + MaskWidth > m_mat.cols ||
    MaskY + MaskHeight > m_mat.rows) {
    AfxMessageBox(_T("ROI is out of image bounds."));
    return;
}

// 1. 建立一張與 ImgSrc 一樣大小的全黑 Mask（8位元單通道）
cv::Mat mask = cv::Mat::zeros(ImgSrc.size(), CV_8UC1);

// 2. 設定 ROI 區域：左上角(X, Y)，寬度Width，高度Height
cv::Rect roi(MaskX, MaskY, MaskWidth, MaskHeight);

// 3. 檢查 ROI 是否在 ImgSrc 範圍內，以避免 out-of-bounds 問題
if ((MaskX >= 0) && (MaskY >= 0) &&
    (MaskY + MaskWidth <= ImgSrc.cols) &&
    (MaskY + MaskHeight <= ImgSrc.rows))
{
    // 4. 設定 ROI 區域像素為 255（白色，有效區域）
    mask(roi) = cv::Scalar(255);
}
else
{
    throw std::invalid_argument("ROI超出原圖範圍");
}

//Get offsetDist with offsetDist and distX, distY
int distOffset = sqrt(pow(Offset.x,2) +pow(Offset.y,2));

GetToolPathWithMask(ImgSrc, mask, distOffset, this->toolPath);
	
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
    // TODO: 在此加入控制項告知處理常式
	//Convert toolPath to m_ToolPathData[20000]
	//toolPath: Tool Path
	//m_ToolPathData: Tool Path Data Array
	//toolPath.Path : Path of the tool
	//Convert toolPath.Path to m_ToolPathData[20000]
	int sizeOfToolPath = this->toolPath.Path.size();

	// int m_ToolPathData[20000];
    //uint16_t* m_ToolPathData = new uint16_t[20000];
    //call ToolPathTransform(ToolPath& toolpath, int* m_ToolPathData)
	//ToolPathTransform(toolPath, m_ToolPathData);

    // int m_ToolPathData[60000];
	uint16_t* m_ToolPathData = new uint16_t[60000];  //增加陣列大小到60000 --> +Z

    //測試機修正參數
    float imagePts[] = { 1097,1063, 1373,1063, 1371,945 };
    float worldPts[] = { 34.79f,205.19f, 187.19f,205.19f, 187.19f,141.79f };

	//use UAX.dll to convert image coordinate to world coordinate
    //call (float x_pixel, float y_pixel, float& x_mm, float& y_mm, float* imagePts, float* worldPts)

    //tooPath 
    //struct ToolPath
    //{
    //    cv::Point2d Offset; // Offset of the tool path
    //    std::vector<cv::Point2d> Path; // Tool path
   // };
	//toolPath_world = toolPath; // copy toolPath to toolPath_world
   
    //Convert toolPath_world to m_ToolPathData[20000], 轉換
    //toolPath_world: Tool Path in world coordinate
    //m_ToolPathData: Tool Path Data Array
    //toolPath_world.Path : Path of the tool in world coordinate
    //Convert toolPath_world.Path to m_ToolPathData[20000]
	//ToolPathTransform(toolPath_world, m_ToolPathData);

    //插入檢查ToolPathTransform32 執行速度功能



    //定義最高10000點(DINT)，m_ToolPathData矩陣尺寸要大4倍+
	ToolPathTransform32(this->toolPath, m_ToolPathData);  //轉換 toolPath到 m_ToolPathData[60000] 矩陣
    ToolPathTransform32B(this->toolPath, z_Machining, z_Retract);

    //Send m_ToolPathData to PLC with modbus tcp
	//call SendToolPathData(int* m_ToolPathData, int sizeOfArray, int stationID)

	//send m_ToolPathData to PLC with modbus tcp
	//一個點4個uint16_t Point(x_low, x_high, y_low, y_high)
    sizeOfToolPath = sizeOfToolPath * 4;
    //Get m_ToolPathData

    //
	SendToolPathData32(m_ToolPathData, sizeOfToolPath, 1);

}

/*



*/
void WorkTab::ToolPathTransform32(ToolPath ToolPapath_Ori, uint16_t* m_ToolPathData)
{
    if (!m_ToolPathData || ToolPapath_Ori.Path.empty()) return;

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
     float scaleFactor = 100.0f;
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
    // 檢查是否會溢出緩衝區（安全檢查）
    if (idx + 6 > buffer.size()) {
        throw std::runtime_error("AppendPointSafe: buffer overflow");
    }

    // 轉換為 uint32_t 以處理位元操作（維持負數的二補數表示）
    uint32_t x_u = static_cast<uint32_t>(x);
    uint32_t y_u = static_cast<uint32_t>(y);
    uint32_t z_u = static_cast<uint32_t>(z);

    // 拆分並寫入低/高位
    buffer[idx++] = static_cast<uint16_t>(x_u & 0xFFFF);       // X low
    buffer[idx++] = static_cast<uint16_t>((x_u >> 16) & 0xFFFF);  // X high

    buffer[idx++] = static_cast<uint16_t>(y_u & 0xFFFF);       // Y low
    buffer[idx++] = static_cast<uint16_t>((y_u >> 16) & 0xFFFF);  // Y high

    buffer[idx++] = static_cast<uint16_t>(z_u & 0xFFFF);       // Z low
    buffer[idx++] = static_cast<uint16_t>((z_u >> 16) & 0xFFFF);  // Z high
}

// 類別 WorkTab 的成員函數
// 此函數將原始工具路徑轉換為機器可讀的 uint16_t 數據格式
// 並在分群變換時插入中間點以處理 Z 軸的 retract 操作
void WorkTab::ToolPathTransform32B(ToolPath ToolPath_Ori,     // 輸入：原始工具路徑結構
    float z_Machining,         // 輸入：加工時的 Z 值 (mm)
    float zRetract) {          // 輸入：退刀時的 Z 值 (mm)
    // 輸入驗證：檢查路徑是否為空，或 Path 和 numClusters 大小是否匹配
    if (ToolPath_Ori.Path.empty() ||
        ToolPath_Ori.Path.size() != ToolPath_Ori.numClusters.size()) {
        throw std::invalid_argument("Invalid input in ToolPathTransform32A");
    }

    // 定義縮放因子：將 mm 轉換為整數 (x100)
    float scaleFactor = 100.0f;

    // 定義三點對應的像素點和世界座標點（用於仿射轉換）
    // 這些點用於計算像素到世界座標的轉換矩陣
    static float imagePts[] = { 1035, 844, 1311, 1247, 1511, 963 };  // 像素點座標
    static float worldPts[] = { -0.01f, 67.59f, 150.79f, 288.83f, 259.71f, 134.03f };  // 對應世界座標 (mm)

    // 靜態初始化仿射矩陣：僅計算一次，提高效率
    static cv::Mat affine = []() {
        cv::Mat mat;
        InitTransformer(imagePts, worldPts, 3, mat);  // 計算仿射轉換矩陣
        return mat;
        }();

    // 預計算所有點的世界座標：避免在迴圈中重複計算，提高效率
    std::vector<std::pair<float, float>> worldCoords(ToolPath_Ori.Path.size());
    for (size_t i = 0; i < ToolPath_Ori.Path.size(); ++i)
    {
        float x_mm = 0.0f, y_mm = 0.0f;
        PixelToWorld(ToolPath_Ori.Path[i].x, ToolPath_Ori.Path[i].y, x_mm, y_mm, affine);

        // 放大並取整
        int32_t x_int = static_cast<int32_t>(std::lround(x_mm * scaleFactor));
        int32_t y_int = static_cast<int32_t>(std::lround(y_mm * scaleFactor));

        // 維持二補數(負數可正確拆分)
        uint32_t x_u = static_cast<uint32_t>(x_int);
        uint32_t y_u = static_cast<uint32_t>(y_int);

        worldCoords[i] = { static_cast<float>(x_u), static_cast<float>(y_u) };
    }

    // 計算分群變換次數：用於預估輸出數據大小
    size_t numClustersChanges = 0;
    for (size_t i = 1; i < ToolPath_Ori.Path.size(); ++i) {
        if (ToolPath_Ori.numClusters[i] != ToolPath_Ori.numClusters[i - 1])
            ++numClustersChanges;
    }

    // 計算總點數：原始點數 + 分群變換次數（每個變換插入一個中間點）
    size_t totalPoints = ToolPath_Ori.Path.size() + numClustersChanges;

    // 修正 E0153/C2228 錯誤：.resize 只能用於 std::vector 類型，不能用於原生指標 (uint16_t*)
    // 原本錯誤寫法：m_ToolPathData.resize(totalPoints * 6);
    // 正確寫法：請確認 m_ToolPathDataA 是 std::vector<uint16_t>，用 .resize
    m_ToolPathDataA.resize(totalPoints * 6);

    // 輸出索引：追蹤當前寫入位置
    size_t idx = 0;

    // 迴圈處理每個原始點
    for (size_t i = 0; i < ToolPath_Ori.Path.size(); ++i) {
        // 檢查是否為分群變換點（從第二點開始）
        if (i > 0 && ToolPath_Ori.numClusters[i] != ToolPath_Ori.numClusters[i - 1]) {
            auto& prev = worldCoords[i - 1];
            auto& curr = worldCoords[i];
            int32_t mx_int = static_cast<int32_t>(std::lround((prev.first + curr.first) / 2 * scaleFactor));
            int32_t my_int = static_cast<int32_t>(std::lround((prev.second + curr.second) / 2 * scaleFactor));
            int32_t zRet_int = static_cast<int32_t>(std::lround(zRetract * scaleFactor));
            AppendPointSafeA(m_ToolPathDataA, idx, mx_int, my_int, zRet_int);
        }

        auto& curr = worldCoords[i];
        int32_t x_int = static_cast<int32_t>(std::lround(curr.first * scaleFactor));
        int32_t y_int = static_cast<int32_t>(std::lround(curr.second * scaleFactor));
        int32_t zWork_int = static_cast<int32_t>(std::lround(z_Machining * scaleFactor));
        AppendPointSafeA(m_ToolPathDataA, idx, x_int, y_int, zWork_int);
    }
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

    CYUFADlg* pParentWnd = dynamic_cast<CYUFADlg*>(GetParent()->GetParent());
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

    CYUFADlg* pParentWnd = dynamic_cast<CYUFADlg*>(GetParent()->GetParent());
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
    CYUFADlg* pParentWnd = dynamic_cast<CYUFADlg*>(GetParent()->GetParent());
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
        int batchSize = batchPoints * 2; // 每個座標佔 2 個暫存器（低位、高位）

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

        // 計算寫入暫存器的起始位置（每點佔 2 個暫存器）
        int writeIndex = pointIndex * 2;

        // 寫入 X 座標資料到暫存器區段 0–19999
        if (modbus_write_registers(pParentWnd->m_modbusCtx, writeIndex, batchSize, xRegs.data()) == -1) {
            CString err;
            err.Format(_T("Failed to write X block at address %d: %S"), writeIndex, modbus_strerror(errno));
            AfxMessageBox(err);
            return;
        }

        // 寫入 Y 座標資料到暫存器區段 20000–39999
        if (modbus_write_registers(pParentWnd->m_modbusCtx, 20000 + writeIndex, batchSize, yRegs.data()) == -1) {
            CString err;
            err.Format(_T("Failed to write Y block at address %d: %S"), 20000 + writeIndex, modbus_strerror(errno));
            AfxMessageBox(err);
            return;
        }

        // 移動到下一批點資料
        pointIndex += batchPoints;
    }
}


void WorkTab::SendToolPathData32A(std::vector<uint16_t> m_ToolPathDataA, int sizeOfArray, int stationID)
{
    // sizeOfArray 必須是 6 的倍數（每點 6 個 uint16）
    if (sizeOfArray <= 0 || sizeOfArray % 6 != 0) {
        AfxMessageBox(_T("Tool path data size must be a multiple of 6 (XLo,XHi,YLo,YHi,ZLo,ZHi)."));
        return;
    }

    const int maxBatchSize = 100; // Modbus 一次最多寫 100~125 個暫存器，保守取 100

    CYUFADlg* pParentWnd = dynamic_cast<CYUFADlg*>(GetParent()->GetParent());
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

    std::lock_guard<std::mutex> lock(pParentWnd->m_modbusMutex);
    modbus_set_slave(pParentWnd->m_modbusCtx, stationID);

    int totalPoints = sizeOfArray / 6;

    // 寫入總點數到 60016
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
        // 每點 6 個寄存器 → 每批最多 floor(100/6)=16 點，保守取 16 點（共 96 個寄存器）
        int batchPoints = std::min(16, remainingPoints);
        int batchRegs = batchPoints * 3 * 2;  // 每軸 2 個寄存器（低+高），共 3 軸 → 6 個/點

        std::vector<uint16_t> xRegs(batchPoints * 2);
        std::vector<uint16_t> yRegs(batchPoints * 2);
        std::vector<uint16_t> zRegs(batchPoints * 2);

        // 填入本批次資料
        for (int i = 0; i < batchPoints; ++i)
        {
            size_t base = (pointIndex + i) * 6;

            xRegs[i * 2 + 0] = m_ToolPathDataA[base + 0]; // X 低位
            xRegs[i * 2 + 1] = m_ToolPathDataA[base + 1]; // X 高位
            yRegs[i * 2 + 0] = m_ToolPathDataA[base + 2]; // Y 低位
            yRegs[i * 2 + 1] = m_ToolPathDataA[base + 3]; // Y 高位
            zRegs[i * 2 + 0] = m_ToolPathDataA[base + 4]; // Z 低位
            zRegs[i * 2 + 1] = m_ToolPathDataA[base + 5]; // Z 高位
        }

        int writeOffset = pointIndex * 2;  // 每個點在該軸區段佔 2 個寄存器

        // 寫入 X (00000 ~ 19999)
        if (modbus_write_registers(pParentWnd->m_modbusCtx, 0 + writeOffset, batchPoints * 2, xRegs.data()) == -1) {
            CString err;
            err.Format(_T("Failed to write X block at %d: %S"), writeOffset, modbus_strerror(errno));
            AfxMessageBox(err);
            return;
        }

        // 寫入 Y (20000 ~ 39999)
        if (modbus_write_registers(pParentWnd->m_modbusCtx, 20000 + writeOffset, batchPoints * 2, yRegs.data()) == -1) {
            CString err;
            err.Format(_T("Failed to write Y block at %d: %S"), 20000 + writeOffset, modbus_strerror(errno));
            AfxMessageBox(err);
            return;
        }

        // 寫入 Z (40000 ~ 59999)
        if (modbus_write_registers(pParentWnd->m_modbusCtx, 40000 + writeOffset, batchPoints * 2, zRegs.data()) == -1) {
            CString err;
            err.Format(_T("Failed to write Z block at %d: %S"), 40000 + writeOffset, modbus_strerror(errno));
            AfxMessageBox(err);
            return;
        }

        pointIndex += batchPoints;
    }

    // 可選：傳送完成後發送一個通知訊號給 PLC（例如寫入 40027 = 1 表示資料已更新）
    // modbus_write_register(pParentWnd->m_modbusCtx, 40027, 1);
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

}

void WorkTab::OnBnClickedWorkImageProcess()
{

}
