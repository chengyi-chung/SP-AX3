


#pragma once
#include "UAX.h"
#include "afxwin.h"
#include "opencv2/core.hpp"

// CMVCalibrationDlg 對話方塊
class CMVCalibrationDlg : public CDialogEx
{
// 建構
public:
	CMVCalibrationDlg(CWnd* pParent = nullptr);	// 標準建構函式

// 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MV_CALIBRATION_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支援


	// 在 CMVCalibrationDlg 類別宣告 PreTranslateMessage 覆寫
protected:
	virtual BOOL PreTranslateMessage(MSG* pMsg) override;
	// 不需修改此檔案的 PreTranslateMessage 實作，只需確保 .h 有宣告即可
	// 若已存在，請保持原有實作
	// MV CalibrationDlg.h: 標頭檔
	//

// 程式碼實作
protected:
	HICON m_hIcon;

	// 產生的訊息對應函式
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedMfcbtnExit();
};
