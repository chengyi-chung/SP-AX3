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
	char* ip_address = (char*)str.GetBuffer();

	//Call the modbus test function
	// int ret = ModbusTcpIpTest
	// -1 : error
	// 0 : success
	// if error, call MessageBox to show the error message
	// if success, call MessageBox to show the success message
	int ret = ModbusTcpIpTest(ip_address);
	if (ret == -1)
	{
		MessageBox(L"Modbus test failed");
	}
	else
	{
		//MessageBox(L"Modbus test success");

	}


}

//Add modbus tcp/ip test function
int UModBus::ModbusTcpIpTest(char* ip_address)
{
    modbus_t* ctx;
    std::vector<uint16_t> reg(10); // 使用vector来存储读取到的寄存器值

    // 创建Modbus TCP连接
    ctx = modbus_new_tcp(ip_address, 502); // 服务器IP和端口
    if (ctx == NULL) 
    {
        std::cerr << "Unable to allocate libmodbus context\n";
        return -1;
    }

    // 连接到服务器
    if (modbus_connect(ctx) == -1)
    {
        std::cerr << "Connection failed: " << modbus_strerror(errno) << std::endl;
        modbus_free(ctx);
		return -2;
    }

    // 读取保持寄存器
    if (modbus_read_registers(ctx, 0, 10, &reg[0]) == -1) 
    {
        std::cerr << "Failed to read register: " << modbus_strerror(errno) << std::endl;
        modbus_close(ctx);
        modbus_free(ctx);
        return -3;
    }

    for (size_t i = 0; i < reg.size(); i++)
    {
        std::cout << "reg[" << i << "]=" << reg[i] << std::endl;
    }




    // 关闭连接并释放资源
    modbus_close(ctx);
    modbus_free(ctx);

    return 0;
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
