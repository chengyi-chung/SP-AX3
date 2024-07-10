#pragma once
#include "afxdialogex.h"
#include "modbus.h"


// UModBus 對話方塊

class UModBus : public CDialog
{
	DECLARE_DYNAMIC(UModBus)

public:
	UModBus(CWnd* pParent = nullptr);   // 標準建構函式
	virtual ~UModBus();

public:
	//Add modbus tcp/ip test function
	char* ip_address;
	int ModbusTcpIpTest(const char* ip_address);

	CButton* m_chk_coil; 
	CButton* m_chk_discrete; 
	CButton* m_chk_input_reg;
	CButton* m_chk_holding_reg; 

	//Enum for modbus function code
	enum ModbusFunctionCode
	{
		Coil = 1,
		Discrete = 2,
		InputReg = 4,
		HoldingReg = 3
	};
	ModbusFunctionCode m_modbus_function_code;

// 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG1 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支援

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnEnChangeEditIpAddress();
	afx_msg void OnBnClickedBtnModbusTest();
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedModbusChkCoil();
	afx_msg void OnBnClickedModbusChkDiscrete();
	afx_msg void OnBnClickedModbusChkInputReg();
	afx_msg void OnBnClickedModbusChkHoldingReg();
};
