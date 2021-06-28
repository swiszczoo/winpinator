#include "page_starting.h"

#include "utils.h"

namespace gui
{

StartingPage::StartingPage( wxWindow* parent )
    : wxPanel( parent, wxID_ANY )
    , m_indicator( nullptr )
    , m_label( nullptr )
{
    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

    mainSizer->AddStretchSpacer();

    m_indicator = new wxActivityIndicator( this, wxID_ANY, wxDefaultPosition,
        FromDIP( wxSize( 64, 64 ) ) );
    m_indicator->Start();
    mainSizer->Add( m_indicator, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 8 );

    m_label = new wxStaticText( this, wxID_ANY, _( "Starting Winpinator..." ));
    wxFont labelFont = m_label->GetFont();
    labelFont.MakeLarger();
    labelFont.MakeLarger();
    m_label->SetFont( labelFont );
    mainSizer->Add( m_label, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 8 );

    mainSizer->AddStretchSpacer();
    
    SetSizer( mainSizer );
}

};
