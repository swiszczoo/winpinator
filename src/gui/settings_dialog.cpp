#include "settings_dialog.hpp"

#include "../main_base.hpp"
#include "utils.hpp"

#include <wx/translation.h>

namespace gui
{

SettingsDialog::SettingsDialog( wxWindow* parent )
    : wxDialog( parent, wxID_ANY, _( "Preferences" ) )
    , m_notebook( nullptr )
{
    SetMinSize( FromDIP( wxSize( 400, 560 ) ) );
    SetSize( FromDIP( wxSize( 400, 560 ) ) );

    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
    sizer->AddSpacer( FromDIP( 10 ) );

    m_notebook = new wxNotebook( this, wxID_ANY );
    sizer->Add( m_notebook, 1, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 8 ) );

    m_panelGeneral = new wxPanel( m_notebook );
    m_panelPermissions = new wxPanel( m_notebook );
    m_panelConnection = new wxPanel( m_notebook );

    m_notebook->AddPage( m_panelGeneral, _( "General" ) );
    m_notebook->AddPage( m_panelPermissions, _( "Unix permissions" ) );
    m_notebook->AddPage( m_panelConnection, _( "Connection" ) );

    sizer->AddSpacer( FromDIP( 10 ) );

    wxSizer* buttons = CreateSeparatedButtonSizer( wxOK | wxCANCEL );
    sizer->Add( buttons, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 8 ) );

    sizer->AddSpacer( FromDIP( 10 ) );

    createGeneralPage();
    createPermissionsPage();
    createConnectionPage();

    loadSettings();

    SetSizer( sizer );
    CenterOnParent();

    // Events
    Bind( wxEVT_BUTTON, &SettingsDialog::onSaveSettings, this, wxID_OK );
}

void SettingsDialog::createGeneralPage()
{
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
    wxFont font = GetFont();
    font.MakeLarger();
    font.MakeLarger();

    sizer->AddSpacer( FromDIP( 10 ) );

    wxStaticText* label;

    label = new wxStaticText( m_panelGeneral, wxID_ANY, _( "Interface language (requires restart to take effect):" ) );
    sizer->Add( label, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    sizer->AddSpacer( FromDIP( 3 ) );

    m_localeName = new wxBitmapComboBox( m_panelGeneral, wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_READONLY );
    fillLocales();
    sizer->Add( m_localeName, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    sizer->AddSpacer( FromDIP( 15 ) );

    label = new wxStaticText( m_panelGeneral, wxID_ANY, _( "Desktop" ) );
    label->SetFont( font );
    label->SetForegroundColour( Utils::get()->getHeaderColor() );
    sizer->Add( label, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    sizer->AddSpacer( FromDIP( 5 ) );

    m_openWindowOnStart = new wxCheckBox( m_panelGeneral, wxID_ANY,
        _( "Start with main window open" ) );
    sizer->Add( m_openWindowOnStart, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    sizer->AddSpacer( FromDIP( 3 ) );

    m_autorun = new wxCheckBox( m_panelGeneral, wxID_ANY,
        _( "Start automatically (on system startup)" ) );
    sizer->Add( m_autorun, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    sizer->AddSpacer( FromDIP( 15 ) );

    label = new wxStaticText( m_panelGeneral, wxID_ANY, _( "File transfers" ) );
    label->SetFont( font );
    label->SetForegroundColour( Utils::get()->getHeaderColor() );
    sizer->Add( label, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    sizer->AddSpacer( FromDIP( 5 ) );

    m_useCompression = new wxCheckBox( m_panelGeneral, wxID_ANY,
        _( "Use compression (if available)" ) );
    sizer->Add( m_useCompression, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    sizer->AddSpacer( FromDIP( 5 ) );

    label = new wxStaticText( m_panelGeneral, wxID_ANY, 
        _( "Compression level:" ) );
    sizer->Add( label, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    m_zlibCompressionLevel = new wxSlider( m_panelGeneral, wxID_ANY, 5,
        1, 9, wxDefaultPosition, wxDefaultSize,
        wxSL_HORIZONTAL | wxSL_AUTOTICKS | wxSL_LABELS );
    sizer->Add( m_zlibCompressionLevel, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    label = new wxStaticText( m_panelGeneral, wxID_ANY, _( "Location for received files:" ) );
    sizer->Add( label, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    sizer->AddSpacer( FromDIP( 3 ) );

    m_outputDir = new wxDirPickerCtrl( m_panelGeneral, wxID_ANY, wxEmptyString );
    sizer->Add( m_outputDir, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    sizer->AddSpacer( FromDIP( 5 ) );

    m_askReceiveFiles = new wxCheckBox( m_panelGeneral, wxID_ANY,
        _( "Require approval before accepting files" ) );
    sizer->Add( m_askReceiveFiles, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    sizer->AddSpacer( FromDIP( 3 ) );

    m_askOverwriteFiles = new wxCheckBox( m_panelGeneral, wxID_ANY,
        _( "Require approval when files would be overwritten" ) );
    sizer->Add( m_askOverwriteFiles, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    m_panelGeneral->SetSizer( sizer );
}

void SettingsDialog::createPermissionsPage()
{
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    sizer->AddSpacer( FromDIP( 10 ) );

    m_filesDefaultPerms = new PermissionPicker( m_panelPermissions, false );
    m_filesDefaultPerms->setLabelWidth( FromDIP( 125 ) );
    m_filesDefaultPerms->setLabel( _( "File permissions:" ) );
    sizer->Add( m_filesDefaultPerms, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    sizer->AddSpacer( FromDIP( 10 ) );

    m_executableDefaultPerms = new PermissionPicker( m_panelPermissions, false );
    m_executableDefaultPerms->setLabelWidth( FromDIP( 125 ) );
    m_executableDefaultPerms->setLabel( _( "Executable permissions:" ) );
    sizer->Add( m_executableDefaultPerms, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    sizer->AddSpacer( FromDIP( 10 ) );

    m_folderDefaultPerms = new PermissionPicker( m_panelPermissions, true );
    m_folderDefaultPerms->setLabelWidth( FromDIP( 125 ) );
    m_folderDefaultPerms->setLabel( _( "Folder permissions:" ) );
    sizer->Add( m_folderDefaultPerms, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    sizer->AddSpacer( FromDIP( 10 ) );
    

    m_panelPermissions->SetSizer( sizer );
}

void SettingsDialog::createConnectionPage()
{
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    wxFont font = GetFont();
    font.MakeLarger();
    font.MakeLarger();

    sizer->AddSpacer( FromDIP( 10 ) );

    wxStaticText* label;
    label = new wxStaticText( m_panelConnection, wxID_ANY, 
        _( "Identification" ) );
    label->SetFont( font );
    label->SetForegroundColour( Utils::get()->getHeaderColor() );
    sizer->Add( label, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    sizer->AddSpacer( FromDIP( 5 ) );

    label = new wxStaticText( m_panelConnection, wxID_ANY, _( "Group code:" ) );
    sizer->Add( label, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    sizer->AddSpacer( FromDIP( 3 ) );

    m_groupCode = new wxTextCtrl( m_panelConnection, wxID_ANY );
    sizer->Add( m_groupCode, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    sizer->AddSpacer( FromDIP( 15 ) );

    label = new wxStaticText( m_panelConnection, wxID_ANY,
        _( "Network" ) );
    label->SetFont( font );
    label->SetForegroundColour( Utils::get()->getHeaderColor() );
    sizer->Add( label, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    sizer->AddSpacer( FromDIP( 5 ) );

    wxBoxSizer* transferSizer = new wxBoxSizer( wxHORIZONTAL );
    wxBoxSizer* registrationSizer = new wxBoxSizer( wxHORIZONTAL );

    label = new wxStaticText( m_panelConnection, wxID_ANY,
        _( "Incoming port for transfers:" ) );
    transferSizer->Add( label, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP( 5 ) );

    m_transferPort = new wxSpinCtrl( m_panelConnection );
    m_transferPort->SetMin( 1 );
    m_transferPort->SetMax( 65535 );
    transferSizer->Add( m_transferPort, 0, wxEXPAND );

    label = new wxStaticText( m_panelConnection, wxID_ANY,
        _( "Incoming port for registration:" ) );
    registrationSizer->Add( label, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP( 5 ) );

    m_registrationPort = new wxSpinCtrl( m_panelConnection );
    m_registrationPort->SetMin( 1 );
    m_registrationPort->SetMax( 65535 );
    registrationSizer->Add( m_registrationPort, 0, wxEXPAND );

    sizer->Add( transferSizer, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );
    sizer->AddSpacer( FromDIP( 3 ) );
    sizer->Add( registrationSizer, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    m_panelConnection->SetSizer( sizer );
}

void SettingsDialog::loadSettings()
{
    SettingsModel& settings = GetApp().m_settings;

    // TODO: locale name
    m_openWindowOnStart->SetValue( settings.openWindowOnStart );
    m_autorun->SetValue( settings.autorun );
    m_useCompression->SetValue( settings.useCompression );
    m_zlibCompressionLevel->SetValue( settings.zlibCompressionLevel );
    m_outputDir->SetPath( settings.outputPath );
    m_askReceiveFiles->SetValue( settings.askReceiveFiles );
    m_askOverwriteFiles->SetValue( settings.askOverwriteFiles );
    m_filesDefaultPerms->setPermissionMask( settings.filesDefaultPermissions );
    m_executableDefaultPerms->setPermissionMask( settings.executablesDefaultPermissions );
    m_folderDefaultPerms->setPermissionMask( settings.foldersDefaultPermissions );
    m_groupCode->SetValue( settings.groupCode );
    m_transferPort->SetValue( settings.transferPort );
    m_registrationPort->SetValue( settings.registrationPort );
}

void SettingsDialog::saveSettings()
{
    SettingsModel& settings = GetApp().m_settings;

    // TODO: locale name
    settings.openWindowOnStart = m_openWindowOnStart->IsChecked();
    settings.autorun = m_autorun->IsChecked();
    settings.useCompression = m_useCompression->IsChecked();
    settings.zlibCompressionLevel = m_zlibCompressionLevel->GetValue();
    settings.outputPath = m_outputDir->GetPath();
    settings.askReceiveFiles = m_askReceiveFiles->IsChecked();
    settings.askOverwriteFiles = m_askOverwriteFiles->IsChecked();
    settings.filesDefaultPermissions = m_filesDefaultPerms->getPermissionMask();
    settings.executablesDefaultPermissions = m_executableDefaultPerms->getPermissionMask();
    settings.foldersDefaultPermissions = m_folderDefaultPerms->getPermissionMask();
    settings.groupCode = m_groupCode->GetValue();
    settings.transferPort = m_transferPort->GetValue();
    settings.registrationPort = m_registrationPort->GetValue();
}

void SettingsDialog::fillLocales()
{
}

void SettingsDialog::onSaveSettings( wxCommandEvent& event )
{
    saveSettings();
    event.Skip();
}

};