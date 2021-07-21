#pragma once
#include "../service/service_observer.hpp"
#include "page_hostlist.hpp"
#include "page_nohosts.hpp"
#include "page_offline.hpp"
#include "page_starting.hpp"

#include <wx/simplebook.h>
#include <wx/wx.h>

namespace gui
{

class ScreenSelector : public wxPanel, srv::IServiceObserver
{
public:
    explicit ScreenSelector( wxWindow* parent );

    // Service listeners

    virtual void onStateChanged();

private:
    enum class SelectorPage
    {
        STARTING,
        OFFLINE,
        HOST_LIST,
        NO_HOSTS
    };

    wxSimplebook* m_book;
    SelectorPage m_currentPage;

    StartingPage* m_page0;
    OfflinePage* m_page1;
    HostListPage* m_page2;
    NoHostsPage* m_page3;

    void onChangePage( wxThreadEvent& event );
    void onNoHostsInTime( wxCommandEvent& event );
    void onRetryClicked( wxCommandEvent& event );

    virtual void onHostCountChanged( size_t newCount ) override;

    void changePage( SelectorPage newPage );
};

};
