// YUFADlg.cpp: 實作檔案
//

#include "pch.h"
#include "framework.h"
#include "YUFA.h"
#include "YUFADlg.h"
#include "afxdialogex.h"
#include "afxwin.h"
#include "SystemParaTab.h"
#include "WorkTab.h"

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
	virtual void OnCancel();
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


// CYUFADlg 對話方塊



CYUFADlg::CYUFADlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_YUFA_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CYUFADlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB_MAIN, m_Tab_Main);
}

BEGIN_MESSAGE_MAP(CYUFADlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_WM_MOUSEMOVE()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BTN_QUIT, &CYUFADlg::OnBnClickedBtnQuit)
	ON_BN_CLICKED(IDC_BTN_SYS_PARA, &CYUFADlg::OnBnClickedBtnSysPara)
	ON_BN_CLICKED(IDC_BTN_WORKING, &CYUFADlg::OnBnClickedBtnWorking)
	ON_BN_CLICKED(IDC_BTN_MODBUS, &CYUFADlg::OnBnClickedBtnModbus)
	ON_BN_CLICKED(IDC_BTN_MACHINE, &CYUFADlg::OnBnClickedBtnMachine)
	ON_NOTIFY(NM_RCLICK, IDC_TAB_MAIN, &CYUFADlg::OnNMRClickTabMain)
END_MESSAGE_MAP()


// CYUFADlg 訊息處理常式

BOOL CYUFADlg::OnInitDialog()
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
    // Get Client Rect
	CRect rect;
	GetClientRect(&rect);
	
	//define indicators
	UINT indicators[] = { ID_INDICATOR_TIME, ID_INDICATOR_FILE };

	// Create the status bar
	if (m_Status_Bar.Create(this))
	{
		m_Status_Bar.SetIndicators(indicators, 2);
		m_Status_Bar.SetPaneInfo(0, ID_INDICATOR_TIME, SBPS_NORMAL, rect.Width() - 100);
		m_Status_Bar.SetPaneInfo(1, ID_INDICATOR_FILE, SBPS_NORMAL, rect.Width() - 50);

		//Add Status Bar Fime Name Data
		m_Status_Bar.SetPaneText(0,_T("File Name :N/A"));

		// Resize the status bar
		m_Status_Bar.MoveWindow(rect.left, rect.bottom - 20, rect.Width(), 20);
	}
	
	

    // Set Timer
	SetTimer(100, 1000, NULL);

	//Set Dialog Maximize and Minimize icon
	ModifyStyle(0, WS_MAXIMIZEBOX | WS_MINIMIZEBOX, 0);

	//Set Dialog Title
	SetWindowText(_T("YUFA 0.0.0"));

	//Table control initial
	m_Tab_Main.InsertItem(0, _T("Working"));
	m_Tab_Main.InsertItem(1, _T("System Parameter"));
	m_Tab_Main.InsertItem(2, _T("Modbus TCP"));
	m_Tab_Main.InsertItem(3, _T("Machine"));

	//Set Table Control Size
	//m_Tab_Main.SetParent(this);
	m_Tab_Main.GetClientRect(&rect);
	rect.top += 10;
	rect.bottom -= 10;
	rect.left += 10;
	rect.right -= 10;
	m_Tab_Main.AdjustRect(FALSE, &rect);
	
	//Add Tab Control Item Working
	m_WorkTab.Create(IDD_TAB_WOK, &m_Tab_Main);
	m_WorkTab.MoveWindow(&rect);
	m_WorkTab.ShowWindow(SW_SHOW);

	//Add Tab Control Item System Parameter
	m_SystemParaTab.Create(IDD_TAB_SYSTEM_PARA, &m_Tab_Main);
	m_SystemParaTab.MoveWindow(&rect);
	m_SystemParaTab.ShowWindow(SW_HIDE);

	//Add Tab Control Item Modbus TCP
	m_ModBusTab.Create(IDD_TAB_MODBUS, &m_Tab_Main);
	m_ModBusTab.MoveWindow(&rect);
	m_ModBusTab.ShowWindow(SW_HIDE);

	//Add Tab Control Item Machine
	m_MachineTab.Create(IDD_TAB_MACHINE, &m_Tab_Main);
	m_MachineTab.MoveWindow(&rect);
	m_MachineTab.ShowWindow(SW_HIDE);



	//Initial m_SystemPara
	m_SystemPara.iStart = 0;
	m_SystemPara.OffsetX = 0.0;
	m_SystemPara.OffsetY = 0.0;
	//Assign 192.168.0.11 to m_SystemPara.IpAddress
   
    m_SystemPara.IpAddress, _T("192.168.0.11");
	//m_SystemPara.IpAddress = _T("168.95.192.1");
	m_SystemPara.StationID = 1;


	return TRUE;  // 傳回 TRUE，除非您對控制項設定焦點
}

void CYUFADlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CYUFADlg::OnPaint()
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
HCURSOR CYUFADlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CYUFADlg::OnClose()
{
	// TODO: 在此加入您的訊息處理常式程式碼和 (或) 呼叫預設值


	//CDialogEx::OnClose();
}


void CYUFADlg::OnOK()
{
	// TODO: 在此加入特定的程式碼和 (或) 呼叫基底類別

	CDialogEx::OnOK();
}


void CYUFADlg::OnBnClickedBtnQuit()
{
	// TODO: 在此加入控制項告知處理常式程式碼

	//Quit Application

	//Stop Grab Thread : WorkTab::m_bGrabThread = false;

	
	// 將 m_WorkTab中的 m_bGrabThread 設為 false，停止Grab Thread
	m_WorkTab.m_bGrabThread = false;

	

	OnCancel();
}


void CYUFADlg::OnCancel()
{
	// TODO: 在此加入特定的程式碼和 (或) 呼叫基底類別
	
	// Show Message Box
	// OK : Quit Application Cancel: Do Nothing
	int nRet = MessageBox(_T("Quit Application?"), _T("Quit"), MB_OKCANCEL);
	
	if (nRet == IDOK)
	{
		// Quit Application
		CDialogEx::OnCancel();
	}
	else
	{
		// Do Nothing
	}
	//CDialogEx::OnCancel();
}


void CYUFADlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此加入您的訊息處理常式程式碼和 (或) 呼叫預設值

	CTime t1;
	t1 = CTime::GetCurrentTime();
	m_Status_Bar.SetPaneText(1, t1.Format("%H:%M:%S"));

	CDialogEx::OnTimer(nIDEvent);
}

void CYUFADlg::OnBnClickedBtnWorking()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	m_WorkTab.ShowWindow(SW_SHOW);
	m_SystemParaTab.ShowWindow(SW_HIDE);
	m_ModBusTab.ShowWindow(SW_HIDE);
	m_MachineTab.ShowWindow(SW_HIDE);
	m_Tab_Main.SetCurSel(0);
}

void CYUFADlg::OnBnClickedBtnSysPara()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	m_WorkTab.ShowWindow(SW_HIDE);
	m_SystemParaTab.ShowWindow(SW_SHOW);
	m_ModBusTab.ShowWindow(SW_HIDE);
	m_MachineTab.ShowWindow(SW_HIDE);
	//change to select tab
	m_Tab_Main.SetCurSel(1);
}

void CYUFADlg::OnBnClickedBtnModbus()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	m_WorkTab.ShowWindow(SW_HIDE);
	m_SystemParaTab.ShowWindow(SW_HIDE);	
	m_ModBusTab.ShowWindow(SW_SHOW);
	m_MachineTab.ShowWindow(SW_HIDE);
	//change to select tab
	m_Tab_Main.SetCurSel(2);
}


void CYUFADlg::OnBnClickedBtnMachine()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	m_WorkTab.ShowWindow(SW_HIDE);
	m_SystemParaTab.ShowWindow(SW_HIDE);
	m_ModBusTab.ShowWindow(SW_HIDE);
	m_MachineTab.ShowWindow(SW_SHOW);
	//change to select tab
	m_Tab_Main.SetCurSel(3);

}

void CYUFADlg::OnSize(UINT nType, int cx, int cy)
{

	CDialogEx::OnSize(nType, cx, cy);

	// TODO: 在此加入您的訊息處理常式程式碼
	if (m_Status_Bar.m_hWnd)
	{
		CRect rect;
		GetClientRect(&rect);
		m_Status_Bar.MoveWindow(rect.left, rect.bottom - 20, rect.Width(), 20);
	}
	
}


void CAboutDlg::OnCancel()
{
	// TODO: 在此加入特定的程式碼和 (或) 呼叫基底類別

	CDialogEx::OnCancel();
}


void CYUFADlg::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 在此加入您的訊息處理常式程式碼和 (或) 呼叫預設值


	CDialogEx::OnMouseMove(nFlags, point);
}






void CYUFADlg::OnNMRClickTabMain(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此加入控制項告知處理常式程式碼
	//Get the current selected tab
	int iCurSel = m_Tab_Main.GetCurSel();
	CString str;
	str.Format(_T("Current Selected Tab: %d"), iCurSel);
	MessageBox(str);

	*pResult = 0;
}
