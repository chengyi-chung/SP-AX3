// YUFADlg.cpp: 實作檔案
//

#include "pch.h"
#include "framework.h"
#include "UAX.h"
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
	, m_SystemPara() // 呼叫 SystemConfig 預設建構子
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	// Read System Parameters from config file
	//ReadSystemParametersFromConfigFile();

}

void CYUFADlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB_MAIN, m_Tab_Main);
	
	// 加入 MFC Button 控制項的 DDX
	DDX_Control(pDX, IDC_BTN_QUIT, m_BtnQuit);
	DDX_Control(pDX, IDC_BTN_WORKING, m_BtnWorking);
	DDX_Control(pDX, IDC_BTN_SYS_PARA, m_BtnSysPara);
	DDX_Control(pDX, IDC_BTN_MODBUS, m_BtnModbus);
	DDX_Control(pDX, IDC_BTN_MACHINE, m_BtnMachine);
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
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_MAIN, &CYUFADlg::OnTcnSelchangeTabMain)
	ON_WM_CTLCOLOR()
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

	
	// Read System Parameters from config file
	ReadSystemParametersFromConfigFile();

	// 使用現有的 GetMacAddress 函式取得 MAC 位址
	GetMacAddress(m_SystemPara.MACKey);

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

	// 修改初始化按鈕樣式的函式
	InitButtonStyle();

	//Get Mac ID assign to m_SystemPara with UAX: GetMacAddress

	return TRUE;  // return TRUE unless you set the focus to a control
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
// 系統呼叫這個功能取得遊標顯示。
HCURSOR CYUFADlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CYUFADlg::OnClose() 
{
	// TODO: 在此加入您的訊息處理常式程式碼


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
	m_WorkTab.SetFocus();
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
	m_SystemParaTab.UpdateControl();
;
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
	m_MachineTab.UpdateControl();
	m_MachineTab.OpenModBus();

	// OnTcnSelchangeTabMain
	// 獲取 Tab Control 的指針
	//CTabCtrl* pTabCtrl = (CTabCtrl*)GetDlgItem(IDC_TAB_MAIN);
	

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

void CYUFADlg::OnTcnSelchangeTabMain(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此加入控制項告知處理常式程式碼
		// 獲取 Tab Control 的指針
	CTabCtrl* pTabCtrl = (CTabCtrl*)GetDlgItem(IDC_TAB_MAIN);
	if (pTabCtrl != nullptr)
	{
		// 獲取當前選中的 Tab 頁面索引
		int nSel = pTabCtrl->GetCurSel();

		// 根據選中的索引通知相應的子頁面
		switch (nSel)
		{
		case 0: //CCD working
			// 通知第一個子頁面
			m_MachineTab.CloseModBus();
			// focus on WorkTab
			OnBnClickedBtnWorking();
			//m_SystemParaTab.OnTabSelected();
			break;
		case 1: //System Parameter
			// 通知第二個子頁面
			m_MachineTab.CloseModBus();
			//m_SystemParaTab.UpdateControl();
			OnBnClickedBtnSysPara();
			//m_WorkTab.OnTabSelected();
			break;
			// 添加更多的 case 來處理其他子頁面
		case 2://Modbus TCP	
			m_MachineTab.CloseModBus();
			OnBnClickedBtnModbus();
			break;
		case 3://Machine
			m_MachineTab.OpenModBus();
			//m_MachineTab.UpdateControl();
			OnBnClickedBtnMachine();
			
			break;
		default:
			break;
		}
	}
	*pResult = 0;
}

//Read System Parameters from config file
void CYUFADlg::ReadSystemParametersFromConfigFile()
{
	std::string appPath;
	// Get the application path
	appPath = GetAppPath();

	//Set System configuration file name add app path
	CString strConfigFile = _T("SystemConfig.ini");
    // 修正 appPath 與 strConfigFile 的串接方式
    strConfigFile = CString(appPath.c_str()) + _T("\\") + strConfigFile;

	//Call UAX :  SystemConfig ReadSystemConfig(const std::string& filename)
	int rt = ReadSystemConfig(std::string(CT2A(strConfigFile)), m_SystemPara);
	IpAddress = m_SystemPara.IpAddress;
	Port = m_SystemPara.Port;
	

}

// 修改初始化按鈕樣式的函式
void CYUFADlg::InitButtonStyle()
{
    // 建立自訂字體
    m_ButtonFont.CreateFont(
        16,                        // 字體高度
        0,                         // 字體寬度
        0,                         // 文字角度
        0,                         // 基線角度
        FW_BOLD,                   // 字體粗細 (FW_NORMAL, FW_BOLD)
        FALSE,                     // 斜體
        FALSE,                     // 底線
        FALSE,                     // 刪除線
        DEFAULT_CHARSET,           // 字元集
        OUT_DEFAULT_PRECIS,        // 輸出精度
        CLIP_DEFAULT_PRECIS,       // 裁剪精度
        DEFAULT_QUALITY,           // 品質
        DEFAULT_PITCH | FF_SWISS,  // 間距與字型系列
        _T("Microsoft JhengHei")   // 字型名稱
    );
    
    // 設定 MFC Button 的樣式
    // 退出按鈕 - 紅色
    m_BtnQuit.SetFaceColor(RGB(220, 53, 69));      // 紅色背景
    m_BtnQuit.SetTextColor(RGB(255, 255, 255));    // 白色文字
    m_BtnQuit.SetFont(&m_ButtonFont);
    m_BtnQuit.EnableWindowsTheming(FALSE);         // 停用 Windows 主題
    
    // 工作按鈕 - 綠色
    m_BtnWorking.SetFaceColor(RGB(40, 167, 69));   // 綠色背景
    m_BtnWorking.SetTextColor(RGB(255, 255, 255)); // 白色文字
    m_BtnWorking.SetFont(&m_ButtonFont);
    m_BtnWorking.EnableWindowsTheming(FALSE);
    
    // 系統參數按鈕 - 藍色
    m_BtnSysPara.SetFaceColor(RGB(0, 122, 204));   // 藍色背景
    m_BtnSysPara.SetTextColor(RGB(255, 255, 255)); // 白色文字
    m_BtnSysPara.SetFont(&m_ButtonFont);
    m_BtnSysPara.EnableWindowsTheming(FALSE);
    
    // Modbus 按鈕 - 橙色
    m_BtnModbus.SetFaceColor(RGB(255, 193, 7));    // 橙色背景
    m_BtnModbus.SetTextColor(RGB(0, 0, 0));        // 黑色文字
    m_BtnModbus.SetFont(&m_ButtonFont);
    m_BtnModbus.EnableWindowsTheming(FALSE);
    
    // 機器按鈕 - 紫色
    m_BtnMachine.SetFaceColor(RGB(108, 117, 125));  // 灰色背景
    m_BtnMachine.SetTextColor(RGB(255, 255, 255));  // 白色文字
    m_BtnMachine.SetFont(&m_ButtonFont);
    m_BtnMachine.EnableWindowsTheming(FALSE);
}

void CYUFADlg::ApplyButtonStyle()
{
    // 這個函式現在不需要了，因為 MFC Button 有自己的樣式設定方法
    // 保留函式以免破壞現有的呼叫，但內容可以清空或移除
}

HBRUSH CYUFADlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
    
    // 由於使用 MFC Button，不再需要手動處理按鈕顏色
    // MFC Button 會自動處理顏色
    
    return hbr;
}

CYUFADlg::~CYUFADlg()
{
    // 清理字體和筆刷資源
    if (m_ButtonFont.GetSafeHandle())
        m_ButtonFont.DeleteObject();
    
    if (m_ButtonBrush.GetSafeHandle())
        m_ButtonBrush.DeleteObject();
}
