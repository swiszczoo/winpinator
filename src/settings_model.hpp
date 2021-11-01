#pragma once
#include <wx/confbase.h>
#include <wx/string.h>

#include <memory>

class SettingsModel
{
public:
    SettingsModel();

    void loadFrom( wxConfigBase* config );
    void saveTo( wxConfigBase* config );

    // Settings fields

    wxString localeName;
    bool openWindowOnStart;
    bool autorun;
    bool autorunHidden;

    bool useCompression;
    int zlibCompressionLevel;
    wxString outputPath;
    bool askReceiveFiles;
    bool askOverwriteFiles;

    int filesDefaultPermissions;
    int executablesDefaultPermissions;
    int foldersDefaultPermissions;

    wxString groupCode;
    wxString networkInterface;
    int transferPort;
    int registrationPort;

private:
    static std::unique_ptr<SettingsModel> s_defaultInstance;
    static SettingsModel* getDefaults();
};

