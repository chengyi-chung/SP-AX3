#pragma once
#include "afxdialogex.h"
#include <opencv2/opencv.hpp>
#include <pylon/PylonIncludes.h>



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

	// Declare CInstantCamera object
	Pylon::CInstantCamera camera;
	// Add a multi-treaded grabber with Basler Pylon
	static UINT GrabThread(LPVOID pParam);
	cv::Mat m_Image;
	//Display MyImage in the dialog IDC_PICCTL_DISPLAY
	CStatic m_PicCtl_Display;
	//Add a button IDC_WORK_GRAB
	CButton m_Work_Grab;
	//Add a button IDC_WORK_STOP


protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支援
	virtual BOOL OnInitDialog(); // 新增的初始化函數

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedWorkGrab();

public:
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

};
