// MV CalibrationDlg.cpp: 實作檔案
//

#include "pch.h"
#include "framework.h"
#include "MV Calibration.h"
#include "MV CalibrationDlg.h"
#include "afxdialogex.h"
#include "MV CalibrationDlg.h"
#include "UAX.h"

#include <shlobj.h>
#include <afxshelllistctrl.h> // 假設使用 MFC 的 CShellListCtrl

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 對 App About 使用 CAboutDlg 對話方塊

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支援

// 程式碼實作
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMVCalibrationDlg 對話方塊



CMVCalibrationDlg::CMVCalibrationDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MV_CALIBRATION_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMVCalibrationDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMVCalibrationDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_MFCBTN_EXIT, &CMVCalibrationDlg::OnBnClickedMfcbtnExit)
END_MESSAGE_MAP()


// CMVCalibrationDlg 訊息處理常式

BOOL CMVCalibrationDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 將 [關於...] 功能表加入系統功能表。

	// IDM_ABOUTBOX 必須在系統命令範圍之中。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 設定此對話方塊的圖示。當應用程式的主視窗不是對話方塊時，
	// 框架會自動從事此作業
	SetIcon(m_hIcon, TRUE);			// 設定大圖示
	SetIcon(m_hIcon, FALSE);		// 設定小圖示

	// TODO: 在此加入額外的初始設定

	//Set IDC_MFCSHL_FILE_LIST path to App Path//Calibration Image
	// Get the application path
	std::string strAppPath = GetAppPath();
	// Convert std::string to CString
	CString strAppPathCString(strAppPath.c_str());

	CString strCalibrationImagePath = strAppPathCString + _T("\\Calibration Image\\");

	// Set the path to the file list control
	// 取得控制項指標
	CMFCShellListCtrl* pFileList = (CMFCShellListCtrl*)GetDlgItem(IDC_MFCSHL_FILE_LIST);
	if (pFileList != nullptr)
	{
		// 檢查路徑是否存在，如果不存在則建立
		if (::GetFileAttributes(strCalibrationImagePath) == INVALID_FILE_ATTRIBUTES)
		{
			::CreateDirectory(strCalibrationImagePath, NULL);
		}

		// 設定控制項的路徑
        // 1. 先移除原本的 EnableMask 呼叫
        // 2. 在 DisplayFolder 之後，過濾只顯示 mp4/avi 檔案

        // ...原本的 OnInitDialog 內容...

        // 設定控制項的路徑
        HRESULT hr = pFileList->DisplayFolder(strCalibrationImagePath);
        //將路徑顯示在對話框的 title 上
        SetWindowText(strCalibrationImagePath);

        // 新增：只顯示png 檔案
        for (int i = pFileList->GetItemCount() - 1; i >= 0; --i)
        {
            CString strPath;
            if (pFileList->GetItemPath(strPath, i))
            {
                CString strExt = strPath.Mid(strPath.ReverseFind(_T('.')));
                strExt.MakeLower();
                if (strExt != _T(".png"))
                {
                    pFileList->DeleteItem(i);
                }
            }
        }

    
		//HRESULT hr = pFileList->DisplayFolder(strCalibrationImagePath);
		//將路徑顯示在對話框的 title 上
		//SetWindowText(strCalibrationImagePath);


		if (FAILED(hr))
		{
			// 如果無法顯示指定路徑，則顯示應用程式路徑
			pFileList->DisplayFolder(strCalibrationImagePath);
		
            // 設定控制項的路徑
            HRESULT hr = pFileList->DisplayFolder(strCalibrationImagePath);
            if (FAILED(hr))
            {
                // 如果無法顯示指定路徑，則顯示應用程式路徑
                pFileList->DisplayFolder(strCalibrationImagePath);
            }
		}

	


	}

	return TRUE;  // 傳回 TRUE，除非您對控制項設定焦點
}

void CMVCalibrationDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果將最小化按鈕加入您的對話方塊，您需要下列的程式碼，
// 以便繪製圖示。對於使用文件/檢視模式的 MFC 應用程式，
// 框架會自動完成此作業。

void CMVCalibrationDlg::OnPaint()
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
	}
}

// 當使用者拖曳最小化視窗時，
// 系統呼叫這個功能取得游標顯示。
HCURSOR CMVCalibrationDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CMVCalibrationDlg::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
    {
        // 攔截 Enter 鍵，不做任何事
        return TRUE;
    }
    return CDialogEx::PreTranslateMessage(pMsg);
}


void CMVCalibrationDlg::OnBnClickedMfcbtnExit()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	// 關閉對話框
	EndDialog(IDOK);
}
