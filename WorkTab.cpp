// WorkTab.cpp: 實作檔案
//

#include "pch.h"
#include "YUFA.h"
#include "afxdialogex.h"
#include "WorkTab.h"

//Add pylon header files to MFC project

#include <pylon/PylonIncludes.h>

using namespace Pylon;

static const uint32_t c_countOfImagesToGrab = 3;

using namespace std;

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
}

          
BEGIN_MESSAGE_MAP(WorkTab, CDialogEx)
	ON_BN_CLICKED(IDC_WORK_GRAB, &WorkTab::OnBnClickedWorkGrab)
    ON_WM_CTLCOLOR()
END_MESSAGE_MAP()


// WorkTab 訊息處理常式

BOOL WorkTab::OnInitDialog()
{
	CDialogEx::OnInitDialog();
    PylonInitialize();

    try
    {
        //CInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice());
        camera.Attach(CTlFactory::GetInstance().CreateFirstDevice());

        cout << "Using device : " << camera.GetDeviceInfo().GetModelName() << endl;
        cout << "Serial Number : " << camera.GetDeviceInfo().GetSerialNumber() << endl;
        camera.MaxNumBuffer = 5;
        camera.StartGrabbing(c_countOfImagesToGrab);

        std::string serialNumberStr = (string)(camera.GetDeviceInfo().GetSerialNumber());

        // Extract the low 32 bits of the serial number as an unsigned integer
        uint32_t serialNumberLow = 0;

        //std::stoul is a standard library function in C++ that converts a string representation of an unsigned integer into its numerical value.
        serialNumberLow = std::stoul(serialNumberStr.substr(0, 8), nullptr, 16);
        cout << "Serial Number Low : " << serialNumberLow << endl;

        /*
         Convert the String_t to a std::string
         std::string serialNumberStr = serialNumber.ToString();

         // Extract the low 32 bits of the serial number as an unsigned integer
         uint32_t serialNumberLow = std::stoul(serialNumberStr.substr(16, 8), nullptr, 16);
        */


        
    }
    catch (const GenericException& e)
    {
        cerr << "An exception occurred." << endl << e.GetDescription() << endl;
        PylonTerminate();
        return 1;
    }

    //PylonTerminate();
    return 0;

}

HBRUSH WorkTab::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

    // 如果是我們的按鈕
    if (pWnd->GetDlgCtrlID() == IDC_WORK_GRAB)
    {
        // 設定文字顏色
        pDC->SetTextColor(RGB(255, 255, 255));

        // 設定背景顏色
        pDC->SetBkColor(RGB(255, 0, 0));

        // 回傳一個創建的筆刷
        if (m_brush.GetSafeHandle() == NULL)
            m_brush.CreateSolidBrush(RGB(255, 0, 0));
        return (HBRUSH)m_brush;

    }

    // 否則，回傳預設的筆刷
    return hbr;
}


void WorkTab::OnBnClickedWorkGrab()
{
	// TODO: 在此加入控制項告知處理常式程式碼
    // Create a thread to grab images
    AfxBeginThread(GrabThread, this);
    GrabThread(this);

}

// Add a multi-treaded grabber with Basler Pylon
UINT WorkTab::GrabThread(LPVOID pParam)
{
	// Get the pointer to the dialog
	WorkTab* pDlg = (WorkTab*)pParam;

    while (pDlg->camera.IsGrabbing())
    {
        CGrabResultPtr ptrGrabResult;

        pDlg->camera.RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);

        if (ptrGrabResult->GrabSucceeded())
        {
            const uint8_t* pImageBuffer = (uint8_t*)ptrGrabResult->GetBuffer();
            pDlg->m_Image = cv::Mat(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(), CV_8UC1, (void*)pImageBuffer);
            //pDlg->MyImage(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(), CV_8UC1, (void*)pImageBuffer);
            // Display the OpenCV image.
            //cv::imshow("Image", img);

            // Resize the image by a factor of 2
            //cv::Mat resizedImg;
            //Show the image in the window
            //cv::imshow("Image", pDlg->m_Image);

            // Call DrawPicToHDC to display the image in the dialog
            pDlg->DrawPicToHDC(pDlg->m_Image, IDC_PICCTL_DISPLAY, false);
            

            //cv::resize(pDlg->MyImage, resizedImg, cv::Size(), 0.2, 0.2);
            // Display the resized image
            //cv::imshow("Resized Image", resizedImg);

            //cv::waitKey(0);
        }
        else
        {
            cout << "Error: " << std::hex << ptrGrabResult->GetErrorCode() << std::dec << " " << ptrGrabResult->GetErrorDescription() << endl;
        }
    }
    return 0;
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