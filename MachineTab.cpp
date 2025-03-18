// MachineTab.cpp: 實作檔案
//

#include "pch.h"
#include "YUFA.h"
#include "YUFADlg.h"
#include "Resource.h"
#include "afxdialogex.h"
#include "MachineTab.h"


// MachineTab 對話方塊

IMPLEMENT_DYNAMIC(MachineTab, CDialog)

MachineTab::MachineTab(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_TAB_MACHINE, pParent)
{

}

MachineTab::~MachineTab()
{
}

void MachineTab::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(MachineTab, CDialog)
	ON_BN_CLICKED(IDC_BTN_JOG_X_PLUS, &MachineTab::OnBnClickedBtnJogXPlus)
	ON_BN_CLICKED(IDC_BTN_JOG_X_MINUX, &MachineTab::OnBnClickedBtnJogXMinux)
	ON_BN_CLICKED(IDC_BTN_JOG_Y_PLUS, &MachineTab::OnBnClickedBtnJogYPlus)
	ON_BN_CLICKED(IDC_BTN_JOG_Y_MINUS, &MachineTab::OnBnClickedBtnJogYMinus)
	ON_BN_CLICKED(IDC_BTN_JOG_Z_PLUS, &MachineTab::OnBnClickedBtnJogZPlus)
	ON_BN_CLICKED(IDC_BTN_JOG_Z_MINUS, &MachineTab::OnBnClickedBtnJogZMinus)
	ON_BN_CLICKED(IDC_RADIO_AUTO, &MachineTab::OnBnClickedRadioAuto)
	ON_BN_CLICKED(IDC_CHECK_HOME, &MachineTab::OnBnClickedCheckHome)
	ON_BN_CLICKED(IDC_CHECK_RESET, &MachineTab::OnBnClickedCheckReset)
	ON_BN_CLICKED(IDC_CHECK_AUTO_WORK_START, &MachineTab::OnBnClickedCheckAutoWorkStart)
	ON_BN_CLICKED(IDC_CHECK_AUTO_WORK_STOP, &MachineTab::OnBnClickedCheckAutoWorkStop)
END_MESSAGE_MAP()


// MachineTab 訊息處理常式
//OnInitialDialog
BOOL MachineTab::OnInitDialog()
{
	CDialog::OnInitDialog();
	//set IDC_RADIO_AUTO check
	((CButton*)GetDlgItem(IDC_RADIO_AUTO))->SetCheck(1);
	m_iMachineMode = 1;

	//Initial Discrete3000
	Discrete3000.reset();
	//Discrete3000.set(0, 1);



	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void MachineTab::OnBnClickedBtnJogXPlus()
{
	// TODO: 在此加入控制項告知處理常式程式碼
}

void MachineTab::OnBnClickedBtnJogXMinux()
{
	// TODO: 在此加入控制項告知處理常式程式碼
}

void MachineTab::OnBnClickedBtnJogYPlus()
{
	// TODO: 在此加入控制項告知處理常式程式碼
}

void MachineTab::OnBnClickedBtnJogYMinus()
{
	// TODO: 在此加入控制項告知處理常式程式碼
}

void MachineTab::OnBnClickedBtnJogZPlus()
{
	// TODO: 在此加入控制項告知處理常式程式碼
}

void MachineTab::OnBnClickedBtnJogZMinus()
{
	// TODO: 在此加入控制項告知處理常式程式碼
}

void MachineTab::OnBnClickedRadioAuto()
{
	// TODO: 在此加入控制項告知處理常式程式碼
}

void MachineTab::OnBnClickedCheckHome()
{
	// TODO: 在此加入控制項告知處理常式程式碼
}

void MachineTab::OnBnClickedCheckReset()
{
	// TODO: 在此加入控制項告知處理常式程式碼
}

void MachineTab::OnBnClickedCheckAutoWorkStart()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	//Check the Auto Work Start is checked or not
	//If checked, send the data to the Modbus server
	//If not checked, do nothing
	int rc;

	// Check if the Modbus context is initialized
	if (m_ctx == NULL)
	{
		AfxMessageBox(_T("Failed to create the libmodbus context."));
		return;
	}

	if (((CButton*)GetDlgItem(IDC_CHECK_AUTO_WORK_START))->GetCheck() == 1)
	{
		Discrete3000.set(12, 1);
	}
	else
	{
		Discrete3000.set(12, 0);
	}

	Discrete3000Word = Discrete3000.to_ulong();
	rc = modbus_write_register(m_ctx, 30000, Discrete3000Word);

    //append Discrete3000Word value to IDC_EDIT_REPORT with m_strReportData
    m_strReportData = m_strReportData + "\r\n" +"Reg[30000] = " + std::to_string(Discrete3000Word) + " " + Discrete3000.to_string();
	SetDlgItemText(IDC_EDIT_REPORT, CString(m_strReportData.c_str()));

	



	if (rc == -1)
	{
		CString errorMessage;
		errorMessage.Format(_T("Failed to write to Modbus register: %S"), modbus_strerror(errno));
		AfxMessageBox(errorMessage);
	}


}

void MachineTab::OnBnClickedCheckAutoWorkStop()
{
	// TODO: 在此加入控制項告知處理常式程式碼
}

//Open Modbus TCP/IP server 
void MachineTab::OpenModBus()
{
	//Initial Modbus TCP/IP
	//get ip address from m_SystemPara of parrent dialog

	CYUFADlg* pParentWnd = (CYUFADlg*)GetParent();
	char* ip = pParentWnd->m_SystemPara.IpAddress;
	int rc;
    
	int port = 502;
	m_ctx = modbus_new_tcp(ip, port);

	//check if the context is created
	if (m_ctx == NULL)
	{
		AfxMessageBox(_T("Failed to create the libmodbus context."));
	}
	else
	{
		//Connect to the Modbus TCP/IP server
		rc = modbus_connect(m_ctx);
		if (rc == -1)
		{
			AfxMessageBox(_T("Failed to connect to the Modbus server."));
		}
	}

	//prinrt the ip address and port on m_strReportData
	m_strReportData = "IP Address: " + string(ip) + " Port: " + to_string(port) + "\r\n";
    SetDlgItemText(IDC_EDIT_REPORT, CString(m_strReportData.c_str()));
	
	int ServerId = 1; // pParentWnd->m_SystemPara.StationID;
	rc = modbus_set_slave(m_ctx, ServerId);  // 設置為設備 ID 1
	
	if (m_ctx == NULL)
	{
		AfxMessageBox(_T("Failed to create the libmodbus context."));
	}
	else
	{
		uint16_t tab_reg[60000] = { 0 };
		// Read 64 holding registers starting from address 0
		rc = modbus_read_registers(m_ctx, 100, 100, tab_reg);
		rc = modbus_write_register(m_ctx, 30001, 1);
	}



}

//Close Modbus TCP/IP server
void MachineTab::CloseModBus()
{
	//Close Modbus TCP/IP
	modbus_close(m_ctx);
	modbus_free(m_ctx);
}

//send data to modbus server
void MachineTab::SendDataToModBus()
{
	//Send data to Modbus TCP/IP server
	//Get the data from the dialog control
	//Send the data to the Modbus TCP/IP server
	//Get the data from the Modbus TCP/IP server
	//Display the data to the dialog control




}