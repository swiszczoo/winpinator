#include "settings_model.hpp"

#include <wx/filename.h>
#include <wx/stdpaths.h>

std::unique_ptr<SettingsModel> SettingsModel::s_defaultInstance = nullptr;

SettingsModel::SettingsModel()
    : localeName( "en_US" )
    , openWindowOnStart( true )
    , autorun( false )
    , autorunHidden( true )
    , useCompression( true )
    , zlibCompressionLevel( 5 )
    , outputPath( wxEmptyString )
    , askReceiveFiles( true )
    , askOverwriteFiles( true )
    , filesDefaultPermissions( 664 )
    , foldersDefaultPermissions( 775 )
    , executablesDefaultPermissions( 775 )
    , groupCode( "Warpinator" )
    , networkInterface( "" )
    , transferPort( 42000 )
    , registrationPort( 42001 )
{
    wxFileName outputFname(
        wxStandardPaths::Get().GetDocumentsDir(), "" );
    outputFname.AppendDir( "Winpinator" );

    outputPath = outputFname.GetFullPath();
}

void SettingsModel::loadFrom( wxConfigBase* config )
{
    localeName = config->Read(
        "General/LocaleName", getDefaults()->localeName );
    openWindowOnStart = config->ReadBool( 
        "General/OpenWindowAtStart", getDefaults()->openWindowOnStart );
    autorun = config->ReadBool(
        "General/Autorun", getDefaults()->autorun );
    autorunHidden = config->ReadBool(
        "General/AutorunHidden", getDefaults()->autorunHidden );

    useCompression = config->ReadBool(
        "Transfer/UseCompression", getDefaults()->useCompression );
    zlibCompressionLevel = config->ReadLong(
        "Transfer/ZlibCompressionLevel", getDefaults()->zlibCompressionLevel );
    outputPath = config->Read(
        "Transfer/OutputPath", getDefaults()->outputPath );
    askReceiveFiles = config->ReadBool(
        "Transfer/AskReceiveFiles", getDefaults()->askReceiveFiles );
    askOverwriteFiles = config->ReadBool(
        "Transfer/AskOverwriteFiles", getDefaults()->askOverwriteFiles );

    filesDefaultPermissions = config->ReadLong(
        "Permissions/File", getDefaults()->filesDefaultPermissions );
    foldersDefaultPermissions = config->ReadLong(
        "Permissions/Folder", getDefaults()->foldersDefaultPermissions );
    executablesDefaultPermissions = config->ReadLong(
        "Permissions/Executable", getDefaults()->executablesDefaultPermissions );

    groupCode = config->Read(
        "Connection/GroupCode", getDefaults()->groupCode );
    networkInterface = config->Read(
        "Connection/NetworkInterface", getDefaults()->networkInterface );
    transferPort = config->ReadLong(
        "Connection/TransferPort", getDefaults()->transferPort );
    registrationPort = config->Read(
        "Connection/RegistrationPort", getDefaults()->registrationPort );
}

void SettingsModel::saveTo( wxConfigBase* config )
{
    config->Write( "General/LocaleName", localeName );
    config->Write( "General/OpenWindowAtStart", openWindowOnStart );
    config->Write( "General/Autorun", autorun );
    config->Write( "General/AutorunHidden", autorunHidden );

    config->Write( "Transfer/UseCompression", useCompression );
    config->Write( "Transfer/ZlibCompressionLevel", zlibCompressionLevel );
    config->Write( "Transfer/OutputPath", outputPath );
    config->Write( "Transfer/AskReceiveFiles", askReceiveFiles );
    config->Write( "Transfer/AskOverwriteFiles", askOverwriteFiles );

    config->Write( "Permissions/File", filesDefaultPermissions );
    config->Write( "Permissions/Folder", foldersDefaultPermissions );
    config->Write( "Permissions/Executable", executablesDefaultPermissions );

    config->Write( "Connection/GroupCode", groupCode );
    config->Write( "Connection/NetworkInterface", networkInterface );
    config->Write( "Connection/TransferPort", transferPort );
    config->Write( "Connection/RegistrationPort", registrationPort );
}

SettingsModel* SettingsModel::getDefaults()
{
    if ( !SettingsModel::s_defaultInstance )
    {
        SettingsModel::s_defaultInstance = std::make_unique<SettingsModel>();
    }

    return SettingsModel::s_defaultInstance.get();
}
