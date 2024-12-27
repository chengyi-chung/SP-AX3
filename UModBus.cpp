#include "pch.h"
#include "YUFA.h"
#include "afxdialogex.h"
#include "UModBus.h"
#include "modbus.h"
#include <iostream>
#include <vector>
#include <string>

#include "YUFADlg.h"
// UModBus 對話方塊
using namespace std;


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
	//pParentWnd = NULL;
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
	ON_BN_CLICKED(IDC_IDC_WORK_GO, &UModBus::OnBnClickedIdcWorkGo)
	ON_EN_KILLFOCUS(IDC_EDIT_IP_ADDRESS, &UModBus::OnEnKillfocusEditIpAddress)
	ON_EN_KILLFOCUS(IDC_EDIT_SERVER_ID, &UModBus::OnEnKillfocusEditServerId)
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
	//char* ip_address = (char*)str.GetBuffer();

	// Use CT2CA for conversion (CString to const char*)
	CT2CA pszConvertedAnsiString(str);
	const char* ip_address = pszConvertedAnsiString;

	//modbus_t * ctx = modbus_new_tcp("127.0.0.1", 502);
	//modbus_t* ctx = modbus_new_tcp("192.168.0.11", 502);
	modbus_t* ctx = modbus_new_tcp(ip_address, 502);

	//Assign the server id to IDC_EDIT_SERVER_ID
	//Get the server id from the edit box
	GetDlgItemText(IDC_EDIT_SERVER_ID, str);
	int ServerId = _ttoi(str);
	modbus_set_slave(ctx, ServerId);  // 設置為設備 ID 1

	//Connection test
	if (modbus_connect(ctx) == -1)
	{
		fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
		modbus_free(ctx);
		return;
	}
	
	// Now you have a connected Modbus context (slave).
	//Test read holding register
	// Allocate space for the register data
	uint16_t tab_reg[10000] = { 0 };
	// Read 64 holding registers starting from address 0
	int rc = modbus_read_registers(ctx, 100,100, tab_reg);
	if (rc == -1)
	{
		fprintf(stderr, "Failed to read registers\n");
		modbus_close(ctx);
		modbus_free(ctx);
		return;
	}

	// Print the register values
	string str_Reg = "";
	for (int i = 0; i < rc; i++)
	{
		//將  tab_reg[i] 轉換為字串
		str_Reg += to_string(tab_reg[i]) + "\r\n";
	}
	// Display str in IDC_MODBUS_EDIT_RETURN
	str = str_Reg.c_str();
	SetDlgItemText(IDC_MODBUS_EDIT_RETURN, str);

	int index = 20;

	tab_reg[0] = 88;
	tab_reg[1] = 7777;
	tab_reg[20] =9999;
	tab_reg[23] = 6688;
	tab_reg[50] = 9990;
	tab_reg[51] = 9991;
	tab_reg[52] = 9992;
	tab_reg[53] = 9993;
	tab_reg[99] = 2222;
	//write to modbus tcp holding register with 
	//rc = modbus_write_register(ctx, 0, 999);

	//write to modbus tcp holding register with tab_reg[64]
	rc = modbus_write_registers(ctx, 100,100, &tab_reg[index]);
	//close the connection annd return
	modbus_close(ctx);
	modbus_free(ctx);
	return;






	//test coil
   // Read 10 coils from address 0
	uint8_t coils[10] = { 0 };

	// Read the coils
	modbus_set_slave(ctx, 1);  // 設置為設備 ID 1
	rc = modbus_read_bits(ctx, 0, 10, coils);

	if (rc == -1)
	{
		std::cerr << "Failed to read coils: " << modbus_strerror(errno) << std::endl;
		modbus_close(ctx);
		modbus_free(ctx);
		return;
	}
	// Print the coils
	for (int i = 0; i < rc; i++) 
	{
		std::cout << "Coil " << i << " = " << (int)coils[i] << std::endl;
	}






	// Write a single coil (set coil at address 21 to ON)
	rc = modbus_write_bit(ctx, 21, TRUE);
	if (rc == -1)
	{
		std::cerr << "Failed to write coil: " << modbus_strerror(errno) << std::endl;
		modbus_close(ctx);
		modbus_free(ctx);
		return;
	}


	//Client wrtie holding register
	// Write a single holding register (set register at address 10 to 0x00FF)
	rc = modbus_write_register(ctx, 10, 0x00FF);
	if (rc == -1) 
	{
		std::cerr << "Failed to write holding register: " << modbus_strerror(errno) << std::endl;
		modbus_close(ctx);
		modbus_free(ctx);
		return;
	}



	//Client read Input register
    // Allocate space for the register data
	uint16_t registers[10];

	// Read 10 holding registers starting from address 10
    rc = modbus_read_registers(ctx, 10, 10, registers);
	if (rc == -1)
	{
		fprintf(stderr, "Failed to read registers\n");
		modbus_close(ctx);
		modbus_free(ctx);
		return;
	}

	// Print the register values
	for (int i = 0; i < rc; i++) {
		printf("Register %d: %d\n", i, registers[i]);
	}
	

	// Close the connection
	modbus_close(ctx);
	modbus_free(ctx);
}

//Add modbus tcp/ip test function
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
	// Now you have a connected Modbus context (slave).
	// ...
	modbus_close(ctx);
	modbus_free(ctx);
	return 0;
}

BOOL UModBus::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此加入額外的初始化
	SetDlgItemText(IDC_EDIT_IP_ADDRESS, L"192.168.0.11");

	SetDlgItemText(IDC_EDIT_SERVER_ID, L"1");

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


void UModBus::OnBnClickedIdcWorkGo()
{
	// TODO: 在此加入控制項告知處理常式程式碼


}

//Set the dialog parameters
//m_SystemPara
//m_SystemPara.IpAddress
//m_SystemPara.StationID
void UModBus::SetDlgParam()
{
	//Get the IP address to the edit box
	CString str;
	GetDlgItemText(IDC_EDIT_IP_ADDRESS, str);
	//char* ip_address = (char*)str.GetBuffer();
	// Use CT2CA for conversion (CString to const char*)
	//CT2CA pszConvertedAnsiString(str);
	//const char* ip_address = pszConvertedAnsiString;

	CYUFADlg* pParentWnd = (CYUFADlg*)GetParent();

	
	if (pParentWnd != nullptr)
	{
		//Assign str to pParentWnd->m_SystemPara.StationID
		GetDlgItemText(IDC_EDIT_IP_ADDRESS, str);
		wcscpy_s(pParentWnd->m_SystemPara.IpAddress, str);

		//Get the server id from the edit box
		GetDlgItemText(IDC_EDIT_SERVER_ID, str);
		pParentWnd->m_SystemPara.StationID = _ttoi(str);
		
	}
	else
	{
		// Handle the error appropriately
	}
}


void UModBus::OnEnKillfocusEditIpAddress()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	SetDlgParam();
	//UpdateData(TRUE);
}


void UModBus::OnEnKillfocusEditServerId()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	SetDlgParam();
}


BOOL UModBus::PreTranslateMessage(MSG* pMsg)
{
	
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	{
		HWND hwndFocus = ::GetFocus();
		if (hwndFocus && (hwndFocus == GetDlgItem(IDC_EDIT1)->GetSafeHwnd()))
		{
			// 模擬 TAB 鍵，以移動焦點或執行其他行為
			::SendMessage(pMsg->hwnd, WM_KEYDOWN, VK_TAB, 0);
			return TRUE;
		}
	} 
	//return UModBus::PreTranslateMessage(pMsg);
	return CDialog::PreTranslateMessage(pMsg); // Call the base class implementation
}


void UModBus::OnOK()
{
}