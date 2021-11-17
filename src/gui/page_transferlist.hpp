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

    wxString getTargetId() const;
    void scrollToTop();

private:
    wxString m_target;

    wxStaticText* m_header;
    wxStaticText* m_details;
    ToolButton* m_backBtn;
    ToolButton* m_fileBtn;
    ToolButton* m_directoryBtn;
    ToolButton* m_historyBtn;
    wxPanel* m_opPanel;
    ScrolledTransferHistory* m_opList;
    StatusText* m_statusLabel;

    wxBitmap m_backBmp;
    wxBitmap m_fileBmp;
    wxBitmap m_dirBmp;
    wxBitmap m_historyBmp;

    void loadIcons();
    void loadSingleIcon( const wxString& res, wxBitmap* loc, ToolButton* btn );

    void onDpiChanged( wxDPIChangedEvent& event );
    void onBackClicked( wxCommandEvent& event );
    void onUpdateStatus( wxThreadEvent& event );
    void onSendFileClicked( wxCommandEvent& event );
    void onSendFolderClicked( wxCommandEvent& event );
    void onClearHistoryClicked( wxCommandEvent& event );
    void onUpdateEmptyState( wxCommandEvent& event );

    void onStateChanged() override;
    void onEditHost( srv::RemoteInfoPtr newInfo ) override;

    void updateForStatus( srv::RemoteStatus status );
};

};
