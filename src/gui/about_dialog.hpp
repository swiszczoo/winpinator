#pragma once
#include <wx/notebook.h>
#include <wx/wx.h>

#include <wx/html/htmlwin.h>

namespace gui
{

class AboutDialog : public wxDialog
{
public:
    explicit AboutDialog( wxWindow* parent );

private:
    wxPanel* m_banner;
    wxBitmap m_bannerBmp;
    wxFont m_versionFont;

    void loadBitmap();
    void createAboutPage( wxNotebook* nb );
    void createLicensePage( wxNotebook* nb );

    void onDpiChanged( wxDPIChangedEvent& event );
    void onPaintBitmap( wxPaintEvent& event );
    void onLinkClicked( wxHtmlLinkEvent& event );
};

};
