
// YUFADlg.h: 標頭檔
//

#pragma once


// CYUFADlg 對話方塊
class CYUFADlg : public CDialogEx
{
// 建構
public:
	CYUFADlg(CWnd* pParent = nullptr);	// 標準建構函式

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

	//Add CStatusBar m_Status_Bar
	CStatusBar m_Status_Bar;
	CTabCtrl m_Tab_Main;
};
