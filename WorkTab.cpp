// WorkTab.cpp: 實作檔案
//

#include "pch.h"
#include "YUFA.h"
#include "afxdialogex.h"
#include "WorkTab.h"


// WorkTab 對話方塊

IMPLEMENT_DYNAMIC(WorkTab, CDialogEx)

WorkTab::WorkTab(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TAB_WOK, pParent)
{

}

WorkTab::~WorkTab()
{
}

void WorkTab::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(WorkTab, CDialogEx)
	ON_BN_CLICKED(IDC_WORK_GRAB, &WorkTab::OnBnClickedWorkGrab)
END_MESSAGE_MAP()


// WorkTab 訊息處理常式


void WorkTab::OnBnClickedWorkGrab()
{
	// TODO: 在此加入控制項告知處理常式程式碼

}
