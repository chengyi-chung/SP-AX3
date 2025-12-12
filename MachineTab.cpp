// MachineTab.cpp: 實作檔案
//

#include "pch.h"
#include "SP.h"
#include "SPDlg.h"
#include "Resource.h"
#include "afxdialogex.h"
#include "MachineTab.h"

// MachineTab 對話方塊

IMPLEMENT_DYNAMIC(MachineTab, CDialog)

MachineTab::MachineTab(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_TAB_MACHINE, pParent)
	, Discrete3000Word(0)
	, m_pCoordinateThread(nullptr)
	, m_bThreadRunning(FALSE)
	, m_hStopThreadEvent(nullptr)
	, m_bModbusError(false) // 初始化 m_bModbusError
{

}

MachineTab::~MachineTab()
{
	StopCoordinateThread();
	{
		CloseHandle(m_hStopThreadEvent);
	}
	// 不再釋放 m_ctx
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
	ON_WM_TIMER()
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

	// Laucjh OnBnClickedRadioAuto
	OnBnClickedRadioAuto();
	
	m_iMachineMode = 1;

	//Initial Discrete3000
	Discrete3000.reset();
	Discrete3000.set(0, 1);

	UpdateControl();

	// 初始化父視窗指標
	m_pParentWnd = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
	

	// 新增：每 30 秒執行一次 Modbus keep-alive
    //SetTimer(200, 30000, NULL); // Timer ID 200, 30秒

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

//Open Modbus TCP/IP server 
void MachineTab::OpenModBus()
{
	CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
	if (!pParentWnd) {
		AfxMessageBox(_T("找不到主視窗，無法建立 Modbus 連線。"));
		return;
	}

	// 取得參數
	std::string ip = pParentWnd->m_SystemPara.IpAddress;
	int port = pParentWnd->m_SystemPara.Port;
	int slaveId = pParentWnd->m_SystemPara.StationID;

	// 呼叫 YUFADlg 的重試連線
	bool ok = pParentWnd->InitModbusWithRetry(ip, port, slaveId, 3, 1000);

	if (!ok) 
	{
		// 連線失敗，已顯示錯誤訊息
		return;
	}

	// 之後可直接用 pParentWnd->m_modbusCtx 做 Modbus 操作
	// 啟動座標讀取執行緒
	StartCoordinateThread();

}

//Close Modbus TCP/IP server
void MachineTab::CloseModBus()
{
	// 先停止執行緒
	StopCoordinateThread();

	// 不再釋放 m_ctx
	m_strReportData += "\r\nModbus connection closed (由主視窗管理).";
	SetDlgItemText(IDC_EDIT_REPORT, CString(m_strReportData.c_str()));
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
	CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
    if (!pParentWnd || !pParentWnd->m_modbusCtx) {
        AfxMessageBox(_T("Modbus 尚未連線，請先建立連線。"));
        return;
    }
    std::lock_guard<std::mutex> lock(pParentWnd->m_modbusMutex);

    // Reset the Discrete3000 bitset (ensure it’s 16 bits for a single register)
    Discrete3000.reset();

    if (intType == 0) // Check or Radio Control
    {
        if (((CButton*)GetDlgItem(nID))->GetCheck() == 1)
        {
            Discrete3000.set(BitAdress, BitValue);
        }
        else
        {
            Discrete3000.set(BitAdress, 0);
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

    Discrete3000Word = Discrete3000.to_ulong();
    int rc = modbus_write_register(pParentWnd->m_modbusCtx, 30000, Discrete3000Word);

    m_strReportData = m_strReportData + "\r\n" + "Reg[30000]." + std::to_string(BitAdress) + " = " +
        std::to_string(Discrete3000Word) + " " + Discrete3000.to_string();
    SetDlgItemText(IDC_EDIT_REPORT, CString(m_strReportData.c_str()));
	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_REPORT);
	if (pEdit) {
		int nLen = pEdit->GetWindowTextLength();
		pEdit->SetSel(nLen, nLen);
	}

    if (rc == -1)
    {
        CString errorMessage;
        errorMessage.Format(_T("Failed to write to Modbus register 30000: %S"), modbus_strerror(errno));
        AfxMessageBox(errorMessage);
        return;
    }
}

void MachineTab::ClearDiscrete3000(int iStartAdress, int iEndAdress)
{
	CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
	if (!pParentWnd || !pParentWnd->m_modbusCtx) {
		AfxMessageBox(_T("Modbus 尚未連線，請先建立連線。"));
		return;
	}
	std::lock_guard<std::mutex> lock(pParentWnd->m_modbusMutex);

	for (int i = iStartAdress; i <= iEndAdress; i++)
	{
		Discrete3000.set(i, 0);
	}

	Discrete3000Word = Discrete3000.to_ulong();
	int rc = modbus_write_register(pParentWnd->m_modbusCtx, 30000, Discrete3000Word);
	m_strReportData = m_strReportData + "\r\n" + "Reg[30000]." + std::to_string(iStartAdress) + "  = " + std::to_string(Discrete3000Word) + " " + Discrete3000.to_string();
	SetDlgItemText(IDC_EDIT_REPORT, CString(m_strReportData.c_str()));
	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_REPORT);
	if (pEdit) {
		int nLen = pEdit->GetWindowTextLength();
		pEdit->SetSel(nLen, nLen);
	}
	if (rc == -1)
	{
		CString errorMessage;
		errorMessage.Format(_T("Failed to write to Modbus register: %S"), modbus_strerror(errno));
		AfxMessageBox(errorMessage);
	}
}

void MachineTab::ClearDiscrete5000(int iStartAdress, int iEndAdress)
{
	CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
	if (!pParentWnd || !pParentWnd->m_modbusCtx) {
		AfxMessageBox(_T("Modbus 尚未連線，請先建立連線。"));
		//建立連線
		return;
	}
	std::lock_guard<std::mutex> lock(pParentWnd->m_modbusMutex);

	for (int i = iStartAdress; i <= iEndAdress; i++)
	{
		Discrete5000.set(i, 0);
	}
	Discrete5000Word = Discrete5000.to_ulong();
	int rc = modbus_write_register(pParentWnd->m_modbusCtx, 63000, Discrete5000Word);
	m_strReportData = m_strReportData + "\r\n" + "Reg[63000]." + std::to_string(iStartAdress) + "  = " + std::to_string(Discrete5000Word) + " " + Discrete5000.to_string();
	SetDlgItemText(IDC_EDIT_REPORT, CString(m_strReportData.c_str()));
	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_REPORT);
	if (pEdit) {
		int nLen = pEdit->GetWindowTextLength();
		pEdit->SetSel(nLen, nLen);
	}
	if (rc == -1)
	{
		CString errorMessage;
		errorMessage.Format(_T("Failed to write to Modbus register: %S"), modbus_strerror(errno));
		AfxMessageBox(errorMessage);
	}
}

/*
void MachineTab::Discrete5000Change(int intType, int BitAdress, int BitValue, int nID)
{
	CYUFADlg* pParentWnd = dynamic_cast<CYUFADlg*>(GetParent()->GetParent());
	if (!pParentWnd || !pParentWnd->m_modbusCtx) {
		AfxMessageBox(_T("Modbus 尚未連線，請先建立連線。"));
		return;
	}
	std::lock_guard<std::mutex> lock(pParentWnd->m_modbusMutex);

	Discrete5000.reset();

	if (intType == 0)
	{
		if (((CButton*)GetDlgItem(nID))->GetCheck() == 1)
		{
			Discrete5000.set(BitAdress, BitValue);
		}
		else
		{
			Discrete5000.set(BitAdress, 0);
		}
	}
	else if (intType == 1)
	{
		Discrete5000.set(BitAdress, 1);
	}
	else
	{
		Discrete5000.set(BitAdress, BitValue);
	}

	Discrete5000Word = Discrete5000.to_ulong();
	int rc = modbus_write_register(pParentWnd->m_modbusCtx, 50000, Discrete5000Word);

	m_strReportData = m_strReportData + "\r\n" + "Reg[50000]." + std::to_string(BitAdress) + " = " +
		std::to_string(Discrete5000Word) + " " + Discrete5000.to_string();
	SetDlgItemText(IDC_EDIT_REPORT, CString(m_strReportData.c_str()));

	if (rc == -1)
	{
		CString errorMessage;
		errorMessage.Format(_T("Failed to write to Modbus register 50000: %S"), modbus_strerror(errno));
		AfxMessageBox(errorMessage);
		return;
	}
}
*/

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
void MachineTab::Discrete5000Change(int intType, int BitAdress, int BitValue, int nID)
{
	CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(GetParent()->GetParent());

	if (!pParentWnd || !pParentWnd->m_modbusCtx) {
		AfxMessageBox(_T("Modbus 尚未連線，請先建立連線。"));
		return;
	}
	std::lock_guard<std::mutex> lock(pParentWnd->m_modbusMutex);

	Discrete5000.reset();

	if (intType == 0)
	{
		if (((CButton*)GetDlgItem(nID))->GetCheck() == 1)
		{
			Discrete5000.set(BitAdress, BitValue);
		}
		else
		{
			Discrete5000.set(BitAdress, 0);
		}
	}
	else if (intType == 1)
	{
		Discrete5000.set(BitAdress, 1);
	}
	else
	{
		Discrete5000.set(BitAdress, BitValue);
	}

	Discrete5000Word = Discrete5000.to_ulong();

    //{
		//std::lock_guard<std::mutex> lock(pParentWnd->m_modbusMutex);
        int rc = modbus_write_register(pParentWnd->m_modbusCtx, 63000, Discrete5000Word);

        m_strReportData = m_strReportData + "\r\n" + "Reg[63000]." + std::to_string(BitAdress) + " = " +
            std::to_string(Discrete5000Word) + " " + Discrete5000.to_string();
        SetDlgItemText(IDC_EDIT_REPORT, CString(m_strReportData.c_str()));

        if (rc == -1) {
            CString errorMessage;
            errorMessage.Format(_T("Failed to write to Modbus register 63000: %S"), modbus_strerror(errno));
            AfxMessageBox(errorMessage);
            return;
        }
    //}
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
	/*
	constexpr uint16_t REG_BASE_Z1 = 40000; // Z1 double word (40000, 40001)
	constexpr uint16_t REG_BASE_Z2 = 40002; // Z2 double word
	constexpr uint16_t REG_BASE_Z3 = 40004; // Z3 double word
	constexpr uint16_t REG_BASE_Z4 = 40006; // Z4 double word
	constexpr uint16_t REG_BASE_Z5 = 40008; // Z5 double word
	*/
	
	constexpr uint16_t REG_JOG_VELOCITY = 60006; // Jog Velocity double word
	constexpr uint16_t REG_AUTO_VELOCITY = 60008; // Auto Velocity double word
	constexpr uint16_t REG_AXIS_DEC_ACC = 60010; // Dec Acceleration double word
	constexpr uint16_t REG_AXIS_INC_ACC = 60012; // Inc Acceleration double word

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

	CString strZ1;
	GetDlgItemText(IDC_EDIT_Z1, strZ1);
	float z1Pos = _ttof(strZ1);
	if (strZ1.IsEmpty()) { AfxMessageBox(_T("Z1 Position input error")); return; }

	CString strZ2;
	GetDlgItemText(IDC_EDIT_Z2, strZ2);
	float z2Pos = _ttof(strZ2);
	if (strZ2.IsEmpty()) { AfxMessageBox(_T("Z2 Position input error")); return; }

	/*

	int32_t z3Pos = GetDlgItemInt(IDC_EDIT_Z3, &ok, TRUE);
	if (!ok) { AfxMessageBox(_T("Z3 Position input error")); return; }

	int32_t z4Pos = GetDlgItemInt(IDC_EDIT_Z4, &ok, TRUE);
	if (!ok) { AfxMessageBox(_T("Z4 Position input error")); return; }

	int32_t z5Pos = GetDlgItemInt(IDC_EDIT_Z5, &ok, TRUE);
	if (!ok) { AfxMessageBox(_T("Z5 Position input error")); return; }
	*/


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
	/*
	splitInt32(z1Pos, z1Low, z1High);
	splitInt32(z2Pos, z2Low, z2High);
	splitInt32(z3Pos, z3Low, z3High);
	splitInt32(z4Pos, z4Low, z4High);
	splitInt32(z5Pos, z5Low, z5High);
	*/

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

	/*
	
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
	
	*/
	

	// ======== Step 4. Update System Parameters ========
	CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
	if (!pParentWnd)
	{
		AfxMessageBox(_T("Failed to retrieve parent dialog"));
		return;
	}

	pParentWnd->m_SystemPara.JogVelocity = jogVelocity;
	pParentWnd->m_SystemPara.AutoVelocity = autoVelocity;
	pParentWnd->m_SystemPara.DecAcceleration = decAcc;
	pParentWnd->m_SystemPara.IncAcceleration = incAcc;


	pParentWnd->m_SystemPara.Z1 = z1Pos;
	pParentWnd->m_SystemPara.Z2 = z2Pos;
	/*
	pParentWnd->m_SystemPara.Z3 = z3Pos;
	pParentWnd->m_SystemPara.Z4 = z4Pos;
	pParentWnd->m_SystemPara.Z5 = z5Pos;
	*/
	

	// ======== Step 5. Handle Pitch and Transfer Factor ========
	CString strPitch, strTransfer;
	GetDlgItemText(IDC_EDIT_PITCH, strPitch);
	GetDlgItemText(IDC_EDIT_TRANSFER_FACTOR, strTransfer);

	double pitch = _ttof(strPitch);
	double transferFactor = _ttof(strTransfer);

	if (pitch <= 0 || transferFactor <= 0)
	{
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
	CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
    if (!pParentWnd || !pParentWnd->m_modbusCtx) {
        AfxMessageBox(_T("Modbus 尚未連線，請先建立連線。"));
        return false;
    }

    std::lock_guard<std::mutex> lock(pParentWnd->m_modbusMutex);
    int rc = modbus_write_registers(pParentWnd->m_modbusCtx, iStartAdress, SizeOfArray, iValue);
    if (rc == -1) {
        CString err;
        err.Format(_T("寫入失敗: %S"), modbus_strerror(errno));
        AfxMessageBox(err);
        return false;
    }
    return true;
}
//Get Holding Register value
//iStartAdress: Start Address
//iEndAdress: End Address
//int iValue[]: Value array to be get
void MachineTab::GetHoldingRegister(int iStartAdress, int iEndAdress, uint16_t* iValue)
{
	CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
    if (!pParentWnd || !pParentWnd->m_modbusCtx) {
        // 沒有連線就直接返回
        return;
    }

    int count = iEndAdress - iStartAdress + 1;
    if (count <= 0) return;

    std::lock_guard<std::mutex> lock(pParentWnd->m_modbusMutex);
    int rc = modbus_read_registers(pParentWnd->m_modbusCtx, iStartAdress, count, iValue);
    if (rc == -1) {
        CString errorMessage;
        errorMessage.Format(_T("Failed to read Modbus holding registers: %S"), modbus_strerror(errno));
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
	CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
	if (!pParentWnd || !pParentWnd->m_modbusCtx) {
		AfxMessageBox(_T("Modbus 尚未連線，請先建立連線。"));
		return;
	}
	std::lock_guard<std::mutex> lock(pParentWnd->m_modbusMutex);

	int rc = modbus_write_registers(pParentWnd->m_modbusCtx, iStartAdress, SizeOfArray * 2, iValue);
	if (rc == -1)
	{
		CString errorMessage;
		errorMessage.Format(_T("Failed to write to Modbus register DINT: %S"), modbus_strerror(errno));
		AfxMessageBox(errorMessage);
	}
}
	

BOOL MachineTab::PreTranslateMessage(MSG* pMsg)
{
	// 攔截按鈕的 Mouse Down 和 Mouse Up 事件
	if (pMsg->message == WM_LBUTTONDOWN || pMsg->message == WM_LBUTTONUP)
	{

		m_IsModbusWrite = TRUE;

		// 攔截 X + 按鈕的 Mouse Down / Up
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
				//this->ClearDiscrete5000(0, 8);
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
		//攔截 X - 按鈕的 Mouse Down / Up
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
			 //ClearDiscrete5000(0, 8);
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
			 //ClearDiscrete5000(0, 8);
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
			 //ClearDiscrete5000(0, 8);
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
			 //ClearDiscrete5000(0, 8);
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
			    //ClearDiscrete5000(0, 8);
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



		m_IsModbusWrite = FALSE;
	}

	// 攔截 Enter 鍵
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

	//寫入 Modbus holding Register 000000: x low, 000001: x high, 000002: y low, 20000: y high : 20001
	uint16_t xLow, xHigh, yLow, yHigh;
	xLow = static_cast<uint16_t>((static_cast<uint32_t>(fX * 100) >> 16) & 0xFFFF);   // 原 high -> low
	xHigh = static_cast<uint16_t>(static_cast<uint32_t>(fX * 100) & 0xFFFF);          // 原 low  -> high
	yLow = static_cast<uint16_t>((static_cast<uint32_t>(fY * 100) >> 16) & 0xFFFF);   // 原 high -> low
	yHigh = static_cast<uint16_t>(static_cast<uint32_t>(fY * 100) & 0xFFFF);          // 原 low  -> high

	uint16_t valuesX[2] = { xLow, xHigh };  
	SetHoldingRegister(0, 1, valuesX, 2);
	uint16_t valuesY[2] = { yLow, yHigh };
	SetHoldingRegister(20000, 20001, valuesY, 2);

	uint16_t values[1];
	SetHoldingRegister(40026, 40026, values, 1);
	
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
//由 PostMessage 呼叫
void MachineTab::UpdateControl()
{
	// Get the parent dialog (CYUFADlg)
	CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
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
	cStr.Format(_T("%d"), static_cast<int>(pParentWnd->m_SystemPara.DecAcceleration));
	SetDlgItemText(IDC_EDIT_AXIS_ACC_DEC, cStr);

	// AxisAccInc (assuming float for precision, adjust if int)
	cStr.Format(_T("%d"), static_cast<int>(pParentWnd->m_SystemPara.IncAcceleration));
	SetDlgItemText(IDC_EDIT_AXIS_ACC_INC, cStr);

	// Pitch (assuming float, adjust if int)
	cStr.Format(_T("%.3f"), pParentWnd->m_SystemPara.Pitch);
	SetDlgItemText(IDC_EDIT_PITCH, cStr);

	// TransferFactor (assuming float, adjust if int)
	cStr.Format(_T("%.4f"), pParentWnd->m_SystemPara.TransferFactor);
	SetDlgItemText(IDC_EDIT_TRANSFER_FACTOR, cStr);
	
	// Z1-Z5 位置值 - 修正 C6273 警告
	cStr.Format(_T("%0.2f"), static_cast<float>(pParentWnd->m_SystemPara.Z1));
	SetDlgItemText(IDC_EDIT_Z1, cStr);
	
	cStr.Format(_T("%0.2f"), static_cast<float>(pParentWnd->m_SystemPara.Z2));
	SetDlgItemText(IDC_EDIT_Z2, cStr);
	
	cStr.Format(_T("%d"), static_cast<int>(pParentWnd->m_SystemPara.Z3));
	SetDlgItemText(IDC_EDIT_Z3, cStr);
	
	cStr.Format(_T("%d"), static_cast<int>(pParentWnd->m_SystemPara.Z4));
	SetDlgItemText(IDC_EDIT_Z4, cStr);
	
	cStr.Format(_T("%d"), static_cast<int>(pParentWnd->m_SystemPara.Z5));
	SetDlgItemText(IDC_EDIT_Z5, cStr);
}

// Custom message handler to update coordinates
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
	if (pThis == nullptr || pThis->m_pParentWnd == nullptr)
		return 0;

	CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(pThis->m_pParentWnd);

	constexpr int startAddress = 60000;
	constexpr int numRegisters = 6;
	const float scalingFactor = 1;
	//const float scalingFactor = -1580.0f / -142606337.0f;

	while (pThis->m_bThreadRunning)
	{
		if (pThis->m_hStopThreadEvent && WaitForSingleObject(pThis->m_hStopThreadEvent, 0) == WAIT_OBJECT_0)
			break;

		if (!pParentWnd->m_modbusCtx || !pThis->flgGetCoord)
		{
			Sleep(100);
			continue;
		}

		if (!pThis->m_IsModbusWrite)
		{

			uint16_t registers[numRegisters] = { 0 };
			{
				std::lock_guard<std::mutex> lock(pParentWnd->m_modbusMutex);

				int rc = modbus_read_registers(pParentWnd->m_modbusCtx, startAddress, numRegisters, registers);

				if (rc == numRegisters)
				{
					// 成功，m_bModbusError 設為 false
					pThis->m_bModbusError = false;
				}
				else {
					if (!pThis->m_bModbusError) {
						// 失敗且尚未標記錯誤，設為 true
						pThis->m_bModbusError = true;
					}
					// ...其他錯誤處理...
				}

				if (rc == numRegisters)
				{
					float coordinates[3] = { 0.0f, 0.0f, 0.0f };
					for (int i = 0; i < 3; ++i)
					{
						// 修正：registers[i*2] 是低位字，registers[i*2+1] 是高位字
						uint32_t raw = (static_cast<uint32_t>(registers[i * 2 + 1]) << 16) | registers[i * 2];
						int32_t signedValue = static_cast<int32_t>(raw);
						coordinates[i] = static_cast<float>(signedValue) * scalingFactor;
					}

					float* coordPtr = new float[3];
					coordPtr[0] = coordinates[0];
					coordPtr[1] = coordinates[1];
					coordPtr[2] = coordinates[2];

					pThis->PostMessage(WM_UPDATE_COORDINATES, 0, reinterpret_cast<LPARAM>(coordPtr));
					pThis->m_bModbusError = false; // 成功時清除錯誤旗標
				}
				else
				{
					if (!pThis->m_bModbusError) {
						CString errorMessage;
						errorMessage.Format(_T("Failed to read coordinates: %S"), modbus_strerror(errno));
						pThis->m_strReportData += "\r\n" + std::string(CT2A(errorMessage));
						pThis->PostMessage(WM_UPDATE_REPORT, 0, reinterpret_cast<LPARAM>(new std::string(pThis->m_strReportData)));
						AfxMessageBox(errorMessage);
						pThis->m_bModbusError = true;
					}
					//pThis->ClearDiscrete5000(0, 8);
					//Sleep(200);
				}
			}
			Sleep(200);

		}
		else
		{
			int test = 0;
		}
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
	 CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
    if (!pParentWnd || !pParentWnd->m_modbusCtx) {
        AfxMessageBox(_T("Modbus 尚未連線，請先建立連線。"));
        return false;
    }

    uint16_t regs[2];
    regs[0] = highWord; // 高位在前
    regs[1] = lowWord;  // 低位在後

    std::lock_guard<std::mutex> lock(pParentWnd->m_modbusMutex);
    int rc = modbus_write_registers(pParentWnd->m_modbusCtx, iStartAdress, 2, regs);
    if (rc == -1)
    {
        CString errorMessage;
        errorMessage.Format(_T("Failed to write to Modbus registers starting %d: %S"), iStartAdress, modbus_strerror(errno));
        AfxMessageBox(errorMessage);
        return false;
    }

    return true;
}

void MachineTab::OnTimer(UINT_PTR nIDEvent)
{
	CDialog::OnTimer(nIDEvent);

	if (nIDEvent == 200) // Timer ID 200
	{
		// 執行 Modbus keep-alive，這裡簡單讀取一個寄存器作為範例
		CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
		if (pParentWnd && pParentWnd->m_modbusCtx)
		{
			uint16_t dummy;
			std::lock_guard<std::mutex> lock(pParentWnd->m_modbusMutex);
			int rc = modbus_read_registers(pParentWnd->m_modbusCtx, 0, 1, &dummy);

			if (rc == -1)
			{
				CString errorMessage;
				errorMessage.Format(_T("Modbus keep-alive failed: %S"), modbus_strerror(errno));
				AfxMessageBox(errorMessage);
			}
		}
	}

	// 可加入其他定時任務
}

//#define WM_UPDATE_REPORT (WM_USER + 2)
