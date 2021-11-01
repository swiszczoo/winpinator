#pragma once
#include "language_adapter.hpp"
#include "network_interface_adapter.hpp"
#include "permission_picker.hpp"

#include <wx/bmpcbox.h>
#include <wx/filepicker.h>
#include <wx/notebook.h>
#include <wx/spinctrl.h>
#include <wx/wx.h>

#include <memory>

namespace gui
{

class SettingsDialog : public wxDialog
{
public:
    explicit SettingsDialog( wxWindow* parent );

private:
    LanguageAdapter m_langAdapter;
    NetworkInterfaceAdapter m_inetAdapter;
    std::vector<std::unique_ptr<wxBitmap>> m_loadedBitmaps;
    std::vector<NetworkInterfaceAdapter::InetInfo> m_interfaces;
    wxNotebook* m_notebook;

    wxPanel* m_panelGeneral;
    wxPanel* m_panelPermissions;
    wxPanel* m_panelConnection;

    // General

    wxBitmapComboBox* m_localeName;
    wxCheckBox* m_openWindowOnStart;
    wxCheckBox* m_autorun;
    wxCheckBox* m_autorunHidden;
    wxCheckBox* m_useCompression;
    wxSlider* m_zlibCompressionLevel;
    wxDirPickerCtrl* m_outputDir;
    wxCheckBox* m_askReceiveFiles;
    wxCheckBox* m_askOverwriteFiles;

    // Permissions

    PermissionPicker* m_filesDefaultPerms;
    PermissionPicker* m_executableDefaultPerms;
    PermissionPicker* m_folderDefaultPerms;

    // Connection

    wxTextCtrl* m_groupCode;
    wxChoice* m_networkInterface;
    wxSpinCtrl* m_transferPort;
    wxSpinCtrl* m_registrationPort;

    void createGeneralPage();
    void createPermissionsPage();
    void createConnectionPage();

    void loadSettings();
    void saveSettings();

    void fillLocales();
    void fillInterfaces();

    void onSaveSettings( wxCommandEvent& event );
    void onUpdateState( wxCommandEvent& event );
    void updateState();
    std::unique_ptr<wxBitmap> loadScaledFlag( const wxString& path, int height );
};

};
