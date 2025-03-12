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
	ON_BN_CLICKED(IDC_BTN_JOG_X_PLUS, &MachineTab::OnBnClickedBtnJogXPlus)
	ON_BN_CLICKED(IDC_BTN_JOG_X_MINUX, &MachineTab::OnBnClickedBtnJogXMinux)
	ON_BN_CLICKED(IDC_BTN_JOG_Y_PLUS, &MachineTab::OnBnClickedBtnJogYPlus)
	ON_BN_CLICKED(IDC_BTN_JOG_Y_MINUS, &MachineTab::OnBnClickedBtnJogYMinus)
	ON_BN_CLICKED(IDC_BTN_JOG_Z_PLUS, &MachineTab::OnBnClickedBtnJogZPlus)
	ON_BN_CLICKED(IDC_BTN_JOG_Z_MINUS, &MachineTab::OnBnClickedBtnJogZMinus)
	ON_BN_CLICKED(IDC_RADIO_AUTO, &MachineTab::OnBnClickedRadioAuto)
	ON_BN_CLICKED(IDC_CHECK_HOME, &MachineTab::OnBnClickedCheckHome)
	ON_BN_CLICKED(IDC_CHECK_RESET, &MachineTab::OnBnClickedCheckReset)
	ON_BN_CLICKED(IDC_CHECK_AUTO_WORK_START, &MachineTab::OnBnClickedCheckAutoWorkStart)
	ON_BN_CLICKED(IDC_CHECK_AUTO_WORK_STOP, &MachineTab::OnBnClickedCheckAutoWorkStop)
END_MESSAGE_MAP()


// MachineTab 訊息處理常式
//OnInitialDialog
BOOL MachineTab::OnInitDialog()
{
	CDialog::OnInitDialog();
	//set IDC_RADIO_AUTO check
	((CButton*)GetDlgItem(IDC_RADIO_AUTO))->SetCheck(1);
	m_iMachineMode = 1;


	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void MachineTab::OnBnClickedBtnJogXPlus()
{
	// TODO: 在此加入控制項告知處理常式程式碼
}

void MachineTab::OnBnClickedBtnJogXMinux()
{
	// TODO: 在此加入控制項告知處理常式程式碼
}

void MachineTab::OnBnClickedBtnJogYPlus()
{
	// TODO: 在此加入控制項告知處理常式程式碼
}

void MachineTab::OnBnClickedBtnJogYMinus()
{
	// TODO: 在此加入控制項告知處理常式程式碼
}

void MachineTab::OnBnClickedBtnJogZPlus()
{
	// TODO: 在此加入控制項告知處理常式程式碼
}

void MachineTab::OnBnClickedBtnJogZMinus()
{
	// TODO: 在此加入控制項告知處理常式程式碼
}

void MachineTab::OnBnClickedRadioAuto()
{
	// TODO: 在此加入控制項告知處理常式程式碼
}

void MachineTab::OnBnClickedCheckHome()
{
	// TODO: 在此加入控制項告知處理常式程式碼
}

void MachineTab::OnBnClickedCheckReset()
{
	// TODO: 在此加入控制項告知處理常式程式碼
}

void MachineTab::OnBnClickedCheckAutoWorkStart()
{
	// TODO: 在此加入控制項告知處理常式程式碼
}

void MachineTab::OnBnClickedCheckAutoWorkStop()
{
	// TODO: 在此加入控制項告知處理常式程式碼
}
