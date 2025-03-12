#pragma once
#include "afxdialogex.h"
//add libmodbus header
#include <modbus.h>


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

	//initial function
	virtual BOOL OnInitDialog();

	//

	//define modbus context
	modbus_t* m_ctx;


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
