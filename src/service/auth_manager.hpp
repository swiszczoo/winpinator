#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include <string>

#include <openssl/pem.h>
#include <openssl/x509.h>

#include <wx/wx.h>
#include <wx/fileconf.h>

#include "../zeroconf/mdns_types.hpp"

namespace srv
{

struct ServerCredentials
{
    std::string publicKey;
    std::string privateKey;
};

class AuthManager
{
    friend std::unique_ptr<AuthManager> std::make_unique<AuthManager>();

public:
    static AuthManager* get();

    void update( zc::MdnsIpPair ips, uint16_t port );
    const std::string& getIdent() const;
    const std::wstring& getGroupCode() const;
    void updateGroupCode( const std::wstring& code );

    ServerCredentials getServerCreds();

    std::string getCachedCert( std::string hostname, zc::MdnsIpPair ips );
    bool processRemoteCert( std::string hostname, zc::MdnsIpPair ips,
        std::string serverData );
    std::string getEncodedLocalCert();

private:
    const static std::wstring DEFAULT_GROUP_CODE;
    const static std::string KEYFILE_GROUP_NAME;
    const static std::string KEYFILE_CODE_KEY;
    const static std::string KEYFILE_UUID_KEY;
    const static std::string CONFIG_FILE_NAME;
    const static std::string CONFIG_FOLDER; 
    const static std::string APPDATA_ENV;
    const static long EXPIRE_TIME;

    static std::unique_ptr<AuthManager> s_inst;

    AuthManager();
    
    std::string m_hostname;
    std::string m_ident;
    zc::MdnsIpPair m_ips;
    uint16_t m_port;
    std::wstring m_code;

    std::string m_serverPubKey;
    std::string m_serverPrivateKey;

    std::map<std::string, std::string> m_remoteCerts;

    std::wstring m_path;

    std::unique_ptr<wxFileConfig> m_keyfile;

    void loadKeyfile();
    void saveKeyfile();
    void readIdent();
    void readGroupCode();
    void makeKeyCertPair();

    static std::string ipPairToString( const zc::MdnsIpPair& pair );
    static EVP_PKEY* generateKey();
    static void setRandomSerialNumber( X509* x509 );
    void setSubjectAltName( X509* x509 );
};

};
