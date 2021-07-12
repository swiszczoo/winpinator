// pem_read.cpp - PEM read routines.
//                Written and placed in the public domain by Jeffrey Walton

///////////////////////////////////////////////////////////////////////////
// For documentation on the PEM read and write routines, see
//   http://www.cryptopp.com/wiki/PEM_Pack
///////////////////////////////////////////////////////////////////////////

#include <string>
#include <algorithm>
#include <cctype>
#include <iterator>

#include <cryptopp/cryptlib.h>
#include <cryptopp/secblock.h>
#include <cryptopp/nbtheory.h>
#include <cryptopp/gfpcrypt.h>
#include <cryptopp/camellia.h>
#include <cryptopp/smartptr.h>
#include <cryptopp/filters.h>
#include <cryptopp/base64.h>
#include <cryptopp/queue.h>
#include <cryptopp/modes.h>
#include <cryptopp/asn.h>
#include <cryptopp/aes.h>
#include <cryptopp/idea.h>
#include <cryptopp/hex.h>

#include "pem.h"
#include "pem_common.h"

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/des.h>
#include <cryptopp/md5.h>

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

ANONYMOUS_NAMESPACE_BEGIN

using namespace CryptoPP;
using namespace CryptoPP::PEM;

// Info from the encapsulated header
struct EncapsulatedHeader
{
    secure_string m_version;
    secure_string m_operation;
    secure_string m_algorithm;

    secure_string m_iv;
};

// GCC 9 compile error using overload PEM_GetType
PEM_Type PEM_GetTypeFromString(const secure_string& str);

bool PEM_ReadLine(BufferedTransformation& source, secure_string& line);

void PEM_StripEncapsulatedBoundary(BufferedTransformation& src, BufferedTransformation& dest,
                                   const secure_string& pre, const secure_string& post);
void PEM_StripEncapsulatedBoundary(secure_string& str, const secure_string& pre, const secure_string& post);

void PEM_StripEncapsulatedHeader(BufferedTransformation& src, BufferedTransformation& dest,
                                 EncapsulatedHeader& header);

void PEM_CipherForAlgorithm(const EncapsulatedHeader& header,
                            const char* password, size_t length,
                            member_ptr<StreamTransformation>& stream);

void PEM_Base64DecodeAndDecrypt(BufferedTransformation& src, BufferedTransformation& dest,
                                const char* password, size_t length);
void PEM_Decrypt(BufferedTransformation& src, BufferedTransformation& dest,
                 member_ptr<StreamTransformation>& stream);

bool PEM_IsEncrypted(const secure_string& str);
bool PEM_IsEncrypted(BufferedTransformation& bt);

void PEM_ParseVersion(const secure_string& proctype, secure_string& version);
void PEM_ParseOperation(const secure_string& proctype, secure_string& operation);

void PEM_ParseAlgorithm(const secure_string& dekinfo, secure_string& algorithm);
void PEM_ParseIV(const secure_string& dekinfo, secure_string& iv);

inline secure_string::const_iterator Search(const secure_string& source, const secure_string& target);

template <class EC>
void PEM_LoadParams(BufferedTransformation& bt, DL_GroupParameters_EC<EC>& params);

void PEM_LoadPublicKey(BufferedTransformation& bt, X509PublicKey& key, bool subjectInfo = false);
void PEM_LoadPrivateKey(BufferedTransformation& bt, PKCS8PrivateKey& key, bool subjectInfo = false);

// Crypto++ expects {version,x}; OpenSSL writes {version,x,y,p,q,g}
void PEM_LoadPrivateKey(BufferedTransformation& bt, DSA::PrivateKey& key);

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

secure_string::const_iterator Search(const secure_string& source, const secure_string& target)
{
    return std::search(source.begin(), source.end(), target.begin(), target.end());
}

PEM_Type PEM_GetTypeFromString(const secure_string& str)
{
    secure_string::const_iterator it;

    // Uses an OID to identify the public key type
    it = Search(str, PUBLIC_BEGIN);
    if (it != str.end())
        return PEM_PUBLIC_KEY;

    // Uses an OID to identify the private key type
    it = Search(str, PRIVATE_BEGIN);
    if (it != str.end())
        return PEM_PRIVATE_KEY;

    // RSA key types
    it = Search(str, RSA_PUBLIC_BEGIN);
    if (it != str.end())
        return PEM_RSA_PUBLIC_KEY;

    it = Search(str, RSA_PRIVATE_BEGIN);
    if (it != str.end())
    {
        it = Search(str, PROC_TYPE_ENC);
        if (it != str.end())
            return PEM_RSA_ENC_PRIVATE_KEY;

        return PEM_RSA_PRIVATE_KEY;
    }

    // DSA key types
    it = Search(str, DSA_PUBLIC_BEGIN);
    if (it != str.end())
        return PEM_DSA_PUBLIC_KEY;

    it = Search(str, DSA_PRIVATE_BEGIN);
    if (it != str.end())
    {
        it = Search(str, PROC_TYPE_ENC);
        if (it != str.end())
            return PEM_DSA_ENC_PRIVATE_KEY;

        return PEM_DSA_PRIVATE_KEY;
    }

    // ElGamal key types
    it = Search(str, ELGAMAL_PUBLIC_BEGIN);
    if (it != str.end())
        return PEM_ELGAMAL_PUBLIC_KEY;

    it = Search(str, ELGAMAL_PRIVATE_BEGIN);
    if (it != str.end())
    {
        it = Search(str, PROC_TYPE_ENC);
        if (it != str.end())
            return PEM_ELGAMAL_ENC_PRIVATE_KEY;

        return PEM_ELGAMAL_PRIVATE_KEY;
    }

    // EC key types
    it = Search(str, EC_PUBLIC_BEGIN);
    if (it != str.end())
        return PEM_EC_PUBLIC_KEY;

    it = Search(str, ECDSA_PUBLIC_BEGIN);
    if (it != str.end())
        return PEM_ECDSA_PUBLIC_KEY;

    it = Search(str, EC_PRIVATE_BEGIN);
    if (it != str.end())
    {
        it = Search(str, PROC_TYPE_ENC);
        if (it != str.end())
            return PEM_EC_ENC_PRIVATE_KEY;

        return PEM_EC_PRIVATE_KEY;
    }

    // EC Parameters
    it = Search(str, EC_PARAMETERS_BEGIN);
    if (it != str.end())
        return PEM_EC_PARAMETERS;

    // DH Parameters
    it = Search(str, DH_PARAMETERS_BEGIN);
    if (it != str.end())
        return PEM_DH_PARAMETERS;

    // DSA Parameters
    it = Search(str, DSA_PARAMETERS_BEGIN);
    if (it != str.end())
        return PEM_DSA_PARAMETERS;

    // Certificate
    it = Search(str, CERTIFICATE_BEGIN);
    if (it != str.end())
        return PEM_CERTIFICATE;

    it = Search(str, X509_CERTIFICATE_BEGIN);
    if (it != str.end())
        return PEM_X509_CERTIFICATE;

    it = Search(str, REQ_CERTIFICATE_BEGIN);
    if (it != str.end())
        return PEM_REQ_CERTIFICATE;

    return PEM_UNSUPPORTED;
}

void PEM_LoadPublicKey(BufferedTransformation& src, X509PublicKey& key, bool subjectInfo)
{
    X509PublicKey& pk = dynamic_cast<X509PublicKey&>(key);

    if (subjectInfo)
        pk.Load(src);
    else
        pk.BERDecode(src);

#if defined(PEM_KEY_OR_PARAMETER_VALIDATION) && !defined(NO_OS_DEPENDENCE)
    AutoSeededRandomPool prng;
    if (!pk.Validate(prng, 2))
        throw Exception(Exception::OTHER_ERROR, "PEM_LoadPublicKey: key validation failed");
#endif
}

void PEM_LoadPrivateKey(BufferedTransformation& src, PKCS8PrivateKey& key, bool subjectInfo)
{
    if (subjectInfo)
        key.Load(src);
    else
        key.BERDecodePrivateKey(src, 0, src.MaxRetrievable());

#if defined(PEM_KEY_OR_PARAMETER_VALIDATION) && !defined(NO_OS_DEPENDENCE)
    AutoSeededRandomPool prng;
    if (!key.Validate(prng, 2))
        throw Exception(Exception::OTHER_ERROR, "PEM_LoadPrivateKey: key validation failed");
#endif
}

void PEM_LoadPrivateKey(BufferedTransformation& bt, DSA::PrivateKey& key)
{
    word32 v;
    Integer p,q,g,y,x;

    // Crypto++ expects {version,x}, while OpenSSL provides {version,p,q,g,y,x}
    BERSequenceDecoder seq(bt);
        BERDecodeUnsigned<word32>(seq, v, INTEGER, 0, 0);  // check version
        p.BERDecode(seq);
        q.BERDecode(seq);
        g.BERDecode(seq);
        y.BERDecode(seq);
        x.BERDecode(seq);
    seq.MessageEnd();

    key.Initialize(p, q, g, x);

#if defined(PEM_KEY_OR_PARAMETER_VALIDATION) && !defined(NO_OS_DEPENDENCE)
    AutoSeededRandomPool prng;
    if (!key.Validate(prng, 2))
        throw Exception(Exception::OTHER_ERROR, "PEM_LoadPrivateKey: key validation failed");

    Integer c = key.GetGroupParameters().ExponentiateBase(x);
    if (y != c)
        throw Exception(Exception::OTHER_ERROR, "PEM_LoadPrivateKey: public element validation failed");
#endif
}

template < class EC >
void PEM_LoadParams(BufferedTransformation& bt, DL_GroupParameters_EC<EC>& params)
{
    ByteQueue t1, t2, t3;
    if (PEM_NextObject(bt, t1) == false)
        throw InvalidArgument("PEM_Load: PEM object not available");

    PEM_Type type = PEM_GetType(t1);
    if (type == PEM_EC_PARAMETERS)
        PEM_StripEncapsulatedBoundary(t1, t2, EC_PARAMETERS_BEGIN, EC_PARAMETERS_END);
    else
        throw InvalidDataFormat("PEM_Read: invalid EC parameters");

    PEM_Base64Decode(t2, t3);
    params.BERDecode(t3);

#if defined(PEM_KEY_OR_PARAMETER_VALIDATION) && !defined(NO_OS_DEPENDENCE)
    AutoSeededRandomPool prng;
    if (!params.Validate(prng, 2))
        throw Exception(Exception::OTHER_ERROR, "PEM_LoadPublicKey: parameter validation failed");
#endif
}

bool PEM_IsEncrypted(const secure_string& str)
{
    secure_string::const_iterator it = std::search(str.begin(), str.end(), PROC_TYPE.begin(), PROC_TYPE.end());
    if (it == str.end()) return false;

    it = std::search(it + PROC_TYPE.size(), str.end(), ENCRYPTED.begin(), ENCRYPTED.end());
    return it != str.end();
}

bool PEM_IsEncrypted(BufferedTransformation& bt)
{
    size_t size = (std::min)(bt.MaxRetrievable(), lword(256));
    secure_string str(size, '\0');
    bt.Peek(byte_ptr(str), str.size());

    return PEM_IsEncrypted(str);
}

void PEM_CipherForAlgorithm(const EncapsulatedHeader& header,
                            const char* password, size_t length,
                            member_ptr<StreamTransformation>& stream)
{
    unsigned int ksize=0, vsize=0;
    stream.release();

    secure_string alg; alg.reserve(header.m_algorithm.size());
    std::transform(header.m_algorithm.begin(), header.m_algorithm.end(),
                   std::back_inserter(alg), (int(*)(int))std::toupper);

    if (alg.empty())
        goto verify;  // verify throws

    if (alg[0] == 'A')
    {
        if (alg == "AES-256-CBC")
        {
            ksize = 32;
            vsize = 16;
            stream.reset(new CBC_Mode<AES>::Decryption);
        }
        else if (alg == "AES-192-CBC")
        {
            ksize = 24;
            vsize = 16;
            stream.reset(new CBC_Mode<AES>::Decryption);
        }
        else if (alg == "AES-128-CBC")
        {
            ksize = 16;
            vsize = 16;
            stream.reset(new CBC_Mode<AES>::Decryption);
        }
    }
    else if (alg[0] == 'C')
    {
        if (alg == "CAMELLIA-256-CBC")
        {
            ksize = 32;
            vsize = 16;
            stream.reset(new CBC_Mode<Camellia>::Decryption);
        }
        else if (alg == "CAMELLIA-192-CBC")
        {
            ksize = 24;
            vsize = 16;
            stream.reset(new CBC_Mode<Camellia>::Decryption);
        }
        else if (alg == "CAMELLIA-128-CBC")
        {
            ksize = 16;
            vsize = 16;
            stream.reset(new CBC_Mode<Camellia>::Decryption);
        }
    }
    else if (alg[0] == 'D')
    {
        if (alg == "DES-EDE3-CBC")
        {
            ksize = 24;
            vsize = 8;
            stream.reset(new CBC_Mode<DES_EDE3>::Decryption);
        }
        else if (alg == "DES-EDE2-CBC")
        {
            ksize = 16;
            vsize = 8;
            stream.reset(new CBC_Mode<DES_EDE2>::Decryption);
        }
        else if (alg == "DES-CBC")
        {
            ksize = 8;
            vsize = 8;
            stream.reset(new CBC_Mode<DES>::Decryption);
        }
    }
    else if (alg[0] == 'I')
    {
        if (alg == "IDEA-CBC")
        {
            ksize = 16;
            vsize = 8;
            stream.reset(new CBC_Mode<IDEA>::Decryption);
        }
    }

verify:

    // Verify a cipher was selected
    if (stream.get() == NULLPTR)
        throw NotImplemented(std::string("PEM_CipherForAlgorithm: '")
                             + header.m_algorithm.c_str() + "' is not implemented");

    // Decode the IV. It used as the Salt in EVP_BytesToKey,
    //   and its used as the IV in the cipher.
    HexDecoder hex;
    hex.Put(byte_ptr(header.m_iv), header.m_iv.size());
    hex.MessageEnd();

    // If the IV size is wrong, SetKeyWithIV will throw an exception.
    const size_t size = (std::min)(hex.MaxRetrievable(), static_cast<lword>(vsize));

    secure_string _key(ksize, '\0');
    secure_string _iv(size, '\0');
    secure_string _salt(size, '\0');

    hex.Get(byte_ptr(_iv), _iv.size());

    // The IV pulls double duty. First, the first PKCS5_SALT_LEN bytes are
    // used as the Salt in EVP_BytesToKey. Second, its used as the IV in the
    // cipher.
    _salt = _iv;

    // MD5 is OpenSSL goodness. MD5, IV and Password are IN; KEY is OUT.
    // {NULL,0} parameters are the OUT IV. However, the original IV in
    // the PEM header is used; and not the derived IV.
    Weak::MD5 md5;
    int ret = OPENSSL_EVP_BytesToKey(md5, byte_ptr(_iv),
                 (const byte*)password, length, 1, byte_ptr(_key), _key.size(), NULL, 0);
    if (ret != static_cast<int>(ksize))
        throw Exception(Exception::OTHER_ERROR, "PEM_CipherForAlgorithm: OPENSSL_EVP_BytesToKey failed");

    SymmetricCipher* cipher = dynamic_cast<SymmetricCipher*>(stream.get());
    cipher->SetKeyWithIV(byte_ptr(_key), _key.size(), byte_ptr(_iv), _iv.size());
}

void PEM_Base64DecodeAndDecrypt(BufferedTransformation& src, BufferedTransformation& dest,
                          const char* password, size_t length)
{
    ByteQueue temp1;
    EncapsulatedHeader header;
    PEM_StripEncapsulatedHeader(src, temp1, header);

    ByteQueue temp2;
    PEM_Base64Decode(temp1, temp2);

    member_ptr<StreamTransformation> stream;
    PEM_CipherForAlgorithm(header, password, length, stream);

    PEM_Decrypt(temp2, dest, stream);
}

void PEM_Decrypt(BufferedTransformation& src, BufferedTransformation& dest,
                 member_ptr<StreamTransformation>& stream)
{
    try
    {
        StreamTransformationFilter filter(*stream, new Redirector(dest));
        src.TransferTo(filter);
        filter.MessageEnd();
    }
    catch (const Exception& ex)
    {
        std::string message(ex.what());
        size_t pos = message.find(":");
        if (pos != std::string::npos && pos+2 < message.size())
            message = message.substr(pos+2);

        throw Exception(Exception::OTHER_ERROR, std::string("PEM_Decrypt: ") + message);
    }
}

void PEM_StripEncapsulatedBoundary(BufferedTransformation& src, BufferedTransformation& dest,
                                   const secure_string& pre, const secure_string& post)
{
    int n = 1, prePos = -1, postPos = -1;

    secure_string line, accum;
    while (PEM_ReadLine(src, line))
    {
        secure_string::const_iterator it;

        // The write associated with an empty line must occur. Otherwise, we
        // loose the EOL in an ecrypted private key between the control
        // fields and the encapsulated text.
        //if (line.empty())
        //    continue;

        it = Search(line, pre);
        if (it != line.end())
        {
            prePos = n;
            continue;
        }
        it = Search(line, post);
        if (it != line.end())
        {
            postPos = n;
            continue;
        }

        accum += line + EOL;
        n++;
    }

    if (prePos == -1)
    {
        throw InvalidDataFormat(std::string("PEM_StripEncapsulatedBoundary: '")
                                + pre.c_str() + "' not found");
    }

    if (postPos == -1)
    {
        throw InvalidDataFormat(std::string("PEM_StripEncapsulatedBoundary: '")
                                + post.c_str() + "' not found");
    }

    if (prePos > postPos)
        throw InvalidDataFormat("PEM_StripEncapsulatedBoundary: header boundary follows footer boundary");

    dest.Put(byte_ptr(accum), accum.size());
}

void PEM_StripEncapsulatedHeader(BufferedTransformation& src, BufferedTransformation& dest, EncapsulatedHeader& header)
{
    if (!src.AnyRetrievable())
        return;

    secure_string line, ending;

    // The first line *must* be Proc-Type. Ensure we read it before dropping
    // into the loop.
    if (! PEM_ReadLine(src, line) || line.empty())
        throw InvalidDataFormat("PEM_StripEncapsulatedHeader: failed to locate Proc-Type");

    secure_string field = GetControlField(line);
    if (field.empty())
        throw InvalidDataFormat("PEM_StripEncapsulatedHeader: failed to locate Proc-Type");

    if (0 != CompareNoCase(field, PROC_TYPE))
        throw InvalidDataFormat("PEM_StripEncapsulatedHeader: failed to locate Proc-Type");

    line = GetControlFieldData(line);

    PEM_ParseVersion(line, header.m_version);
    if (header.m_version != "4")
        throw NotImplemented(std::string("PEM_StripEncapsulatedHeader: encryption version ")
                             + header.m_version.c_str() + " not supported");

    PEM_ParseOperation(line, header.m_operation);
    if (header.m_operation != "ENCRYPTED")
        throw NotImplemented(std::string("PEM_StripEncapsulatedHeader: operation ")
                             + header.m_operation.c_str() + " not supported");

    // Next, we have to read until the first empty line
    while (PEM_ReadLine(src, line))
    {
        if (line.size() == 0) break; // size is zero; empty line

        field = GetControlField(line);
        if (0 == CompareNoCase(field, DEK_INFO))
        {
            line = GetControlFieldData(line);

            PEM_ParseAlgorithm(line, header.m_algorithm);
            PEM_ParseIV(line, header.m_iv);

            continue;
        }

        if (0 == CompareNoCase(field, CONTENT_DOMAIN))
        {
            // Silently ignore
            // Content-Domain: RFC822
            continue;
        }

        if (0 == CompareNoCase(field, COMMENT))
        {
            // Silently ignore
            // SSH key: Comment field
            continue;
        }

        if (!field.empty())
            throw NotImplemented(std::string("PEM_StripEncapsulatedHeader: ")
                                 + field.c_str() + " not supported");
    }

    if (header.m_algorithm.empty())
        throw InvalidArgument("PEM_StripEncapsulatedHeader: no encryption algorithm");

    if (header.m_iv.empty())
        throw InvalidArgument("PEM_StripEncapsulatedHeader: no IV present");

    // After the empty line is the encapsulated text. Transfer it to the
    // destination.
    src.TransferTo(dest);
}

// The string will be similar to " 4, ENCRYPTED"
void PEM_ParseVersion(const secure_string& proctype, secure_string& version)
{
    size_t pos1 = 0;
    while (pos1 < proctype.size() && std::isspace(proctype[pos1])) pos1++;

    size_t pos2 = proctype.find(",");
    if (pos2 == secure_string::npos)
        throw InvalidDataFormat("PEM_ParseVersion: failed to locate version");

    while (pos2 > pos1 && std::isspace(proctype[pos2])) pos2--;
    version = proctype.substr(pos1, pos2 - pos1);
}

// The string will be similar to " 4, ENCRYPTED"
void PEM_ParseOperation(const secure_string& proctype, secure_string& operation)
{
    size_t pos1 = proctype.find(",");
    if (pos1 == secure_string::npos)
        throw InvalidDataFormat("PEM_ParseOperation: failed to locate operation");

    pos1++;
    while (pos1 < proctype.size() && std::isspace(proctype[pos1])) pos1++;

    operation = proctype.substr(pos1, secure_string::npos);
    std::transform(operation.begin(), operation.end(), operation.begin(), (int(*)(int))std::toupper);
}

// The string will be similar to " AES-128-CBC, XXXXXXXXXXXXXXXX"
void PEM_ParseAlgorithm(const secure_string& dekinfo, secure_string& algorithm)
{
    size_t pos1 = 0;
    while (pos1 < dekinfo.size() && std::isspace(dekinfo[pos1])) pos1++;

    size_t pos2 = dekinfo.find(",");
    if (pos2 == secure_string::npos)
        throw InvalidDataFormat("PEM_ParseVersion: failed to locate algorithm");

    while (pos2 > pos1 && std::isspace(dekinfo[pos2])) pos2--;

    algorithm = dekinfo.substr(pos1, pos2 - pos1);
    std::transform(algorithm.begin(), algorithm.end(), algorithm.begin(),  (int(*)(int))std::toupper);
}

// The string will be similar to " AES-128-CBC, XXXXXXXXXXXXXXXX"
void PEM_ParseIV(const secure_string& dekinfo, secure_string& iv)
{
    size_t pos1 = dekinfo.find(",");
    if (pos1 == secure_string::npos)
        throw InvalidDataFormat("PEM_ParseIV: failed to locate initialization vector");

    pos1++;
    while (pos1 < dekinfo.size() && std::isspace(dekinfo[pos1])) pos1++;

    iv = dekinfo.substr(pos1, secure_string::npos);
    std::transform(iv.begin(), iv.end(), iv.begin(), (int(*)(int))std::toupper);
}

// Read a line of text, until an EOL is encountered. The EOL can be
// CRLF, CR or LF. The last line does not need an EOL. An empty line
// with just EOL still counts as a line for PEM_ReadLine. The function
// returns true if a line was read, false otherwise.
bool PEM_ReadLine(BufferedTransformation& source, secure_string& line)
{
    // In case of early out
    line.clear();

    if (!source.AnyRetrievable()) {
        return false;
    }

    // Assume standard PEM line size, with CRLF
    line.reserve(PEM_LINE_BREAK+2);

    byte b;
    while (source.Get(b))
    {
        // LF ?
        if (b == '\n') {
            break;
        }

        // CR ?
        if (b == '\r')
        {
            // CRLF ?
            if (source.Peek(b) && b == '\n') {
                source.Skip(1);
            }
            break;
        }

        // Not End-of-Line, accumulate it.
        line += b;
    }

    return true;
}

inline void PEM_TrimLeadingWhitespace(BufferedTransformation& source)
{
    byte b;
    while (source.Peek(b) && std::isspace(b)) {
        source.Skip(1);
    }
}

NAMESPACE_END

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

NAMESPACE_BEGIN(CryptoPP)

using namespace CryptoPP::PEM;

PEM_Type PEM_GetType(const BufferedTransformation& bt)
{
    lword size = (std::min)(bt.MaxRetrievable(), lword(128));
    secure_string str(size, '\0');
    bt.Peek(byte_ptr(str), str.size());
    return PEM_GetTypeFromString(str);
}

bool PEM_NextObject(BufferedTransformation& src, BufferedTransformation& dest)
{
    // Skip leading whitespace
    PEM_TrimLeadingWhitespace(src);

    // Anything to parse?
    if (!src.AnyRetrievable())
        return false;

    // We have four things to find:
    //   1. -----BEGIN (the leading begin)
    //   2. ----- (the trailing dashes)
    //   3. -----END (the leading end)
    //   4. ----- (the trailing dashes)

    // Once we parse something that purports to be PEM encoded, another routine
    //  will have to look for something particular, like a RSA key. We *will*
    //  inadvertently parse garbage, like -----BEGIN FOO BAR-----. It will
    //  be caught later when a PEM_Load routine is called.

    const size_t BAD_IDX = PEM_INVALID;

    // We use iterators for the search. However, an interator is invalidated
    //  after each insert that grows the container. So we save indexes
    //  from begin() to speed up searching. On each iteration, we simply
    //  reinitialize them.
    secure_string::const_iterator it;
    size_t idx1 = BAD_IDX, idx2 = BAD_IDX, idx3 = BAD_IDX, idx4 = BAD_IDX;

    // The idea is to read chunks in case there are multiple keys or
    //  paramters in a BufferedTransformation. So we use CopyTo to
    //  extract what we are interested in. We don't take anything
    //  out of the BufferedTransformation (yet).

    // We also use indexes because the iterator will be invalidated
    //   when we append to the ByteQueue. Even though the iterator
    //   is invalid, `accum.begin() + index` will be valid.

    // Reading 8 or 10 lines at a time is an optimization from testing
    //   cacerts.pem. The file has 150 or so certs, so its a good test.
    // +2 to allow for CR + LF line endings. There's no guarantee a line
    //   will be present, or it will be PEM_LINE_BREAK in size.
    const size_t READ_SIZE = (PEM_LINE_BREAK + 1) * 10;
    const size_t REWIND_SIZE = (std::max)(PEM_BEGIN.size(), PEM_END.size()) + 2;

    secure_string accum;
    size_t idx = 0, next = 0;

    size_t available = src.MaxRetrievable();
    while (available)
    {
        // How much can we read?
        const size_t size = (std::min)(available, READ_SIZE);

        // Ideally, we would only scan the line we are reading. However,
        //   we need to rewind a bit in case a token spans the previous
        //   block and the block we are reading. But we can't rewind
        //   into a previous index. Once we find an index, the variable
        //   next is set to it. Hence the reason for the std::max()
        if (idx > REWIND_SIZE)
        {
            const size_t x = idx - REWIND_SIZE;
            next = (std::max)(next, x);
        }

        // We need a temp queue to use CopyRangeTo. We have to use it
        //   because there's no Peek that allows us to peek a range.
        ByteQueue tq;
        src.CopyRangeTo(tq, static_cast<lword>(idx), static_cast<lword>(size));

        const size_t offset = accum.size();
        accum.resize(offset + size);
        tq.Get(byte_ptr(accum) + offset, size);

        // Adjust sizes
        idx += size;
        available -= size;

        // Locate '-----BEGIN'
        if (idx1 == BAD_IDX)
        {
            it = std::search(accum.begin() + next, accum.end(), PEM_BEGIN.begin(), PEM_BEGIN.end());
            if (it == accum.end())
                continue;

            idx1 = it - accum.begin();
            next = idx1 + PEM_BEGIN.size();
        }

        // Locate '-----'
        if (idx2 == BAD_IDX && idx1 != BAD_IDX)
        {
            it = std::search(accum.begin() + next, accum.end(), PEM_TAIL.begin(), PEM_TAIL.end());
            if (it == accum.end())
                continue;

            idx2 = it - accum.begin();
            next = idx2 + PEM_TAIL.size();
        }

        // Locate '-----END'
        if (idx3 == BAD_IDX && idx2 != BAD_IDX)
        {
            it = std::search(accum.begin() + next, accum.end(), PEM_END.begin(), PEM_END.end());
            if (it == accum.end())
                continue;

            idx3 = it - accum.begin();
            next = idx3 + PEM_END.size();
        }

        // Locate '-----'
        if (idx4 == BAD_IDX && idx3 != BAD_IDX)
        {
            it = std::search(accum.begin() + next, accum.end(), PEM_TAIL.begin(), PEM_TAIL.end());
            if (it == accum.end())
                continue;

            idx4 = it - accum.begin();
            next = idx4 + PEM_TAIL.size();
        }
    }

    // Did we find `-----BEGIN XXX-----` (RFC 1421 calls this pre-encapsulated
    // boundary)?
    if (idx1 == BAD_IDX || idx2 == BAD_IDX)
        throw InvalidDataFormat("PEM_NextObject: could not locate boundary header");

    // Did we find `-----END XXX-----` (RFC 1421 calls this post-encapsulated
    // boundary)?
    if (idx3 == BAD_IDX || idx4 == BAD_IDX)
        throw InvalidDataFormat("PEM_NextObject: could not locate boundary footer");

    // *IF* the trailing '-----' occurred in the last 5 bytes in accum, then
    // we might miss the End of Line. We need to peek 2 more bytes if
    // available and append them to accum.
    if (available >= 2)
    {
        ByteQueue tq;
        src.CopyRangeTo(tq, static_cast<lword>(idx), static_cast<lword>(2));

        const size_t offset = accum.size();
        accum.resize(offset + 2);
        tq.Get(byte_ptr(accum) + offset, 2);
    }
    else if (available == 1)
    {
        ByteQueue tq;
        src.CopyRangeTo(tq, static_cast<lword>(idx), static_cast<lword>(1));

        const size_t offset = accum.size();
        accum.resize(offset + 1);
        tq.Get(byte_ptr(accum) + offset, 1);
    }

    // Final book keeping
    const char* ptr = accum.data() + idx1;
    const size_t used = idx4 + PEM_TAIL.size();
    const size_t len = used - idx1;

    dest.Put(byte_ptr(ptr), len);
    dest.Put(byte_ptr(EOL), EOL.size());
    dest.MessageEnd();

    src.Skip(used);
    PEM_TrimLeadingWhitespace(src);

    return true;
}

void PEM_Load(BufferedTransformation& bt, RSA::PublicKey& rsa)
{
    ByteQueue t1, t2, t3;
    if (PEM_NextObject(bt, t1) == false)
        throw InvalidArgument("PEM_Load: PEM object not available");

    PEM_Type type = PEM_GetType(t1);
    if (type == PEM_PUBLIC_KEY)
        PEM_StripEncapsulatedBoundary(t1, t2, PUBLIC_BEGIN, PUBLIC_END);
    else if (type == PEM_RSA_PUBLIC_KEY)
        PEM_StripEncapsulatedBoundary(t1, t2, RSA_PUBLIC_BEGIN, RSA_PUBLIC_END);
    else
        throw InvalidDataFormat("PEM_Load: not a RSA public key");

    PEM_Base64Decode(t2, t3);
    PEM_LoadPublicKey(t3, rsa, type == PEM_PUBLIC_KEY);
}

void PEM_Load(BufferedTransformation& bt, RSA::PrivateKey& rsa)
{
    return PEM_Load(bt, rsa, NULL, 0);
}

void PEM_Load(BufferedTransformation& bt, RSA::PrivateKey& rsa, const char* password, size_t length)
{
    ByteQueue t1, t2, t3;
    if (PEM_NextObject(bt, t1) == false)
        throw InvalidArgument("PEM_Load: PEM object not available");

    PEM_Type type = PEM_GetType(t1);
    if (type == PEM_PRIVATE_KEY)
        PEM_StripEncapsulatedBoundary(t1, t2, PRIVATE_BEGIN, PRIVATE_END);
    else if (type == PEM_RSA_PRIVATE_KEY || (type == PEM_RSA_ENC_PRIVATE_KEY && password != NULL))
        PEM_StripEncapsulatedBoundary(t1, t2, RSA_PRIVATE_BEGIN, RSA_PRIVATE_END);
    else if (type == PEM_RSA_ENC_PRIVATE_KEY && password == NULL)
        throw InvalidArgument("PEM_Load: RSA private key is encrypted");
    else
        throw InvalidDataFormat("PEM_Load: not a RSA private key");

    if (type == PEM_RSA_ENC_PRIVATE_KEY)
        PEM_Base64DecodeAndDecrypt(t2, t3, password, length);
    else
        PEM_Base64Decode(t2, t3);

    PEM_LoadPrivateKey(t3, rsa, type == PEM_PRIVATE_KEY);
}

void PEM_Load(BufferedTransformation& bt, DSA::PublicKey& dsa)
{
    ByteQueue t1, t2, t3;
    if (PEM_NextObject(bt, t1) == false)
        throw InvalidArgument("PEM_Load: PEM object not available");

    PEM_Type type = PEM_GetType(t1);
    if (type == PEM_PUBLIC_KEY)
        PEM_StripEncapsulatedBoundary(t1, t2, PUBLIC_BEGIN, PUBLIC_END);
    else if (type == PEM_DSA_PUBLIC_KEY)
        PEM_StripEncapsulatedBoundary(t1, t2, DSA_PUBLIC_BEGIN, DSA_PUBLIC_END);
    else
        throw InvalidDataFormat("PEM_Load: not a DSA public key");

    PEM_Base64Decode(t2, t3);
    PEM_LoadPublicKey(t3, dsa, type == PEM_PUBLIC_KEY);
}

void PEM_Load(BufferedTransformation& bt, DSA::PrivateKey& dsa)
{
    return PEM_Load(bt, dsa, NULL, 0);
}

void PEM_Load(BufferedTransformation& bt, DSA::PrivateKey& dsa, const char* password, size_t length)
{
    ByteQueue t1, t2, t3;
    if (PEM_NextObject(bt, t1) == false)
        throw InvalidArgument("PEM_Load: PEM object not available");

    PEM_Type type = PEM_GetType(t1);
    if (type == PEM_PRIVATE_KEY)
        PEM_StripEncapsulatedBoundary(t1, t2, PRIVATE_BEGIN, PRIVATE_END);
    else if (type == PEM_DSA_PRIVATE_KEY || (type == PEM_DSA_ENC_PRIVATE_KEY && password != NULL))
        PEM_StripEncapsulatedBoundary(t1, t2, DSA_PRIVATE_BEGIN, DSA_PRIVATE_END);
    else if (type == PEM_DSA_ENC_PRIVATE_KEY && password == NULL)
        throw InvalidArgument("PEM_Load: DSA private key is encrypted");
    else
        throw InvalidDataFormat("PEM_Load: not a DSA private key");

    if (type == PEM_DSA_ENC_PRIVATE_KEY)
        PEM_Base64DecodeAndDecrypt(t2, t3, password, length);
    else
        PEM_Base64Decode(t2, t3);

    PEM_LoadPrivateKey(t3, dsa);
}

void PEM_Load(BufferedTransformation& bt, ElGamalKeys::PublicKey& key)
{
    ByteQueue t1, t2, t3;
    if (PEM_NextObject(bt, t1) == false)
        throw InvalidArgument("PEM_Load: PEM object not available");

    PEM_Type type = PEM_GetType(t1);
    if (type == PEM_PUBLIC_KEY)
        PEM_StripEncapsulatedBoundary(t1, t2, PUBLIC_BEGIN, PUBLIC_END);
    else if (type == PEM_ELGAMAL_PUBLIC_KEY)
        PEM_StripEncapsulatedBoundary(t1, t2, ELGAMAL_PUBLIC_BEGIN, ELGAMAL_PUBLIC_END);
    else
        throw InvalidDataFormat("PEM_Load: not a ElGamal public key");

    PEM_Base64Decode(t2, t3);
    PEM_LoadPublicKey(t3, key, type == PEM_PUBLIC_KEY);
}

void PEM_Load(BufferedTransformation& bt, ElGamalKeys::PrivateKey& key)
{
    return PEM_Load(bt, key, NULL, 0);
}

void PEM_Load(BufferedTransformation& bt, ElGamalKeys::PrivateKey& key, const char* password, size_t length)
{
    ByteQueue t1, t2, t3;
    if (PEM_NextObject(bt, t1) == false)
        throw InvalidArgument("PEM_Load: PEM object not available");

    PEM_Type type = PEM_GetType(t1);
    if (type == PEM_PRIVATE_KEY)
        PEM_StripEncapsulatedBoundary(t1, t2, PRIVATE_BEGIN, PRIVATE_END);
    else if (type == PEM_ELGAMAL_PRIVATE_KEY || (type == PEM_ELGAMAL_ENC_PRIVATE_KEY && password != NULL))
        PEM_StripEncapsulatedBoundary(t1, t2, ELGAMAL_PRIVATE_BEGIN, ELGAMAL_PRIVATE_END);
    else if (type == PEM_ELGAMAL_ENC_PRIVATE_KEY && password == NULL)
        throw InvalidArgument("PEM_Load: ElGamal private key is encrypted");
    else
        throw InvalidDataFormat("PEM_Load: not a ElGamal private key");

    if (type == PEM_ELGAMAL_ENC_PRIVATE_KEY)
        PEM_Base64DecodeAndDecrypt(t2, t3, password, length);
    else
        PEM_Base64Decode(t2, t3);

    PEM_LoadPrivateKey(t3, key, type == PEM_PRIVATE_KEY);
}

void PEM_Load(BufferedTransformation& bt, DL_GroupParameters_EC<ECP>& params)
{
    PEM_LoadParams(bt, params);
}

void PEM_Load(BufferedTransformation& bt, DL_GroupParameters_EC<EC2N>& params)
{
    PEM_LoadParams(bt, params);
}

void PEM_Load(BufferedTransformation& bt, DL_PublicKey_EC<ECP>& ec)
{
    ByteQueue t1, t2, t3;
    if (PEM_NextObject(bt, t1) == false)
        throw InvalidArgument("PEM_Load: PEM object not available");

    PEM_Type type = PEM_GetType(t1);
    if (type == PEM_PUBLIC_KEY)
        PEM_StripEncapsulatedBoundary(t1, t2, PUBLIC_BEGIN, PUBLIC_END);
    else if (type == PEM_EC_PUBLIC_KEY)
        PEM_StripEncapsulatedBoundary(t1, t2, EC_PUBLIC_BEGIN, EC_PUBLIC_END);
    else
        throw InvalidDataFormat("PEM_Load: not a public EC key");

    PEM_Base64Decode(t2, t3);
    PEM_LoadPublicKey(t3, ec, type == PEM_PUBLIC_KEY);
}

void PEM_Load(BufferedTransformation& bt, DL_PrivateKey_EC<ECP>& ec)
{
    return PEM_Load(bt, ec, NULL, 0);
}

void PEM_Load(BufferedTransformation& bt, DL_PrivateKey_EC<ECP>& ec, const char* password, size_t length)
{
    ByteQueue t1, t2, t3;
    if (PEM_NextObject(bt, t1) == false)
        throw InvalidArgument("PEM_Load: PEM object not available");

    PEM_Type type = PEM_GetType(t1);
    if (type == PEM_PRIVATE_KEY)
        PEM_StripEncapsulatedBoundary(t1, t2, PRIVATE_BEGIN, PRIVATE_END);
    else if (type == PEM_EC_PRIVATE_KEY || (type == PEM_EC_ENC_PRIVATE_KEY && password != NULL))
        PEM_StripEncapsulatedBoundary(t1, t2, EC_PRIVATE_BEGIN, EC_PRIVATE_END);
    else if (type == PEM_EC_ENC_PRIVATE_KEY && password == NULL)
        throw InvalidArgument("PEM_Load: EC private key is encrypted");
    else
        throw InvalidDataFormat("PEM_Load: not a private EC key");

    if (type == PEM_EC_ENC_PRIVATE_KEY)
        PEM_Base64DecodeAndDecrypt(t2, t3, password, length);
    else
        PEM_Base64Decode(t2, t3);

    PEM_LoadPrivateKey(t3, ec, type == PEM_PRIVATE_KEY);
}

void PEM_Load(BufferedTransformation& bt, DL_PublicKey_EC<EC2N>& ec)
{
    ByteQueue t1, t2, t3;
    if (PEM_NextObject(bt, t1) == false)
        throw InvalidArgument("PEM_Load: PEM object not available");

    PEM_Type type = PEM_GetType(t1);
    if (type == PEM_PUBLIC_KEY)
        PEM_StripEncapsulatedBoundary(t1, t2, PUBLIC_BEGIN, PUBLIC_END);
    else if (type == PEM_EC_PUBLIC_KEY)
        PEM_StripEncapsulatedBoundary(t1, t2, EC_PUBLIC_BEGIN, EC_PUBLIC_END);
    else
        throw InvalidDataFormat("PEM_Load: not a public EC key");

    PEM_Base64Decode(t2, t3);
    PEM_LoadPublicKey(t3, ec, type == PEM_PUBLIC_KEY);
}

void PEM_Load(BufferedTransformation& bt, DL_PrivateKey_EC<EC2N>& ec)
{
    return PEM_Load(bt, ec, NULL, 0);
}

void PEM_Load(BufferedTransformation& bt, DL_PrivateKey_EC<EC2N>& ec, const char* password, size_t length)
{
    ByteQueue t1, t2, t3;
    if (PEM_NextObject(bt, t1) == false)
        throw InvalidArgument("PEM_Load: PEM object not available");

    PEM_Type type = PEM_GetType(t1);
    if (type == PEM_PRIVATE_KEY)
        PEM_StripEncapsulatedBoundary(t1, t2, PRIVATE_BEGIN, PRIVATE_END);
    else if (type == PEM_EC_PRIVATE_KEY || (type == PEM_EC_ENC_PRIVATE_KEY && password != NULL))
        PEM_StripEncapsulatedBoundary(t1, t2, EC_PRIVATE_BEGIN, EC_PRIVATE_END);
    else if (type == PEM_EC_ENC_PRIVATE_KEY && password == NULL)
        throw InvalidArgument("PEM_Load: EC private key is encrypted");
    else
        throw InvalidDataFormat("PEM_Load: not a private EC key");

    ByteQueue temp;
    if (type == PEM_EC_ENC_PRIVATE_KEY)
        PEM_Base64DecodeAndDecrypt(t2, t3, password, length);
    else
        PEM_Base64Decode(t2, t3);

    PEM_LoadPrivateKey(t3, ec, type == PEM_PRIVATE_KEY);
}

void PEM_Load(BufferedTransformation& bt, DL_Keys_ECDSA<ECP>::PrivateKey& ecdsa)
{
    return PEM_Load(bt, ecdsa, NULL, 0);
}

void PEM_Load(BufferedTransformation& bt, DL_Keys_ECDSA<ECP>::PrivateKey& ecdsa, const char* password, size_t length)
{
    PEM_Load(bt, dynamic_cast<DL_PrivateKey_EC<ECP>&>(ecdsa), password, length);
}

void PEM_Load(BufferedTransformation& bt, DL_Keys_ECDSA<EC2N>::PrivateKey& ecdsa)
{
    return PEM_Load(bt, ecdsa, NULL, 0);
}

void PEM_Load(BufferedTransformation& bt, DL_Keys_ECDSA<EC2N>::PrivateKey& ecdsa, const char* password, size_t length)
{
    PEM_Load(bt, dynamic_cast<DL_PrivateKey_EC<EC2N>&>(ecdsa), password, length);
}

void PEM_Load(BufferedTransformation& bt, DL_GroupParameters_DSA& params)
{
    ByteQueue t1, t2, t3;
    if (PEM_NextObject(bt, t1) == false)
        throw InvalidArgument("PEM_Load: PEM object not available");

    PEM_Type type = PEM_GetType(t1);
    if (type == PEM_DSA_PARAMETERS)
        PEM_StripEncapsulatedBoundary(t1, t2, DSA_PARAMETERS_BEGIN, DSA_PARAMETERS_END);
    else
        throw InvalidDataFormat("PEM_Read: invalid DSA parameters");

    PEM_Base64Decode(t2, t3);
    params.Load(t3);
}

void PEM_Load(BufferedTransformation& bt, X509Certificate& cert)
{
    CRYPTOPP_UNUSED(cert);

    ByteQueue t1, t2, t3;
    if (PEM_NextObject(bt, t1) == false)
        throw InvalidArgument("PEM_Load: PEM object not available");

    PEM_Type type = PEM_GetType(t1);
    if (type == PEM_X509_CERTIFICATE)
        PEM_StripEncapsulatedBoundary(t1, t2, X509_CERTIFICATE_BEGIN, X509_CERTIFICATE_END);
    else if (type == PEM_CERTIFICATE)
        PEM_StripEncapsulatedBoundary(t1, t2, CERTIFICATE_BEGIN, CERTIFICATE_END);
    else
        throw InvalidDataFormat("PEM_Read: invalid X.509 certificate");

    PEM_Base64Decode(t2, t3);
    cert.Load(t3);

    // throw NotImplemented("PEM_Load: X.509 certificate is not implemented");
}

void PEM_DH_Load(BufferedTransformation& bt, Integer& p, Integer& g)
{
    ByteQueue t1, t2;
    if (PEM_NextObject(bt, t1) == false)
        throw InvalidArgument("PEM_Load: PEM object not available");

    PEM_Type type = PEM_GetType(t1);
    if (type == PEM_DH_PARAMETERS)
        PEM_StripEncapsulatedBoundary(t1, t2, DH_PARAMETERS_BEGIN, DH_PARAMETERS_END);
    else
        throw InvalidDataFormat("PEM_DH_Load: invalid DH parameters");

    PEM_Base64Decode(t1, t2);

    BERSequenceDecoder dh(t2);
        p.BERDecode(dh);
        g.BERDecode(dh);
    dh.MessageEnd();

#if defined(PEM_KEY_OR_PARAMETER_VALIDATION) && !defined(NO_OS_DEPENDENCE)
    AutoSeededRandomPool prng;
    if (!VerifyPrime(prng, p, 3))
        throw Exception(Exception::OTHER_ERROR, "PEM_DH_Load: p is not prime");

    // https://crypto.stackexchange.com/questions/12961/diffie-hellman-parameter-check-when-g-2-must-p-mod-24-11
    long residue = static_cast<long>(p.Modulo(24));
    if (residue != 11 && residue != 23)
        throw Exception(Exception::OTHER_ERROR, "PEM_DH_Load: g is not a suitable generator");
#endif
}

void PEM_DH_Load(BufferedTransformation& bt, Integer& p, Integer& q, Integer& g)
{
    ByteQueue t1, t2, t3;
    if (PEM_NextObject(bt, t1) == false)
        throw InvalidArgument("PEM_Load: PEM object not available");

    PEM_Type type = PEM_GetType(t1);
    if (type == PEM_DH_PARAMETERS)
        PEM_StripEncapsulatedBoundary(t1, t2, DH_PARAMETERS_BEGIN, DH_PARAMETERS_END);
    else
        throw InvalidDataFormat("PEM_DH_Load: invalid DH parameters");

    PEM_Base64Decode(t2, t3);

    BERSequenceDecoder dh(t3);
        p.BERDecode(dh);
        q.BERDecode(dh);
        g.BERDecode(dh);
    dh.MessageEnd();

#if defined(PEM_KEY_OR_PARAMETER_VALIDATION) && !defined(NO_OS_DEPENDENCE)
    AutoSeededRandomPool prng;
    if (!VerifyPrime(prng, p, 3))
        throw Exception(Exception::OTHER_ERROR, "PEM_DH_Load: p is not prime");

    // https://crypto.stackexchange.com/questions/12961/diffie-hellman-parameter-check-when-g-2-must-p-mod-24-11
    long residue = static_cast<long>(p.Modulo(24));
    if (residue != 11 && residue != 23)
        throw Exception(Exception::OTHER_ERROR, "PEM_DH_Load: g is not a suitable generator");
#endif
}

NAMESPACE_END
