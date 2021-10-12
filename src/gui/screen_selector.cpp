#include "screen_selector.hpp"

#include "../globals.hpp"

namespace gui
{

wxDEFINE_EVENT( EVT_UPDATE_BANNER_TARGET, wxCommandEvent );

ScreenSelector::ScreenSelector( wxWindow* parent )
    : srv::IServiceObserver()
    , wxPanel( parent, wxID_ANY )
    , m_book( nullptr )
    , m_currentPage( SelectorPage::STARTING )
    , m_page0( nullptr )
    , m_page1( nullptr )
    , m_page2( nullptr )
    , m_page3( nullptr )
    , m_page4( nullptr )
    , m_page5( nullptr )
{
    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

    m_book = new wxSimplebook( this );
    m_book->SetDoubleBuffered( true );
    //m_book->SetEffect( wxShowEffect::wxSHOW_EFFECT_SLIDE_TO_LEFT );
    //m_book->SetEffectTimeout( 300 );
    mainSizer->Add( m_book, 1, wxEXPAND );

    m_page0 = new StartingPage( m_book );
    m_book->AddPage( m_page0, wxEmptyString );
    m_page1 = new OfflinePage( m_book );
    m_book->AddPage( m_page1, wxEmptyString );
    m_page2 = new HostListPage( m_book );
    m_book->AddPage( m_page2, wxEmptyString );
    m_page3 = new NoHostsPage( m_book );
    m_book->AddPage( m_page3, wxEmptyString );
    m_page4 = new ErrorPage( m_book );
    m_book->AddPage( m_page4, wxEmptyString );
    //m_page5 = new TransferListPage( m_book, "" );
    //m_book->AddPage( m_page5, wxEmptyString );

    srv::WinpinatorService* serv = Globals::get()->getWinpinatorServiceInstance();

    m_book->SetSelection( 0 );
    onStateChanged();

    SetSizer( mainSizer );

    observeService( serv );

    // Events
    Bind( wxEVT_THREAD, &ScreenSelector::onChangePage, this );

    m_page2->Bind( EVT_NO_HOSTS_IN_TIME,
        &ScreenSelector::onNoHostsInTime, this );
    m_page2->Bind( EVT_TARGET_SELECTED,
        &ScreenSelector::onTargetSelected, this );

    m_page3->Bind( wxEVT_BUTTON, &ScreenSelector::onRetryClicked, this );
}

void ScreenSelector::onStateChanged()
{
    srv::WinpinatorService* serv = Globals::get()->getWinpinatorServiceInstance();

    if ( serv->hasError() && m_currentPage != SelectorPage::ERROR_PAGE )
    {
        m_currentPage = SelectorPage::ERROR_PAGE;

        wxThreadEvent event( wxEVT_THREAD );
        event.SetInt( (int)SelectorPage::ERROR_PAGE );
        wxQueueEvent( this, event.Clone() );
        return;
    }

    if ( !serv->isOnline() && m_currentPage != SelectorPage::OFFLINE )
    {
        m_currentPage = SelectorPage::OFFLINE;

        wxThreadEvent event( wxEVT_THREAD );
        event.SetInt( (int)SelectorPage::OFFLINE );
        wxQueueEvent( this, event.Clone() );
        return;
    }

    if ( serv->isOnline() && !serv->isServiceReady()
        && m_currentPage != SelectorPage::STARTING )
    {
        m_currentPage = SelectorPage::STARTING;

        wxThreadEvent event( wxEVT_THREAD );
        event.SetInt( (int)SelectorPage::STARTING );
        wxQueueEvent( this, event.Clone() );
        return;
    }

    if ( serv->isServiceReady() && m_currentPage != SelectorPage::HOST_LIST )
    {
        m_currentPage = SelectorPage::HOST_LIST;

        wxThreadEvent event( wxEVT_THREAD );
        event.SetInt( (int)SelectorPage::HOST_LIST );
        wxQueueEvent( this, event.Clone() );
        return;
    }
}

void ScreenSelector::onChangePage( wxThreadEvent& event )
{
    changePage( (SelectorPage)event.GetInt() );
}

void ScreenSelector::onNoHostsInTime( wxCommandEvent& event )
{
    if ( m_currentPage == SelectorPage::HOST_LIST )
    {
        changePage( SelectorPage::NO_HOSTS );
    }
}

void ScreenSelector::onTargetSelected( wxCommandEvent& event )
{
    if ( m_currentPage == SelectorPage::HOST_LIST )
    {
        m_page5 = new TransferListPage( m_book, event.GetString() );
        m_book->AddPage( m_page5, wxEmptyString );

        setupTransferScreenEvents();

        auto serv = Globals::get()->getWinpinatorServiceInstance();

        srv::RemoteInfoPtr infoObj = serv->getRemoteManager()
                                         ->getRemoteInfo( event.GetString() );
        wxCommandEvent bannerEvent( EVT_UPDATE_BANNER_TARGET );
        // FIXME: this shared_ptr may no longer exist!
        bannerEvent.SetClientData( &infoObj ); 
        wxPostEvent( this, bannerEvent );

        changePage( SelectorPage::TRANSFER_LIST );
    }
}

void ScreenSelector::onRetryClicked( wxCommandEvent& event )
{
    changePage( SelectorPage::HOST_LIST );

    m_page2->refreshAll();
}

void ScreenSelector::onHostCountChanged( size_t newCount )
{
    if ( newCount > 0 && m_currentPage == SelectorPage::NO_HOSTS )
    {
        m_currentPage = SelectorPage::HOST_LIST;

        wxThreadEvent event( wxEVT_THREAD );
        event.SetInt( (int)SelectorPage::HOST_LIST );
        wxQueueEvent( this, event.Clone() );
    }
}

void ScreenSelector::onTransferBackClicked( wxCommandEvent& event )
{
    changePage( SelectorPage::HOST_LIST );
}

void ScreenSelector::changePage( SelectorPage page )
{
    //m_page0->Disable();
    //m_page1->Disable();
    m_page2->Disable();
    m_page3->Disable();
    m_page4->Disable();
    if ( m_page5 )
    {
        m_page5->Disable();
    }

    m_currentPage = page;
    m_book->SetSelection( (int)page );

    // Remove transfer list page
    if ( page != SelectorPage::TRANSFER_LIST && m_page5 )
    {
        m_book->DeletePage( (int)SelectorPage::TRANSFER_LIST );
        m_page5 = nullptr;

        wxCommandEvent bannerEvent( EVT_UPDATE_BANNER_TARGET );
        bannerEvent.SetClientData( nullptr );
        wxPostEvent( this, bannerEvent );
    }

    if ( page == SelectorPage::NO_HOSTS )
    {
        m_page0->Enable();
        m_page0->Refresh();
    }

    if ( page == SelectorPage::STARTING )
    {
        m_page1->Enable();
        m_page1->Refresh();
    }

    if ( page == SelectorPage::HOST_LIST )
    {
        m_page2->Enable();
        m_page2->Refresh();
        m_page2->refreshAll();
    }

    if ( page == SelectorPage::NO_HOSTS )
    {
        m_page3->Enable();
        m_page3->Refresh();
    }

    if ( page == SelectorPage::ERROR_PAGE )
    {
        auto serv = Globals::get()->getWinpinatorServiceInstance();

        m_page4->Enable();
        m_page4->setServiceError( serv->getError() );
        m_page4->Refresh();
    }

    if ( page == SelectorPage::TRANSFER_LIST )
    {
        m_page5->Enable();
        m_page5->Refresh();
    }
}

void ScreenSelector::setupTransferScreenEvents()
{
    if ( !m_page5 )
    {
        return;
    }

    m_page5->Bind( EVT_GO_BACK, &ScreenSelector::onTransferBackClicked, this );
}

};
