#include "transfer_history.hpp"

#include "../service/database_types.hpp"
#include "../service/database_utils.hpp"
#include "../globals.hpp"
#include "history_element_pending.hpp"
#include "history_element_finished.hpp"

#include "../../win32/resource.h"
#include "utils.hpp"

#include <wx/translation.h>

namespace gui
{

const std::vector<wxString> ScrolledTransferHistory::TIME_SPECS = {
    wxTRANSLATE( "Soon in the future" ),
    wxTRANSLATE( "Today" ),
    wxTRANSLATE( "Yesterday" ),
    wxTRANSLATE( "Earlier this week" ),
    wxTRANSLATE( "Last week" ),
    wxTRANSLATE( "Earlier this month" ),
    wxTRANSLATE( "Last month" ),
    wxTRANSLATE( "Earlier this year" ),
    wxTRANSLATE( "Last year" ),
    wxTRANSLATE( "A long time ago" )
};

ScrolledTransferHistory::ScrolledTransferHistory( wxWindow* parent,
    const wxString& targetId )
    : wxScrolledWindow( parent, wxID_ANY )
    , m_emptyLabel( nullptr )
    , m_stdBitmaps()
    , m_targetId( targetId )
{
    SetWindowStyle( wxBORDER_NONE | wxVSCROLL );
    SetScrollRate( 0, FromDIP( 15 ) );

    m_stdBitmaps.separatorPen = wxPen( wxColour( 220, 220, 220 ), 1 );

    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    m_emptyLabel = new wxStaticText( this, wxID_ANY, _( "There is no transfer "
        "history for this device..." ) );
    m_emptyLabel->SetForegroundColour( 
        wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );

    sizer->Add( m_emptyLabel, 0, 
        wxALIGN_CENTER_HORIZONTAL | wxALL, FromDIP( 12 ) );
    
    sizer->AddSpacer( FromDIP( 5 ) );

    // "In progress" ops view

    m_pendingGroup.header = new HistoryGroupHeader( this, _( "In progress" ) );
    sizer->Add( m_pendingGroup.header, 0, 
        wxEXPAND | wxLEFT | wxRIGHT, FromDIP( 5 ) );

    m_pendingGroup.panel = new wxPanel( this );
    m_pendingGroup.sizer = new wxBoxSizer( wxVERTICAL );
    m_pendingGroup.panel->SetSizer( m_pendingGroup.sizer );

    sizer->Add( m_pendingGroup.panel, 0, wxEXPAND | wxBOTTOM, FromDIP( 5 ) );
    registerHistoryItem( m_pendingGroup.header );

    HistoryPendingElement* test = new HistoryPendingElement( 
        m_pendingGroup.panel, &m_stdBitmaps );
    HistoryPendingData testData;
    testData.filePaths.push_back( "C:\\Users\\test\\Documents\\abc.html" );
    testData.numFiles = 1;
    testData.numFolders = 0;
    testData.opStartTime = 1631240663;
    testData.opState = HistoryPendingState::TRANSFER_RUNNING;
    testData.outcoming = true;
    testData.sentBytes = 34683;
    testData.singleElementName = "abc.html";
    testData.totalSizeBytes = 65536;
    test->setData( testData );
    test->setIsLast( true );
    test->setPeerName( "John Smith" );
    m_pendingGroup.sizer->Add( test, 0, 
        wxEXPAND | wxLEFT | wxRIGHT, FromDIP( 11 ) );
    registerHistoryItem( test );

    // Historical ops view

    for ( const wxString& spec : ScrolledTransferHistory::TIME_SPECS )
    {
        HistoryGroupHeader* header = new HistoryGroupHeader( this, 
            wxGetTranslation( spec ) );
        sizer->Add( header, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP( 5 ) );

        wxPanel* panel = new wxPanel( this );
        wxBoxSizer* panelSizer = new wxBoxSizer( wxVERTICAL );
        panel->SetSizer( panelSizer );

        sizer->Add( panel, 0, wxEXPAND | wxBOTTOM, FromDIP( 2 ) );

        TimeGroup group;
        group.header = header;
        group.panel = panel;
        group.sizer = panelSizer;

        m_timeGroups.push_back( group );

        registerHistoryItem( header );
    }

    SetSizer( sizer );
    sizer->SetSizeHints( this );
    SetDropTarget( new DropTargetImpl( this ) );

    reloadStdBitmaps();
    updateTimeGroups();

    // Events

    Bind( wxEVT_SCROLLWIN_BOTTOM,
        &ScrolledTransferHistory::onScrollWindow, this );
    Bind( wxEVT_SCROLLWIN_LINEDOWN,
        &ScrolledTransferHistory::onScrollWindow, this );
    Bind( wxEVT_SCROLLWIN_LINEUP,
        &ScrolledTransferHistory::onScrollWindow, this );
    Bind( wxEVT_SCROLLWIN_PAGEDOWN,
        &ScrolledTransferHistory::onScrollWindow, this );
    Bind( wxEVT_SCROLLWIN_PAGEUP,
        &ScrolledTransferHistory::onScrollWindow, this );
    Bind( wxEVT_SCROLLWIN_THUMBRELEASE, 
        &ScrolledTransferHistory::onScrollWindow, this );
    Bind( wxEVT_SCROLLWIN_THUMBTRACK,
        &ScrolledTransferHistory::onScrollWindow, this );
    Bind( wxEVT_SCROLLWIN_TOP,
        &ScrolledTransferHistory::onScrollWindow, this );
    Bind( wxEVT_ENTER_WINDOW, &ScrolledTransferHistory::onMouseEnter, this );
    Bind( wxEVT_LEAVE_WINDOW, &ScrolledTransferHistory::onMouseLeave, this );
    Bind( wxEVT_MOTION, &ScrolledTransferHistory::onMouseMotion, this );
    Bind( wxEVT_DPI_CHANGED, &ScrolledTransferHistory::onDpiChanged, this );
}

void ScrolledTransferHistory::registerHistoryItem( HistoryItem* item )
{
    m_historyItems.push_back( item );

    item->Bind( wxEVT_ENTER_WINDOW, 
        &ScrolledTransferHistory::onMouseEnter, this );
    item->Bind( wxEVT_LEAVE_WINDOW, 
        &ScrolledTransferHistory::onMouseLeave, this );
    item->Bind( wxEVT_MOTION, &ScrolledTransferHistory::onMouseMotion, this );
}

void ScrolledTransferHistory::unregisterHistoryItem( HistoryItem* item )
{
    for ( int i = 0; i < m_historyItems.size(); i++ )
    {
        if ( m_historyItems[i] == item )
        {
            m_historyItems.erase( m_historyItems.begin() + i );
            break;
        }
    }

    item->Unbind( wxEVT_ENTER_WINDOW,
        &ScrolledTransferHistory::onMouseEnter, this );
    item->Unbind( wxEVT_LEAVE_WINDOW,
        &ScrolledTransferHistory::onMouseLeave, this );
    item->Unbind( wxEVT_MOTION, 
        &ScrolledTransferHistory::onMouseMotion, this );
}

void ScrolledTransferHistory::refreshAllHistoryItems( bool insideParent )
{
    for ( HistoryItem* item : m_historyItems )
    {
        item->updateHoverState( insideParent );
    }
}

void ScrolledTransferHistory::reloadStdBitmaps()
{
    loadSingleBitmap( IDB_TRANSFER_FILE_X, 
        &m_stdBitmaps.transferFileX, 64 );
    loadSingleBitmap( IDB_TRANSFER_FILE_FILE, 
        &m_stdBitmaps.transferFileFile, 64 );
    loadSingleBitmap( IDB_TRANSFER_DIR_X,
        &m_stdBitmaps.transferDirX, 64 );
    loadSingleBitmap( IDB_TRANSFER_DIR_DIR,
        &m_stdBitmaps.transferDirDir, 64 );
    loadSingleBitmap( IDB_TRANSFER_DIR_FILE,
        &m_stdBitmaps.transferDirFile, 64 );

    loadSingleBitmap( IDB_TRANSFER_UP,
        &m_stdBitmaps.badgeUp, 20 );
    loadSingleBitmap( IDB_TRANSFER_DOWN,
        &m_stdBitmaps.badgeDown, 20 );

    loadSingleBitmap( IDB_TRANSFER_SUCCESS,
        &m_stdBitmaps.statusSuccess, 16 );
    loadSingleBitmap( IDB_TRANSFER_CANCEL,
        &m_stdBitmaps.statusCancelled, 16 );
    loadSingleBitmap( IDB_TRANSFER_ERROR,
        &m_stdBitmaps.statusError, 16 );
}

void ScrolledTransferHistory::loadSingleBitmap( 
    int resId, wxBitmap* dest, int dip )
{
    wxBitmap orig( Utils::makeIntResource( resId ), 
        wxBITMAP_TYPE_PNG_RESOURCE );

    wxImage img = orig.ConvertToImage();

    int targetSize = FromDIP( dip );

    *dest = img.Scale( targetSize, targetSize, wxIMAGE_QUALITY_BICUBIC );
}

void ScrolledTransferHistory::updateTimeGroups()
{
    auto srv = Globals::get()->getWinpinatorServiceInstance();
    srv::DatabaseManager* db = srv->getDb();

    std::time_t tim = time( NULL );
    bool anyElement = false;

    for ( int i = 0; i < m_timeGroups.size(); i++ )
    {
        TimeGroup& group = m_timeGroups[i];

        std::string conditions = srv::DatabaseUtils::getSpecSQLCondition(
            "transfer_timestamp", (srv::TimeSpec)i, tim );
        auto records = db->queryTransfers( false, 
            m_targetId.ToStdWstring(), conditions );

        int lookupIdx = 0;

        for ( int j = 0; j < records.size(); j++ )
        {
            srv::db::Transfer& record = records[j];
            bool processed = false;

            for ( ; lookupIdx < group.currentIds.size(); lookupIdx++ )
            {
                if ( group.currentIds[lookupIdx] == record.id )
                {
                    // Just update
                    group.elements[lookupIdx]->setData( record );
                    processed = true;
                    break;
                }
                
                const srv::db::Transfer& data = group.elements[lookupIdx]->getData();
                
                if ( data.transferTimestamp <= record.transferTimestamp )
                {
                    // We should insert a new record here
                    HistoryFinishedElement* elem = new HistoryFinishedElement( 
                        group.panel, &m_stdBitmaps );
                    elem->setData( record );

                    group.sizer->Insert( lookupIdx, elem, 0,
                        wxEXPAND | wxLEFT | wxRIGHT, FromDIP( 11 ) );
                    group.elements.insert(
                        group.elements.begin() + lookupIdx, elem );
                    group.currentIds.insert(
                        group.currentIds.begin() + lookupIdx, record.id );

                    registerHistoryItem( elem );

                    lookupIdx++;
                    processed = true;
                    break;
                }
                else
                {
                    // We need to remove current element
                    HistoryFinishedElement* elem = group.elements[lookupIdx];

                    unregisterHistoryItem( elem );

                    group.sizer->Remove( lookupIdx );
                    elem->Destroy();

                    group.elements.erase( group.elements.begin() + lookupIdx );
                    group.currentIds.erase( 
                        group.currentIds.begin() + lookupIdx );

                    lookupIdx--;
                }
            }

            if ( !processed )
            {
                // We need to append it to the end of the group
                HistoryFinishedElement* elem = new HistoryFinishedElement(
                    group.panel, &m_stdBitmaps );
                elem->setData( record );

                group.sizer->Add( elem, 0,
                    wxEXPAND | wxLEFT | wxRIGHT, FromDIP( 11 ) );
                group.elements.push_back( elem );
                group.currentIds.push_back( record.id );

                registerHistoryItem( elem );
                lookupIdx++;
            }
        }

        for ( size_t j = 0; j < group.elements.size(); j++ )
        {
            bool last = j == group.elements.size() - 1;

            if ( group.elements[j]->isLast() != last )
            {
                group.elements[j]->setIsLast( last );
            }
        }

        if ( group.elements.empty() )
        {
            if ( group.panel->IsShown() )
            {
                group.header->Hide();
                group.panel->Hide();
            }
        }
        else
        {
            if ( !group.panel->IsShown() )
            {
                group.header->Show();
                group.panel->Show();
            }

            wxString fmt = wxString::Format( "%s (%d)",
                wxGetTranslation( ScrolledTransferHistory::TIME_SPECS[i] ),
                (int)group.elements.size() );

            group.header->SetLabel( fmt );

            anyElement = true;
        }
    }

    // TODO: also ensure that we don't have any pending transfers yet
    if ( anyElement )
    {
        if ( m_emptyLabel->IsShown() )
        {
            m_emptyLabel->Hide();
        }
    }
    else
    {
        if ( !m_emptyLabel->IsShown() )
        {
            m_emptyLabel->Show();
        }
    }
}

void ScrolledTransferHistory::onScrollWindow( wxScrollWinEvent& event )
{
    refreshAllHistoryItems( true );
    event.Skip( true );
}

void ScrolledTransferHistory::onMouseEnter( wxMouseEvent& event )
{
    refreshAllHistoryItems( true );
    event.Skip( true );
}

void ScrolledTransferHistory::onMouseLeave( wxMouseEvent& event )
{
    bool outside = event.GetEventObject() == this;

    refreshAllHistoryItems( !outside );
    event.Skip( true );
}

void ScrolledTransferHistory::onMouseMotion( wxMouseEvent& event )
{
    refreshAllHistoryItems( true );
    event.Skip( true );
}

void ScrolledTransferHistory::onDpiChanged( wxDPIChangedEvent& event )
{
    reloadStdBitmaps();

    for ( HistoryItem* item : m_historyItems )
    {
        item->Refresh();
    }
}

// Drop target implementation

ScrolledTransferHistory::DropTargetImpl::DropTargetImpl( 
    ScrolledTransferHistory* obj )
    : m_instance( obj )
{
}

bool ScrolledTransferHistory::DropTargetImpl::OnDropFiles( wxCoord x, 
    wxCoord y, const wxArrayString& filenames )
{
    return true;
}

};
