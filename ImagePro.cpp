#include "pch.h"
#include "ImagePro.h"
#include "afxdialogex.h"
#include "resource.h"
#include <algorithm>

#ifndef IDC_IDC_IMAGE_PRO_HEIGHT
#define IDC_IDC_IMAGE_PRO_HEIGHT 11011
#endif

IMPLEMENT_DYNAMIC(ImagePro, CDialogEx)

ImagePro::ImagePro(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_DLG_IMAGE_PRO, pParent)
{
}

ImagePro::~ImagePro()
{
    if (m_displayBitmap.GetSafeHandle() != nullptr) {
        m_displayBitmap.DeleteObject();
    }
}

void ImagePro::OnOK()
{
    CDialogEx::OnOK();
}

void ImagePro::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_IMAGE_PRO_PICTURE_CTL, m_pictureCtl);
    DDX_Control(pDX, IDC_IDC_IMAGE_PRO_SLIDER_H, m_sliderHigh);
    DDX_Control(pDX, IDC_IDC_IMAGE_PRO_SLIDER_L, m_sliderLow);
    DDX_Control(pDX, IDC_IDC_IMAGE_PRO_BINARY_CHK, m_binaryCheck);
}

void ImagePro::SetImage(const cv::Mat& image)
{
    m_image = image.empty() ? cv::Mat() : image.clone();
}

void ImagePro::SetSliderValues(int lowValue, int highValue)
{
    m_sliderLowValue = (std::max)(0, (std::min)(255, lowValue));
    m_sliderHighValue = (std::max)(0, (std::min)(255, highValue));
    if (m_sliderLowValue > m_sliderHighValue) {
        std::swap(m_sliderLowValue, m_sliderHighValue);
    }

    if (::IsWindow(m_sliderHigh.GetSafeHwnd()) && ::IsWindow(m_sliderLow.GetSafeHwnd())) {
        m_sliderHigh.SetRange(0, 255, TRUE);
        m_sliderHigh.SetPageSize(1);
        m_sliderHigh.SetLineSize(1);
        m_sliderHigh.SetPos(m_sliderHighValue);

        m_sliderLow.SetRange(0, 255, TRUE);
        m_sliderLow.SetPageSize(1);
        m_sliderLow.SetLineSize(1);
        m_sliderLow.SetPos(m_sliderLowValue);

        UpdateSliderText();
    }
}

void ImagePro::GetSliderValues(int& lowValue, int& highValue) const
{
    lowValue = m_sliderLowValue;
    highValue = m_sliderHighValue;
}

void ImagePro::SetBinaryPreviewEnabled(bool enabled)
{
    m_binaryPreviewEnabled = enabled;
    if (::IsWindow(m_binaryCheck.GetSafeHwnd())) {
        m_binaryCheck.SetCheck(m_binaryPreviewEnabled ? BST_CHECKED : BST_UNCHECKED);
        RenderImageZoomAll();
    }
}

bool ImagePro::IsBinaryPreviewEnabled() const
{
    return m_binaryPreviewEnabled;
}

BOOL ImagePro::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    m_pictureCtl.ModifyStyle(SS_TYPEMASK, SS_BITMAP);
    m_sliderHigh.SetRange(0, 255, TRUE);
    m_sliderHigh.SetPageSize(1);
    m_sliderHigh.SetLineSize(1);
    m_sliderHigh.SetPos(m_sliderHighValue);
    m_sliderLow.SetRange(0, 255, TRUE);
    m_sliderLow.SetPageSize(1);
    m_sliderLow.SetLineSize(1);
    m_sliderLow.SetPos(m_sliderLowValue);
    m_binaryCheck.SetCheck(m_binaryPreviewEnabled ? BST_CHECKED : BST_UNCHECKED);
    UpdateSliderText();
    RenderImageZoomAll();
    return TRUE;
}

void ImagePro::OnSize(UINT nType, int cx, int cy)
{
    CDialogEx::OnSize(nType, cx, cy);

    if (::IsWindow(m_pictureCtl.GetSafeHwnd())) {
        RenderImageZoomAll();
    }
}

void ImagePro::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);

    if (pScrollBar != nullptr) {
        if (pScrollBar->GetSafeHwnd() == m_sliderHigh.GetSafeHwnd()) {
            m_sliderHighValue = m_sliderHigh.GetPos();
            if (m_sliderHighValue < m_sliderLowValue) {
                m_sliderLowValue = m_sliderHighValue;
                m_sliderLow.SetPos(m_sliderLowValue);
            }
        }
        else if (pScrollBar->GetSafeHwnd() == m_sliderLow.GetSafeHwnd()) {
            m_sliderLowValue = m_sliderLow.GetPos();
            if (m_sliderLowValue > m_sliderHighValue) {
                m_sliderHighValue = m_sliderLowValue;
                m_sliderHigh.SetPos(m_sliderHighValue);
            }
        }
        UpdateSliderText();
        RenderImageZoomAll();
    }
}

void ImagePro::OnEnChangeImageProHeight()
{
    UpdateSliderFromEdit(IDC_IDC_IMAGE_PRO_HEIGHT, true);
}

void ImagePro::OnEnChangeImageProLow()
{
    UpdateSliderFromEdit(IDC_IDC_IMAGE_PRO_LOW, false);
}

void ImagePro::OnBnClickedImageProBinaryChk()
{
    m_binaryPreviewEnabled = (m_binaryCheck.GetCheck() == BST_CHECKED);
    RenderImageZoomAll();
}

void ImagePro::UpdateSliderText()
{
    m_isUpdatingSliderText = true;
    if (::IsWindow(GetSafeHwnd())) {
        SetDlgItemInt(IDC_IDC_IMAGE_PRO_HEIGHT, static_cast<UINT>(m_sliderHighValue), FALSE);
        SetDlgItemInt(IDC_IDC_IMAGE_PRO_LOW, static_cast<UINT>(m_sliderLowValue), FALSE);
    }
    m_isUpdatingSliderText = false;
}

void ImagePro::UpdateSliderFromEdit(UINT editControlId, bool isHighValue)
{
    if (m_isUpdatingSliderText || !::IsWindow(GetSafeHwnd())) {
        return;
    }

    CString text;
    GetDlgItemText(editControlId, text);
    if (text.IsEmpty()) {
        return;
    }

    const int value = (std::max)(0, (std::min)(255, _ttoi(text)));
    if (isHighValue) {
        m_sliderHighValue = value;
        if (m_sliderHighValue < m_sliderLowValue) {
            m_sliderLowValue = m_sliderHighValue;
            m_sliderLow.SetPos(m_sliderLowValue);
        }
        m_sliderHigh.SetPos(m_sliderHighValue);
    }
    else {
        m_sliderLowValue = value;
        if (m_sliderLowValue > m_sliderHighValue) {
            m_sliderHighValue = m_sliderLowValue;
            m_sliderHigh.SetPos(m_sliderHighValue);
        }
        m_sliderLow.SetPos(m_sliderLowValue);
    }

    UpdateSliderText();
    RenderImageZoomAll();
}

void ImagePro::RenderImageZoomAll()
{
    if (m_image.empty() || !::IsWindow(m_pictureCtl.GetSafeHwnd())) {
        return;
    }

    CRect rect;
    m_pictureCtl.GetClientRect(&rect);
    const int targetWidth = rect.Width();
    const int targetHeight = rect.Height();
    if (targetWidth <= 0 || targetHeight <= 0) {
        return;
    }

    cv::Mat src;
    if (m_binaryPreviewEnabled) {
        cv::Mat gray;
        if (m_image.channels() == 1) {
            gray = m_image;
        }
        else if (m_image.channels() == 4) {
            cv::cvtColor(m_image, gray, cv::COLOR_BGRA2GRAY);
        }
        else {
            cv::cvtColor(m_image, gray, cv::COLOR_BGR2GRAY);
        }
        cv::Mat binary;
        cv::inRange(gray, cv::Scalar(m_sliderLowValue), cv::Scalar(m_sliderHighValue), binary);
        cv::cvtColor(binary, src, cv::COLOR_GRAY2BGR);
    }
    else if (m_image.channels() == 1) {
        cv::cvtColor(m_image, src, cv::COLOR_GRAY2BGR);
    }
    else if (m_image.channels() == 4) {
        cv::cvtColor(m_image, src, cv::COLOR_BGRA2BGR);
    }
    else {
        src = m_image;
    }

    const double scaleX = static_cast<double>(targetWidth) / static_cast<double>(src.cols);
    const double scaleY = static_cast<double>(targetHeight) / static_cast<double>(src.rows);
    const double scale = (std::min)(scaleX, scaleY);
    const int drawWidth = (std::max)(1, static_cast<int>(src.cols * scale));
    const int drawHeight = (std::max)(1, static_cast<int>(src.rows * scale));

    cv::Mat resized;
    cv::resize(src, resized, cv::Size(drawWidth, drawHeight), 0, 0, cv::INTER_AREA);

    cv::Mat canvas(targetHeight, targetWidth, CV_8UC3, cv::Scalar(0, 0, 0));
    const int offsetX = (targetWidth - drawWidth) / 2;
    const int offsetY = (targetHeight - drawHeight) / 2;
    resized.copyTo(canvas(cv::Rect(offsetX, offsetY, drawWidth, drawHeight)));

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = canvas.cols;
    bmi.bmiHeader.biHeight = -canvas.rows;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HDC hdc = ::GetDC(m_pictureCtl.GetSafeHwnd());
    HBITMAP hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    ::ReleaseDC(m_pictureCtl.GetSafeHwnd(), hdc);
    if (hBitmap == nullptr || bits == nullptr) {
        return;
    }

    const size_t srcBytesPerLine = static_cast<size_t>(canvas.cols) * 3;
    const size_t dibStride = ((srcBytesPerLine + 3u) / 4u) * 4u;
    memset(bits, 0, dibStride * static_cast<size_t>(canvas.rows));
    for (int y = 0; y < canvas.rows; ++y) {
        memcpy(static_cast<unsigned char*>(bits) + static_cast<size_t>(y) * dibStride,
               canvas.ptr(y),
               srcBytesPerLine);
    }

    HBITMAP hOldBitmap = m_pictureCtl.SetBitmap(hBitmap);
    if (m_displayBitmap.GetSafeHandle() != nullptr) {
        m_displayBitmap.DeleteObject();
    }
    m_displayBitmap.Attach(hBitmap);
    if (hOldBitmap != nullptr) {
        ::DeleteObject(hOldBitmap);
    }
    m_pictureCtl.Invalidate();
    m_pictureCtl.UpdateWindow();
}

BEGIN_MESSAGE_MAP(ImagePro, CDialogEx)
    ON_EN_CHANGE(IDC_IDC_IMAGE_PRO_HEIGHT, &ImagePro::OnEnChangeImageProHeight)
    ON_EN_CHANGE(IDC_IDC_IMAGE_PRO_LOW, &ImagePro::OnEnChangeImageProLow)
    ON_BN_CLICKED(IDC_IDC_IMAGE_PRO_BINARY_CHK, &ImagePro::OnBnClickedImageProBinaryChk)
    ON_WM_HSCROLL()
    ON_WM_SIZE()
END_MESSAGE_MAP()
