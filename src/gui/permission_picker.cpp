#include "permission_picker.hpp"

namespace gui
{

PermissionPicker::PermissionPicker( wxWindow* parent, bool directory )
    : wxPanel( parent, wxID_ANY )
    , m_checkboxes()
    , m_label( wxEmptyString )
    , m_labelCtrl( nullptr )
    , m_labelWidth( 100 )
{
    wxBoxSizer* mainSizer = new wxBoxSizer( wxHORIZONTAL );

    m_offset = mainSizer->AddSpacer( m_labelWidth );
    m_labelCtrl = new wxStaticText( this, wxID_ANY, wxEmptyString );

    wxFont font = m_labelCtrl->GetFont();
    font.MakeBold();
    m_labelCtrl->SetFont( font );

    wxGridSizer* grid = new wxGridSizer( 4, 4, FromDIP( wxSize( 0, 0 ) ) );
    
    for ( int i = 0; i < 9; i++ )
    {
        m_checkboxes[i] = new wxCheckBox( this, wxID_ANY, wxEmptyString );
    }

    wxStaticText* label;
    label = new wxStaticText( this, wxID_ANY, wxEmptyString );
    grid->Add( label, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL );
    label = new wxStaticText( this, wxID_ANY, _( "Read" ) );
    grid->Add( label, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL );
    label = new wxStaticText( this, wxID_ANY, _( "Write" ) );
    grid->Add( label, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL );
    label = new wxStaticText( this, wxID_ANY, _( "Execute" ) );
    grid->Add( label, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL );

    label = new wxStaticText( this, wxID_ANY, _( "Owner" ) );
    grid->Add( label, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL );
    grid->Add( m_checkboxes[0], 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL );
    grid->Add( m_checkboxes[1], 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL );
    grid->Add( m_checkboxes[2], 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL );

    label = new wxStaticText( this, wxID_ANY, _( "Group" ) );
    grid->Add( label, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL );
    grid->Add( m_checkboxes[3], 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL );
    grid->Add( m_checkboxes[4], 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL );
    grid->Add( m_checkboxes[5], 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL );

    label = new wxStaticText( this, wxID_ANY, _( "Others" ) );
    grid->Add( label, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL );
    grid->Add( m_checkboxes[6], 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL );
    grid->Add( m_checkboxes[7], 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL );
    grid->Add( m_checkboxes[8], 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL );

    mainSizer->Add( grid, 1, wxEXPAND );

    SetSizer( mainSizer );
}

void PermissionPicker::setLabel( const wxString& label )
{
    m_label = label;
    m_labelCtrl->SetLabel( label );
}

wxString PermissionPicker::getLabel() const
{
    return m_label;
}

void PermissionPicker::setLabelWidth( int width )
{
    m_labelWidth = width;
    m_offset->SetInitSize( width, width );

    Layout();
}

int PermissionPicker::getLabelWidth() const
{
    return m_labelWidth;
}

void PermissionPicker::setPermissionMask( int mask )
{
    for ( int i = 0; i < 3; i++ )
    {
        int digit = mask % 10;
        int offset = ( 2 - i ) * 3;

        m_checkboxes[offset + 0]->SetValue( digit & 4 );
        m_checkboxes[offset + 1]->SetValue( digit & 2 );
        m_checkboxes[offset + 2]->SetValue( digit & 1 );

        mask /= 10;
    }
}

int PermissionPicker::getPermissionMask() const
{
    int result = 0;

    for (int i = 0; i < 3; i++)
    {
        result *= 10;

        int offset = i * 3;

        int cb1 = m_checkboxes[offset + 0]->IsChecked() ? 1 : 0;
        int cb2 = m_checkboxes[offset + 1]->IsChecked() ? 1 : 0;
        int cb3 = m_checkboxes[offset + 2]->IsChecked() ? 1 : 0;
        
        result += cb1 * 4 + cb2 * 2 + cb3 * 1;
    }

    return result;
}

};
