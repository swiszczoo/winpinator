#include "auth_manager.hpp"

#include "service_utils.hpp"

#include <sodium.h>

#include <cpp-base64/base64.h>

#include <wx/filename.h>
#include <wx/stdpaths.h>


namespace srv
{

std::unique_ptr<AuthManager> AuthManager::s_inst = nullptr;
const std::wstring AuthManager::DEFAULT_GROUP_CODE = L"Warpinator";
const std::string AuthManager::KEYFILE_GROUP_NAME = "warpinator";
const std::string AuthManager::KEYFILE_CODE_KEY = "code";
const std::string AuthManager::KEYFILE_UUID_KEY = "connect_id";
const std::string AuthManager::CONFIG_FILE_NAME = "auth.ini";
const std::string AuthManager::CONFIG_FOLDER = "Winpinator";
const std::string AuthManager::APPDATA_ENV = "APPDATA";

AuthManager::AuthManager()
    : m_hostname( "" )
    , m_ident( "" )
    , m_ips( { false } )
    , m_port( 0 )
    , m_code( L"" )
    , m_privateKey( "" )
    , m_serverCert( "" )
    , m_remoteCerts( {} )
    , m_path( L"" )
    , m_keyfile( nullptr )
{
    m_hostname = Utils::getHostname();

    wxString appDataPath;
    wxGetEnv( AuthManager::APPDATA_ENV, &appDataPath );

    wxFileName configFileName( appDataPath, AuthManager::CONFIG_FOLDER );
    wxString configPath = configFileName.GetFullPath();

    if ( !wxDirExists( configPath ) )
    {
        wxMkDir( configPath );
    }

    wxFileName fname( configPath, AuthManager::CONFIG_FILE_NAME );
    m_path = fname.GetFullPath().ToStdWstring();
}

AuthManager* AuthManager::get()
{
    if ( !AuthManager::s_inst )
    {
        AuthManager::s_inst = std::make_unique<AuthManager>();
    }

    return AuthManager::s_inst.get();
}

void AuthManager::update( zc::MdnsIpPair ips, uint16_t port )
{
    m_ips = ips;
    m_port = port;

    loadKeyfile();

    readIdent();
    readGroupCode();

    makeKeyCertPair();
}

const std::string& AuthManager::getIdent() const
{
    return m_ident;
}

const std::wstring& AuthManager::getGroupCode() const
{
    return m_code;
}

void AuthManager::updateGroupCode( const std::wstring& code )
{
    if ( code == m_code )
    {
        return;
    }

    wxString path = wxString::Format( "%s/%s",
        AuthManager::KEYFILE_GROUP_NAME, AuthManager::KEYFILE_CODE_KEY );
    m_keyfile->Write( path, wxString( code ) );

    saveKeyfile();
}

std::string AuthManager::getCachedCert( std::string hostname,
    zc::MdnsIpPair ips )
{
    std::string key = hostname + '.' + ipPairToString( ips );

    if ( m_remoteCerts.find( key ) != m_remoteCerts.end() )
    {
        return m_remoteCerts[key];
    }

    return "";
}

bool AuthManager::processRemoteCert( std::string hostname, 
    zc::MdnsIpPair ips, std::string serverData )
{
    if ( serverData.empty() )
    {
        return false;
    }

    std::string decoded = base64_decode( serverData );
    wxScopedCharBuffer codeUtf8 = wxString( m_code ).ToUTF8();

    unsigned char key[crypto_hash_sha256_BYTES];
    crypto_hash_sha256( key, (unsigned char*)codeUtf8.data(), 
        codeUtf8.length() );

    if ( decoded.length() <= 24 )
    {
        return false;
    }

    return true;
}

std::string AuthManager::getEncodedLocalCert()
{
    return "";
}

void AuthManager::loadKeyfile()
{
    m_keyfile = std::make_unique<wxFileConfig>(
        wxEmptyString, wxEmptyString,
        m_path, wxEmptyString, wxCONFIG_USE_LOCAL_FILE
    );

    m_keyfile->DisableAutoSave();
}

void AuthManager::saveKeyfile()
{
    m_keyfile->Flush();
}

void AuthManager::readIdent()
{
    bool genNew = false;

    wxString path = wxString::Format( "%s/%s",
        AuthManager::KEYFILE_GROUP_NAME, AuthManager::KEYFILE_UUID_KEY );
    wxString ident = m_keyfile->Read( path, "" );

    if ( ident.empty() )
    {
        genNew = true;
    }

    if ( genNew )
    {
        ident = wxString::Format( "%s-%s", 
            wxString( m_hostname ).Upper(), Utils::generateUUID() );
        m_keyfile->Write( path, ident );
        saveKeyfile();
    }
    
    m_ident = ident.ToStdString();
}

void AuthManager::readGroupCode()
{
    wxString path = wxString::Format( "%s/%s",
        AuthManager::KEYFILE_GROUP_NAME, AuthManager::KEYFILE_CODE_KEY );
    wxString code = m_keyfile->Read( path, "" );

    if ( code.empty() )
    {
        m_code = AuthManager::DEFAULT_GROUP_CODE;
        m_keyfile->Write( path, wxString( m_code ) );
        saveKeyfile();
        return;
    }

    m_code = code.ToStdWstring();
}

void AuthManager::makeKeyCertPair()
{
    // Stub!
}

std::string AuthManager::ipPairToString( const zc::MdnsIpPair& pair )
{
    if ( pair.valid )
    {
        return wxString::Format( "%s\\%s", pair.ipv4, pair.ipv6 );
    }
    
    return "invalid_pair";
}

};
