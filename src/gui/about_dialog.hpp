#pragma once
#include <wx/notebook.h>
#include <wx/wx.h>

#include <wx/html/htmlwin.h>
#include <wx/richtext/richtextctrl.h>

namespace gui
{

class AboutDialog : public wxDialog
{
public:
    explicit AboutDialog( wxWindow* parent );

private:
    static const wxString ABOUT_XML;

    wxPanel* m_banner;
    wxBitmap m_bannerBmp;
    wxFont m_versionFont;

    void loadBitmap();
    void loadAboutText( wxRichTextCtrl* rt );
    void createAboutPage( wxNotebook* nb );
    void createLicensePage( wxNotebook* nb );

    void onDpiChanged( wxDPIChangedEvent& event );
    void onPaintBitmap( wxPaintEvent& event );
    void onLinkClicked( wxHtmlLinkEvent& event );
};

};
