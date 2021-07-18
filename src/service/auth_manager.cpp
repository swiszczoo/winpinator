#define WIN32_LEAN_AND_MEAN
#include "auth_manager.hpp"

#include "service_utils.hpp"

#include <sodium.h>

#include <cpp-base64/base64.h>

#include <openssl/bn.h>
#include <openssl/rand.h>
#include <openssl/x509v3.h>

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
const long AuthManager::EXPIRE_TIME = 30 * ( 24 * 60 * 60 );

AuthManager::AuthManager()
    : m_hostname( "" )
    , m_ident( "" )
    , m_ips( { false } )
    , m_port( 0 )
    , m_code( L"" )
    , m_serverPubKey( "" )
    , m_serverPrivateKey( "" )
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

    RAND_poll();
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
    std::lock_guard<std::recursive_mutex> guard( m_mutex );

    m_ips = ips;
    m_port = port;

    loadKeyfile();

    readIdent();
    readGroupCode();

    makeKeyCertPair();
}

const std::string& AuthManager::getIdent()
{
    std::lock_guard<std::recursive_mutex> guard( m_mutex );

    return m_ident;
}

const std::wstring& AuthManager::getGroupCode()
{
    std::lock_guard<std::recursive_mutex> guard( m_mutex );

    return m_code;
}

void AuthManager::updateGroupCode( const std::wstring& code )
{
    std::lock_guard<std::recursive_mutex> guard( m_mutex );

    if ( code == m_code )
    {
        return;
    }

    wxString path = wxString::Format( "%s/%s",
        AuthManager::KEYFILE_GROUP_NAME, AuthManager::KEYFILE_CODE_KEY );
    m_keyfile->Write( path, wxString( code ) );

    saveKeyfile();
}

ServerCredentials AuthManager::getServerCreds()
{
    std::lock_guard<std::recursive_mutex> guard( m_mutex );

    ServerCredentials creds;
    creds.privateKey = m_serverPrivateKey;
    creds.publicKey = m_serverPubKey;
    return creds;
}

std::string AuthManager::getCachedCert( std::string hostname,
    zc::MdnsIpPair ips )
{
    std::lock_guard<std::recursive_mutex> guard( m_mutex );

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
    std::lock_guard<std::recursive_mutex> guard( m_mutex );

    if ( serverData.empty() )
    {
        return false;
    }

    std::string decoded = base64_decode( serverData );
    std::string codeUtf8 = wxString( m_code ).utf8_string();

    unsigned char key[crypto_hash_sha256_BYTES];
    crypto_hash_sha256( key, (unsigned char*)codeUtf8.data(),
        codeUtf8.length() );

    if ( decoded.length() <= crypto_secretbox_noncebytes() )
    {
        return false;
    }

    size_t msgSize = decoded.length() - crypto_secretbox_noncebytes();
    msgSize -= crypto_secretbox_macbytes();

    std::unique_ptr<char[]> msg = std::make_unique<char[]>( msgSize );

    int cryptoResult = crypto_secretbox_open_easy(
        (unsigned char*)msg.get(),
        (const unsigned char*)decoded.data() + crypto_secretbox_noncebytes(),
        decoded.length() - crypto_secretbox_noncebytes(),
        (const unsigned char*)decoded.data(),
        key );

    if ( cryptoResult != 0 || msgSize == 0 )
    {
        // Error, return false
        return false;
    }

    std::string cert( msg.get(), msgSize );

    std::string mapKey = hostname + '.' + ipPairToString( ips );
    m_remoteCerts[mapKey] = cert;

    return true;
}

std::string AuthManager::getEncodedLocalCert()
{
    std::lock_guard<std::recursive_mutex> guard( m_mutex );

    unsigned char key[crypto_hash_sha256_BYTES];
    std::string codeUtf8 = wxString( m_code ).utf8_string();
    const char* datatest = codeUtf8.data();
    crypto_hash_sha256( key, (unsigned char*)codeUtf8.data(),
        codeUtf8.length() );

    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    randombytes_buf( nonce, crypto_secretbox_NONCEBYTES );

    int encryptedSize = m_serverPubKey.size() + crypto_secretbox_macbytes();
    std::unique_ptr<char[]> encrypted = std::make_unique<char[]>( encryptedSize );

    crypto_secretbox_easy(
        (unsigned char*)encrypted.get(),
        (unsigned char*)m_serverPubKey.data(),
        m_serverPubKey.length(),
        nonce,
        key );

    int bufferSize = crypto_secretbox_NONCEBYTES + encryptedSize;
    std::unique_ptr<char[]> buffer = std::make_unique<char[]>( bufferSize );

    memcpy( buffer.get(), nonce, crypto_secretbox_NONCEBYTES );
    memcpy( buffer.get() + crypto_secretbox_NONCEBYTES,
        encrypted.get(), encryptedSize );

    std::string encoded = base64_encode( (unsigned char*)buffer.get(),
        bufferSize );

    return encoded;
}

void AuthManager::loadKeyfile()
{
    std::lock_guard<std::recursive_mutex> guard( m_mutex );

    m_keyfile = std::make_unique<wxFileConfig>(
        wxEmptyString, wxEmptyString,
        m_path, wxEmptyString, 
        wxCONFIG_USE_LOCAL_FILE );

    m_keyfile->DisableAutoSave();
}

void AuthManager::saveKeyfile()
{
    std::lock_guard<std::recursive_mutex> guard( m_mutex );

    m_keyfile->Flush();
}

void AuthManager::readIdent()
{
    std::lock_guard<std::recursive_mutex> guard( m_mutex );

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
    std::lock_guard<std::recursive_mutex> guard( m_mutex );

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
    std::lock_guard<std::recursive_mutex> guard( m_mutex );

    EVP_PKEY* pkey = AuthManager::generateKey();

    if ( !pkey )
    {
        return;
    }

    X509* builder = X509_new();

    if ( !builder )
    {
        EVP_PKEY_free( pkey );
        return;
    }

    X509_NAME* subjectName = X509_get_subject_name( builder );
    X509_NAME_add_entry_by_NID( subjectName, NID_commonName, MBSTRING_ASC,
        (const unsigned char*)m_hostname.data(), -1, -1, 0 );

    X509_NAME* issuerName = X509_get_issuer_name( builder );
    X509_NAME_add_entry_by_NID( issuerName, NID_commonName, MBSTRING_ASC,
        (const unsigned char*)m_hostname.data(), -1, -1, 0 );

    X509_gmtime_adj( X509_get_notBefore( builder ), -24 * 60 * 60 );
    X509_gmtime_adj( X509_get_notAfter( builder ), AuthManager::EXPIRE_TIME );
    
    AuthManager::setRandomSerialNumber( builder );

    X509_set_pubkey( builder, pkey );

    AuthManager::setSubjectAltName( builder );

    if ( !X509_sign( builder, pkey, EVP_sha256() ) )
    {
        X509_free( builder );
        EVP_PKEY_free( pkey );
        return;
    }

    BIO* serPrivateKeyBio = BIO_new( BIO_s_mem() );
    PEM_write_bio_PKCS8PrivateKey( serPrivateKeyBio, pkey, 
        NULL, NULL, NULL, NULL, NULL);

    char* serPrivateKeyBuf;
    int serPrivateKeySize = BIO_get_mem_data( serPrivateKeyBio, 
        &serPrivateKeyBuf );

    std::string serPrivateKey( serPrivateKeyBuf, serPrivateKeySize );

    BIO_free( serPrivateKeyBio );

    BIO* serPublicKeyBio = BIO_new( BIO_s_mem() );
    PEM_write_bio_X509( serPublicKeyBio, builder );

    char* serPublicKeyBuf;
    int serPublicKeySize = BIO_get_mem_data( serPublicKeyBio, 
        &serPublicKeyBuf );

    std::string serPublicKey( serPublicKeyBuf, serPublicKeySize );

    BIO_free( serPublicKeyBio );

    X509_free( builder );
    EVP_PKEY_free( pkey );

    m_serverPubKey = serPublicKey;
    m_serverPrivateKey = serPrivateKey;
}

std::string AuthManager::ipPairToString( const zc::MdnsIpPair& pair )
{
    if ( pair.valid )
    {
        return wxString::Format( "%s\\%s", pair.ipv4, pair.ipv6 );
    }

    return "invalid_pair";
}

EVP_PKEY* AuthManager::generateKey()
{
    EVP_PKEY* pkey = EVP_PKEY_new();

    if ( !pkey )
    {
        return NULL;
    }

    RSA* rsa = RSA_generate_key( 2048, 65537, NULL, NULL );
    if ( !EVP_PKEY_assign_RSA( pkey, rsa ) )
    {
        EVP_PKEY_free( pkey );
        return NULL;
    }

    return pkey;
}

void AuthManager::setRandomSerialNumber( X509* x509 )
{
    BIGNUM* bn = BN_new();

    BN_rand( bn, 128, 0, 0 );
    BN_to_ASN1_INTEGER( bn, X509_get_serialNumber( x509 ) );

    BN_free( bn );
}

void AuthManager::setSubjectAltName( X509* x509 )
{
    if ( m_ips.valid )
    {
        std::string altName = "IP:" + m_ips.ipv4;

        X509_EXTENSION* ext = X509V3_EXT_conf_nid( NULL, NULL,
            NID_subject_alt_name, altName.c_str() );

        if ( ext )
        {
            X509_EXTENSION_set_critical( ext, 1 );
            X509_add_ext( x509, ext, -1 );
            X509_EXTENSION_free( ext );
        }
    }
}

};
