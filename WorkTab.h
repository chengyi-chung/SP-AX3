#pragma once
#include "afxdialogex.h"
#include <atlimage.h>
#include <opencv2/opencv.hpp>
#include <pylon/PylonIncludes.h>
#include "UAX.h"
#include "UAXVision.h"
#include "afxcmn.h"
#include "afxbutton.h" // 加入 MFC Button 支援
#include "UAXTypes.h"

using namespace Pylon;

enum class CrossStyle
{
	Solid,
	Dashed
};


//float imagePts[6] = { 1035, 844, 1311, 1247, 1511, 963 };  // 像素點座標
//float worldPts[6] = { -0.01f, 67.59f, 150.79f, 288.83f, 259.71f, 134.03f };  // 對應世界座標 (mm)

class WorkTab : public CDialogEx
{
	DECLARE_DYNAMIC(WorkTab)

public:
	WorkTab(CWnd* pParent = nullptr);
	virtual ~WorkTab();

	UAXVision m_vision;

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

	ToolPath toolPath;     // ToolPath in image coordinate : Pixel
	ToolPath toolPath_world;   // ToolPath in world coordinate : mm

	uint16_t m_ToolPathData[30000];
	bool flgCenter;
	// 可選：用來記錄是否處於「ROI 選取模式」
	bool m_bROIMode = false;

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

	
	int MaskX;
	int MaskY;
	int MaskWidth;
	int MaskHeight;
	int referenceX;
	int referenceY;

	GluePath m_OptimizedGluePath;  // 優化後膠路，原點為影像左上角，單位為 pixel
	GluePath m_machineGluePath;  // 以 referenceX/referenceY 為機械原點的膠路，單位為 pixel
	GluePath m_machineGluePath_mm;  // 以機械原點為基準的膠路，單位為 mm
	GluePath m_HMIGluePath_temp;  // HMI 暫存座標，定義為 machine mm * 10
	GluePath m_HMIGluePath;  // 最終送往 HMI 的顯示座標，為 HMI temp 直接取整數後的結果



	void ConvertToMachineCoordinates();
	//void convertToMachinePath(const GluePath& optimizedPath, GluePath& machinePath, const Point2d& machineOrigin);


	void ToolPathTransform(ToolPath& toolpath, uint16_t* m_ToolPathData);
	void ToolPathTransform32(ToolPath ToolPapath_Ori, uint16_t* m_ToolPathData);
	void ToolPathTransform32A(ToolPath ToolPapath_Ori, uint16_t* m_ToolPathData , size_t outCapacity, float z_Machining, float z_Retract);
	
	float z_Machining = -1.0f;  //加工高度
	float z_Retract = 5.0f;     //退回高度
	std::vector<uint16_t> m_ToolPathDataA;   // 用於 ToolPathTransform32A 的動態陣列, 修改於運動點定義 由 x,y 改為 x,y,z 2025.11.18
	void ToolPathTransform32B(ToolPath ToolPath_Ori, float z_Machining, float zRetract);  //修改於運動點定義 由 x,y 改為 x,y,z 2025.11.18

	void SendToolPathData(uint16_t* m_ToolPathData, int sizeOfArray, int stationID);
	void SendToolPathDataA(uint16_t* m_ToolPathData, int sizeOfArray, int stationID);
	void SendToolPathData32(uint16_t* m_ToolPathData, int sizeOfArray, int stationID);   //modbus tcp 傳送 ToolPath Data 32bit
	//void SendToolPathData32A(std::vector<uint16_t> m_ToolPathDataA, int sizeOfArray, int stationID); //點定義 由 x,y 改為 x,y,z 2025.11.18
	void SendToolPathData32A(const std::vector<uint16_t>& data, int sizeOfArray, int stationID); //點定義 由 x,y 改為 x,y,z 2025.11.18
	//HMI Test Read Holding Registers
	void HMIReadHoldingRegistersTest(int stationID = 1);




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
	CMFCButton m_btnExample;          // 範例 MFC Button
	CMFCButton m_Work_ImageProcess;   // 圖像處理 MFC Button

	CFont m_btnFont;                 // 按鈕字型
	COLORREF m_btnTextColor;         // 文字顏色
	COLORREF m_btnBkColor;           // 背景顏色
	CBrush m_btnBkBrush;             // 背景刷子

	CFont m_fontBoldBig;

	void DrawPicToHDC(cv::Mat cvImg, UINT ID, bool bOnPaint);

	// 一次讀取 139~156 共 18 個寄存器
    // outValues 會被 resize 成 18 個元素，索引 0 對應 139，索引 17 對應 156
	bool ReadSystemParaBatch_139_to_156(std::vector<uint16_t>& outValues, int stationID = 1);

	// 一次寫入 139~156 共 18 個寄存器
	// inValues 必須正好有 18 個元素，索引 0 寫入 139，索引 17 寫入 156
	bool WriteSystemParaBatch_139_to_156(const std::vector<uint16_t>& inValues, int stationID = 1);

	// 方便函數：讀取後自動更新到 pParent->m_SystemPara 的對應欄位
	bool SyncReadAndUpdateSystemPara(int stationID = 1);

	// 方便函數：從 m_SystemPara 收集值 → 寫入 PLC
	bool SyncWriteFromSystemPara(int stationID = 1);
	bool SyncReadAndUpdateMemStruct(int stationID = 1);
	bool SyncWriteFromMemStruct(int stationID = 1);
	

	

	HICON m_hIcon;

private:
	enum : UINT_PTR {
		kHmiSyncTimerId = 0x5101
	};

	bool m_bDrawingROI = false;        // 是否正在拖曳框選
	CPoint m_ROIStart;                 // ROI 起始點（Picture Control 客戶區座標）
	CPoint m_ROICurrent;               // ROI 當前點（拖曳時的終點）
	CRect m_SelectedROI;               // 最終確定的 ROI（像素座標，對應原始圖像）

	bool m_bROIConfirmed = false;      // 是否已確認 ROI（可選）
	bool m_bFactorSelectMode = false;  // 是否正在進行 Factor 框選
	double m_pendingGridLengthMm = 0.0;
	cv::Mat m_factorPreviewMat;        // Factor 計算用的校正後預覽圖
	std::vector<cv::Point2f> m_factorCorners;
	cv::Size m_factorBoardSize;
	bool m_hmiSyncEnabled = false;
	bool m_hmiSyncBusy = false;
	UINT m_hmiSyncIntervalMs = 300;
	SystemConfigA m_lastSyncedSystemPara{};
	MemStruct_SP m_lastSyncedMemStruct{};

	void StartHmiSyncTimer();
	void StopHmiSyncTimer();
	void SyncHmiData(int stationID = 1);
	bool ReadHoldingRegistersBlock(int startAddress, int count, std::vector<uint16_t>& outValues, int stationID = 1);
	bool WriteHoldingRegistersBlock(int startAddress, const std::vector<uint16_t>& values, int stationID = 1);
	void BuildSystemConfigRegisters(const SystemConfigA& src, std::vector<uint16_t>& outValues) const;
	void ApplySystemConfigRegisters(const std::vector<uint16_t>& values, SystemConfigA& dst) const;
	void BuildMemStructRegisters(const MemStruct_SP& src, std::vector<uint16_t>& outValues) const;
	void ApplyMemStructRegisters(const std::vector<uint16_t>& values, MemStruct_SP& dst) const;
	bool IsSystemConfigDisplayDataValid(const SystemConfigA& value) const;
	bool IsSystemConfigEqual(const SystemConfigA& lhs, const SystemConfigA& rhs) const;
	bool IsMemStructEqual(const MemStruct_SP& lhs, const MemStruct_SP& rhs) const;

	




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
 afx_msg void OnDestroy();
	afx_msg void OnBnClickedWorkStopGrab();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnBnClickedWorkTempImg();
	afx_msg void OnBnClickedWorkMatchTemp();
	afx_msg void OnBnClickedIdcWorkToolPath();
	afx_msg void OnBnClickedIdcWorkLoadImg();
	afx_msg void OnBnClickedIdcWorkSaveImg();
	afx_msg void OnBnClickedIdcWorkGo();
	afx_msg void OnBnClickedCheckWorkCenter();
	afx_msg void OnBnClickedWorkImageProcess();
	afx_msg void OnBnClickedMfcbtnWorkImgCalibrate();

	// 新增：讀取 Holding Registers
	bool ReadModbusRegisters(int startAddress, int numRegisters, std::vector<uint16_t>& outRegs, int stationID = 1);
	afx_msg void OnBnClickedCheckWorkRoi();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedMfcbtnWorkImgFactor();
};

