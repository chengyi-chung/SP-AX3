#pragma once
#include "afxdialogex.h"
#include <atlimage.h>
#include <opencv2/opencv.hpp>
#include <pylon/PylonIncludes.h>
#include "UAX.h"
#include "Calibration.h"
#include "afxcmn.h"
#include "afxbutton.h" // 加入 MFC Button 支援

using namespace Pylon;

enum class CrossStyle
{
	Solid,
	Dashed
};

class WorkTab : public CDialogEx
{
	DECLARE_DYNAMIC(WorkTab)

public:
	WorkTab(CWnd* pParent = nullptr);
	virtual ~WorkTab();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_OLE_PROPPAGE_LARGE };
#endif

public:
	bool m_bGrabThread;
	CPoint m_MousePos;
	CString m_strMousePos;

	struct SystemPara
	{
		int iStart;
		float OffsetX;
		float OffsetY;
		int iPara4;
	};
	SystemPara m_SystemPara;

	ToolPath toolPath;
	uint16_t m_ToolPathData[20000];
	bool flgCenter;

protected:
	CBrush m_brush;
	static UINT GrabThread(LPVOID pParam);
	cv::Mat m_mat;
	cv::Mat m_matTemp;
	CImage m_image;
	CDC* pDC;
	CWnd* pWnd;
	uint8_t* pImageBuffer = nullptr;
	uint8_t* pResizedImage = nullptr;
	int oriImageWidth;
	int oriImageHeight;
	int imgFlip;

	void ToolPathTransform(ToolPath& toolpath, uint16_t* m_ToolPathData);
	void ShowImageOnPictureCtl();
	void ShowImageOnPictureControl(bool flgCenter = false,
		cv::Scalar crossColor = cv::Scalar(0, 0, 255, 255),
		int lineThickness = 1,
		CrossStyle style = CrossStyle::Solid);
	void ShowImageOnPictureControlWithCImage();
	void ResizeGrayImage(uint8_t* pImageBuffer, int originalWidth, int originalHeight, uint8_t*& pResizedBuffer, int targetWidth, int targetHeight);
	void DisplayGrayImageInControl(uint8_t* pImage, int width, int height, CStatic& pictureControl);
	void ShowImageWithOpenCV(cv::Mat m_mat, int ScreenHeight, int ScreenWidth);
	void GetToolPathData(cv::Mat& ImgSrc, cv::Point2d Offset, ToolPath& toolpath);

	CStatic m_PicCtl_Display;
	CMFCButton m_Work_Grab;           // MFC Button
	CMFCButton m_Work_StopGrab;       // MFC Button
	CMFCButton m_Work_TempImg;        // MFC Button
	CMFCButton m_Work_MatchTemp;      // MFC Button
	CMFCButton m_Work_ToolPath;       // MFC Button
	CMFCButton m_Work_LoadImg;        // MFC Button
	CMFCButton m_Work_SaveImg;        // MFC Button
	CMFCButton m_Work_Go;             // MFC Button
	CMFCButton m_Btn_Calibration;     // MFC Button
	CMFCButton m_btnExample;          // 範例 MFC Button

	CFont m_btnFont;                 // 按鈕字型
	COLORREF m_btnTextColor;         // 文字顏色
	COLORREF m_btnBkColor;           // 背景顏色
	CBrush m_btnBkBrush;             // 背景刷子

	void DrawPicToHDC(cv::Mat cvImg, UINT ID, bool bOnPaint);
	void SendToolPathData(uint16_t *m_ToolPathData, int sizeOfArray, int stationID);
	void SendToolPathDataA(uint16_t* m_ToolPathData, int sizeOfArray, int stationID);
	HICON m_hIcon;

protected:
	void MatConvertCimg(cv::Mat mat, CImage *CImg, int Width, int Height);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedWorkGrab();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnPaint();
	afx_msg void OnBnClickedWorkStopGrab();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnBnClickedWorkTempImg();
	afx_msg void OnBnClickedWorkMatchTemp();
	afx_msg void OnBnClickedIdcWorkToolPath();
	afx_msg void OnBnClickedIdcWorkLoadImg();
	afx_msg void OnBnClickedIdcWorkSaveImg();
	afx_msg void OnBnClickedIdcWorkGo();
	afx_msg void OnBnClickedIdcWorkCalibration();
};
