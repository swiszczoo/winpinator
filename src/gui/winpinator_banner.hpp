#pragma once
#include <wx/wx.h>

#include <memory>

namespace gui
{

class WinpinatorBanner : public wxPanel
{
public:
    WinpinatorBanner( wxWindow* parent, int height );

private:
    static wxColour s_gradient1;
    static wxColour s_gradient2;

    int m_height;
    std::unique_ptr<wxFont> m_headerFont;
    wxBitmap m_logo;

    void onDpiChanged( wxDPIChangedEvent& event );
    void onDraw( wxPaintEvent& event );
    void onSize( wxSizeEvent& event );
    void loadResources();

    void draw( wxDC& dc );
};

};
