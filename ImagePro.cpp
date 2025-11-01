#include "pch.h"
#include "ImagePro.h"
#include "afxdialogex.h"
#include "resource.h"

IMPLEMENT_DYNAMIC(ImagePro, CDialogEx)

ImagePro::ImagePro(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_DLG_IMAGE_PRO, pParent) // 使用 Dialog ID 初始化
{
}

ImagePro::~ImagePro()
{
}

void ImagePro::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    // 在此使用 DDX_Control(...) 等做控制項綁定
}

BOOL ImagePro::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 在此初始化控制項或做其他啟始設定

    return TRUE; // 除非將焦點設定到控制項，否則回傳 TRUE
}

BEGIN_MESSAGE_MAP(ImagePro, CDialogEx)
    // ON_BN_CLICKED(...) 等訊息映射放這裡
END_MESSAGE_MAP()