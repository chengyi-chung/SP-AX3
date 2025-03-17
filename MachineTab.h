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



	//define machine mode : Auto or Manual
	//0: Manual 1: Auto
	int m_iMachineMode = 0;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnJogXPlus();
	afx_msg void OnBnClickedBtnJogXMinux();
	afx_msg void OnBnClickedBtnJogYPlus();
	afx_msg void OnBnClickedBtnJogYMinus();
	afx_msg void OnBnClickedBtnJogZPlus();
	afx_msg void OnBnClickedBtnJogZMinus();
	afx_msg void OnBnClickedRadioAuto();
	afx_msg void OnBnClickedCheckHome();
	afx_msg void OnBnClickedCheckReset();
	afx_msg void OnBnClickedCheckAutoWorkStart();
	afx_msg void OnBnClickedCheckAutoWorkStop();
};
