#pragma once
#include "afxdialogex.h"
//add libmodbus header
#include <modbus.h>
#include <bitset>

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

	//Open Modbus TCP/IP server 
	void OpenModBus();
	//send data to modbus server
	void SendDataToModBus();
	//Close Modbus TCP/IP server
	void CloseModBus();
	//define modbus context
	modbus_t* m_ctx;

	//define Report Data on  IDC_EDIT_REPORT
	string m_strReportData;

	//Update data in Edit control with SystemConfig m_SystemPara
	void UpdateControl();

	// 執行緒相關
	static UINT ReadCoordinatesThread(LPVOID pParam); // 執行緒函數
	void StartCoordinateThread(); // 啟動執行緒
	void StopCoordinateThread();  // 停止執行緒

	

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支援

	//initial function
	virtual BOOL OnInitDialog();

	//define Holding Register bitset for Modbus
	bitset<16> Discrete3000;
	bitset<16> Discrete5000;
	//Convert bitset to word
	uint16_t Discrete3000Word = 0;
	uint16_t Discrete5000Word= 0;

	//Define Holding Register  bitset for Modbus
	bitset<16> HoldingRegister5000;
	


	void ConvertBitsetToWord(bitset<16> bitset, uint16_t* word);
	//Convert word to bitset
	void ConvertWordToBitset(uint16_t word, bitset<16>* bitset);

	//define functio for Discrete3000 value change in Control
	//intType: 0: check 1: button
	//BitAdress: Bit Adress
	//BitValue: Bit Value
	//nID: check or button Control ID
	void Discrete3000Change(int intType, int BitAdress, int BitValue, int nID);

	//Clear All Discrete3000
	//int Start Adress: iStartAdress
	//int End Adress: iEndAdress
	void ClearDiscrete3000(int iStartAdress, int iEndAdress);

	//define functio for Discrete5000 value change in Control
	//intType: 0: check 1: button
	//BitAdress: Bit Adress
	//BitValue: Bit Value
	//nID: check or button Control ID
	void Discrete5000Change(int intType, int BitAdress, int BitValue, int nID);

	//Clear All Discrete3000
	//int Start Adress: iStartAdress
	//int End Adress: iEndAdress
	void ClearDiscrete5000(int iStartAdress, int iEndAdress);

	//Set Holding Register value
	//iStartAdress: Start Address
	//iEndAdress: End Address
	//int iValue[]: Value array to be set
	void SetHoldingRegister(int iStartAdress, int iEndAdress, uint16_t* iValue, int SizeOfArray);
	//Get Holding Register value
	//iStartAdress: Start Address
	//iEndAdress: End Address
	//int iValue[]: Value array to be get
	void GetHoldingRegister(int iStartAdress, int iEndAdress, uint16_t*  iValue);

	


	// Double word (DINT) functions
	//Set Holding Register value
	//iStartAdress: Start Address
	//iEndAdress: End Address
	//int iValue[]: Value array to be set
	void SetHoldingRegisteDInt(int iStartAdress, int iEndAdress, uint16_t* iValue, int SizeOfArray);
	//Get Holding Register value
	//iStartAdress: Start Address
	//iEndAdress: End Address
	//int iValue[]: Value array to be get
	void GetHoldingRegisterDInt(int iStartAdress, int iEndAdress, uint16_t* iValue);





	//define machine mode : Auto or Manual
	//0: Manual 1: Auto
	int m_iMachineMode = 0;

	//flgGetCoord : TRUE: get coordinate FALSE: not get coordinate
	BOOL flgGetCoord = FALSE;

	//
	//BOOL m_bXPlusPressed;   // X+ 按鈕狀態
	//BOOL m_bXMinusPressed;  // X- 按鈕狀態
	//UINT m_nActiveButton;   // 記錄目前按下的按鈕 ID

	DECLARE_MESSAGE_MAP()

	bool isValidFloat(const std::string& str, float& outValue);

public:
	//afx_msg void OnBnClickedBtnJogXPlus();
	//afx_msg void OnBnClickedBtnJogXMinux();
	//afx_msg void OnBnClickedBtnJogYPlus();
	//afx_msg void OnBnClickedBtnJogYMinus();
	//afx_msg void OnBnClickedBtnJogZPlus();
	//afx_msg void OnBnClickedBtnJogZMinus();
	afx_msg void OnBnClickedRadioAuto();
	afx_msg void OnBnClickedRadioManual();
	//afx_msg void OnBnClickedCheckHome();
	//afx_msg void OnBnClickedCheckReset();
	//afx_msg void OnBnClickedCheckAutoWorkStart();
	//afx_msg void OnBnClickedCheckAutoWorkStop();
	afx_msg void OnBnClickedBtnMachineSaveMotion();
	// 滑鼠放開事件 (Button Up)
	//afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	// 輔助函式
	//BOOL IsMouseInButton(CWnd* pButton, CPoint point);
	virtual BOOL PreTranslateMessage(MSG* pMsg) override;
	afx_msg void OnBnClickedMfcbtnMachineHome();
	
	afx_msg void OnBnClickedMfcbtnMachineAutoWorkSart();
	afx_msg void OnBnClickedMfcbtnMachineAutoWorkStop();
	afx_msg void OnBnClickedMfcbtnMachineResetSw();
	afx_msg void OnBnClickedMfcbtnMachineGo();
	afx_msg void OnEnChangeEditManualX();
	virtual void OnOK(); // 新增 OnOK 方法


	// 新增的消息處理函數，用於更新 UI
	afx_msg LRESULT OnUpdateCoordinates(WPARAM wParam, LPARAM lParam);


	protected:
		// 執行緒控制
		CWinThread* m_pCoordinateThread; // 執行緒指針
		volatile BOOL m_bThreadRunning;  // 執行緒運行標誌
		HANDLE m_hStopThreadEvent;       // 停止執行緒的事件
		
};
// 自定義消息，用於執行緒通知 UI 更新
//#define WM_UPDATE_COORDINATES (WM_USER + 100)