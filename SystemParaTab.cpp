// SystemParaTab.cpp: 實作檔案
//

#include "pch.h"
#include "SP.h"
#include "SPDlg.h"
#include "WorkTab.h"
#include "afxdialogex.h"
#include "SystemParaTab.h"
#include "Resource.h"
#include "UAX.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
// SystemParaTab 對話方塊

IMPLEMENT_DYNAMIC(SystemParaTab, CDialog)

SystemParaTab::SystemParaTab(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_TAB_SYSTEM_PARA, pParent)
{
	
}

SystemParaTab::~SystemParaTab()
{
	
}

void SystemParaTab::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(SystemParaTab, CDialog)
	ON_BN_CLICKED(IDC_SYSTEM_CREATE_DATA, &SystemParaTab::OnBnClickedSystemCreateData)
	ON_EN_CHANGE(IDD_TAB_SYS_OFFSET_VALUE, &SystemParaTab::OnEnChangeTabSysOffsetValue)
	ON_BN_CLICKED(IDC_MFCBTN_SAVE_SYSTEM, &SystemParaTab::OnBnClickedMfcbtnSaveSystem)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// SystemParaTab 訊息處理常式

BOOL SystemParaTab::OnInitDialog()
{
	CDialog::OnInitDialog();

	CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(GetParent()->GetParent());


	CString str;
	str.Format(_T("%d"), 10);

	/*
	
	// Replace IDD_TAB_SYS_OFFSET_VALUE with the correct control ID
	float offsetValue = sqrt(pParentWnd->m_SystemPara.OffsetX * pParentWnd->m_SystemPara.OffsetX +
		pParentWnd->m_SystemPara.OffsetY * pParentWnd->m_SystemPara.OffsetY);
	str.Format(_T("%0.4f"), offsetValue);
	SetDlgItemText(IDD_TAB_SYS_OFFSET_VALUE, str);

	str.Format(_T("%0.4f"), pParentWnd->m_SystemPara.OffsetX);
	SetDlgItemText(IDD_TAB_SYS_X_OFFSET, str);
	str.Format(_T("%0.4f"), pParentWnd->m_SystemPara.OffsetY);
	SetDlgItemText(IDD_TAB_SYS_Y_OFFSET, str);

	//pParentWnd = (CYUFADlg*)GetParent();

		// 確保視窗已正確初始化
	if (m_hWnd == NULL)
	{
		return FALSE; // 初始化失敗
	}
	*/
	CWnd* pSystemConfigEdit = GetDlgItem(IDC_EDIT_SYSTEM_CONFIG_LIVE);
	CWnd* pMemStructEdit = GetDlgItem(IDC_EDIT_MEMSTRUCT_LIVE);
	if (pSystemConfigEdit != nullptr) {
		CFont* pBaseFont = pSystemConfigEdit->GetFont();
		if (pBaseFont != nullptr) {
			LOGFONT lf{};
			if (pBaseFont->GetLogFont(&lf) != 0) {
				lf.lfHeight = static_cast<LONG>(lf.lfHeight * 1.25);
				if (m_liveDataFont.GetSafeHandle() != nullptr) {
					m_liveDataFont.DeleteObject();
				}
				if (m_liveDataFont.CreateFontIndirect(&lf)) {
					pSystemConfigEdit->SetFont(&m_liveDataFont);
					if (pMemStructEdit != nullptr) {
						pMemStructEdit->SetFont(&m_liveDataFont);
					}
				}
			}
		}
	}

	SetTimer(1, 1000, nullptr);
	UpdateControl();
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX 屬性頁應傳回 FALSE
}

void SystemParaTab::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1) {
		UpdateControl();
		return;
	}

	CDialog::OnTimer(nIDEvent);
}


void SystemParaTab::OnBnClickedSystemCreateData()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	//Create a database with sqlite3, Use UAX.dll function to create database
	sqlite3* db = nullptr;
	const char* db_name = "SystemConfig.db";
	
	int ret = CreateDatabase(db, db_name);
	
	

}


void SystemParaTab::OnEnChangeTabSysOffsetValue()
{
	// TODO:  如果這是 RICHEDIT 控制項，控制項將不會
	// 傳送此告知，除非您覆寫 CDialogEx::OnInitDialog()
	// 函式和呼叫 CRichEditCtrl().SetEventMask()
	// 讓具有 ENM_CHANGE 旗標 ORed 加入遮罩。

	// TODO:  在此加入控制項告知處理常式程式碼
	CString str;
	GetDlgItemText(IDD_TAB_SYS_OFFSET_VALUE, str);
	//AfxMessageBox(str);
	float fValue = _ttof(str);
	double iResult;
	double Radian = 45.0 * (M_PI / 180);
		//iResult = iValue pluse cosine 45 degree
	iResult = fValue * cos(Radian);
	
	//float iResult = iValue * 0.525322;
	str.Format(_T("%0.3f"), iResult);
	SetDlgItemText(IDD_TAB_SYS_X_OFFSET, str);
	SetDlgItemText(IDD_TAB_SYS_Y_OFFSET, str);

// // 檢查父視窗指標有效性
	//CWnd* pWnd = GetParent();
	CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
	
		if (pParentWnd)
		{
			pParentWnd->m_SystemPara.OffsetValue = fValue;
			//pParentWnd->m_SystemPara.OffsetX = iResult;
			//pParentWnd->m_SystemPara.OffsetY = iResult;
		}
	
    
	
}

void SystemParaTab::OnOK()
{
}


void SystemParaTab::OnBnClickedMfcbtnSaveSystem()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	std::string appPath;
	// Get the application path
	appPath = GetAppPath();

	//Set System configuration file name add app path
	CString strConfigFile = _T("SystemConfig.ini");
	// 修正 appPath 與 strConfigFile 的串接方式
	strConfigFile = CString(appPath.c_str()) + _T("\\") + strConfigFile;

	//YUFADlg 的 m_SystemPara 資料成員寫入到系統配置檔案
	CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(GetParent()->GetParent());

	// CString 轉 std::string
	CT2A pszConverted(strConfigFile);
	std::string stdConfigFile(pszConverted);

	//Call UAX :  SystemConfig WriteConfigToFile(const std::string& filename,  SystemConfig &SysConfig)
	WriteConfigToFile_SP(stdConfigFile, pParentWnd->m_SystemPara);
}

//Update data in Edit control with SystemConfig m_SystemPara
void SystemParaTab::UpdateControl()
{
	CSPDlg* pParentWnd = dynamic_cast<CSPDlg*>(GetParent()->GetParent());
	if (pParentWnd != nullptr)
	{
		CEdit* pSystemConfigEdit = reinterpret_cast<CEdit*>(GetDlgItem(IDC_EDIT_SYSTEM_CONFIG_LIVE));
		CEdit* pMemStructEdit = reinterpret_cast<CEdit*>(GetDlgItem(IDC_EDIT_MEMSTRUCT_LIVE));
		const int systemConfigFirstVisibleLine = pSystemConfigEdit ? pSystemConfigEdit->GetFirstVisibleLine() : 0;
		const int memStructFirstVisibleLine = pMemStructEdit ? pMemStructEdit->GetFirstVisibleLine() : 0;

		CString str;
		
	//	str.Format(_T("%0.4f"), pParentWnd->m_SystemPara.OffsetX);
	//	SetDlgItemText(IDD_TAB_SYS_X_OFFSET, str);
	//	str.Format(_T("%0.4f"), pParentWnd->m_SystemPara.OffsetY);
	//	SetDlgItemText(IDD_TAB_SYS_Y_OFFSET, str);

		//OffsetX and OffsetY compound value, set to IDD_TAB_SYS_OFFSET_VALUE
		//Square root of (OffsetX^2 + OffsetY^2)
	//	double offsetX = pParentWnd->m_SystemPara.OffsetX;
	//	double offsetY = pParentWnd->m_SystemPara.OffsetY;
		double offsetValue = pParentWnd->m_SystemPara.OffsetValue;
		str.Format(_T("%0.3f"), offsetValue);
		SetDlgItemText(IDD_TAB_SYS_OFFSET_VALUE, str);

		// MACKey / GoldenKey 為 char[17]，轉為 CString 顯示
		CString macKey(pParentWnd->m_SystemPara.MACKey);
		CString goldenKey(pParentWnd->m_SystemPara.GoldenKey);
		CString hmiId(pParentWnd->m_SystemPara.HMI_ID);
		CString plcId(pParentWnd->m_SystemPara.PLC_ID);
		if (hmiId.IsEmpty()) {
			hmiId = _T("N/A");
		}
		if (plcId.IsEmpty()) {
			plcId = _T("N/A");
		}

		//Fill in struct SystemConfig components to IDC_EDIT_SYSTEM_DATA
		// 格式化資料以顯示在 IDC_EDIT_SYSTEM_DATA 控制項中
        CString displayText;
		displayText.Format(_T("Modbus TCP 配置:\r\n")
			_T("IP 地址: %s\r\n")
			_T("端口: %d\r\n")
			_T("站點 ID: %d\r\n\r\n")
			_T("工具路徑配置:\r\n")
			_T("偏移值: %.4f\r\n\r\n")
			_T("相機配置:\r\n")
			_T("相機 ID: %d\r\n")
			_T("MAC 位址: %s\r\n")
			_T("解密金鑰: %s\r\n")
			_T("HMI ID: %s\r\n")
			_T("PLC ID: %s\r\n")
			_T("相機寬度: %d\r\n")
			_T("相機高度: %d\r\n")
			_T("轉換因子: %.4f\r\n")
			_T("影像方向: %d\r\n\r\n")
			_T("遮罩配置:\r\n")
			_T("遮罩 X: %d\r\n")
			_T("遮罩 Y: %d\r\n")
			_T("遮罩高度: %d\r\n")
			_T("遮罩寬度: %d\r\n\r\n")
			_T("機器配置:\r\n")
			_T("機器類型: %s"),
			CString(pParentWnd->m_SystemPara.IpAddress.c_str()),
			pParentWnd->m_SystemPara.Port,
			pParentWnd->m_SystemPara.StationID,
			pParentWnd->m_SystemPara.OffsetValue,
			pParentWnd->m_SystemPara.CameraID,
			macKey,
			goldenKey,
			hmiId,
			plcId,
			pParentWnd->m_SystemPara.CameraWidth,
			pParentWnd->m_SystemPara.CameraHeight,
			pParentWnd->m_SystemPara.TransferFactor,
			pParentWnd->m_SystemPara.ImageFlip,
			pParentWnd->m_SystemPara.MaskX,
			pParentWnd->m_SystemPara.MaskY,
			pParentWnd->m_SystemPara.MaskHeight,
			pParentWnd->m_SystemPara.MaskWidth,
			CString(pParentWnd->m_SystemPara.MachineType.c_str())
			);

		// 將格式化後的文字設定到 IDC_EDIT_SYSTEM_DATA 控制項中
		SetDlgItemText(IDC_EDIT_SYSTEM_DATA, displayText);

		CString liveSystemConfig;
		liveSystemConfig.Format(
			_T("ImageBinary: %d\r\n")
			_T("CreateToolPath: %d\r\n")
			_T("DispalyToolPath: %d\r\n")
			_T("DisplayROI: %d\r\n")
			_T("DisplayRefLine: %d\r\n")
			_T("TabWork: %d\r\n")
			_T("OffsetValue: %.3f\r\n")
			_T("BinaryUpper: %d\r\n")
			_T("BinaryLower: %d\r\n")
			_T("MaskX/Y/W/H: %d / %d / %d / %d\r\n")
			_T("StationID: %d\r\n")
			_T("CameraID: %d\r\n")
			_T("HMI_ID: %s\r\n")
			_T("PLC_ID: %s\r\n")
			_T("RefCenterX/Y: %d / %d\r\n")
			_T("ImageFlip: %d"),
			pParentWnd->m_SystemPara.ImageBinary,
			pParentWnd->m_SystemPara.CreateToolPath,
			pParentWnd->m_SystemPara.DispalyToolPath,
			pParentWnd->m_SystemPara.DisplayROI,
			pParentWnd->m_SystemPara.DisplayRefLine,
			pParentWnd->m_SystemPara.TabWork,
			pParentWnd->m_SystemPara.OffsetValue,
			pParentWnd->m_SystemPara.BinaryUpper,
			pParentWnd->m_SystemPara.BinaryLower,
			pParentWnd->m_SystemPara.MaskX,
			pParentWnd->m_SystemPara.MaskY,
			pParentWnd->m_SystemPara.MaskWidth,
			pParentWnd->m_SystemPara.MaskHeight,
			pParentWnd->m_SystemPara.StationID,
			pParentWnd->m_SystemPara.CameraID,
			hmiId,
			plcId,
			pParentWnd->m_SystemPara.RefCenterX,
			pParentWnd->m_SystemPara.RefCenterY,
			pParentWnd->m_SystemPara.ImageFlip);
		SetDlgItemText(IDC_EDIT_SYSTEM_CONFIG_LIVE, liveSystemConfig);

		CString liveMemStruct;
		liveMemStruct.Format(
			_T("RecipeID: %d\r\n")
			_T("CurrentProduction: %d\r\n")
			_T("SetTemp0/Temp0: %d / %d\r\n")
			_T("SetTemp1/Temp1: %d / %d\r\n")
			_T("SetTemp2/Temp2: %d / %d\r\n")
			_T("Servo ALE: %d, %d, %d, %d\r\n")
			_T("Process/System Time: %d / %d\r\n")
			_T("MachineID/Model: %d / %d\r\n")
			_T("Alm tem not reach: %u\r\n")
			_T("AL overload: %u\r\n")
			_T("Air pressure low: %u\r\n")
			_T("AL emergency: %u\r\n")
			_T("Midside sensor: %u\r\n")
			_T("ManualY GoOut: %u\r\n")
			_T("MachineStatus: %u\r\n")
			_T("WorkingMode: %u\r\n")
			_T("p19: %.4f"),
			pParentWnd->m_MemStruct_SP.RecipeID,
			pParentWnd->m_MemStruct_SP.CurrentProduction,
			pParentWnd->m_MemStruct_SP.Set_temperature0,
			pParentWnd->m_MemStruct_SP.Temperature0,
			pParentWnd->m_MemStruct_SP.Set_Temperature1,
			pParentWnd->m_MemStruct_SP.Temperature1,
			pParentWnd->m_MemStruct_SP.Set_temperature2,
			pParentWnd->m_MemStruct_SP.Temperature2,
			pParentWnd->m_MemStruct_SP.Servo_ALE0,
			pParentWnd->m_MemStruct_SP.Servo_ALE1,
			pParentWnd->m_MemStruct_SP.Servo_ALE2,
			pParentWnd->m_MemStruct_SP.Servo_ALE3,
			pParentWnd->m_MemStruct_SP.i_ProcessingTimeCount,
			pParentWnd->m_MemStruct_SP.i_SystemTimeCount,
			pParentWnd->m_MemStruct_SP.MachineID,
			pParentWnd->m_MemStruct_SP.MachineModel,
			static_cast<unsigned>(pParentWnd->m_MemStruct_SP.Alm_tem_not_reach),
			static_cast<unsigned>(pParentWnd->m_MemStruct_SP.flag_AL_overload),
			static_cast<unsigned>(pParentWnd->m_MemStruct_SP.Alm_airPressureLow),
			static_cast<unsigned>(pParentWnd->m_MemStruct_SP.flag_AL_emergency),
			static_cast<unsigned>(pParentWnd->m_MemStruct_SP.flag_AL_midside_sensor),
			static_cast<unsigned>(pParentWnd->m_MemStruct_SP.Alm_ManualY_GoOut),
			static_cast<unsigned>(pParentWnd->m_MemStruct_SP.MachineStatus),
			static_cast<unsigned>(pParentWnd->m_MemStruct_SP.WorkingMode),
			pParentWnd->m_MemStruct_SP.p19);
		SetDlgItemText(IDC_EDIT_MEMSTRUCT_LIVE, liveMemStruct);

		if (pSystemConfigEdit) {
			pSystemConfigEdit->LineScroll(systemConfigFirstVisibleLine - pSystemConfigEdit->GetFirstVisibleLine());
		}

		if (pMemStructEdit) {
			pMemStructEdit->LineScroll(memStructFirstVisibleLine - pMemStructEdit->GetFirstVisibleLine());
		}
	}
}

