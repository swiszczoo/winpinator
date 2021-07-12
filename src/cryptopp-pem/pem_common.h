// pem_common.h - commom PEM routines.
//                Written and placed in the public domain by Jeffrey Walton
//                pem_common.h is an internal header. Include pem.h instead.

///////////////////////////////////////////////////////////////////////////
// For documentation on the PEM read and write routines, see
//   http://www.cryptopp.com/wiki/PEM_Pack
///////////////////////////////////////////////////////////////////////////

#ifndef CRYPTOPP_PEM_COMMON_H
#define CRYPTOPP_PEM_COMMON_H

#include <cryptopp/cryptlib.h>
#include <cryptopp/secblock.h>
#include <cryptopp/osrng.h>
#include "pem.h"

#include <string>

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////

// By default, keys and parameters are validated after reading in Debug builds.
//   You will have to call key.Validate() yourself if desired. If you want automatic
//   validation, then uncomment the line below or set it on the command line.
// #define PEM_KEY_OR_PARAMETER_VALIDATION 1

#if defined(CRYPTOPP_DEBUG) && !defined(PEM_KEY_OR_PARAMETER_VALIDATION)
# define PEM_KEY_OR_PARAMETER_VALIDATION 1
#endif

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////

NAMESPACE_BEGIN(CryptoPP)
NAMESPACE_BEGIN(PEM)

typedef std::basic_string<char, std::char_traits<char>, AllocatorWithCleanup<char> > secure_string;

inline const byte* byte_ptr(const char* cstr)
{
    return reinterpret_cast<const byte*>(cstr);
}

inline byte* byte_ptr(char* cstr)
{
    return reinterpret_cast<byte*>(cstr);
}

inline const byte* byte_ptr(const secure_string& str)
{
    static const char empty[1] = {0};
    return str.empty() ?
        reinterpret_cast<const byte*>(empty) : reinterpret_cast<const byte*>(&str[0]);
}

inline byte* byte_ptr(secure_string& str)
{
    static char empty[1] = {0};
    return str.empty() ?
        reinterpret_cast<byte*>(empty) : reinterpret_cast<byte*>(&str[0]);
}

inline const byte* byte_ptr(const std::string& str)
{
    static const char empty[1] = {0};
    return str.empty() ?
        reinterpret_cast<const byte*>(empty) : reinterpret_cast<const byte*>(&str[0]);
}

inline byte* byte_ptr(std::string& str)
{
    static char empty[1] = {0};
    return str.empty() ?
        reinterpret_cast<byte*>(empty) : reinterpret_cast<byte*>(&str[0]);
}

// Attempts to locate a control field in a line
secure_string GetControlField(const secure_string& line);

// Attempts to fetch the data from a control line
secure_string GetControlFieldData(const secure_string& line);

// Returns 0 if a match, non-0 otherwise
int CompareNoCase(const secure_string& first, const secure_string& second);

// Base64 Encode
void PEM_Base64Encode(BufferedTransformation& source, BufferedTransformation& dest);

// Base64 Decode
void PEM_Base64Decode(BufferedTransformation& source, BufferedTransformation& dest);

// Write to a BufferedTransformation
void PEM_WriteLine(BufferedTransformation& bt, const SecByteBlock& line);
void PEM_WriteLine(BufferedTransformation& bt, const std::string& line);
void PEM_WriteLine(BufferedTransformation& bt, const secure_string& line);

// Signature changed a bit to match Crypto++. Salt must be PKCS5_SALT_LEN in length.
//  Salt, Data and Count are IN; Key and IV are OUT.
int OPENSSL_EVP_BytesToKey(HashTransformation& hash,
                           const unsigned char *salt, const unsigned char* data, size_t dlen,
                           size_t count, unsigned char *key, size_t ksize,
                           unsigned char *iv, size_t vsize);

// From OpenSSL, crypto/evp/evp.h.
static const unsigned int OPENSSL_PKCS5_SALT_LEN = 8;

// Signals failure
static const size_t PEM_INVALID = static_cast<size_t>(-1);

// 64-character line length is required by RFC 1421.
static const unsigned int PEM_LINE_BREAK = 64;

extern const secure_string CR;
extern const secure_string LF;
extern const secure_string EOL;
extern const secure_string CRLF;

extern const secure_string COMMA;
extern const secure_string SPACE;
extern const secure_string COLON;

extern const secure_string PEM_BEGIN;
extern const secure_string PEM_TAIL;
extern const secure_string PEM_END;

extern const secure_string PUBLIC_BEGIN;
extern const secure_string PUBLIC_END;

extern const secure_string PRIVATE_BEGIN;
extern const secure_string PRIVATE_END;

extern const secure_string RSA_PUBLIC_BEGIN;
extern const secure_string RSA_PUBLIC_END;

extern const secure_string RSA_PRIVATE_BEGIN;
extern const secure_string RSA_PRIVATE_END;

extern const secure_string DSA_PUBLIC_BEGIN;
extern const secure_string DSA_PUBLIC_END;

extern const secure_string DSA_PRIVATE_BEGIN;
extern const secure_string DSA_PRIVATE_END;

extern const secure_string ELGAMAL_PUBLIC_BEGIN;
extern const secure_string ELGAMAL_PUBLIC_END;

extern const secure_string ELGAMAL_PRIVATE_BEGIN;
extern const secure_string ELGAMAL_PRIVATE_END;

extern const secure_string EC_PUBLIC_BEGIN;
extern const secure_string EC_PUBLIC_END;

extern const secure_string ECDSA_PUBLIC_BEGIN;
extern const secure_string ECDSA_PUBLIC_END;

extern const secure_string EC_PRIVATE_BEGIN;
extern const secure_string EC_PRIVATE_END;

extern const secure_string EC_PARAMETERS_BEGIN;
extern const secure_string EC_PARAMETERS_END;

extern const secure_string DH_PARAMETERS_BEGIN;
extern const secure_string DH_PARAMETERS_END;

extern const secure_string DSA_PARAMETERS_BEGIN;
extern const secure_string DSA_PARAMETERS_END;

extern const secure_string CERTIFICATE_BEGIN;
extern const secure_string CERTIFICATE_END;

extern const secure_string X509_CERTIFICATE_BEGIN;
extern const secure_string X509_CERTIFICATE_END;

extern const secure_string REQ_CERTIFICATE_BEGIN;
extern const secure_string REQ_CERTIFICATE_END;

extern const secure_string PROC_TYPE;
extern const secure_string PROC_TYPE_ENC;
extern const secure_string ENCRYPTED;
extern const secure_string DEK_INFO;
extern const secure_string CONTENT_DOMAIN;
extern const secure_string COMMENT;

NAMESPACE_END  // PEM
NAMESPACE_END  // CryptoPP

#endif // CRYPTOPP_PEM_COMMON_H
