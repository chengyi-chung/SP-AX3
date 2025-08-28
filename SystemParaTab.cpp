// SystemParaTab.cpp: 實作檔案
//

#include "pch.h"
#include "YUFA.h"
#include "YUFADlg.h"
#include "WorkTab.h"
#include "afxdialogex.h"
#include "SystemParaTab.h"
#include "Resource.h"
#include "UAX.h"

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
END_MESSAGE_MAP()


// SystemParaTab 訊息處理常式

BOOL SystemParaTab::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString str;
	str.Format(_T("%d"), 10);

	// Replace IDD_TAB_SYS_OFFSET_VALUE with the correct control ID
	SetDlgItemText(IDD_TAB_SYS_OFFSET_VALUE, str);
	str.Format(_T("%0.3f"), 5.253);
	SetDlgItemText(IDD_TAB_SYS_X_OFFSET, str);
	SetDlgItemText(IDD_TAB_SYS_Y_OFFSET, str);

	//pParentWnd = (CYUFADlg*)GetParent();

		// 確保視窗已正確初始化
	if (m_hWnd == NULL)
	{
		return FALSE; // 初始化失敗
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX 屬性頁應傳回 FALSE
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
	int iValue = _ttoi(str);
	float iResult = iValue * 0.525322;
	str.Format(_T("%0.3f"), iResult);
	SetDlgItemText(IDD_TAB_SYS_X_OFFSET, str);
	SetDlgItemText(IDD_TAB_SYS_Y_OFFSET, str);

// // 檢查父視窗指標有效性
	CWnd* pWnd = GetParent();
	if (pWnd && ::IsWindow(pWnd->GetSafeHwnd()))
	{
		CYUFADlg* pParentWnd = dynamic_cast<CYUFADlg*>(pWnd);
		if (pParentWnd)
		{
			pParentWnd->m_SystemPara.OffsetX = iResult;
			pParentWnd->m_SystemPara.OffsetY = iResult;
		}
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
	CYUFADlg* pParentWnd = dynamic_cast<CYUFADlg*>(GetParent()->GetParent());

	// CString 轉 std::string
	CT2A pszConverted(strConfigFile);
	std::string stdConfigFile(pszConverted);

	//Call UAX :  SystemConfig WriteConfigToFile(const std::string& filename,  SystemConfig &SysConfig)
	WriteConfigToFile(stdConfigFile, pParentWnd->m_SystemPara);
}

//Update data in Edit control with SystemConfig m_SystemPara
void SystemParaTab::UpdateControl()
{
	CYUFADlg* pParentWnd = dynamic_cast<CYUFADlg*>(GetParent()->GetParent());
	if (pParentWnd != nullptr)
	{
		CString str;
		
		str.Format(_T("%0.3f"), pParentWnd->m_SystemPara.OffsetX);
		SetDlgItemText(IDD_TAB_SYS_X_OFFSET, str);
		str.Format(_T("%0.3f"), pParentWnd->m_SystemPara.OffsetY);
		SetDlgItemText(IDD_TAB_SYS_Y_OFFSET, str);

		//OffsetX and OffsetY compound value, set to IDD_TAB_SYS_OFFSET_VALUE
		//Square root of (OffsetX^2 + OffsetY^2)
		double offsetX = pParentWnd->m_SystemPara.OffsetX;
		double offsetY = pParentWnd->m_SystemPara.OffsetY;
		double offsetValue = sqrt(offsetX * offsetX + offsetY * offsetY);
		str.Format(_T("%0.3f"), offsetValue);
		SetDlgItemText(IDD_TAB_SYS_OFFSET_VALUE, str);

		//Fill in struct SystemConfig components to IDC_EDIT_SYSTEM_DATA
		// 格式化資料以顯示在 IDC_EDIT_SYSTEM_DATA 控制項中
		CString displayText;
		displayText.Format(_T("Modbus TCP 配置:\r\n")
			_T("IP 地址: %s\r\n")
			_T("端口: %d\r\n")
			_T("站點 ID: %d\r\n\r\n")
			_T("工具路徑配置:\r\n")
			_T("偏移 X: %.2f\r\n")
			_T("偏移 Y: %.2f\r\n\r\n")
			_T("相機配置:\r\n")
			_T("相機 ID: %d\r\n")
			_T("MAC 位址: %s\r\n")
			_T("解密金鑰: %s\r\n")
			_T("相機寬度: %d\r\n")
			_T("相機高度: %d\r\n")
			_T("轉換因子: %.4f\r\n\r\n")
			_T("影像方向 :%d\r\n\r\n")
			_T("機器配置:\r\n")
			_T("機器類型: %s\r\n")
			_T("點動速度: %d\r\n")
			_T("自動速度: %d\r\n")
			_T("減速加速度: %d\r\n")
			_T("加速加速度: %d\r\n")
			_T("螺距: %.2f"),
			CString(pParentWnd->m_SystemPara.IpAddress.c_str()),
			pParentWnd->m_SystemPara.Port,
			pParentWnd->m_SystemPara.StationID,
			pParentWnd->m_SystemPara.OffsetX,
			pParentWnd->m_SystemPara.OffsetY,
			pParentWnd->m_SystemPara.CameraID,

			
			pParentWnd->m_SystemPara.CameraWidth,
			pParentWnd->m_SystemPara.CameraHeight,
			pParentWnd->m_SystemPara.TransferFactor,
			pParentWnd->m_SystemPara.ImageFlip,
			CString(pParentWnd->m_SystemPara.MachineType.c_str()),
			pParentWnd->m_SystemPara.JogVelocity,
			pParentWnd->m_SystemPara.AutoVelocity,
			pParentWnd->m_SystemPara.DecAcceleration,
			pParentWnd->m_SystemPara.IncAcceleration,
			pParentWnd->m_SystemPara.Pitch);

		// 將格式化後的文字設定到 IDC_EDIT_SYSTEM_DATA 控制項中
		SetDlgItemText(IDC_EDIT_SYSTEM_DATA, displayText);

	}
}

