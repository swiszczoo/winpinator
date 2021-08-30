#include "transfer_history.hpp"

#include "utils.hpp"

#include <wx/translation.h>

namespace gui
{

const std::vector<wxString> ScrolledTransferHistory::TIME_SPECS = {
    _( "Today" ),
    _( "Yesterday" ),
    _( "Earlier this week" ),
    _( "Last week" ),
    _( "Earlier this month" ),
    _( "Last month" ),
    _( "Earlier this year" ),
    _( "A long time ago" )
};

ScrolledTransferHistory::ScrolledTransferHistory( wxWindow* parent )
    : wxScrolledWindow( parent, wxID_ANY )
{
    SetWindowStyle( wxBORDER_NONE | wxVSCROLL );
    SetScrollRate( 0, FromDIP( 8 ) );

    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    m_emptyLabel = new wxStaticText( this, wxID_ANY, _( "There is no transfer "
        "history for this device..." ) );
    m_emptyLabel->SetForegroundColour( 
        wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );

    sizer->Add( m_emptyLabel, 0, 
        wxALIGN_CENTER_HORIZONTAL | wxALL, FromDIP( 12 ) );

    for ( const wxString& spec : ScrolledTransferHistory::TIME_SPECS )
    {
        HistoryGroupHeader* header = new HistoryGroupHeader( this, spec );
        sizer->Add( header, 0, wxEXPAND | wxTOP | wxBOTTOM, FromDIP( 3 ) );

        wxPanel* panel = new wxPanel( this );
        wxBoxSizer* panelSizer = new wxBoxSizer( wxVERTICAL );
        panel->SetSizer( panelSizer );

        sizer->Add( panel, 0, wxEXPAND );

        m_timeHeaders.push_back( header );
        m_timeGroups.push_back( panel );
        m_timeSizers.push_back( panelSizer );
    }

    SetSizer( sizer );
    sizer->SetSizeHints( this );
    SetDropTarget( new DropTargetImpl( this ) );

    // Events

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
