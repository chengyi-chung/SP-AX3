#pragma once
#include "afxdialogex.h"
#include <opencv2/opencv.hpp>


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
	CBrush m_brush;


protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支援
	virtual BOOL OnInitDialog(); // 新增的初始化函數

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedWorkGrab();

public:
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

};
