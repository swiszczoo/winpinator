#pragma once
#include <wx/wx.h>

#include "../service/service_observer.hpp"
#include "status_text.hpp"
#include "tool_button.hpp"
#include "transfer_history.hpp"

namespace gui
{

wxDECLARE_EVENT( EVT_GO_BACK, wxCommandEvent );

class TransferListPage : public wxPanel, srv::IServiceObserver
{
public:
    explicit TransferListPage( wxWindow* parent, const wxString& targetId );

private:
    wxString m_target;

    wxStaticText* m_header;
    wxStaticText* m_details;
    ToolButton* m_backBtn;
    ToolButton* m_fileBtn;
    ToolButton* m_directoryBtn;
    wxPanel* m_opPanel;
    ScrolledTransferHistory* m_opList;
    StatusText* m_statusLabel;

    wxBitmap m_backBmp;
    wxBitmap m_fileBmp;
    wxBitmap m_dirBmp;

    void loadIcons();
    void loadSingleIcon( const wxString& res, wxBitmap* loc, ToolButton* btn );

    void onDpiChanged( wxDPIChangedEvent& event );
    void onBackClicked( wxCommandEvent& event );
    void onUpdateStatus( wxThreadEvent& event );

    void onStateChanged() override;
    void onEditHost( srv::RemoteInfoPtr newInfo ) override;

    void updateForStatus( srv::RemoteStatus status );
};

};
