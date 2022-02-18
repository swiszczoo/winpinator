#pragma once
#include "language_adapter.hpp"
#include "network_interface_adapter.hpp"
#include "permission_picker.hpp"
#include "../service/database_types.hpp"

#include <wx/bmpcbox.h>
#include <wx/filepicker.h>
#include <wx/listctrl.h>
#include <wx/notebook.h>
#include <wx/spinctrl.h>
#include <wx/wx.h>

#include <memory>
#include <vector>

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
    wxPanel* m_panelHistory;
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
    wxCheckBox* m_preserveZoneInfo;

    // Permissions

    PermissionPicker* m_filesDefaultPerms;
    PermissionPicker* m_executableDefaultPerms;
    PermissionPicker* m_folderDefaultPerms;

    // History

    std::vector<srv::db::TargetInfoData> m_historyData;
    wxListCtrl* m_historyList;
    wxButton* m_historyRemove;
    wxButton* m_historyClear;

    // Connection

    wxTextCtrl* m_groupCode;
    wxChoice* m_networkInterface;
    wxSpinCtrl* m_transferPort;
    wxSpinCtrl* m_registrationPort;

    wxString m_autorunCommand;

    void createGeneralPage();
    void createPermissionsPage();
    void createHistoryPage();
    void createConnectionPage();

    void fillHistoryList();
    void updateHistoryButtons();

    void loadSettings();
    void saveSettings();

    void fillLocales();
    void fillInterfaces();

    void onSaveSettings( wxCommandEvent& event );
    void onUpdateState( wxCommandEvent& event );
    void onHistorySelectionChanged( wxListEvent& event );
    void onHistoryRemove( wxCommandEvent& event );
    void onHistoryClear( wxCommandEvent& event );

    void updateState();
    std::unique_ptr<wxBitmap> loadScaledFlag( const wxString& path, int height );
    void writeSettingsToConfigBase();
};

};
