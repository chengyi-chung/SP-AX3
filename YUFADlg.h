
// YUFADlg.h: 標頭檔
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "SystemParaTab.h"
#include "WorkTab.h"
#include "UModBus.h"
#include "MachineTab.h"



// CYUFADlg 對話方塊
class CYUFADlg : public CDialogEx
{
// 建構
public:
	CYUFADlg(CWnd* pParent = nullptr);	// 標準建構函式

	/*
	struct SystemPara
	{
		int iStart; //Coil Start flag : 0:stop 1:start
		float OffsetX;  //Tool Path Offset X
		float OffsetY;  //Tool Path Offset Y
		char* IpAddress ; //Modbus TCP/IP IP Address
		int StationID; //Modbus TCP/IP Station ID
	};
	*/

	
    struct SystemPara
    {
        int iStart; //Coil Start flag : 0:stop 1:start 
        float OffsetX;  //Tool Path Offset X
        float OffsetY;  //Tool Path Offset Y
        //wchar_t IpAddress[16]; //Modbus TCP/IP IP Address
		char IpAddress[16]; //Modbus TCP/IP IP Address
        int StationID; //Modbus TCP/IP Station ID
    };

	SystemPara m_SystemPara;
	
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
	//Add CTime m_Time, for display time
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
};
