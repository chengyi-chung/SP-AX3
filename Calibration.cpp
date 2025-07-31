// Calibration.cpp: 實作檔案
//

#include "pch.h"
#include "YUFA.h"
#include "afxdialogex.h"
#include "Calibration.h"


// Calibration 對話方塊

IMPLEMENT_DYNAMIC(Calibration, CDialogEx)

Calibration::Calibration(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_CALIBRATION, pParent)
{

}

Calibration::~Calibration()
{
}

void Calibration::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(Calibration, CDialogEx)
	ON_BN_CLICKED(IDOK, &Calibration::OnBnClickedOk)
END_MESSAGE_MAP()


// Calibration 訊息處理常式

void Calibration::OnBnClickedOk()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	CDialogEx::OnOK();
}
