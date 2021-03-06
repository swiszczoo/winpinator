#pragma once
#include "../service/service_observer.hpp"
#include "page_error.hpp"
#include "page_hostlist.hpp"
#include "page_nohosts.hpp"
#include "page_offline.hpp"
#include "page_starting.hpp"
#include "page_transferlist.hpp"
#include "pointer_event.hpp"

#include <wx/simplebook.h>
#include <wx/wx.h>

#include <vector>

namespace gui
{

wxDECLARE_EVENT( EVT_UPDATE_BANNER_TARGET, PointerEvent );
wxDECLARE_EVENT( EVT_UPDATE_BANNER_QSIZE, wxCommandEvent );

class ScreenSelector : public wxPanel, srv::IServiceObserver
{
public:
    explicit ScreenSelector( wxWindow* parent );

    bool showTransferScreen( const wxString& remoteId );
    void setTransferList( const std::vector<wxString>& paths );

    // Service listeners

    virtual void onStateChanged();

private:
    enum class SelectorPage
    {
        STARTING,
        OFFLINE,
        HOST_LIST,
        NO_HOSTS,
        ERROR_PAGE,
        TRANSFER_LIST
    };

    wxSimplebook* m_book;
    SelectorPage m_currentPage;

    StartingPage* m_page0;
    OfflinePage* m_page1;
    HostListPage* m_page2;
    NoHostsPage* m_page3;
    ErrorPage* m_page4;
    TransferListPage* m_page5;

    std::vector<wxString> m_transferBuffer;

    void onChangePage( wxThreadEvent& event );
    void onNoHostsInTime( wxCommandEvent& event );
    void onTargetSelected( wxCommandEvent& event );
    void onRetryClicked( wxCommandEvent& event );

    virtual void onHostCountChanged( size_t newCount ) override;

    void onTransferBackClicked( wxCommandEvent& event );

    void changePage( SelectorPage newPage );
    void removeTransferListPage();
    void setupTransferListPage( const wxString& targetId, bool useBuffer = false );

    void setupTransferScreenEvents();
};

};
