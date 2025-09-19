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

	// Laucxh OnBnClickedRadioAuto
	OnBnClickedRadioAuto();
	
	m_iMachineMode = 1;

	//Initial Discrete3000
	Discrete3000.reset();
	Discrete3000.set(0, 1);

	UpdateControl();

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
	// ====== Double Word Register Address Definitions ======
	constexpr uint16_t REG_BASE_Z1 = 40000; // Z1 double word (40000, 40001)
	constexpr uint16_t REG_BASE_Z2 = 40002; // Z2 double word
	constexpr uint16_t REG_BASE_Z3 = 40004; // Z3 double word
	constexpr uint16_t REG_BASE_Z4 = 40006; // Z4 double word
	constexpr uint16_t REG_BASE_Z5 = 40008; // Z5 double word
	constexpr uint16_t REG_JOG_VELOCITY = 40016; // Jog Velocity double word
	constexpr uint16_t REG_AUTO_VELOCITY = 40018; // Auto Velocity double word
	constexpr uint16_t REG_AXIS_DEC_ACC = 40020; // Dec Acceleration double word
	constexpr uint16_t REG_AXIS_INC_ACC = 40022; // Inc Acceleration double word

	// ======== Step 1. Retrieve motion parameters from UI ========
	BOOL ok = FALSE;
	int32_t jogVelocity = GetDlgItemInt(IDC_EDIT_JOG_VELOCITY, &ok, TRUE);
	if (!ok) { AfxMessageBox(_T("Jog Velocity input error")); return; }

	int32_t autoVelocity = GetDlgItemInt(IDC_EDIT_AUTO_VELOCITY, &ok, TRUE);
	if (!ok) { AfxMessageBox(_T("Auto Velocity input error")); return; }

	int32_t decAcc = GetDlgItemInt(IDC_EDIT_AXIS_ACC_DEC, &ok, TRUE);
	if (!ok) { AfxMessageBox(_T("Deceleration input error")); return; }

	int32_t incAcc = GetDlgItemInt(IDC_EDIT_AXIS_ACC_INC, &ok, TRUE);
	if (!ok) { AfxMessageBox(_T("Acceleration input error")); return; }

	int32_t z1Pos = GetDlgItemInt(IDC_EDIT_Z1, &ok, TRUE);
	if (!ok) { AfxMessageBox(_T("Z1 Position input error")); return; }

	int32_t z2Pos = GetDlgItemInt(IDC_EDIT_Z2, &ok, TRUE);
	if (!ok) { AfxMessageBox(_T("Z2 Position input error")); return; }

	int32_t z3Pos = GetDlgItemInt(IDC_EDIT_Z3, &ok, TRUE);
	if (!ok) { AfxMessageBox(_T("Z3 Position input error")); return; }

	int32_t z4Pos = GetDlgItemInt(IDC_EDIT_Z4, &ok, TRUE);
	if (!ok) { AfxMessageBox(_T("Z4 Position input error")); return; }

	int32_t z5Pos = GetDlgItemInt(IDC_EDIT_Z5, &ok, TRUE);
	if (!ok) { AfxMessageBox(_T("Z5 Position input error")); return; }

	// ======== Step 2. Split int32_t into high/low 16-bit ========
	auto splitInt32 = [](int32_t value, uint16_t& low, uint16_t& high)
		{
			uint32_t uval = static_cast<uint32_t>(value);
			// 交換：將高 16 位寫入 low，將低 16 位寫入 high
			low = static_cast<uint16_t>((uval >> 16) & 0xFFFF);   // 原 high -> low
			high = static_cast<uint16_t>(uval & 0xFFFF);          // 原 low  -> high
		};

	uint16_t jogLow, jogHigh, autoLow, autoHigh;
	uint16_t decLow, decHigh, incLow, incHigh;
	uint16_t z1Low, z1High, z2Low, z2High, z3Low, z3High, z4Low, z4High, z5Low, z5High;

	splitInt32(jogVelocity, jogLow, jogHigh);
	splitInt32(autoVelocity, autoLow, autoHigh);
	splitInt32(decAcc, decLow, decHigh);
	splitInt32(incAcc, incLow, incHigh);
	splitInt32(z1Pos, z1Low, z1High);
	splitInt32(z2Pos, z2Low, z2High);
	splitInt32(z3Pos, z3Low, z3High);
	splitInt32(z4Pos, z4Low, z4High);
	splitInt32(z5Pos, z5Low, z5High);


	// ======== Step 3. Write to PLC (Double Word, each value 2 registers) ========
	if (!SetHoldingRegister32(REG_JOG_VELOCITY, jogLow, jogHigh)) {
		AfxMessageBox(_T("Failed to write Jog Velocity"));
		return;
	}
	if (!SetHoldingRegister32(REG_AUTO_VELOCITY, autoLow, autoHigh)) {
		AfxMessageBox(_T("Failed to write Auto Velocity"));
		return;
	}
	if (!SetHoldingRegister32(REG_AXIS_DEC_ACC, decLow, decHigh)) {
		AfxMessageBox(_T("Failed to write Deceleration"));
		return;
	}
	if (!SetHoldingRegister32(REG_AXIS_INC_ACC, incLow, incHigh)) {
		AfxMessageBox(_T("Failed to write Acceleration"));
		return;
	}
	if (!SetHoldingRegister32(REG_BASE_Z1, z1Low, z1High)) {
		AfxMessageBox(_T("Failed to write Z1 Position"));
		return;
	}
	if (!SetHoldingRegister32(REG_BASE_Z2, z2Low, z2High)) {
		AfxMessageBox(_T("Failed to write Z2 Position"));
		return;
	}
	if (!SetHoldingRegister32(REG_BASE_Z3, z3Low, z3High)) {
		AfxMessageBox(_T("Failed to write Z3 Position"));
		return;
	}
	if (!SetHoldingRegister32(REG_BASE_Z4, z4Low, z4High)) {
		AfxMessageBox(_T("Failed to write Z4 Position"));
		return;
	}
	if (!SetHoldingRegister32(REG_BASE_Z5, z5Low, z5High)) {
		AfxMessageBox(_T("Failed to write Z5 Position"));
		return;
	}

	// ======== Step 4. Update System Parameters ========
	CYUFADlg* pParentWnd = dynamic_cast<CYUFADlg*>(GetParent()->GetParent());
	if (!pParentWnd) {
		AfxMessageBox(_T("Failed to retrieve parent dialog"));
		return;
	}

	pParentWnd->m_SystemPara.JogVelocity = jogVelocity;
	pParentWnd->m_SystemPara.AutoVelocity = autoVelocity;
	pParentWnd->m_SystemPara.DecAcceleration = decAcc;
	pParentWnd->m_SystemPara.IncAcceleration = incAcc;
	pParentWnd->m_SystemPara.Z1 = z1Pos;
	pParentWnd->m_SystemPara.Z2 = z2Pos;
	pParentWnd->m_SystemPara.Z3 = z3Pos;
	pParentWnd->m_SystemPara.Z4 = z4Pos;
	pParentWnd->m_SystemPara.Z5 = z5Pos;


	// ======== Step 5. Handle Pitch and Transfer Factor ========
	CString strPitch, strTransfer;
	GetDlgItemText(IDC_EDIT_PITCH, strPitch);
	GetDlgItemText(IDC_EDIT_TRANSFER_FACTOR, strTransfer);

	double pitch = _ttof(strPitch);
	double transferFactor = _ttof(strTransfer);

	if (pitch <= 0 || transferFactor <= 0) {
		AfxMessageBox(_T("Pitch / Transfer Factor must be positive numbers"));
		return;
	}
	pParentWnd->m_SystemPara.Pitch = pitch;
	pParentWnd->m_SystemPara.TransferFactor = transferFactor;

	// ======== Step 6. Save Configuration ========
	std::string appPath = GetAppPath();
	CString configFile = CString(appPath.c_str()) + _T("\\SystemConfig.ini");
	WriteConfigToFile(std::string(CT2A(configFile)), pParentWnd->m_SystemPara);
}


//Set Holding Register value
//iStartAdress: Start Address
//iEndAdress: End Address
//int iValue[]: Value array to be set
bool MachineTab::SetHoldingRegister(int iStartAdress, int iEndAdress, uint16_t* iValue, int SizeOfArray)
{
	// 檢查 Modbus context
	if (m_ctx == nullptr)
	{
		AfxMessageBox(_T("Modbus context is not initialized."));
		return false;
	}

	// 檢查輸入參數
	if (iValue == nullptr || SizeOfArray <= 0)
	{
		AfxMessageBox(_T("Invalid parameters for SetHoldingRegister."));
		return false;
	}

	// 計算並檢查結束位址一致性（非致命）
	int expectedEnd = iStartAdress + SizeOfArray - 1;
	if (iEndAdress != expectedEnd)
	{
		// 記錄警告到報告框，但仍按 SizeOfArray 執行寫入
		CString warn;
		warn.Format(_T("Warning: iEndAdress(%d) != expectedEnd(%d). Using sizeOfArray=%d."),
			iEndAdress, expectedEnd, SizeOfArray);
		m_strReportData += "\r\n" + std::string(CT2A(warn));
		SetDlgItemText(IDC_EDIT_REPORT, CString(m_strReportData.c_str()));
	}

	// libmodbus 一次寫入的最大暫存器數量（安全上限）
	const int MODBUS_MAX_REGS = 125;
	if (SizeOfArray > MODBUS_MAX_REGS)
	{
		CString err;
		err.Format(_T("Too many registers to write: %d (max %d)."), SizeOfArray, MODBUS_MAX_REGS);
		AfxMessageBox(err);
		return false;
	}

	// 執行寫入
	int rc = modbus_write_registers(m_ctx, iStartAdress, SizeOfArray, iValue);
	if (rc == -1 || rc != SizeOfArray)
	{
		CString errorMessage;
		errorMessage.Format(_T("Failed to write %d registers starting at %d: %S"),
			SizeOfArray, iStartAdress, modbus_strerror(errno));
		AfxMessageBox(errorMessage);

		// 記錄錯誤
		m_strReportData += "\r\n" + std::string(CT2A(errorMessage));
		SetDlgItemText(IDC_EDIT_REPORT, CString(m_strReportData.c_str()));
		return false;
	}

	// 成功，記錄並回傳 true
	CString okMsg;
	okMsg.Format(_T("Wrote %d registers starting at %d successfully."), SizeOfArray, iStartAdress);
	m_strReportData += "\r\n" + std::string(CT2A(okMsg));
	SetDlgItemText(IDC_EDIT_REPORT, CString(m_strReportData.c_str()));

	return true;
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
	Discrete5000Change(0, bitAdress, bitValue, nID);

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
	Discrete5000Change(0, bitAdress, bitValue, nID);


}

void MachineTab::OnBnClickedMfcbtnMachineAutoWorkSart()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	int bitAdress = 12;
	int bitValue = 1;
	int nID = 0;  // IDC_CHECK_AUTO_WORK_START;
	//ClearDiscrete3000(0, 7);
	Discrete5000Change(1, bitAdress, bitValue, nID);
}

void MachineTab::OnBnClickedMfcbtnMachineAutoWorkStop()
{
	// TODO: 在此加入控制項告知處理常式程式碼


	int bitAdress = 13;
	int bitValue = 1;
	int nID = 0;  // IDC_CHECK_AUTO_WORK_STOP;
	//ClearDiscrete3000(0, 7);
	Discrete5000Change(1, bitAdress, bitValue, nID);


}

void MachineTab::OnBnClickedMfcbtnMachineHome()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	int bitAdress = 10;
	int bitValue = 1;
	int nID = 0; // IDC_CHECK_HOME;
	//ClearDiscrete3000(0, 7);
	Discrete5000Change(1, bitAdress, bitValue, nID);
}

void MachineTab::OnBnClickedMfcbtnMachineResetSw()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	int bitAdress = 11;
	int bitValue = 1;
	int nID = 0; // IDC_CHECK_RESET;
	//ClearDiscrete3000(0, 7);
	Discrete5000Change(1, bitAdress, bitValue, nID);
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
	// Z1 (assuming int, adjust if float)
	//cStr.Format(_T("%d"), pParentWnd->m_SystemPara.Z1);
    // 原本的寫法會有 C6273 警告，因為 pParentWnd->m_SystemPara.Z1 是 float 型別
    // cStr.Format(_T("%d"), pParentWnd->m_SystemPara.Z1);

    // 修正方式：先將 float 轉為 int，再傳給 Format
    cStr.Format(_T("%d"), static_cast<int>(pParentWnd->m_SystemPara.Z1));
	SetDlgItemText(IDC_EDIT_Z1, cStr);
	// Z2 (assuming int, adjust if float)
	cStr.Format(_T("%d"), static_cast<int>(pParentWnd->m_SystemPara.Z2));
	SetDlgItemText(IDC_EDIT_Z2, cStr);
	// Z3 (assuming int, adjust if float)
	cStr.Format(_T("%d"), static_cast<int>(pParentWnd->m_SystemPara.Z3));
	SetDlgItemText(IDC_EDIT_Z3, cStr);
	// Z4 (assuming int, adjust if float)
	cStr.Format(_T("%d"), static_cast<int>(pParentWnd->m_SystemPara.Z4));
	SetDlgItemText(IDC_EDIT_Z4, cStr);
	// Z5 (assuming int, adjust if float)
	cStr.Format(_T("%d"), static_cast<int>(pParentWnd->m_SystemPara.Z5));
	SetDlgItemText(IDC_EDIT_Z5, cStr);

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
	if (pThis == nullptr)
		return 0;

	constexpr int startAddress = 40010;
	constexpr int numRegisters = 6; // 3 個座標，每個座標 2 個暫存器
	const float scalingFactor = -1580.0f / -142606337.0f; // 縮放因子

	while (pThis->m_bThreadRunning)
	{
		// 檢查是否被要求停止
		if (pThis->m_hStopThreadEvent && WaitForSingleObject(pThis->m_hStopThreadEvent, 0) == WAIT_OBJECT_0)
			break;

		// 如果沒有 Modbus context 或不允許抓取座標，就短暫休息並繼續迴圈
		if (pThis->m_ctx == nullptr || !pThis->flgGetCoord)
		{
			Sleep(100);
			continue;
		}

		uint16_t registers[numRegisters] = { 0 };
		int rc = modbus_read_registers(pThis->m_ctx, startAddress, numRegisters, registers);

		if (rc == numRegisters)
		{
			// 合併並轉換
			float coordinates[3] = { 0.0f, 0.0f, 0.0f };
			for (int i = 0; i < 3; ++i)
			{
				uint32_t raw = (static_cast<uint32_t>(registers[i * 2]) << 16) | registers[i * 2 + 1];
				int32_t signedValue = static_cast<int32_t>(raw);
				coordinates[i] = static_cast<float>(signedValue) * scalingFactor;
			}

			// 動態配置並傳給主執行緒更新 UI（UI 負責 delete[]）
			float* coordPtr = new float[3];
			coordPtr[0] = coordinates[0];
			coordPtr[1] = coordinates[1];
			coordPtr[2] = coordinates[2];

			pThis->PostMessage(WM_UPDATE_COORDINATES, 0, reinterpret_cast<LPARAM>(coordPtr));
		}
		else
		{
			// 讀取失敗，記錄錯誤並清除相關輸出位元
			CString errorMessage;
			errorMessage.Format(_T("Failed to read coordinates: %S"), modbus_strerror(errno));
			pThis->m_strReportData += "\r\n" + std::string(CT2A(errorMessage));
			//SetDlgItemText(IDC_EDIT_REPORT, CString(pThis->m_strReportData.c_str()));
			// 背景執行緒發送更新請求
			pThis->PostMessage(WM_UPDATE_REPORT, 0, reinterpret_cast<LPARAM>(new std::string(pThis->m_strReportData)));

			// 清除輸出位元以保護系統
			pThis->ClearDiscrete5000(0, 8);

			// 若讀取失敗，可稍作延遲再重試
			Sleep(200);
		}

		// 迴圈頻率控制
		Sleep(800);
	}

	return 0;
}

void MachineTab::StartCoordinateThread()
{
	if (!m_bThreadRunning)
	{
		// 若尚未建立停止事件，建立一個 manual-reset 事件
		if (m_hStopThreadEvent == nullptr)
		{
			m_hStopThreadEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
			if (m_hStopThreadEvent == nullptr)
			{
				AfxMessageBox(_T("Failed to create stop-event for coordinate thread."));
				return;
			}
		}

		// 確保事件為未觸發狀態
		ResetEvent(m_hStopThreadEvent);

		m_bThreadRunning = TRUE;
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
	// 若尚未啟動或執行緒指標為空，直接返回
	if (!m_bThreadRunning || m_pCoordinateThread == nullptr)
		return;

	// 要求執行緒結束
	m_bThreadRunning = FALSE;

	// 觸發停止事件，讓執行緒可以立刻跳出等待
	if (m_hStopThreadEvent)
	{
		SetEvent(m_hStopThreadEvent);
	}

	// 等待執行緒結束 (安全檢查 m_pCoordinateThread->m_hThread)
	if (m_pCoordinateThread->m_hThread)
	{
		WaitForSingleObject(m_pCoordinateThread->m_hThread, INFINITE);
	}

	// 清除執行緒物件指標（CWinThread 由 MFC 管理，AfxBeginThread 建立後 MFC 負責釋放）
	m_pCoordinateThread = nullptr;
}

// 新增：實作 bool SetHoldingRegister(int, uint16_t, uint16_t)
// 這個 overload 會寫入 2 個連續的保持暫存器 (reg, reg+1)
// 注意：讀取時程式把 registers[0] 當作 high word、registers[1] 當作 low word
// 因此寫入也要採用相同的順序 (high first, low second)。
 bool MachineTab::SetHoldingRegister32(int iStartAdress, uint16_t lowWord, uint16_t highWord)
{
	// 確認 Modbus context 已初始化
	if (m_ctx == nullptr)
	{
		AfxMessageBox(_T("Modbus context is not initialized."));
		return false;
	}

	// 按照系統中讀取時的約定：先寫入 high word，再寫入 low word
	uint16_t regs[2];
	regs[0] = highWord; // 高位在前
	regs[1] = lowWord;  // 低位在後

	int rc = modbus_write_registers(m_ctx, iStartAdress, 2, regs);
	if (rc == -1)
	{
		CString errorMessage;
		errorMessage.Format(_T("Failed to write to Modbus registers starting %d: %S"), iStartAdress, modbus_strerror(errno));
		AfxMessageBox(errorMessage);
		return false;
	}

	return true ;
}