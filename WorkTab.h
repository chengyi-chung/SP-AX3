#pragma once
#include "afxdialogex.h"


// WorkTab 對話方塊

class WorkTab : public CDialogEx
{
	DECLARE_DYNAMIC(WorkTab)

public:
	WorkTab(CWnd* pParent = nullptr);   // 標準建構函式
	virtual ~WorkTab();

// 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_OLE_PROPPAGE_LARGE };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支援

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedWorkGrab();
};
