#pragma once
#include "../service/service_errors.hpp"

#include <wx/wx.h>

namespace gui
{

class ErrorPage : public wxPanel
{
public:
    explicit ErrorPage( wxWindow* parent );

    void setServiceError( srv::ServiceError err );
    srv::ServiceError getServiceError() const;

private:
    static const wxString s_text;

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
