// SystemParaTab.cpp: 實作檔案
//

#include "pch.h"
#include "YUFA.h"
#include "afxdialogex.h"
#include "SystemParaTab.h"


// SystemParaTab 對話方塊

IMPLEMENT_DYNAMIC(SystemParaTab, CDialogEx)

SystemParaTab::SystemParaTab(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TAB_SYSTEM_PARA, pParent)
{

}

SystemParaTab::~SystemParaTab()
{
}

void SystemParaTab::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(SystemParaTab, CDialogEx)
	ON_BN_CLICKED(IDC_SYSTEM_CREATE_DATA, &SystemParaTab::OnBnClickedSystemCreateData)
END_MESSAGE_MAP()


// SystemParaTab 訊息處理常式


void SystemParaTab::OnBnClickedSystemCreateData()
{
	// TODO: 在此加入控制項告知處理常式程式碼

}
