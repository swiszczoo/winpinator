#pragma once
#include <wx/wx.h>

#include "../service/remote_info.hpp"

#include <memory>

namespace gui
{

class WinpinatorBanner : public wxPanel
{
public:
    WinpinatorBanner( wxWindow* parent, int height );

    void setTargetInfo( srv::RemoteInfoPtr targetPtr );
    void setSendQueueSize( int size );
    int getSendQueueSize() const;

private:
    static wxColour s_gradient1;
    static wxColour s_gradient2;

    int m_height;
    std::unique_ptr<wxFont> m_headerFont;
    std::unique_ptr<wxFont> m_detailsFont;
    wxBitmap m_logo;

    bool m_targetMode;
    wxString m_targetName;
    wxString m_targetDetails;
    wxImage m_targetAvatarImg;
    wxBitmap m_targetAvatarBmp;

    int m_queueSize;

    void onDpiChanged( wxDPIChangedEvent& event );
    void onDraw( wxPaintEvent& event );
    void onSize( wxSizeEvent& event );
    void loadResources();
    void rescaleTargetAvatar();

    void drawBasic( wxDC& dc );
    void drawTargetInfo( wxDC& dc );
};

};
