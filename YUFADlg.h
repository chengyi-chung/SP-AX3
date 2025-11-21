// YUFADlg.h: 標頭檔
//
#pragma once
#include "UAX.h"
#include "afxwin.h"
#include "afxcmn.h"
#include "afxbutton.h"  // 加入 MFC Button 標頭檔
#include "SystemParaTab.h"
#include "WorkTab.h"
#include "UModBus.h"
#include "MachineTab.h"

#include <thread> // for std::this_thread::sleep_for
#include <chrono> // for std::chrono::milliseconds
#include <mutex>
#include "modbus.h" // 請確保已包含 modbus 函式庫標頭

// CYUFADlg 對話方塊
class CYUFADlg : public CDialogEx
{
// 建構
public:
	CYUFADlg(CWnd* pParent = nullptr);	// 標準建構函式
	virtual ~CYUFADlg();                // <--- 加入這一行

	//System Configuration file name constant at application path
	const std::string SystemConfigFileName = "SystemConfig.ini";
	//SystemPara m_SystemPara;
	SystemConfig m_SystemPara; // System Configuration

	//Read System Parameters from config file
	void ReadSystemParametersFromConfigFile();
	std::string IpAddress;
	int Port;
	
// 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_YUFA_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支援

// 程式碼實作
protected:
	HICON m_hIcon;


	//CStatic m_StatusBar;
	//Add CTime m_Time,for display time
	CTime m_Time;
	
	// Tab Control
	CTabCtrl m_Tab_Main;
	// Add CSystemParaTab m_SystemParaTab
    SystemParaTab m_SystemParaTab;
	// Add CWorkTab m_WorkTab
	WorkTab m_WorkTab;
	//Add  UModBus  m_ModBusTab
	UModBus m_ModBusTab;
	//Add MachineTab m_MachineTab
	MachineTab m_MachineTab;

	//Add CStatusBar m_Status_Bar
	CStatusBar m_Status_Bar;

	// MFC Button 控制項
	CMFCButton m_BtnQuit;       // 退出按鈕
	CMFCButton m_BtnWorking;    // 工作按鈕
	CMFCButton m_BtnSysPara;    // 系統參數按鈕
	CMFCButton m_BtnModbus;     // Modbus 按鈕
	CMFCButton m_BtnMachine;    // 機器按鈕
	
protected:
    CFont m_ButtonFont;          // 按鈕字體
    CBrush m_ButtonBrush;        // 按鈕背景筆刷
    COLORREF m_ButtonTextColor;  // 按鈕文字顏色
    COLORREF m_ButtonBkColor;    // 按鈕背景顏色


	// 產生的訊息對應函式
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnClose();
	virtual void OnOK();
	afx_msg void OnBnClickedBtnQuit();
	virtual void OnCancel();
	afx_msg void OnTimer(UINT_PTR nIDEvent);


	afx_msg void OnBnClickedBtnSysPara();
	afx_msg void OnBnClickedBtnWorking();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnBnClickedBtnModbus();
	afx_msg void OnBnClickedBtnMachine();
	afx_msg void OnNMRClickTabMain(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTcnSelchangeTabMain(NMHDR* pNMHDR, LRESULT* pResult);

protected:
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

public:
    void InitButtonStyle();   // 初始化按鈕樣式
    void ApplyButtonStyle();  // 套用按鈕樣式

    // 共用 Modbus 連線物件與互斥鎖
    modbus_t* m_modbusCtx = nullptr;
    std::mutex m_modbusMutex;

    // Modbus 連線重試機制
    bool InitModbusWithRetry(const std::string& ip, int port, int slaveId, int maxRetry, int retryDelayMs);
	bool EnsureModbusConnected(int stationID, int retryCount = 3);
	bool InitModbusWithRetry(const CString& ip, int port, int slaveID, int timeoutMs = 1000, int retry = 3);

};