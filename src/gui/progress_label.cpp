#include "progress_label.hpp"

#include <wx/filename.h>

namespace gui
{

ProgressLabel::ProgressLabel( wxWindow* parent, const wxString& label )
    : wxPanel( parent )
    , m_stText( nullptr )
    , m_anim( nullptr )
{
    wxBoxSizer* bSizer = new wxBoxSizer( wxHORIZONTAL );

    wxString windows;
    if ( wxGetEnv( "WINDIR", &windows ) )
    {
        wxFileName animName( windows, "aero_busy.ani" );
        animName.AppendDir( "Cursors" );

        if ( animName.Exists() )
        {
            m_anim = new wxAnimationCtrl( this, wxID_ANY );
            //m_anim->LoadFile( animName.GetFullPath(), wxANIMATION_TYPE_ANI );
            //bSizer->Add( m_anim, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8 );
        }
    }

    m_stText = new wxStaticText( this, wxID_ANY, label );
    m_stText->SetForegroundColour( 
        wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );
    bSizer->Add( m_stText, 0, wxALIGN_CENTER_VERTICAL, 0 );

    SetSizer( bSizer );
}

void ProgressLabel::setLabel( const wxString& label )
{
    m_stText->SetLabel( label );
}

const wxString& ProgressLabel::getLabel() const
{
    return m_stText->GetLabel();
}

};