#include "progress_label.hpp"

#include "../../win32/resource.h"
#include "utils.hpp"

#include <wx/filename.h>

namespace gui
{

ProgressLabel::ProgressLabel( wxWindow* parent, const wxString& label )
    : wxPanel( parent )
    , m_bitmap( wxNullBitmap )
    , m_stText( nullptr )
    , m_anim( nullptr )
{
    wxBoxSizer* bSizer = new wxBoxSizer( wxHORIZONTAL );

    m_anim = new SpritesheetAnimator( this );
    m_anim->setBitmap( m_bitmap );
    m_anim->SetBackgroundColour( 
        wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
    m_anim->SetSize( FromDIP( wxSize( 24, 24 ) ) );
    m_anim->SetMinSize( FromDIP( wxSize( 24, 24 ) ) );
    m_anim->setIntervalMs( 50 );
    bSizer->Add( m_anim, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4 );

    loadImages();

    m_stText = new wxStaticText( this, wxID_ANY, label );
    m_stText->SetForegroundColour( 
        wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );
    bSizer->Add( m_stText, 0, wxALIGN_CENTER_VERTICAL, 0 );

    SetSizer( bSizer );

    // Events
    Bind( wxEVT_DPI_CHANGED, &ProgressLabel::onDpiChanged, this );
}

void ProgressLabel::setLabel( const wxString& label )
{
    m_stText->SetLabel( label );
}

wxString ProgressLabel::getLabel() const
{
    return m_stText->GetLabel();
}

void ProgressLabel::loadImages()
{
    wxBitmap bmp;
    bmp.LoadFile( Utils::makeIntResource( IDB_SPINNER_LG ),
        wxBITMAP_TYPE_PNG_RESOURCE );

    wxImage img = bmp.ConvertToImage();
    float scale = FromDIP( 24 ) / 48.f;

    int w = scale * bmp.GetWidth();
    int h = scale * bmp.GetHeight();

    m_bitmap = img.Scale( w, h, wxIMAGE_QUALITY_BICUBIC );
    m_anim->setBitmap( m_bitmap );
}

void ProgressLabel::onDpiChanged( wxDPIChangedEvent& event )
{
    m_anim->SetSize( FromDIP( wxSize( 24, 24 ) ) );
    m_anim->SetMinSize( FromDIP( wxSize( 24, 24 ) ) );

    loadImages();
}

};