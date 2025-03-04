// SystemParaTab.cpp: 實作檔案
//

#include "pch.h"
#include "YUFA.h"
#include "YUFADlg.h"
#include "WorkTab.h"
#include "afxdialogex.h"
#include "SystemParaTab.h"
#include "Resource.h"


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

// Set iResult to m_SystemPara.OffsetX, m_SystemPara.OffsetY from class WorkTab
// Get a pointer to the parent property sheet
// 獲取父對話框
    CYUFADlg* pParentWnd = (CYUFADlg*)GetParent();

    pParentWnd->m_SystemPara.OffsetX = iResult;
    pParentWnd->m_SystemPara.OffsetY = iResult;

	//釋放 pParentWnd
	
}

void SystemParaTab::OnOK()
{
}