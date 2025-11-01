#pragma once
#include "afxwin.h"
#include "resource.h"

class ImagePro : public CDialogEx
{
    DECLARE_DYNAMIC(ImagePro)

public:
    ImagePro(CWnd* pParent = nullptr);
    virtual ~ImagePro();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_DLG_IMAGE_PRO }; // 關聯對話方塊資源 ID
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;

    DECLARE_MESSAGE_MAP()
};