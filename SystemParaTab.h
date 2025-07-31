#pragma once
#include "afxdialogex.h"


// SystemParaTab 對話方塊

class SystemParaTab : public CDialog
{
	DECLARE_DYNAMIC(SystemParaTab)

public:
	SystemParaTab(CWnd* pParent = nullptr);   // 標準建構函式
	virtual ~SystemParaTab();

	virtual void OnOK();

	void UpdateControl();

// 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_OLE_PROPPAGE_LARGE1 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支援

	DECLARE_MESSAGE_MAP()


public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedSystemCreateData();
	afx_msg void OnEnChangeTabSysOffsetValue();
	afx_msg void OnBnClickedMfcbtnSaveSystem();
};
