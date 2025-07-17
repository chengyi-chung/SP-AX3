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

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支援

	//initial function
	virtual BOOL OnInitDialog();

	//define Holding Register bitset for Modbus
	bitset<16> Discrete3000;
	//Convert bitset to word
	uint16_t Discrete3000Word = 0;
	void ConvertBitsetToWord(bitset<16> bitset, uint16_t* word);
	//Convert word to bitset
	void ConvertWordToBitset(uint16_t word, bitset<16>* bitset);

	//define functio for Discrete3000 value change in Control
	//intType: 0: check 1: button
	//BitAdress: Bit Adress
	//BitValue: Bit Value
	//nID: check or button Control ID
	void Discrete3000Change(int intType, int BitAdress, int BitValue, int nID);

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

	//Clear All Discrete3000
	//int Start Adress: iStartAdress
	//int End Adress: iEndAdress
	void ClearDiscrete3000(int iStartAdress, int iEndAdress);



	//define machine mode : Auto or Manual
	//0: Manual 1: Auto
	int m_iMachineMode = 0;

	//
	//BOOL m_bXPlusPressed;   // X+ 按鈕狀態
	//BOOL m_bXMinusPressed;  // X- 按鈕狀態
	//UINT m_nActiveButton;   // 記錄目前按下的按鈕 ID

	DECLARE_MESSAGE_MAP()

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
};
