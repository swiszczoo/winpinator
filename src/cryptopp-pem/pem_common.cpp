// pem_common.cpp - commom PEM routines.
//                  Written and placed in the public domain by Jeffrey Walton

///////////////////////////////////////////////////////////////////////////
// For documentation on the PEM read and write routines, see
//   http://www.cryptopp.com/wiki/PEM_Pack
///////////////////////////////////////////////////////////////////////////

#include <cryptopp/cryptlib.h>
#include "x509cert.h"
#include <cryptopp/secblock.h>
#include <cryptopp/integer.h>
#include <cryptopp/base64.h>
#include <cryptopp/osrng.h>

#include <algorithm>
#include <cctype>
#include <cstring>

#include "pem.h"
#include "pem_common.h"

NAMESPACE_BEGIN(CryptoPP)
NAMESPACE_BEGIN(PEM)

const secure_string CR("\r");
const secure_string LF("\n");
const secure_string EOL("\r\n");
const secure_string CRLF("\r\n");

const secure_string COMMA(",");
const secure_string SPACE(" ");
const secure_string COLON(":");

const secure_string PEM_BEGIN("-----BEGIN");
const secure_string PEM_TAIL("-----");
const secure_string PEM_END("-----END");

const secure_string PUBLIC_BEGIN("-----BEGIN PUBLIC KEY-----");
const secure_string PUBLIC_END("-----END PUBLIC KEY-----");

const secure_string PRIVATE_BEGIN("-----BEGIN PRIVATE KEY-----");
const secure_string PRIVATE_END("-----END PRIVATE KEY-----");

const secure_string RSA_PUBLIC_BEGIN("-----BEGIN RSA PUBLIC KEY-----");
const secure_string RSA_PUBLIC_END("-----END RSA PUBLIC KEY-----");

const secure_string RSA_PRIVATE_BEGIN("-----BEGIN RSA PRIVATE KEY-----");
const secure_string RSA_PRIVATE_END("-----END RSA PRIVATE KEY-----");

const secure_string DSA_PUBLIC_BEGIN("-----BEGIN DSA PUBLIC KEY-----");
const secure_string DSA_PUBLIC_END("-----END DSA PUBLIC KEY-----");

const secure_string DSA_PRIVATE_BEGIN("-----BEGIN DSA PRIVATE KEY-----");
const secure_string DSA_PRIVATE_END("-----END DSA PRIVATE KEY-----");

const secure_string ELGAMAL_PUBLIC_BEGIN("-----BEGIN ELGAMAL PUBLIC KEY-----");
const secure_string ELGAMAL_PUBLIC_END("-----END ELGAMAL PUBLIC KEY-----");

const secure_string ELGAMAL_PRIVATE_BEGIN("-----BEGIN ELGAMAL PRIVATE KEY-----");
const secure_string ELGAMAL_PRIVATE_END("-----END ELGAMAL PRIVATE KEY-----");

const secure_string EC_PUBLIC_BEGIN("-----BEGIN EC PUBLIC KEY-----");
const secure_string EC_PUBLIC_END("-----END EC PUBLIC KEY-----");

const secure_string ECDSA_PUBLIC_BEGIN("-----BEGIN ECDSA PUBLIC KEY-----");
const secure_string ECDSA_PUBLIC_END("-----END ECDSA PUBLIC KEY-----");

const secure_string EC_PRIVATE_BEGIN("-----BEGIN EC PRIVATE KEY-----");
const secure_string EC_PRIVATE_END("-----END EC PRIVATE KEY-----");

const secure_string EC_PARAMETERS_BEGIN("-----BEGIN EC PARAMETERS-----");
const secure_string EC_PARAMETERS_END("-----END EC PARAMETERS-----");

const secure_string DH_PARAMETERS_BEGIN("-----BEGIN DH PARAMETERS-----");
const secure_string DH_PARAMETERS_END("-----END DH PARAMETERS-----");

const secure_string DSA_PARAMETERS_BEGIN("-----BEGIN DSA PARAMETERS-----");
const secure_string DSA_PARAMETERS_END("-----END DSA PARAMETERS-----");

const secure_string CERTIFICATE_BEGIN("-----BEGIN CERTIFICATE-----");
const secure_string CERTIFICATE_END("-----END CERTIFICATE-----");

const secure_string X509_CERTIFICATE_BEGIN("-----BEGIN X509 CERTIFICATE-----");
const secure_string X509_CERTIFICATE_END("-----END X509 CERTIFICATE-----");

const secure_string REQ_CERTIFICATE_BEGIN("-----BEGIN CERTIFICATE REQUEST-----");
const secure_string REQ_CERTIFICATE_END("-----END CERTIFICATE REQUEST-----");

const secure_string PROC_TYPE("Proc-Type");
const secure_string PROC_TYPE_ENC("Proc-Type: 4,ENCRYPTED");
const secure_string ENCRYPTED("ENCRYPTED");
const secure_string DEK_INFO("DEK-Info");
const secure_string CONTENT_DOMAIN("Content-Domain");
const secure_string COMMENT("Comment");

void PEM_WriteLine(BufferedTransformation& bt, const SecByteBlock& line)
{
    bt.Put(line.data(), line.size());
    bt.Put(byte_ptr(EOL), EOL.size());
}

void PEM_WriteLine(BufferedTransformation& bt, const secure_string& line)
{
    bt.Put(byte_ptr(line), line.size());
    bt.Put(byte_ptr(EOL), EOL.size());
}

void PEM_WriteLine(BufferedTransformation& bt, const std::string& line)
{
    bt.Put(byte_ptr(line), line.size());
    bt.Put(byte_ptr(EOL), EOL.size());
}

void PEM_Base64Decode(BufferedTransformation& source, BufferedTransformation& dest)
{
    Base64Decoder decoder(new Redirector(dest));
    source.TransferTo(decoder);
    decoder.MessageEnd();
}

void PEM_Base64Encode(BufferedTransformation& source, BufferedTransformation& dest)
{
    Base64Encoder encoder(new Redirector(dest), true, PEM_LINE_BREAK);
    source.TransferTo(encoder);
    encoder.MessageEnd();
}

secure_string GetControlField(const secure_string& line)
{
    secure_string::const_iterator it = std::search(line.begin(), line.end(), COLON.begin(), COLON.end());
    if (it != line.end())
    {
        const size_t len = it - line.begin();
        return secure_string(line.data(), len);
    }

    return secure_string();
}

secure_string GetControlFieldData(const secure_string& line)
{
    secure_string::const_iterator it = std::search(line.begin(), line.end(), COLON.begin(), COLON.end());
    if (it != line.end() && ++it != line.end())
    {
        const size_t len = line.end() - it;
        return secure_string(it, it + len);
    }

    return secure_string();
}

struct ByteToLower {
    byte operator() (byte val) {
        return (byte)std::tolower((int)(word32)val);
    }
};

// Returns 0 if a match, non-0 otherwise
int CompareNoCase(const secure_string& first, const secure_string& second)
{
    if (first.size() < second.size())
        return -1;
    else if (first.size() > second.size())
        return 1;

    // Same size, compare them...
    int d=0;
    const size_t n = first.size();
    for (size_t i=0; d==0 && i<n; ++i)
        d = std::tolower(static_cast<int>(first[i])) - std::tolower(static_cast<int>(second[i]));

    return d;
}

// From crypto/evp/evp_key.h. Signature changed a bit to match Crypto++.
int OPENSSL_EVP_BytesToKey(HashTransformation& hash,
    const unsigned char *salt, const unsigned char* data, size_t dlen,
    size_t count, unsigned char *key, size_t ksize,
    unsigned char *iv, size_t vsize)
{
    unsigned int niv,nkey,nhash;
    unsigned int addmd=0,i;

    nkey = static_cast<unsigned int>(ksize);
    niv = static_cast<unsigned int>(vsize);
    nhash = static_cast<unsigned int>(hash.DigestSize());

    secure_string digest(hash.DigestSize(), '\0');

    if (data == NULL) return (0);

    for (;;)
    {
        hash.Restart();

        if (addmd++)
            hash.Update(byte_ptr(digest), digest.size());

        hash.Update(data, dlen);

        if (salt != NULL)
            hash.Update(salt, OPENSSL_PKCS5_SALT_LEN);

        hash.TruncatedFinal(byte_ptr(digest), digest.size());

        for (i=1; i<count; i++)
        {
            hash.Restart();
            hash.Update(byte_ptr(digest), digest.size());
            hash.TruncatedFinal(byte_ptr(digest), digest.size());
        }

        i=0;
        if (nkey)
        {
            for (;;)
            {
                if (nkey == 0) break;
                if (i == nhash) break;
                if (key != NULL)
                    *(key++)=digest[i];
                nkey--;
                i++;
            }
        }
        if (niv && (i != nhash))
        {
            for (;;)
            {
                if (niv == 0) break;
                if (i == nhash) break;
                if (iv != NULL)
                    *(iv++)=digest[i];
                niv--;
                i++;
            }
        }
        if ((nkey == 0) && (niv == 0)) break;
    }

    return static_cast<int>(ksize);
}

NAMESPACE_END  // PEM
NAMESPACE_END  // Cryptopp
