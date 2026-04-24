#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "resource.h"
#include <opencv2/opencv.hpp>

class ImagePro : public CDialogEx
{
    DECLARE_DYNAMIC(ImagePro)

public:
    ImagePro(CWnd* pParent = nullptr);
    virtual ~ImagePro();
    void SetImage(const cv::Mat& image);
    void SetSliderValues(int lowValue, int highValue);
    virtual void OnOK() override;

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_DLG_IMAGE_PRO };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnEnChangeImageProHeight();
    afx_msg void OnEnChangeImageProLow();
    void RenderImageZoomAll();
    void UpdateSliderText();
    void UpdateSliderFromEdit(UINT editControlId, bool isHighValue);

    DECLARE_MESSAGE_MAP()

private:
    cv::Mat m_image;
    CStatic m_pictureCtl;
    CSliderCtrl m_sliderHigh;
    CSliderCtrl m_sliderLow;
    CBitmap m_displayBitmap;
    int m_sliderLowValue = 0;
    int m_sliderHighValue = 255;
    bool m_isUpdatingSliderText = false;
};
