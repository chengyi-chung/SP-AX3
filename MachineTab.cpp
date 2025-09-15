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
	, m_ctx(nullptr)
	, Discrete3000Word(0)
	, m_pCoordinateThread(nullptr)
	, m_bThreadRunning(FALSE)
	, m_hStopThreadEvent(nullptr)
{

}

MachineTab::~MachineTab()
{
	StopCoordinateThread();
	if (m_hStopThreadEvent)
	{
		CloseHandle(m_hStopThreadEvent);
	}


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

	// ... 現有消息映射 ...
	ON_MESSAGE(WM_UPDATE_COORDINATES, &MachineTab::OnUpdateCoordinates)

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
	CYUFADlg* pParentWnd = dynamic_cast<CYUFADlg*>(GetParent()->GetParent());
	if (!pParentWnd)
	{
		AfxMessageBox(_T("Failed to get CYUFADlg parent window."));
		return;
	}

	if (pParentWnd->m_SystemPara.IpAddress.empty())
	{
		AfxMessageBox(_T("IP Address is not set in System Parameters."));
		return;
	}

	std::string ip = pParentWnd->m_SystemPara.IpAddress;
	int port = 502;
	m_ctx = modbus_new_tcp(ip.c_str(), port);

	if (m_ctx == NULL)
	{
		AfxMessageBox(_T("Failed to create the libmodbus context."));
		return;
	}

	int rc = modbus_connect(m_ctx);
	if (rc == -1)
	{
		AfxMessageBox(_T("Failed to connect to the Modbus server."));
		modbus_free(m_ctx);
		m_ctx = nullptr;
		return;
	}

	int ServerId = 1; // pParentWnd->m_SystemPara.StationID;
	rc = modbus_set_slave(m_ctx, ServerId);
	if (rc == -1)
	{
		AfxMessageBox(_T("Failed to set Modbus slave ID."));
		modbus_free(m_ctx);
		m_ctx = nullptr;
		return;
	}

	m_strReportData = "IP Address: " + ip + " Port: " + std::to_string(port) + "\r\n";
	SetDlgItemText(IDC_EDIT_REPORT, CString(m_strReportData.c_str()));

	// 啟動座標讀取執行緒
	StartCoordinateThread();
}
//Close Modbus TCP/IP server
void MachineTab::CloseModBus()
{
	// 先停止執行緒
	StopCoordinateThread();

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

//Clear All Discrete5000
//int Start Adress: iStartAdress
//int End Adress: iEndAdress
void MachineTab::ClearDiscrete5000(int iStartAdress, int iEndAdress)
{
	//Clear All Discrete5000
	//Discrete5000.reset();
	for (int i = iStartAdress; i <= iEndAdress; i++)
	{
		Discrete5000.set(i, 0);
	}
	Discrete5000Word = Discrete5000.to_ulong();
	int rc = modbus_write_register(m_ctx, 50000, Discrete5000Word);
	//append Discrete5000Word value to IDC_EDIT_REPORT with m_strReportData
	m_strReportData = m_strReportData + "\r\n" + "Reg[50000]." + std::to_string(iStartAdress) + "  = " + std::to_string(Discrete5000Word) + " " + Discrete5000.to_string();
	SetDlgItemText(IDC_EDIT_REPORT, CString(m_strReportData.c_str()));
	if (rc == -1)
	{
		CString errorMessage;
		errorMessage.Format(_T("Failed to write to Modbus register: %S"), modbus_strerror(errno));
		AfxMessageBox(errorMessage);
	}
}

//define functio for Discrete5000 value change in Control, data type is world
//intType: 0: check 1: button
//BitAdress: Bit Adress
//BitValue: Bit Value
//nID: check or button Control ID
void MachineTab::Discrete5000Change(int intType, int BitAdress, int BitValue, int nID)
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
	
    // Reset the Discrete5000 bitset (ensure it’s 16 bits for a single register)
	Discrete5000.reset(); // Assuming Discrete3000 is std::bitset<16>

	if (intType == 0) // Check or Radio Control
	{
		if (((CButton*)GetDlgItem(nID))->GetCheck() == 1)
		{
			Discrete5000.set(BitAdress, BitValue); // Set the bit to 1 if checked
		}
		else
		{
			Discrete5000.set(BitAdress, 0); // Set the bit to 0 if unchecked
		}
	}
	else if (intType == 1) // Button Control
	{
		Discrete5000.set(BitAdress, 1);
	}
	else
	{
		Discrete5000.set(BitAdress, BitValue);
	}
	
	// Convert bitset to unsigned long (ensure it fits in 16 bits)
	Discrete5000Word = Discrete5000.to_ulong();

	// Write to Modbus register (using 29999 for 0-based addressing)
	int rc = modbus_write_register(m_ctx, 50000, Discrete5000Word);

	// Log the operation
	m_strReportData = m_strReportData + "\r\n" + "Reg[50000]." + std::to_string(BitAdress) + " = " +
		std::to_string(Discrete5000Word) + " " + Discrete5000.to_string();
	SetDlgItemText(IDC_EDIT_REPORT, CString(m_strReportData.c_str()));

	if (rc == -1)
	{
		CString errorMessage;
		errorMessage.Format(_T("Failed to write to Modbus register 50000: %S"), modbus_strerror(errno));
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

//***************************************************************************
//            extend word to double word (DINT)
//***************************************************************************

//define functio for Discrete5000 value change in Control
//intType: 0: check 1: button
//BitAdress: Bit Adress
//BitValue: Bit Value
//nID: check or button Control ID
//void MachineTab::Discrete5000Change(int intType, int BitAdress, int BitValue, int nID)




void MachineTab::OnBnClickedBtnMachineSaveMotion()
{
	// TODO: 在此加入控制項告知處理常式程式碼  
	// 20014 : Jog Velocity  
	// 20015 : Auto Velocity  
	// 20016 : Axis Dec Acceleration  
	// 20017 : Axis Inc Acceleration  

	int16_t iValueSigned[4] = { 0 }; // 用於儲存帶符號的值
	uint16_t iValueUnsigned[4] = { 0 }; // 用於傳遞給 SetHoldingRegister

	// 獲取 UI 輸入值，並儲存為帶符號的值
	iValueSigned[0] = static_cast<int16_t>(GetDlgItemInt(IDC_EDIT_JOG_VELOCITY));
	iValueSigned[1] = static_cast<int16_t>(GetDlgItemInt(IDC_EDIT_AUTO_VELOCITY));
	iValueSigned[2] = static_cast<int16_t>(GetDlgItemInt(IDC_EDIT_AXIS_ACC_DEC));
	iValueSigned[3] = static_cast<int16_t>(GetDlgItemInt(IDC_EDIT_AXIS_ACC_INC));

	// 將 int16_t 的補碼值直接賦值給 uint16_t，不改變二進位表示
	for (int i = 0; i < 4; i++)
	{
		// 使用 reinterpret_cast 或直接賦值，確保二進位數據不變
		iValueUnsigned[i] = *reinterpret_cast<uint16_t*>(&iValueSigned[i]);
	}

	// 調用 SetHoldingRegister，傳遞 uint16_t 類型的陣列
	SetHoldingRegister(20014, 20017, iValueUnsigned, sizeof(iValueUnsigned) / sizeof(iValueUnsigned[0]));

	// 將 iValueSigned 陣列的值寫入 YUDADlg 的 m_SystemPara
	CYUFADlg* pParentWnd = dynamic_cast<CYUFADlg*>(GetParent()->GetParent());
	if (!pParentWnd)
	{
		AfxMessageBox(_T("無法取得 CYUFADlg 父視窗。"));
		return;
	}

	// 存取 m_SystemPara
	if (pParentWnd->m_SystemPara.IpAddress.empty())
	{
		AfxMessageBox(_T("系統參數中未設定 IP 位址。"));
		return;
	}

	// 使用帶符號的值儲存到 m_SystemPara 中，以便保留負值資訊
	pParentWnd->m_SystemPara.JogVelocity = iValueSigned[0];
	pParentWnd->m_SystemPara.AutoVelocity = iValueSigned[1];
	pParentWnd->m_SystemPara.DecAcceleration = iValueSigned[2];
	pParentWnd->m_SystemPara.IncAcceleration = iValueSigned[3];

	// 將 IDC_EDIT_PITCH、IDC_EDIT_TRANSFER_FACTOR 的值賦予 pParentWnd->m_SystemPara
	CString strPitch, strTransferFactor;
	GetDlgItemText(IDC_EDIT_PITCH, strPitch);
	GetDlgItemText(IDC_EDIT_TRANSFER_FACTOR, strTransferFactor);

	// 將 strPitch 和 strTransferFactor 轉換為 double
	double pitch = _ttof(strPitch);
	double transferFactor = _ttof(strTransferFactor);

	pParentWnd->m_SystemPara.Pitch = pitch;
	pParentWnd->m_SystemPara.TransferFactor = transferFactor;

	std::string appPath;
	// 取得應用程式路徑
	appPath = GetAppPath();

	// 設定系統設定檔名稱並加入應用程式路徑
	CString strConfigFile = _T("SystemConfig.ini");
	strConfigFile = CString(appPath.c_str()) + _T("\\") + strConfigFile;

	// 呼叫 UAX: SystemConfig ReadSystemConfig(const std::string& filename)
	WriteConfigToFile(std::string(CT2A(strConfigFile)), pParentWnd->m_SystemPara);
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

// Double word (DINT) functions
//Set Holding Registers value
//iStartAdress: Start Address, double word need 2 word address
//iEndAdress: End Address
//int iValue[]: Value array to be set
void MachineTab::SetHoldingRegisteDInt(int iStartAdress, int iEndAdress, uint16_t* iValue, int SizeOfArray)
{
	int rc = modbus_write_registers(m_ctx, iStartAdress, SizeOfArray * 2, iValue);
	if (rc == -1)
	{
		CString errorMessage;
		errorMessage.Format(_T("Failed to write to Modbus register DINT: %S"), modbus_strerror(errno));
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

				flgGetCoord = FALSE;
				this->ClearDiscrete5000(0, 8);
				Discrete5000Change(1, bitAdress, bitValue, nID);
				
			}
			else if (pMsg->message == WM_LBUTTONUP)
			{
				// 處理 Button Up
			   // AfxMessageBox(_T("X+ Up"));
				int bitAdress = 3;  // Bit address for X+ button
				int bitValue = 0;    // Bit value for X+ button pressed
				int nID = IDC_BTN_JOG_X_PLUS;
				ClearDiscrete5000(0, 8);
				//Discrete3000Change(1, bitAdress, bitValue, nID);
				flgGetCoord = TRUE;
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
				flgGetCoord = FALSE;
				ClearDiscrete5000(0, 8);
				Discrete5000Change(1, bitAdress, bitValue, nID);
			}
			else if (pMsg->message == WM_LBUTTONUP)
			{
				// 處理 Button Up
				//AfxMessageBox(_T("X- Up"));
				int bitAdress = 4;
				int bitValue = 0;
				int nID = IDC_BTN_JOG_X_MINUS;
				ClearDiscrete5000(0, 8);
				//Discrete3000Change(1, bitAdress, bitValue, nID);
				flgGetCoord = TRUE;
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

				flgGetCoord = FALSE;
				ClearDiscrete5000(0, 8);
				Discrete5000Change(1, bitAdress, bitValue, nID);
			}
			else if (pMsg->message == WM_LBUTTONUP)
			{
				// 處理 Button Up
				//AfxMessageBox(_T("Y+ Up"));
				int bitAdress = 5;
				int bitValue = 0;
				int nID = IDC_BTN_JOG_Y_PLUS;
				ClearDiscrete5000(0, 8);
				//Discrete3000Change(1, bitAdress, bitValue, nID);
				flgGetCoord = TRUE;
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

				flgGetCoord = FALSE;
				ClearDiscrete5000(0, 8);
				Discrete5000Change(1, bitAdress, bitValue, nID);
			}
			else if (pMsg->message == WM_LBUTTONUP)
			{
				// 處理 Button Up
				//AfxMessageBox(_T("Y- Up"));
				int bitAdress = 6;
				int bitValue = 0;
				int nID = IDC_BTN_JOG_Y_MINUS;
				ClearDiscrete5000(0, 8);
				//Discrete3000Change(1, bitAdress, bitValue, nID);
				flgGetCoord = TRUE;

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

				flgGetCoord = FALSE;
				ClearDiscrete5000(0, 8);
				Discrete5000Change(1, bitAdress, bitValue, nID);
			}
			else if (pMsg->message == WM_LBUTTONUP)
			{
				// 處理 Button Up
				//AfxMessageBox(_T("Z- Up"));
				int bitAdress = 7;
				int bitValue = 0;
				int nID = IDC_BTN_JOG_Z_PLUS;
				ClearDiscrete5000(0, 8);
				//Discrete3000Change(1, bitAdress, bitValue, nID);
				flgGetCoord = TRUE;
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

				flgGetCoord = FALSE;
				ClearDiscrete5000(0, 8);
				Discrete5000Change(1, bitAdress, bitValue, nID);
			}
			else if (pMsg->message == WM_LBUTTONUP)
			{
				// 處理 Button Up
				//AfxMessageBox(_T("Z- Up"));
				int bitAdress = 8;
				int bitValue = 0;
				int nID = IDC_BTN_JOG_Z_MINUS;
				ClearDiscrete5000(0, 8);
				//Discrete3000Change(1, bitAdress, bitValue, nID);
				flgGetCoord = TRUE;
			}
		}
	}

	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	{
		// 攔截 Enter 鍵，不做任何事
		return TRUE;
	}

	flgGetCoord = TRUE;
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
	cStr.Format(_T("%.4f"), pParentWnd->m_SystemPara.TransferFactor);
	SetDlgItemText(IDC_EDIT_TRANSFER_FACTOR, cStr);
}

LRESULT MachineTab::OnUpdateCoordinates(WPARAM wParam, LPARAM lParam)
{
	float* coordinates = reinterpret_cast<float*>(lParam);

	if (coordinates)
	{
		// 更新 UI 控制項
		CString strX, strY, strZ;
		strX.Format(_T("%.2f"), coordinates[0]/100);
		strY.Format(_T("%.2f"), coordinates[1]/100);
		strZ.Format(_T("%.2f"), coordinates[2]/100);

		SetDlgItemText(IDC_EDIT_MACHINE_X, strX);
		SetDlgItemText(IDC_EDIT_MACHINE_Y, strY);
		SetDlgItemText(IDC_EDIT_MACHINE_Z, strZ);

		// 釋放記憶體
		delete[] coordinates;
	}
	return 0;
}

UINT MachineTab::ReadCoordinatesThread(LPVOID pParam)
{
	MachineTab* pThis = static_cast<MachineTab*>(pParam);
	uint16_t registers[3] = { 0 }; // 用於儲存 X, Y, Z 座標
	const int startAddress = 20011;
	const int numRegisters = 3;

	while (pThis->m_bThreadRunning)
	{
		// 檢查停止事件
		if (WaitForSingleObject(pThis->m_hStopThreadEvent, 0) == WAIT_OBJECT_0)
		{
			break; // 停止執行緒
		}

		// 確保 Modbus 上下文有效
		if (pThis->m_ctx != nullptr)
		{

			// if flgGetCoord is TRUE, read coordinates
			if (!pThis->flgGetCoord)
			{
				Sleep(100); // 等待 100 毫秒
				continue; // 跳過此次迴圈
			}
			int rc = modbus_read_registers(pThis->m_ctx, startAddress, numRegisters, registers);
			if (rc != -1)
			{
				// 成功讀取，將 uint16_t 轉換為 int16_t 以處理負值，然後轉為 float
				float coordinates[3] = {
					static_cast<float>(static_cast<int16_t>(registers[0])), // X
					static_cast<float>(static_cast<int16_t>(registers[1])), // Y
					static_cast<float>(static_cast<int16_t>(registers[2]))  // Z
				};
				// 動態分配記憶體並發送消息到 UI 更新
			
				float* coordPtr = new float[3];
				coordPtr[0] = coordinates[0];
				coordPtr[1] = coordinates[1];
				coordPtr[2] = coordinates[2];
				pThis->PostMessage(WM_UPDATE_COORDINATES, 0, reinterpret_cast<LPARAM>(coordPtr));
			}
			else
			{
				// 記錄錯誤
				CString errorMessage;
				errorMessage.Format(_T("Failed to read coordinates: %S"), modbus_strerror(errno));
				pThis->m_strReportData += "\r\n" + std::string(CT2A(errorMessage));

				pThis->ClearDiscrete3000(0, 8); // 清除 Discrete3000 狀態
				
				//pThis->PostMessage(WM_SETITEMTEXT, IDC_EDIT_REPORT, reinterpret_cast<LPARAM>(new CString(pThis->m_strReportData.c_str())));
			}
		}

		// 每 800 毫秒讀取一次
		Sleep(800);
	}

	return 0;
}

void MachineTab::StartCoordinateThread()
{
	if (!m_bThreadRunning)
	{
		m_bThreadRunning = TRUE;
		ResetEvent(m_hStopThreadEvent); // 重置停止事件
		m_pCoordinateThread = AfxBeginThread(ReadCoordinatesThread, this, THREAD_PRIORITY_NORMAL);
		if (m_pCoordinateThread == nullptr)
		{
			AfxMessageBox(_T("Failed to start coordinate reading thread."));
			m_bThreadRunning = FALSE;
		}
	}
}

void MachineTab::StopCoordinateThread()
{
	if (m_bThreadRunning && m_pCoordinateThread)
	{
		m_bThreadRunning = FALSE;
		SetEvent(m_hStopThreadEvent); // 觸發停止事件
		WaitForSingleObject(m_pCoordinateThread->m_hThread, INFINITE); // 等待執行緒結束
		m_pCoordinateThread = nullptr;
	}
}