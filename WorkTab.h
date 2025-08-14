#pragma once
#include "afxdialogex.h"
#include <atlimage.h>
#include <opencv2/opencv.hpp>
#include <pylon/PylonIncludes.h>

//add UAX.h
#include "UAX.h"
#include "Calibration.h"


// WorkTab 對話方塊
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
	WorkTab(CWnd* pParent = nullptr);   // 標準建構函式
	virtual ~WorkTab();

// 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_OLE_PROPPAGE_LARGE };
#endif

public:
	//flag of Brab Thread
	bool m_bGrabThread;

	//mouse position on Picture Control
	CPoint m_MousePos;

	//String of mouse position
	CString m_strMousePos;

	struct SystemPara
	{
		int iStart; //Coil Start flag : 0:stop 1:start 
		float OffsetX;
		float OffsetY;
		int iPara4;
	};
	SystemPara m_SystemPara;

	ToolPath toolPath;
	//ToolPathData Array of Tool Path : Max 20000
	//index 0~9999: x
	//index 10000~19999: y
	uint16_t m_ToolPathData[20000];

	//flag of center cross line the image
	bool flgCenter;



protected:
	CBrush m_brush;

	
	//Pylon::CInstantCamera camera;

	//static CGrabResultPtr ptrGrabResult;


	// Declare CInstantCamera object
	//UINT AFX_CDECL GrabThread(LPVOID pParam);

	//CGrabResultPtr ptrGrabResult;
	// Add a multi-treaded grabber with Basler Pylon
	static UINT GrabThread(LPVOID pParam);

	// m_mat 儲存 cv::Mat
	cv::Mat m_mat;

	//m_matTemp 儲存 cv::Mat for template image
	cv::Mat m_matTemp;



	// m_image 儲存 CImage
	CImage m_image;
	CDC* pDC;
	CWnd* pWnd;

	// 影像数据指標
	uint8_t* pImageBuffer = nullptr;
	// 调整大小后的影像数据指標
	uint8_t* pResizedImage = nullptr;

	int oriImageWidth;
	int oriImageHeight;

	//structure of system parameter for YUFA system


	//Array Data of Tool Path : Max 20000
	//Convert toolPath to m_ToolPathData[20000]
	//toolPath: Tool Path
	//m_ToolPathData: Tool Path Data Array
	//toolPath.Path : Path of the tool
	//Convert toolPath.Path to m_ToolPathData[20000]
	void ToolPathTransform(ToolPath& toolpath, uint16_t* m_ToolPathData);

	//

	void ShowImageOnPictureCtl(); // 在Picture Control上直接显示图像的函数。

	//void ShowImageOnPictureControl(); // 在Picture Control上显示图像的函数。

	void ShowImageOnPictureControl(bool flgCenter = false,
	                                                    	cv::Scalar crossColor = cv::Scalar(0, 0, 255, 255),
		                                                    int lineThickness = 1,
		                                                    CrossStyle style = CrossStyle::Solid);

	void ShowImageOnPictureControlWithCImage();

	void ResizeGrayImage(uint8_t* pImageBuffer, int originalWidth, int originalHeight, uint8_t*& pResizedBuffer, int targetWidth, int targetHeight);

	void DisplayGrayImageInControl(uint8_t* pImage, int width, int height, CStatic& pictureControl);

	// Show Image with OpenCV
	// cv::Mat m_mat : Dispaly Image Data
	// int ScreenHeight : Screen Height
	// int ScreenWidth : Screen Width
	// ScreenHeight and ScreenWidth are used to resize the image to fit the screen
	void ShowImageWithOpenCV(cv::Mat m_mat, int ScreenHeight, int ScreenWidth);

	//Get tools path from the image
	// cv::Mat& src: the input image
	// ToolPath: the output tool path
	void GetToolPathData(cv::Mat& ImgSrc, cv::Point2d Offset, ToolPath& toolpath);

	//flag of Brab Thread
	//bool m_bGrabThread;

	//Display MyImage in the dialog IDC_PICCTL_DISPLAY
	CStatic m_PicCtl_Display;
	//Add a button IDC_WORK_GRAB
	CButton m_Work_Grab;
	//Add a button IDC_WORK_STOP
	void DrawPicToHDC(cv::Mat cvImg, UINT ID, bool bOnPaint);



	//Send Tm_ToolPath[] Data to PLC with Modbus TCP
	//m_ToolPathData[]: Tool Path Data Array
	//int sizeOfArray: Size of the Tool Path Data Array
	void SendToolPathData(uint16_t *m_ToolPathData, int sizeOfArray, int stationID);

	void SendToolPathDataA(uint16_t* m_ToolPathData, int sizeOfArray, int stationID);

	//void SendToolPathData(m_ToolPathData, int sizeOfArray);


	HICON m_hIcon; // 這裡宣告 m_hIcon


 //private function
protected:
	//Create a function to convert cv:mat to CImage
	void MatConvertCimg(cv::Mat mat, CImage *CImg, int Width, int Height);

	//Add PreTranslateMessage
	virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支援
	virtual BOOL OnInitDialog(); // 新增的初始化函數

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedWorkGrab();

public:
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

	afx_msg void OnPaint();
	afx_msg void OnBnClickedWorkStopGrab();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	//afx_msg void OnBnClickedWorkTempImg();
	//afx_msg void OnBnClickedWorkMatchTemp();
	//afx_msg void OnBnClickedIdcWorkToolPath();
	afx_msg void OnBnClickedWorkTempImg();
	afx_msg void OnBnClickedWorkMatchTemp();
	afx_msg void OnBnClickedIdcWorkToolPath();
	afx_msg void OnBnClickedIdcWorkLoadImg();
	afx_msg void OnBnClickedIdcWorkSaveImg();
	afx_msg void OnBnClickedIdcWorkGo();

	//Add Calibration Dialog
	afx_msg void OnBnClickedIdcWorkCalibration();
};
