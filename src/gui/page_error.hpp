#pragma once
#include "../service/service_errors.hpp"

#include <wx/wx.h>

namespace gui
{

wxDECLARE_EVENT( EVT_OPEN_SETTINGS, wxCommandEvent );

class ErrorPage : public wxPanel
{
public:
    explicit ErrorPage( wxWindow* parent );

    void setServiceError( srv::ServiceError err );
    srv::ServiceError getServiceError() const;

private:
    static const wxString TEXT;

    srv::ServiceError m_srvError;

    wxStaticBitmap* m_icon;
    wxStaticText* m_label;
    wxTextCtrl* m_description;
    wxButton* m_retryBtn;
    wxButton* m_settingsBtn;
    wxBitmap m_iconBmp;

    void onDpiChanged( wxDPIChangedEvent& event );
    void onRetryClicked( wxCommandEvent& event );
    void onSettingsClicked( wxCommandEvent& event );

    void loadIcon();
    void updateLabel();
};

};
