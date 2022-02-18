#include "settings_dialog.hpp"

#include "../globals.hpp"
#include "../main_base.hpp"
#include "autorun_setter.hpp"
#include "utils.hpp"

#include <wx/stdpaths.h>
#include <wx/translation.h>

#include <wx/msw/uxtheme.h>

namespace gui
{

SettingsDialog::SettingsDialog( wxWindow* parent )
    : wxDialog( parent, wxID_ANY, _( "Preferences" ) )
    , m_notebook( nullptr )
    , m_autorunCommand( wxEmptyString )
{
    SetMinSize( FromDIP( wxSize( 450, 590 ) ) );
    SetSize( FromDIP( wxSize( 450, 590 ) ) );

    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
    sizer->AddSpacer( FromDIP( 10 ) );

    m_notebook = new wxNotebook( this, wxID_ANY );
    sizer->Add( m_notebook, 1, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 8 ) );

    m_panelGeneral = new wxPanel( m_notebook );
    m_panelPermissions = new wxPanel( m_notebook );
    m_panelHistory = new wxPanel( m_notebook );
    m_panelConnection = new wxPanel( m_notebook );

    m_notebook->AddPage( m_panelGeneral, _( "General" ) );
    m_notebook->AddPage( m_panelPermissions, _( "Unix permissions" ) );
    m_notebook->AddPage( m_panelHistory, _( "History" ) );
    m_notebook->AddPage( m_panelConnection, _( "Connection" ) );

    sizer->AddSpacer( FromDIP( 10 ) );

    wxSizer* buttons = CreateSeparatedButtonSizer( wxOK | wxCANCEL );
    sizer->Add( buttons, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 8 ) );

    sizer->AddSpacer( FromDIP( 10 ) );

    createGeneralPage();
    createPermissionsPage();
    createHistoryPage();
    createConnectionPage();

    loadSettings();
    updateState();

    SetSizer( sizer );
    CenterOnParent();

    m_autorunCommand.Printf( "\"%s\" /autorun",
        wxStandardPaths::Get().GetExecutablePath() );

    // Events
    Bind( wxEVT_BUTTON, &SettingsDialog::onSaveSettings, this, wxID_OK );
    m_autorun->Bind( wxEVT_CHECKBOX, &SettingsDialog::onUpdateState, this );
    m_historyList->Bind( wxEVT_LIST_ITEM_SELECTED,
        &SettingsDialog::onHistorySelectionChanged, this );
    m_historyList->Bind( wxEVT_LIST_ITEM_DESELECTED,
        &SettingsDialog::onHistorySelectionChanged, this );
    m_historyRemove->Bind( wxEVT_BUTTON, &SettingsDialog::onHistoryRemove, this );
    m_historyClear->Bind( wxEVT_BUTTON, &SettingsDialog::onHistoryClear, this );
}

void SettingsDialog::createGeneralPage()
{
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    sizer->AddSpacer( FromDIP( 10 ) );

    wxStaticBoxSizer* interfac = new wxStaticBoxSizer(
        wxVERTICAL, m_panelGeneral, _( "Interface" ) );

    interfac->AddSpacer( FromDIP( 5 ) );

    wxStaticText* label;

    label = new wxStaticText( m_panelGeneral, wxID_ANY, _( "Language (requires restart to take effect):" ) );
    interfac->Add( label, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    interfac->AddSpacer( FromDIP( 3 ) );

    m_localeName = new wxBitmapComboBox( m_panelGeneral, wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_READONLY );
    fillLocales();
    interfac->Add( m_localeName, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    interfac->AddSpacer( FromDIP( 10 ) );

    sizer->Add( interfac, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    sizer->AddSpacer( FromDIP( 5 ) );

    wxStaticBoxSizer* desktop = new wxStaticBoxSizer(
        wxVERTICAL, m_panelGeneral, _( "Desktop" ) );

    desktop->AddSpacer( FromDIP( 5 ) );

    m_openWindowOnStart = new wxCheckBox( m_panelGeneral, wxID_ANY,
        _( "Start with main window open" ) );
    desktop->Add( m_openWindowOnStart, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    desktop->AddSpacer( FromDIP( 10 ) );

    m_autorun = new wxCheckBox( m_panelGeneral, wxID_ANY,
        _( "Start automatically (on system startup)" ) );
    desktop->Add( m_autorun, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    desktop->AddSpacer( FromDIP( 3 ) );

    m_autorunHidden = new wxCheckBox( m_panelGeneral, wxID_ANY,
        _( "Do not show main window on system startup" ) );
    desktop->Add( m_autorunHidden, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    desktop->AddSpacer( FromDIP( 10 ) );

    sizer->Add( desktop, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    sizer->AddSpacer( FromDIP( 5 ) );

    wxStaticBoxSizer* transfers = new wxStaticBoxSizer(
        wxVERTICAL, m_panelGeneral, _( "File transfers" ) );

    transfers->AddSpacer( FromDIP( 5 ) );

    m_useCompression = new wxCheckBox( m_panelGeneral, wxID_ANY,
        _( "Use compression (if available)" ) );
    transfers->Add( m_useCompression, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    transfers->AddSpacer( FromDIP( 5 ) );

    label = new wxStaticText( m_panelGeneral, wxID_ANY,
        _( "Compression level:" ) );
    transfers->Add( label, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    m_zlibCompressionLevel = new wxSlider( m_panelGeneral, wxID_ANY, 5,
        1, 9, wxDefaultPosition, wxDefaultSize,
        wxSL_HORIZONTAL | wxSL_AUTOTICKS | wxSL_LABELS );
    transfers->Add( m_zlibCompressionLevel, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    label = new wxStaticText( m_panelGeneral, wxID_ANY, _( "Location for received files:" ) );
    transfers->Add( label, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    transfers->AddSpacer( FromDIP( 3 ) );

    m_outputDir = new wxDirPickerCtrl( m_panelGeneral, wxID_ANY, wxEmptyString );
    transfers->Add( m_outputDir, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    transfers->AddSpacer( FromDIP( 8 ) );

    m_askReceiveFiles = new wxCheckBox( m_panelGeneral, wxID_ANY,
        _( "Require approval before accepting files" ) );
    transfers->Add( m_askReceiveFiles, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    transfers->AddSpacer( FromDIP( 3 ) );

    m_askOverwriteFiles = new wxCheckBox( m_panelGeneral, wxID_ANY,
        _( "Require approval when files would be overwritten" ) );
    transfers->Add( m_askOverwriteFiles, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    transfers->AddSpacer( FromDIP( 3 ) );

    m_preserveZoneInfo = new wxCheckBox( m_panelGeneral, wxID_ANY,
        _( "Preserve zone information in incoming files" ) );
    transfers->Add( m_preserveZoneInfo, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    transfers->AddSpacer( FromDIP( 10 ) );

    sizer->Add( transfers, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    m_panelGeneral->SetSizer( sizer );
}

void SettingsDialog::createPermissionsPage()
{
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    sizer->AddSpacer( FromDIP( 10 ) );

    m_filesDefaultPerms = new PermissionPicker( m_panelPermissions, false );
    m_filesDefaultPerms->setLabelWidth( FromDIP( 150 ) );
    m_filesDefaultPerms->setLabel( _( "File permissions:" ) );
    sizer->Add( m_filesDefaultPerms, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    sizer->AddSpacer( FromDIP( 10 ) );

    m_executableDefaultPerms = new PermissionPicker( m_panelPermissions, false );
    m_executableDefaultPerms->setLabelWidth( FromDIP( 150 ) );
    m_executableDefaultPerms->setLabel( _( "Executable permissions:" ) );
    sizer->Add( m_executableDefaultPerms, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    sizer->AddSpacer( FromDIP( 10 ) );

    m_folderDefaultPerms = new PermissionPicker( m_panelPermissions, true );
    m_folderDefaultPerms->setLabelWidth( FromDIP( 150 ) );
    m_folderDefaultPerms->setLabel( _( "Folder permissions:" ) );
    sizer->Add( m_folderDefaultPerms, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    sizer->AddSpacer( FromDIP( 10 ) );

    m_panelPermissions->SetSizer( sizer );
}

void SettingsDialog::createHistoryPage()
{
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    sizer->AddSpacer( FromDIP( 10 ) );

    wxStaticBoxSizer* history = new wxStaticBoxSizer(
        wxVERTICAL, m_panelHistory, _( "Transfer history" ) );

    history->AddSpacer( FromDIP( 5 ) );

    wxStaticText* label = new wxStaticText( m_panelHistory, wxID_ANY,
        _( "Warning! Every action you perform on this page is irreversible!" ) );
    history->Add( label, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    history->AddSpacer( FromDIP( 10 ) );

    m_historyList = new wxListCtrl( m_panelHistory, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES );
    m_historyList->SetMinSize( FromDIP( wxSize( 0, 170 ) ) );
    SetWindowTheme( m_historyList->GetHWND(), L"Explorer", NULL );

    m_historyList->AppendColumn( _( "Hostname" ), wxLIST_FORMAT_LEFT, 100 );
    m_historyList->AppendColumn( _( "Full name" ), wxLIST_FORMAT_LEFT, 120 );
    m_historyList->AppendColumn( _( "IP address" ), wxLIST_FORMAT_LEFT, 100 );
    m_historyList->AppendColumn( _( "OS" ), wxLIST_FORMAT_LEFT, 70 );
    m_historyList->AppendColumn( _( "Transfers registered" ), wxLIST_FORMAT_LEFT, 100 );

    fillHistoryList();

    history->Add( m_historyList, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    history->AddSpacer( FromDIP( 15 ) );

    wxGridSizer* btnSizer = new wxGridSizer( 1, 2, FromDIP( wxSize( 5, 5 ) ) );
    m_historyRemove = new wxButton( m_panelHistory, wxID_ANY,
        _( "Remove selected" ) );
    m_historyClear = new wxButton( m_panelHistory, wxID_ANY,
        _( "Clear all history" ) );
    btnSizer->Add( m_historyRemove, 1, wxEXPAND );
    btnSizer->Add( m_historyClear, 1, wxEXPAND );

    history->Add( btnSizer, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    history->AddSpacer( FromDIP( 10 ) );

    sizer->Add( history, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    m_panelHistory->SetSizer( sizer );

    updateHistoryButtons();
}

void SettingsDialog::createConnectionPage()
{
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    sizer->AddSpacer( FromDIP( 10 ) );

    wxStaticBoxSizer* ident = new wxStaticBoxSizer(
        wxVERTICAL, m_panelConnection, _( "Identification" ) );

    wxStaticText* label;

    ident->AddSpacer( FromDIP( 5 ) );

    label = new wxStaticText( m_panelConnection, wxID_ANY, _( "Group code:" ) );
    ident->Add( label, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    ident->AddSpacer( FromDIP( 3 ) );

    m_groupCode = new wxTextCtrl( m_panelConnection, wxID_ANY );
    ident->Add( m_groupCode, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    ident->AddSpacer( FromDIP( 10 ) );

    sizer->Add( ident, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    sizer->AddSpacer( FromDIP( 5 ) );

    wxStaticBoxSizer* network = new wxStaticBoxSizer(
        wxVERTICAL, m_panelConnection, _( "Network" ) );

    network->AddSpacer( FromDIP( 5 ) );

    label = new wxStaticText( m_panelConnection, wxID_ANY,
        _( "Network interface to use:" ) );
    network->Add( label, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    network->AddSpacer( FromDIP( 3 ) );

    m_networkInterface = new wxChoice( m_panelConnection, wxID_ANY );
    fillInterfaces();
    network->Add( m_networkInterface, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    network->AddSpacer( FromDIP( 8 ) );

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

    network->Add( transferSizer, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );
    network->AddSpacer( FromDIP( 3 ) );
    network->Add( registrationSizer, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );
    network->AddSpacer( FromDIP( 10 ) );

    sizer->Add( network, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP( 10 ) );

    m_panelConnection->SetSizer( sizer );
}

void SettingsDialog::fillHistoryList()
{
    auto serv = Globals::get()->getWinpinatorServiceInstance();
    m_historyData = serv->getDb()->queryTargets();

    m_historyList->DeleteAllItems();

    int i = 0;
    for ( const auto& record : m_historyData )
    {
        m_historyList->InsertItem( i, record.hostname );
        m_historyList->SetItem( i, 1, record.fullName );
        m_historyList->SetItem( i, 2, record.ip );
        m_historyList->SetItem( i, 3, record.os );
        m_historyList->SetItem( i, 4,
            wxString::Format( "%lld", record.transferCount ) );

        i++;
    }
}

void SettingsDialog::updateHistoryButtons()
{
    if ( m_historyList->GetSelectedItemCount() > 0 )
    {
        m_historyRemove->Enable();
    }
    else
    {
        m_historyRemove->Disable();
    }

    m_historyClear->Enable( m_historyList->GetItemCount() > 0 );
}

void SettingsDialog::loadSettings()
{
    SettingsModel& settings = GetApp().m_settings;

    m_localeName->SetSelection( 0 );

    auto locales = m_langAdapter.getAllLanguages();
    int i = 0;
    for ( auto& locale : locales )
    {
        if ( locale.icuCode == settings.localeName )
        {
            m_localeName->SetSelection( i );
            break;
        }

        i++;
    }

    m_openWindowOnStart->SetValue( settings.openWindowOnStart );
    m_autorun->SetValue( settings.autorun );
    m_autorunHidden->SetValue( settings.autorunHidden );
    m_useCompression->SetValue( settings.useCompression );
    m_zlibCompressionLevel->SetValue( settings.zlibCompressionLevel );
    m_outputDir->SetPath( settings.outputPath );
    m_askReceiveFiles->SetValue( settings.askReceiveFiles );
    m_askOverwriteFiles->SetValue( settings.askOverwriteFiles );
    m_preserveZoneInfo->SetValue( settings.preserveZoneInfo );
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

    settings.localeName = m_langAdapter.getLanguageInfoByIndex(
                                           m_localeName->GetSelection() )
                              .icuCode;
    settings.openWindowOnStart = m_openWindowOnStart->IsChecked();
    settings.autorun = m_autorun->IsChecked();
    settings.autorunHidden = m_autorunHidden->IsChecked();
    settings.useCompression = m_useCompression->IsChecked();
    settings.zlibCompressionLevel = m_zlibCompressionLevel->GetValue();
    settings.outputPath = m_outputDir->GetPath();
    settings.askReceiveFiles = m_askReceiveFiles->IsChecked();
    settings.askOverwriteFiles = m_askOverwriteFiles->IsChecked();
    settings.preserveZoneInfo = m_preserveZoneInfo->IsChecked();
    settings.filesDefaultPermissions = m_filesDefaultPerms->getPermissionMask();
    settings.executablesDefaultPermissions = m_executableDefaultPerms->getPermissionMask();
    settings.foldersDefaultPermissions = m_folderDefaultPerms->getPermissionMask();
    settings.groupCode = m_groupCode->GetValue();
    if ( m_networkInterface->GetSelection() > 0 )
        settings.networkInterface = m_interfaces[m_networkInterface->GetSelection() - 1].name;
    else
        settings.networkInterface = "";
    settings.transferPort = m_transferPort->GetValue();
    settings.registrationPort = m_registrationPort->GetValue();

    // Write autorun options to registry
    AutorunSetter setter( GetApp().GetAppName(), m_autorunCommand );

    if ( settings.autorun != setter.isAutorunEnabled() )
    {
        if ( settings.autorun )
        {
            setter.enableAutorun();
        }
        else
        {
            setter.disableAutorun();
        }
    }
}

void SettingsDialog::fillLocales()
{
    auto languages = m_langAdapter.getAllLanguages();

    for ( auto& language : languages )
    {
        m_loadedBitmaps.push_back( loadScaledFlag( language.flagPath, 16 ) );

        wxBitmap* bmp = m_loadedBitmaps.back().get();
        m_localeName->Append( language.localName, *bmp );
    }
}

void SettingsDialog::fillInterfaces()
{
    SettingsModel& settings = GetApp().m_settings;
    m_interfaces = m_inetAdapter.getAllInterfaces();

    m_networkInterface->AppendString( _( "Automatic" ) );
    m_networkInterface->SetSelection( 0 );

    bool selected = settings.networkInterface.empty();

    for ( auto& interf : m_interfaces )
    {
        m_networkInterface->AppendString( wxString( interf.displayName ) );
        if ( interf.name == settings.networkInterface )
        {
            m_networkInterface->SetSelection( m_networkInterface->GetCount() - 1 );
            selected = true;
        }
    }

    if ( !selected )
    {
        std::string defaultName = m_inetAdapter.getDefaultInterfaceName();

        int i = 1;
        for ( auto& interf : m_interfaces )
        {
            if ( interf.name == defaultName )
            {
                m_networkInterface->SetSelection( i );
                break;
            }
            i++;
        }
    }
}

void SettingsDialog::onSaveSettings( wxCommandEvent& event )
{
    saveSettings();
    writeSettingsToConfigBase();
    event.Skip();
}

void SettingsDialog::onUpdateState( wxCommandEvent& event )
{
    updateState();
}

void SettingsDialog::onHistorySelectionChanged( wxListEvent& event )
{
    updateHistoryButtons();
}

void SettingsDialog::onHistoryRemove( wxCommandEvent& event )
{
    int selmark = m_historyList->GetNextItem(
        -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );

    if ( selmark >= 0 )
    {
        auto serv = Globals::get()->getWinpinatorServiceInstance();
        serv->getDb()->removeTarget( m_historyData[selmark].targetId );

        fillHistoryList();
    }
}

void SettingsDialog::onHistoryClear( wxCommandEvent& event )
{
    wxMessageDialog dialog( this,
        _( "Are you sure you want to clear all history?" ),
        _( "Clear history" ) );
    dialog.SetExtendedMessage(
        _( "This operation cannot be undone!" ) );
    dialog.SetMessageDialogStyle( wxYES_NO );

    if ( dialog.ShowModal() == wxID_YES )
    {
        auto serv = Globals::get()->getWinpinatorServiceInstance();
        serv->getDb()->removeAllTargets();

        fillHistoryList();
    }
}

void SettingsDialog::updateState()
{
    m_autorunHidden->Enable( m_autorun->IsChecked() );
}

std::unique_ptr<wxBitmap> SettingsDialog::loadScaledFlag(
    const wxString& path, int height )
{
    wxImage img;
    img.LoadFile( path, wxBITMAP_TYPE_PNG );

    float scale = FromDIP( height ) / (float)img.GetSize().y;

    int newWidth = (int)round( scale * img.GetSize().x );
    int newHeight = (int)round( scale * img.GetSize().y );

    auto bitmap = std::make_unique<wxBitmap>(
        img.Scale( newWidth, newHeight, wxIMAGE_QUALITY_BICUBIC ) );
    return bitmap;
}

void SettingsDialog::writeSettingsToConfigBase()
{
    wxConfigBase* confbase = GetApp().m_config.get();

    if ( confbase )
    {
        GetApp().m_settings.saveTo( confbase );
    }
}

};
