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
END_MESSAGE_MAP()


// SystemParaTab 訊息處理常式
