#pragma once
#include "afxdialogex.h"


// MachineTab 對話方塊

class MachineTab : public CDialog
{
	DECLARE_DYNAMIC(MachineTab)

public:
	MachineTab(CWnd* pParent = nullptr);   // 標準建構函式
	virtual ~MachineTab();

// 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TAB_MACHINE };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支援

	DECLARE_MESSAGE_MAP()
};
