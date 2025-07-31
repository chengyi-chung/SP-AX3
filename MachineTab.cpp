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
	//ON_BN_CLICKED(IDC_BTN_JOG_X_PLUS, &MachineTab::OnBnClickedBtnJogXPlus)
	//ON_BN_CLICKED(IDC_BTN_JOG_X_MINUS, &MachineTab::OnBnClickedBtnJogXMinux)
	//ON_BN_CLICKED(IDC_BTN_JOG_Y_PLUS, &MachineTab::OnBnClickedBtnJogYPlus)
	//ON_BN_CLICKED(IDC_BTN_JOG_Y_MINUS, &MachineTab::OnBnClickedBtnJogYMinus)
	//ON_BN_CLICKED(IDC_BTN_JOG_Z_PLUS, &MachineTab::OnBnClickedBtnJogZPlus)
	//ON_BN_CLICKED(IDC_BTN_JOG_Z_MINUS, &MachineTab::OnBnClickedBtnJogZMinus)
	ON_BN_CLICKED(IDC_RADIO_AUTO, &MachineTab::OnBnClickedRadioAuto)
	ON_BN_CLICKED(IDC_RADIO_MANUAL, &MachineTab::OnBnClickedRadioManual)
	//ON_BN_CLICKED(IDC_CHECK_HOME, &MachineTab::OnBnClickedCheckHome)
	//ON_BN_CLICKED(IDC_CHECK_RESET, &MachineTab::OnBnClickedCheckReset)
	//ON_BN_CLICKED(IDC_CHECK_AUTO_WORK_START, &MachineTab::OnBnClickedCheckAutoWorkStart)
	//ON_BN_CLICKED(IDC_CHECK_AUTO_WORK_STOP, &MachineTab::OnBnClickedCheckAutoWorkStop)
	ON_BN_CLICKED(IDC_BTN_MACHINE_SAVE_MOTION, &MachineTab::OnBnClickedBtnMachineSaveMotion)
	//Add IDC_BTN_JOG_X_PLUS button control , button down and up event
	ON_BN_CLICKED(IDC_MFCBTN_MACHINE_HOME, &MachineTab::OnBnClickedMfcbtnMachineHome)
	ON_BN_CLICKED(IDC_MFCBTN_MACHINE_AUTO_WORK_SART, &MachineTab::OnBnClickedMfcbtnMachineAutoWorkSart)
	ON_BN_CLICKED(IDC_MFCBTN_MACHINE_AUTO_WORK_STOP, &MachineTab::OnBnClickedMfcbtnMachineAutoWorkStop)
	ON_BN_CLICKED(IDC_MFCBTN_MACHINE_RESET_SW, &MachineTab::OnBnClickedMfcbtnMachineResetSw)
	ON_BN_CLICKED(IDC_MFCBTN_MACHINE_GO, &MachineTab::OnBnClickedMfcbtnMachineGo)
	ON_EN_CHANGE(IDC_EDIT_MANUAL_X, &MachineTab::OnEnChangeEditManualX)
END_MESSAGE_MAP()


// MachineTab 訊息處理常式
//OnInitialDialog
BOOL MachineTab::OnInitDialog()
{
	CDialog::OnInitDialog();
	//set IDC_RADIO_AUTO check
	//((CButton*)GetDlgItem(IDC_RADIO_AUTO))->SetCheck(1);

	// Set Click Auto Radio Button and launch Radio Button click event
	((CButton*)GetDlgItem(IDC_RADIO_AUTO))->SetCheck(TRUE);
	((CButton*)GetDlgItem(IDC_RADIO_MANUAL))->SetCheck(FALSE);

	// Laucvh OnBnClickedRadioAuto
	OnBnClickedRadioAuto();
	
	m_iMachineMode = 1;

	//Initial Discrete3000
	Discrete3000.reset();
	Discrete3000.set(0, 1);


	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


//Open Modbus TCP/IP server 
void MachineTab::OpenModBus()
{
	//Initial Modbus TCP/IP
	//get ip address from m_SystemPara of parrent dialog
	
	CYUFADlg* pParentWnd = dynamic_cast<CYUFADlg*>(GetParent()->GetParent());
	if (!pParentWnd)
	{
		AfxMessageBox(_T("Failed to get CYUFADlg parent window."));
		return;
	}

	// Access m_SystemPara
	if (pParentWnd->m_SystemPara.IpAddress.empty())
	{
		AfxMessageBox(_T("IP Address is not set in System Parameters."));
		return;
	}
	std::string ip = pParentWnd->m_SystemPara.IpAddress;

	int rc;

	// convert string ip to const char*
	const char* ip_cstr = ip.c_str();
	int port = 502;
	m_ctx = modbus_new_tcp(ip_cstr, port);

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
	if (m_ctx == nullptr)
	{
		m_strReportData += "\r\nModbus context is already null.";
		SetDlgItemText(IDC_EDIT_REPORT, CString(m_strReportData.c_str()));
		return;
	}

	m_strReportData += "\r\nClosing Modbus connection...";
	errno = 0;
	modbus_close(m_ctx);
	if (errno != 0)
	{
		CString errorMessage;
		errorMessage.Format(_T("Failed to close Modbus connection: %S"), modbus_strerror(errno));
		AfxMessageBox(errorMessage);
		m_strReportData += "\r\nFailed to close Modbus connection: " + std::string(modbus_strerror(errno));
	}
	else
	{
		m_strReportData += "\r\nModbus connection closed successfully.";
	}
	SetDlgItemText(IDC_EDIT_REPORT, CString(m_strReportData.c_str()));

	modbus_free(m_ctx);
	m_ctx = nullptr;
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

//define functio for Discrete3000 value change in Control
//intType: 0: check 1: button
//BitAdress: Bit Adress
//BitValue: Bit Value
//nID: check or button Control ID
void MachineTab::Discrete3000Change(int intType, int BitAdress, int BitValue, int nID)
{
	// Check if the Modbus context is initialized
	if (m_ctx == NULL)
	{
		// Get the parent dialog (CYUFADlg)
		CYUFADlg* pParentWnd = dynamic_cast<CYUFADlg*>(GetParent()->GetParent());
		if (!pParentWnd)
		{
			AfxMessageBox(_T("Failed to get CYUFADlg parent window."));
			return;
		}

		// Access m_SystemPara
		if (pParentWnd->m_SystemPara.IpAddress.empty())
		{
			AfxMessageBox(_T("IP Address is not set in System Parameters."));
			return;
		}

		std::string ip = pParentWnd->m_SystemPara.IpAddress;
		const char* ip_cstr = ip.c_str();
		int port = pParentWnd->Port;
		int stationId = pParentWnd->m_SystemPara.StationID; // Assuming stationId is available

		m_ctx = modbus_new_tcp(ip_cstr, port);
		if (m_ctx == NULL)
		{
			AfxMessageBox(_T("Failed to create the libmodbus context."));
			return;
		}

		// Set the Modbus slave/station ID
		if (modbus_set_slave(m_ctx, stationId) == -1)
		{
			CString errorMessage;
			errorMessage.Format(_T("Failed to set Modbus slave ID: %S"), modbus_strerror(errno));
			modbus_free(m_ctx);
			m_ctx = NULL;
			AfxMessageBox(errorMessage);
			return;
		}

		// Establish connection
		if (modbus_connect(m_ctx) == -1)
		{
			CString errorMessage;
			errorMessage.Format(_T("Failed to connect to Modbus server: %S"), modbus_strerror(errno));
			modbus_free(m_ctx);
			m_ctx = NULL;
			AfxMessageBox(errorMessage);
			return;
		}
	}

	// Reset the Discrete3000 bitset (ensure it’s 16 bits for a single register)
	Discrete3000.reset(); // Assuming Discrete3000 is std::bitset<16>

	if (intType == 0) // Check or Radio Control
	{
		if (((CButton*)GetDlgItem(nID))->GetCheck() == 1)
		{
			Discrete3000.set(BitAdress, BitValue); // Set the bit to 1 if checked
		}
		else
		{
			Discrete3000.set(BitAdress, 0); // Set the bit to 0 if unchecked
		}
	}
	else if (intType == 1) // Button Control
	{
		Discrete3000.set(BitAdress, 1);
	}
	else
	{
		Discrete3000.set(BitAdress, BitValue);
	}

	// Convert bitset to unsigned long (ensure it fits in 16 bits)
	Discrete3000Word = Discrete3000.to_ulong();

	// Write to Modbus register (using 29999 for 0-based addressing)
	int rc = modbus_write_register(m_ctx, 30000, Discrete3000Word);

	// Log the operation
	m_strReportData = m_strReportData + "\r\n" + "Reg[30000]." + std::to_string(BitAdress) + " = " +
		std::to_string(Discrete3000Word) + " " + Discrete3000.to_string();
	SetDlgItemText(IDC_EDIT_REPORT, CString(m_strReportData.c_str()));

	if (rc == -1)
	{
		CString errorMessage;
		errorMessage.Format(_T("Failed to write to Modbus register 30000: %S"), modbus_strerror(errno));
		AfxMessageBox(errorMessage);
		// Close and free the context to reset it
		modbus_close(m_ctx);
		modbus_free(m_ctx);
		m_ctx = NULL;
		return;
	}
}

//Clear All Discrete3000
//int Start Adress: iStartAdress
//int End Adress: iEndAdress
void MachineTab::ClearDiscrete3000(int iStartAdress, int iEndAdress)
{
	//Clear All Discrete3000
	//Discrete3000.reset();
	for (int i = iStartAdress; i <= iEndAdress; i++)
	{
		Discrete3000.set(i, 0);
	}

	Discrete3000Word = Discrete3000.to_ulong();
	int rc = modbus_write_register(m_ctx, 30000, Discrete3000Word);
	//append Discrete3000Word value to IDC_EDIT_REPORT with m_strReportData
	m_strReportData = m_strReportData + "\r\n" + "Reg[30000]." + std::to_string(iStartAdress) + "  = " + std::to_string(Discrete3000Word) + " " + Discrete3000.to_string();
	SetDlgItemText(IDC_EDIT_REPORT, CString(m_strReportData.c_str()));
	if (rc == -1)
	{
		CString errorMessage;
		errorMessage.Format(_T("Failed to write to Modbus register: %S"), modbus_strerror(errno));
		AfxMessageBox(errorMessage);
	}
}


void MachineTab::OnBnClickedBtnMachineSaveMotion()  
{  
// TODO: 在此加入控制項告知處理常式程式碼  
//20014 : Jog Velocity  
//20015 : Auto Velocity  
//20016 : Axis Dec Acceleration  
//20017 : Axis Inc Acceleration  

uint16_t iValue[4] = { 0 }; // 修正：初始化陣列以避免 NULL 指標錯誤  
iValue[0] = GetDlgItemInt(IDC_EDIT_JOG_VELOCITY);  
iValue[1] = GetDlgItemInt(IDC_EDIT_AUTO_VELOCITY);  
iValue[2] = GetDlgItemInt(IDC_EDIT_AXIS_ACC_DEC);  
iValue[3] = GetDlgItemInt(IDC_EDIT_AXIS_ACC_INC);  

SetHoldingRegister(20014, 20017, iValue, sizeof(iValue) / sizeof(iValue[0]));  
//將iValue陣列的值寫入 YUDADlg 的 m_SystemPara
CYUFADlg* pParentWnd = dynamic_cast<CYUFADlg*>(GetParent()->GetParent());
if (!pParentWnd)
{
	AfxMessageBox(_T("Failed to get CYUFADlg parent window."));
	return;
}

// Access m_SystemPara
if (pParentWnd->m_SystemPara.IpAddress.empty())
{
	AfxMessageBox(_T("IP Address is not set in System Parameters."));
	return;
}

	pParentWnd->m_SystemPara.JogVelocity = iValue[0];
	pParentWnd->m_SystemPara.AutoVelocity = iValue[1];
	pParentWnd->m_SystemPara.DecAcceleration = iValue[2];
	pParentWnd->m_SystemPara.IncAcceleration = iValue[3];
	//Assign the values to pParentWnd->m_SystemPara IDC_EDIT_PITCH、IDC_EDIT_TRANSFER_FACTOR
	CString strPitch, strTransferFactor;
	GetDlgItemText(IDC_EDIT_PITCH, strPitch);
	GetDlgItemText(IDC_EDIT_TRANSFER_FACTOR, strTransferFactor);
	//將strPitch和strTransferFactor轉換為double
	double pitch = _ttof(strPitch);
	double transferFactor = _ttof(strTransferFactor);
	
	pParentWnd->m_SystemPara.Pitch = pitch;
	pParentWnd->m_SystemPara.TransferFactor = transferFactor;


	std::string appPath;
	// Get the application path
	appPath = GetAppPath();

	//Set System configuration file name add app path
	CString strConfigFile = _T("SystemConfig.ini");
	// 修正 appPath 與 strConfigFile 的串接方式
	strConfigFile = CString(appPath.c_str()) + _T("\\") + strConfigFile;

	//Call UAX :  SystemConfig ReadSystemConfig(const std::string& filename)
	 WriteConfigToFile(std::string(CT2A(strConfigFile)), pParentWnd->m_SystemPara);
	//顯示成功訊息
	//AfxMessageBox(_T("Motion parameters saved successfully."));

}

//Set Holding Register value
//iStartAdress: Start Address
//iEndAdress: End Address
//int iValue[]: Value array to be set
void MachineTab::SetHoldingRegister(int iStartAdress, int iEndAdress, uint16_t* iValue, int sizeOfArray)
{
	int rc = modbus_write_registers(m_ctx, iStartAdress, sizeOfArray, iValue);

	if (rc == -1)
	{
		CString errorMessage;
		errorMessage.Format(_T("Failed to write to Modbus register: %S"), modbus_strerror(errno));
		AfxMessageBox(errorMessage);
	}
}
//Get Holding Register value
//iStartAdress: Start Address
//iEndAdress: End Address
//int iValue[]: Value array to be get
void MachineTab::GetHoldingRegister(int iStartAdress, int iEndAdress, uint16_t* iValue)
{
	//get size of iValue
	int sizeOfArray = sizeof(iValue) / sizeof(iValue[0]);
	//Check if the size of iValue is less than 100
	if (sizeOfArray > 100)
	{
		AfxMessageBox(_T("The size of iValue is too large."));
		return;
	}
	//Check if the size of iValue is less than 0	
	if (sizeOfArray < 0)
	{
		AfxMessageBox(_T("The size of iValue is too small."));
		return;
	}
	//Get Holding Register value
	int rc = modbus_read_registers(m_ctx, iStartAdress, sizeOfArray, (uint16_t*)iValue);
	
	if (rc == -1)
	{
		CString errorMessage;
		errorMessage.Format(_T("Failed to read from Modbus register: %S"), modbus_strerror(errno));
		AfxMessageBox(errorMessage);
	}
}


BOOL MachineTab::PreTranslateMessage(MSG* pMsg)
{
	// 範例：攔截 X+ 按鈕的 Mouse Down/Up
	if (pMsg->message == WM_LBUTTONDOWN || pMsg->message == WM_LBUTTONUP)
	{
		CWnd* pBtnXPlus = GetDlgItem(IDC_BTN_JOG_X_PLUS);
		if (pBtnXPlus && pBtnXPlus->m_hWnd == pMsg->hwnd)
		{
			if (pMsg->message == WM_LBUTTONDOWN)
			{
				// 處理 Button Down
				//AfxMessageBox(_T("X+ Down"));

				// X+ 按鈕按下 (Button Down)
				//m_bXPlusPressed = TRUE;
				//m_nActiveButton = IDC_BTN_JOG_X_PLUS;
				//SetCapture();  // 捕捉滑鼠事件
				// TODO: 在此加入控制項告知處理常式程式碼
				int bitAdress = 3;  // Bit address for X+ button
				int bitValue = 1;    // Bit value for X+ button pressed
				int nID = IDC_BTN_JOG_X_PLUS;
				ClearDiscrete3000(0, 8);
				Discrete3000Change(1, bitAdress, bitValue, nID);
				
			}
			else if (pMsg->message == WM_LBUTTONUP)
			{
				// 處理 Button Up
			   // AfxMessageBox(_T("X+ Up"));
				int bitAdress = 3;  // Bit address for X+ button
				int bitValue = 0;    // Bit value for X+ button pressed
				int nID = IDC_BTN_JOG_X_PLUS;
				ClearDiscrete3000(0, 8);
				//Discrete3000Change(1, bitAdress, bitValue, nID);
				
			}
		}
		//Add similar checks for IDC_BTN_JOG_X_PLUS
		CWnd* pBtnXMinus = GetDlgItem(IDC_BTN_JOG_X_MINUS);
		if (pBtnXMinus && pBtnXMinus->m_hWnd == pMsg->hwnd)
		{
			if (pMsg->message == WM_LBUTTONDOWN)
			{
				// 處理 Button Down
				//AfxMessageBox(_T("X- Down"));
				int bitAdress = 4;
				int bitValue = 1;
				int nID = IDC_BTN_JOG_X_MINUS;
				ClearDiscrete3000(0, 8);
				Discrete3000Change(1, bitAdress, bitValue, nID);
			}
			else if (pMsg->message == WM_LBUTTONUP)
			{
				// 處理 Button Up
				//AfxMessageBox(_T("X- Up"));
				int bitAdress = 4;
				int bitValue = 0;
				int nID = IDC_BTN_JOG_X_MINUS;
				ClearDiscrete3000(0, 8);
				//Discrete3000Change(1, bitAdress, bitValue, nID);
			}
		}
		//攔截 Y + 按鈕的 Mouse Down / Up
		CWnd* pBtnYPlus = GetDlgItem(IDC_BTN_JOG_Y_PLUS);
		if (pBtnYPlus && pBtnYPlus->m_hWnd == pMsg->hwnd)
		{
			if (pMsg->message == WM_LBUTTONDOWN)
			{
				// 處理 Button Down
				//AfxMessageBox(_T("Y+ Down"));
				int bitAdress = 5;
				int bitValue = 1;
				int nID = IDC_BTN_JOG_Y_PLUS;
				ClearDiscrete3000(0, 8);
				Discrete3000Change(1, bitAdress, bitValue, nID);
			}
			else if (pMsg->message == WM_LBUTTONUP)
			{
				// 處理 Button Up
				//AfxMessageBox(_T("Y+ Up"));
				int bitAdress = 5;
				int bitValue = 0;
				int nID = IDC_BTN_JOG_Y_PLUS;
				ClearDiscrete3000(0, 8);
				//Discrete3000Change(1, bitAdress, bitValue, nID);
			}
		}

		//攔截 Y - 按鈕的 Mouse Down / Up
		CWnd* pBtnYMinus = GetDlgItem(IDC_BTN_JOG_Y_MINUS);
		if (pBtnYMinus && pBtnYMinus->m_hWnd == pMsg->hwnd)
		{
			if (pMsg->message == WM_LBUTTONDOWN)
			{
				// 處理 Button Down
				//AfxMessageBox(_T("Y- Down"));
				int bitAdress = 6;
				int bitValue = 1;
				int nID = IDC_BTN_JOG_Y_MINUS;
				ClearDiscrete3000(0, 8);
				Discrete3000Change(1, bitAdress, bitValue, nID);
			}
			else if (pMsg->message == WM_LBUTTONUP)
			{
				// 處理 Button Up
				//AfxMessageBox(_T("Y- Up"));
				int bitAdress = 6;
				int bitValue = 0;
				int nID = IDC_BTN_JOG_Y_MINUS;
				ClearDiscrete3000(0, 8);
				//Discrete3000Change(1, bitAdress, bitValue, nID);

			}
		}

		//攔截 Z + 按鈕的 Mouse Down / Up
		CWnd* pBtnZPlus = GetDlgItem(IDC_BTN_JOG_Z_PLUS);
		if (pBtnZPlus && pBtnZPlus->m_hWnd == pMsg->hwnd)
		{
			if (pMsg->message == WM_LBUTTONDOWN)
			{
				// 處理 Button Down
				//AfxMessageBox(_T("Z- Down"));
				int bitAdress = 7;
				int bitValue = 1;
				int nID = IDC_BTN_JOG_Z_PLUS;
				ClearDiscrete3000(0, 8);
				Discrete3000Change(1, bitAdress, bitValue, nID);
			}
			else if (pMsg->message == WM_LBUTTONUP)
			{
				// 處理 Button Up
				//AfxMessageBox(_T("Z- Up"));
				int bitAdress = 7;
				int bitValue = 0;
				int nID = IDC_BTN_JOG_Z_PLUS;
				ClearDiscrete3000(0, 8);
				//Discrete3000Change(1, bitAdress, bitValue, nID);
			}
		}

		//攔截 Z - 按鈕的 Mouse Down / Up
		CWnd* pBtnZMinus = GetDlgItem(IDC_BTN_JOG_Z_MINUS);
		if (pBtnZMinus && pBtnZMinus->m_hWnd == pMsg->hwnd)
		{
			if (pMsg->message == WM_LBUTTONDOWN)
			{
				// 處理 Button Down
				//AfxMessageBox(_T("Z- Down"));
				int bitAdress = 8;
				int bitValue = 1;
				int nID = IDC_BTN_JOG_Z_MINUS;
				ClearDiscrete3000(0, 8);
				Discrete3000Change(1, bitAdress, bitValue, nID);
			}
			else if (pMsg->message == WM_LBUTTONUP)
			{
				// 處理 Button Up
				//AfxMessageBox(_T("Z- Up"));
				int bitAdress = 8;
				int bitValue = 0;
				int nID = IDC_BTN_JOG_Z_MINUS;
				ClearDiscrete3000(0, 8);
				//Discrete3000Change(1, bitAdress, bitValue, nID);
			}
		}
	}

	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	{
		// 攔截 Enter 鍵，不做任何事
		return TRUE;
	}


	return CDialog::PreTranslateMessage(pMsg);
}



void MachineTab::OnBnClickedRadioAuto()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	int bitAdress = 0;
	int bitValue = 1;
	int nID = IDC_RADIO_AUTO;
	//ClearDiscrete3000(0, 7);
	Discrete3000Change(0, bitAdress, bitValue, nID);

	//Set DlgItemID : IDC_MFCBTN_MACHINE_AUTO_WORK_SART、IDC_MFCBTN_MACHINE_AUTO_WORK_STOP to enable
	((CButton*)GetDlgItem(IDC_MFCBTN_MACHINE_AUTO_WORK_SART))->EnableWindow(TRUE);
	((CButton*)GetDlgItem(IDC_MFCBTN_MACHINE_AUTO_WORK_STOP))->EnableWindow(TRUE);

	//Set DlgItemID : IDC_CHECK_HOME、IDC_CHECK_RESET to disable
	((CButton*)GetDlgItem(IDC_MFCBTN_MACHINE_HOME))->EnableWindow(FALSE);
	((CButton*)GetDlgItem(IDC_MFCBTN_MACHINE_RESET_SW))->EnableWindow(FALSE);
	//加上所有 Jog 按鈕的disableWindow
	((CButton*)GetDlgItem(IDC_BTN_JOG_X_PLUS))->EnableWindow(FALSE);
	((CButton*)GetDlgItem(IDC_BTN_JOG_X_MINUS))->EnableWindow(FALSE);
	((CButton*)GetDlgItem(IDC_BTN_JOG_Y_PLUS))->EnableWindow(FALSE);
	((CButton*)GetDlgItem(IDC_BTN_JOG_Y_MINUS))->EnableWindow(FALSE);
	((CButton*)GetDlgItem(IDC_BTN_JOG_Z_PLUS))->EnableWindow(FALSE);
	((CButton*)GetDlgItem(IDC_BTN_JOG_Z_MINUS))->EnableWindow(FALSE);
	
}

void MachineTab::OnBnClickedRadioManual()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	//Enable Manual Mode: IDC_MFCBTN_MACHINE_HOME、IDC_MFCBTN_MACHINE_RESET_SW
	//Disable Auto Mode: IDC_CHECK_AUTO_WORK_START、IDC_CHECK_AUTO_WORK_STOP
	 
	//Set DlgItemID : IDC_MFCBTN_MACHINE_HOME、IDC_MFCBTN_MACHINE_RESET_SW to enable
	((CButton*)GetDlgItem(IDC_MFCBTN_MACHINE_HOME))->EnableWindow(TRUE);
	((CButton*)GetDlgItem(IDC_MFCBTN_MACHINE_RESET_SW))->EnableWindow(TRUE);
	//Enable all Jog buttons
	((CButton*)GetDlgItem(IDC_BTN_JOG_X_PLUS))->EnableWindow(TRUE);
	((CButton*)GetDlgItem(IDC_BTN_JOG_X_MINUS))->EnableWindow(TRUE);
	((CButton*)GetDlgItem(IDC_BTN_JOG_Y_PLUS))->EnableWindow(TRUE);
	((CButton*)GetDlgItem(IDC_BTN_JOG_Y_MINUS))->EnableWindow(TRUE);
	((CButton*)GetDlgItem(IDC_BTN_JOG_Z_PLUS))->EnableWindow(TRUE);
	((CButton*)GetDlgItem(IDC_BTN_JOG_Z_MINUS))->EnableWindow(TRUE);

	//Set DlgItemID : IDC_MFCBTN_MACHINE_AUTO_WORK_SART、IDC_MFCBTN_MACHINE_AUTO_WORK_STOP to disable
	((CButton*)GetDlgItem(IDC_MFCBTN_MACHINE_AUTO_WORK_SART))->EnableWindow(FALSE);
	((CButton*)GetDlgItem(IDC_MFCBTN_MACHINE_AUTO_WORK_STOP))->EnableWindow(FALSE);


	int bitAdress = 0;
	int bitValue = 2;
	int nID = IDC_RADIO_AUTO;
	//ClearDiscrete3000(0, 7);
	Discrete3000Change(0, bitAdress, bitValue, nID);


}

void MachineTab::OnBnClickedMfcbtnMachineAutoWorkSart()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	int bitAdress = 12;
	int bitValue = 1;
	int nID = 0;  // IDC_CHECK_AUTO_WORK_START;
	//ClearDiscrete3000(0, 7);
	Discrete3000Change(1, bitAdress, bitValue, nID);
}

void MachineTab::OnBnClickedMfcbtnMachineAutoWorkStop()
{
	// TODO: 在此加入控制項告知處理常式程式碼


	int bitAdress = 13;
	int bitValue = 1;
	int nID = 0;  // IDC_CHECK_AUTO_WORK_STOP;
	//ClearDiscrete3000(0, 7);
	Discrete3000Change(1, bitAdress, bitValue, nID);


}

void MachineTab::OnBnClickedMfcbtnMachineHome()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	int bitAdress = 10;
	int bitValue = 1;
	int nID = 0; // IDC_CHECK_HOME;
	//ClearDiscrete3000(0, 7);
	Discrete3000Change(1, bitAdress, bitValue, nID);
}

void MachineTab::OnBnClickedMfcbtnMachineResetSw()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	int bitAdress = 11;
	int bitValue = 1;
	int nID = 0; // IDC_CHECK_RESET;
	//ClearDiscrete3000(0, 7);
	Discrete3000Change(1, bitAdress, bitValue, nID);
}

void MachineTab::OnBnClickedMfcbtnMachineGo()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	//檢查 // IDC_EDIT_MANUAL_X, IDC_EDIT_MANUAL_Y 的值為float若不是 GO disabled
	

	// IDC_EDIT_MANUAL_X, IDC_EDIT_MANUAL_Y 的值轉換為float, float fX, fY;
	float fX = GetDlgItemInt(IDC_EDIT_MANUAL_X);
	float fY = GetDlgItemInt(IDC_EDIT_MANUAL_Y);

	
}

//isValidFloat
bool MachineTab::isValidFloat(const std::string& str, float& outValue)
{
	try 
	{
		outValue = std::stof(str); // 嘗試轉換為 float
		return true;
	}
	catch (const std::invalid_argument&)
	{
		return false; // 非數字格式
	}
	catch (const std::out_of_range&) {
		return false; // 超出 float 範圍
	}

}

void MachineTab::OnEnChangeEditManualX()
{
	// TODO:  如果這是 RICHEDIT 控制項，控制項將不會
	// 傳送此告知，除非您覆寫 CDialog::OnInitDialog()
	// 函式和呼叫 CRichEditCtrl().SetEventMask()
	// 讓具有 ENM_CHANGE 旗標 ORed 加入遮罩。

	
	// TODO:  在此加入控制項告知處理常式程式碼
}

void MachineTab::OnOK()
{
    // 可在此加入資料驗證、保存等邏輯
    // 例如：UpdateData(TRUE); // 將 UI 資料同步到變數

    // 若需要呼叫父類別的 OnOK，可加上：
    CDialog::OnOK();
}

//Update data in Edit control with SystemConfig m_SystemPara
void MachineTab::UpdateControl()
{
	// Get the parent dialog (CYUFADlg)
	CYUFADlg* pParentWnd = dynamic_cast<CYUFADlg*>(GetParent()->GetParent());
	if (pParentWnd == nullptr)
	{
		AfxMessageBox(_T("Failed to get CYUFADlg parent window."));
		return;
	}

	// Update edit controls with values from m_SystemPara
	CString cStr;

	// JogVelocity (assuming int, adjust if float)
	cStr.Format(_T("%d"), pParentWnd->m_SystemPara.JogVelocity);
	SetDlgItemText(IDC_EDIT_JOG_VELOCITY, cStr);

	// AutoVelocity (assuming int, adjust if float)
	cStr.Format(_T("%d"), pParentWnd->m_SystemPara.AutoVelocity);
	SetDlgItemText(IDC_EDIT_AUTO_VELOCITY, cStr);

	// AxisAccDec (assuming float for precision, adjust if int)
	cStr.Format(_T("%.d"), pParentWnd->m_SystemPara.DecAcceleration);
	SetDlgItemText(IDC_EDIT_AXIS_ACC_DEC, cStr);

	// AxisAccInc (assuming float for precision, adjust if int)
	cStr.Format(_T("%.d"), pParentWnd->m_SystemPara.IncAcceleration);
	SetDlgItemText(IDC_EDIT_AXIS_ACC_INC, cStr);

	// Pitch (assuming float, adjust if int)
	cStr.Format(_T("%.3f"), pParentWnd->m_SystemPara.Pitch);
	SetDlgItemText(IDC_EDIT_PITCH, cStr);

	// TransferFactor (assuming float, adjust if int)
	cStr.Format(_T("%.3f"), pParentWnd->m_SystemPara.TransferFactor);
	SetDlgItemText(IDC_EDIT_TRANSFER_FACTOR, cStr);
}

