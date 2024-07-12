// UModBus.cpp: 實作檔案
//

#include "pch.h"
#include "YUFA.h"
#include "afxdialogex.h"
#include "UModBus.h"
#include "modbus.h"
#include <iostream>
#include <vector>


// UModBus 對話方塊

IMPLEMENT_DYNAMIC(UModBus, CDialog)

UModBus::UModBus(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_TAB_MODBUS, pParent)
{
	// make related check box member variables to identify nunber of check boxes

	

	//delclare the member variables of check boxes
	m_chk_coil = FALSE;
	m_chk_discrete = FALSE;
	m_chk_input_reg = FALSE;
	m_chk_holding_reg = FALSE;


}

UModBus::~UModBus()
{
}

void UModBus::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	//Add DDX for check boxes
	
}


BEGIN_MESSAGE_MAP(UModBus, CDialog)
	ON_EN_CHANGE(IDC_EDIT_IP_ADDRESS, &UModBus::OnEnChangeEditIpAddress)
	ON_BN_CLICKED(IDC_BTN_MODBUS_TEST, &UModBus::OnBnClickedBtnModbusTest)
	ON_BN_CLICKED(IDC_MODBUS_CHK_COIL, &UModBus::OnBnClickedModbusChkCoil)
	ON_BN_CLICKED(IDC_MODBUS_CHK_DISCRETE, &UModBus::OnBnClickedModbusChkDiscrete)
	ON_BN_CLICKED(IDC_MODBUS_CHK_INPUT_REG, &UModBus::OnBnClickedModbusChkInputReg)
	ON_BN_CLICKED(IDC_MODBUS_CHK_HOLDING_REG, &UModBus::OnBnClickedModbusChkHoldingReg)
END_MESSAGE_MAP()


// UModBus 訊息處理常式


void UModBus::OnEnChangeEditIpAddress()
{
	// TODO:  如果這是 RICHEDIT 控制項，控制項將不會
	// 傳送此告知，除非您覆寫 CDialog::OnInitDialog()
	// 函式和呼叫 CRichEditCtrl().SetEventMask()
	// 讓具有 ENM_CHANGE 旗標 ORed 加入遮罩。

	// TODO:  在此加入控制項告知處理常式程式碼
}


void UModBus::OnBnClickedBtnModbusTest()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	//Get the IP address from the edit box
	CString str;
	GetDlgItemText(IDC_EDIT_IP_ADDRESS, str);

	// Use CT2CA for conversion (CString to const char*)
	CT2CA pszConvertedAnsiString(str);
	const char* ip_address = pszConvertedAnsiString;

	//modbus_t* ctx = modbus_new_tcp("127.0.0.1", 502);
	//modbus_t* ctx = modbus_new_tcp(ip_address, 502);
	//int ret = ModbusTcpIpTest(ip_address);

	//if (modbus_connect(ctx) == -1)
	//{
	//	fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
	//	modbus_free(ctx);
	//	//return -1;
	//}
	// Now you have a connected Modbus context (slave).
	// ...
	//modbus_close(ctx);
	//modbus_free(ctx);
	//return 0;

	int ret = ModbusTcpIpTest(ip_address);
	if (ret == -1)
	{
		MessageBox(L"Modbus test failed");
	}
	else
	{
		//MessageBox(L"Modbus test success");

	}

	//Test modbus function code
	modbus_t* ctx = modbus_new_tcp(ip_address, 502);
	if (modbus_connect(ctx) == -1)
	{
		fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
		modbus_free(ctx);
		//return -1;
	}

	// Coil test
	if (m_modbus_function_code == Coil)
	{
		int ret = test_coils(ctx);
		if (ret == -10)
		{
			MessageBox(L"Failed to read coils");
		}
		else if (ret == -11)
		{
			MessageBox(L"Failed to write single coil");
		}
		else if (ret == -12)
		{
			MessageBox(L"Failed to write multiple coils");
		}
		else
		{
			MessageBox(L"Coil test success");
		}
	}




}

//Add modbus tcp/ip test function
//char* ip_address: IP address of the modbus slave
//int port: port number of the modbus slave: 502
//return 0 : new test  success, -1: if failed
//return -1: if failed
//return -10: if failed to read coils
//return -11: if failed to write single coil
//return -12: if failed to write multiple coils
//return -13: if failed to read discrete inputs
int UModBus::ModbusTcpIpTest(const char* ip_address)
{
	//modbus_t* ctx = modbus_new_tcp("127.0.0.1", 502);
	// ip_address : IP address of the modbus server
	// port : port number of the modbus server
	modbus_t* ctx = modbus_new_tcp(ip_address, 502);

	if (modbus_connect(ctx) == -1)
	{
		fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
		modbus_free(ctx);
		return -1;
	}

	// Coil test


	// Now you have a connected Modbus context (slave).
	// ...
	modbus_close(ctx);
	modbus_free(ctx);
	return 0;
}

int UModBus::test_coils(modbus_t* ctx) 
{
	uint8_t coils[10];

	// Read coils
	int rc = modbus_read_bits(ctx, 0, 10, coils);
	if (rc == -1)
	{
		fprintf(stderr, "Failed to read coils: %s\n", modbus_strerror(errno));
		return -10;
	}
	printf("Coils state: ");
	for (int i = 0; i < 10; i++) {
		printf("%d ", coils[i]);
	}
	printf("\n");

	// Write single coil
	rc = modbus_write_bit(ctx, 0, 1);
	if (rc == -1) {
		fprintf(stderr, "Failed to write single coil: %s\n", modbus_strerror(errno));
		return -11;
	}

	// Write multiple coils
	uint8_t coils_to_write[10] = { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 };
	rc = modbus_write_bits(ctx, 0, 10, coils_to_write);
	if (rc == -1) {
		fprintf(stderr, "Failed to write multiple coils: %s\n", modbus_strerror(errno));
		return -12;
	}

	return 1;
}

// Test discrete inputs
// return 2: discrete success
// -20: if failed to read discrete inputs
//
int UModBus::test_discrete_inputs(modbus_t* ctx)
{
	uint8_t discrete_inputs[10];

	// Read discrete inputs
	int rc = modbus_read_input_bits(ctx, 0, 10, discrete_inputs);
	if (rc == -1)
	{
		fprintf(stderr, "Failed to read discrete inputs: %s\n", modbus_strerror(errno));
		return -20;
	}
	printf("Discrete inputs state: ");
	for (int i = 0; i < 10; i++) {
		printf("%d ", discrete_inputs[i]);
	}
	printf("\n");

	return 2;
}

// Test input registers
// return 3: input registers success
// -30: if failed to read input registers
int UModBus::test_input_registers(modbus_t* ctx)
{
	uint16_t input_registers[10];

	// Read input registers
	int rc = modbus_read_input_registers(ctx, 0, 10, input_registers);
	if (rc == -1)
	{
		fprintf(stderr, "Failed to read input registers: %s\n", modbus_strerror(errno));
		return -30;
	}
	printf("Input registers: ");
	for (int i = 0; i < 10; i++) {
		printf("%d ", input_registers[i]);
	}
	printf("\n");
	return 3;
}

// Test holding registers
// return 4: holding registers success
// -40: if failed to read holding registers
// -41: if failed to write single register
// -42: if failed to write multiple registers

int UModBus::test_holding_registers(modbus_t* ctx)
{
	uint16_t holding_registers[10];

	// Read holding registers
	int rc = modbus_read_registers(ctx, 0, 10, holding_registers);
	if (rc == -1) 
	{
		fprintf(stderr, "Failed to read holding registers: %s\n", modbus_strerror(errno));
		return -40;
	}
	printf("Holding registers: ");
	for (int i = 0; i < 10; i++)
	{
		printf("%d ", holding_registers[i]);
	}
	printf("\n");

	// Write single register
	rc = modbus_write_register(ctx, 0, 1234);
	if (rc == -1)
	{
		fprintf(stderr, "Failed to write single register: %s\n", modbus_strerror(errno));
		return -41;
	}

	// Write multiple registers
	uint16_t registers_to_write[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	rc = modbus_write_registers(ctx, 0, 10, registers_to_write);
	if (rc == -1)
	{
		fprintf(stderr, "Failed to write multiple registers: %s\n", modbus_strerror(errno));
		return -42;
	}

}

BOOL UModBus::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此加入額外的初始化
	SetDlgItemText(IDC_EDIT_IP_ADDRESS, L"127.0.0.1");

	// 初始化 Checkbox 控制項指針
	m_chk_coil = (CButton*)GetDlgItem(IDC_MODBUS_CHK_COIL);
	m_chk_discrete = (CButton*)GetDlgItem(IDC_MODBUS_CHK_DISCRETE);
	m_chk_input_reg = (CButton*)GetDlgItem(IDC_MODBUS_CHK_INPUT_REG);
	m_chk_holding_reg = (CButton*)GetDlgItem(IDC_MODBUS_CHK_HOLDING_REG);

	m_chk_coil->SetCheck(1);
	m_chk_discrete->SetCheck(0);
	m_chk_input_reg->SetCheck(0);
	m_chk_holding_reg->SetCheck(0);
	m_modbus_function_code = Coil;
    

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX 屬性頁應傳回 FALSE
}


void UModBus::OnBnClickedModbusChkCoil()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	// Set ckeckbox to true and others to false

		// 使用指針設置 Checkbox 狀態
	if (m_chk_coil != nullptr)
	{
		m_chk_coil->SetCheck(BST_CHECKED);
		m_chk_discrete->SetCheck(BST_UNCHECKED);
		m_chk_input_reg->SetCheck(BST_UNCHECKED);
		m_chk_holding_reg->SetCheck(BST_UNCHECKED);
		m_modbus_function_code = Coil;
	}
	
	UpdateData(TRUE);
	
}


void UModBus::OnBnClickedModbusChkDiscrete()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	if (m_chk_discrete != nullptr)
	{
		m_chk_coil->SetCheck(BST_UNCHECKED);
		m_chk_discrete->SetCheck(BST_CHECKED);
		m_chk_input_reg->SetCheck(BST_UNCHECKED);
		m_chk_holding_reg->SetCheck(BST_UNCHECKED);
		m_modbus_function_code = Discrete;
	}
	UpdateData(TRUE);

}


void UModBus::OnBnClickedModbusChkInputReg()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	if (m_chk_input_reg != nullptr)
	{
		m_chk_coil->SetCheck(BST_UNCHECKED);
		m_chk_discrete->SetCheck(BST_UNCHECKED);
		m_chk_input_reg->SetCheck(BST_CHECKED);
		m_chk_holding_reg->SetCheck(BST_UNCHECKED);
		m_modbus_function_code = InputReg;
	}
	UpdateData(TRUE);
}


void UModBus::OnBnClickedModbusChkHoldingReg()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	if (m_chk_holding_reg != nullptr)
	{
		m_chk_coil->SetCheck(BST_UNCHECKED);
		m_chk_discrete->SetCheck(BST_UNCHECKED);
		m_chk_input_reg->SetCheck(BST_UNCHECKED);
		m_chk_holding_reg->SetCheck(BST_CHECKED);
		m_modbus_function_code = HoldingReg;
	}
	UpdateData(TRUE);
}
