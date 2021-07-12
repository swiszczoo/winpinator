// pem_write.cpp - PEM write routines.
//                 Written and placed in the public domain by Jeffrey Walton

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
#include <cryptopp/camellia.h>
#include <cryptopp/smartptr.h>
#include <cryptopp/filters.h>
#include <cryptopp/base64.h>
#include <cryptopp/files.h>
#include <cryptopp/queue.h>
#include <cryptopp/modes.h>
#include <cryptopp/osrng.h>
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

// This class saves the existing EncodeAsOID setting for EC group parameters.
// PEM_Save unconditionally sets it to TRUE for OpenSSL compatibility. See
// https://wiki.openssl.org/index.php/Elliptic_Curve_Cryptography#Named_Curves
template <class T>
struct OID_State
{
    OID_State(const T& obj);
    virtual ~OID_State();

    const T& m_gp;
    bool m_flag;
};

template <>
OID_State<DL_GroupParameters_EC<ECP> >::OID_State(const DL_GroupParameters_EC<ECP>& gp)
: m_gp(gp), m_flag(gp.GetEncodeAsOID()) {
    DL_GroupParameters_EC<ECP>& obj = const_cast<DL_GroupParameters_EC<ECP>&>(m_gp);
    obj.SetEncodeAsOID(true);
}

template <>
OID_State<DL_GroupParameters_EC<ECP> >::~OID_State() {
    DL_GroupParameters_EC<ECP>& obj = const_cast<DL_GroupParameters_EC<ECP>&>(m_gp);
    obj.SetEncodeAsOID(m_flag);
}

template <>
OID_State<DL_GroupParameters_EC<EC2N> >::OID_State(const DL_GroupParameters_EC<EC2N>& gp)
: m_gp(gp), m_flag(gp.GetEncodeAsOID()) {
    DL_GroupParameters_EC<EC2N>& obj = const_cast<DL_GroupParameters_EC<EC2N>&>(m_gp);
    obj.SetEncodeAsOID(true);
}

template <>
OID_State<DL_GroupParameters_EC<EC2N> >::~OID_State() {
    DL_GroupParameters_EC<EC2N>& obj = const_cast<DL_GroupParameters_EC<EC2N>&>(m_gp);
    obj.SetEncodeAsOID(m_flag);
}

// Returns a keyed StreamTransformation ready to use to encrypt a DER encoded key
void PEM_CipherForAlgorithm(RandomNumberGenerator& rng, std::string algorithm,
                            member_ptr<StreamTransformation>& stream,
                            secure_string& key, secure_string& iv,
                            const char* password, size_t length);

void PEM_DEREncode(BufferedTransformation& bt, const PKCS8PrivateKey& key);
void PEM_DEREncode(BufferedTransformation& bt, const X509PublicKey& key);

// Ambiguous call; needs a best match. Provide an overload.
void PEM_DEREncode(BufferedTransformation& bt, const RSA::PrivateKey& key);

// Special handling for DSA private keys. Crypto++ provides {version,x},
//   while OpenSSL expects {version,p,q,g,y,x}.
void PEM_DEREncode(BufferedTransformation& bt, const DSA::PrivateKey& key);

// Special handling for EC private keys. Crypto++ provides {version,x},
//   while OpenSSL expects {version,x,curve oid,y}.
template <class EC>
void PEM_DEREncode(BufferedTransformation& bt, const DL_PrivateKey_EC<EC>& key);

void PEM_Encrypt(BufferedTransformation& src, BufferedTransformation& dest,
                 member_ptr<StreamTransformation>& stream);
void PEM_EncryptAndBase64Encode(BufferedTransformation& src, BufferedTransformation& dest,
                                member_ptr<StreamTransformation>& stream);

template <class EC>
void PEM_SaveParams(BufferedTransformation& bt, const DL_GroupParameters_EC< EC >& params,
                    const secure_string& pre, const secure_string& post);

template <class KEY>
void PEM_SaveKey(BufferedTransformation& bt, const KEY& key,
                 const secure_string& pre, const secure_string& post);

template <class PUBLIC_KEY>
void PEM_SavePublicKey(BufferedTransformation& bt, const PUBLIC_KEY& key,
                       const secure_string& pre, const secure_string& post);

template <class PRIVATE_KEY>
void PEM_SavePrivateKey(BufferedTransformation& bt, const PRIVATE_KEY& key,
                        const secure_string& pre, const secure_string& post);

template <class PRIVATE_KEY>
void PEM_SavePrivateKey(BufferedTransformation& bt, const PRIVATE_KEY& key,
                        RandomNumberGenerator& rng, const std::string& algorithm,
                        const char* password, size_t length,
                        const secure_string& pre, const secure_string& post);

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

template <class EC>
void PEM_SaveParams(BufferedTransformation& bt, const DL_GroupParameters_EC< EC >& params,
                    const secure_string& pre, const secure_string& post)
{
    PEM_WriteLine(bt, pre);

    Base64Encoder encoder(new Redirector(bt), true, PEM_LINE_BREAK);

    params.DEREncode(encoder);
    encoder.MessageEnd();

    PEM_WriteLine(bt, post);

    bt.MessageEnd();
}

template <class EC>
void PEM_SavePrivateKey(BufferedTransformation& bt, const DL_PrivateKey_EC<EC>& key,
                        const secure_string& pre, const secure_string& post)
{
    PEM_WriteLine(bt, pre);

    ByteQueue queue;
    PEM_DEREncode(queue, key);

    PEM_Base64Encode(queue, bt);

    PEM_WriteLine(bt, post);

    bt.MessageEnd();
}

void PEM_DEREncode(BufferedTransformation& bt, const DSA::PrivateKey& key)
{
    // Crypto++ provides {version,x}, while OpenSSL expects {version,p,q,g,y,x}.
    const DL_GroupParameters_DSA& params = key.GetGroupParameters();

    DSA::PublicKey pkey;
    key.MakePublicKey(pkey);

    DERSequenceEncoder seq(bt);
        DEREncodeUnsigned<word32>(seq, 0);         // version
        params.GetModulus().DEREncode(seq);        // p
        params.GetSubgroupOrder().DEREncode(seq);  // q
        params.GetGenerator().DEREncode(seq);      // g
        pkey.GetPublicElement().DEREncode(seq);    // y
        key.GetPrivateExponent().DEREncode(seq);   // x
    seq.MessageEnd();
}

template <class EC>
void PEM_DEREncode(BufferedTransformation& bt, const DL_PrivateKey_EC<EC>& key)
{
    // Crypto++ provides {version,x}, while OpenSSL expects {version,x,curve oid,y}.
    typedef typename DL_PrivateKey_EC<EC>::Element Element;
    const DL_GroupParameters_EC<EC>& params = key.GetGroupParameters();
    const Integer& x = key.GetPrivateExponent();
    const Element& y = params.ExponentiateBase(x);

    // Named curve
    OID oid;
    if (key.GetVoidValue(Name::GroupOID(), typeid(oid), &oid) == false)
        throw Exception(Exception::OTHER_ERROR, "PEM_DEREncode: failed to retrieve curve OID");

    DERSequenceEncoder seq(bt);
        DEREncodeUnsigned<word32>(seq, 1);  // version
        x.DEREncodeAsOctetString(seq, params.GetSubgroupOrder().ByteCount());

        DERGeneralEncoder cs1(seq, CONTEXT_SPECIFIC | CONSTRUCTED | 0);
            oid.DEREncode(cs1);
        cs1.MessageEnd();

        DERGeneralEncoder cs2(seq, CONTEXT_SPECIFIC | CONSTRUCTED | 1);
            DERGeneralEncoder cs3(cs2, BIT_STRING);
                cs3.Put(0x00);        // Unused bits
                params.GetCurve().EncodePoint(cs3, y, false);
            cs3.MessageEnd();
        cs2.MessageEnd();
    seq.MessageEnd();
    bt.MessageEnd();
}

void PEM_DEREncode(BufferedTransformation& bt, const PKCS8PrivateKey& key)
{
    key.DEREncodePrivateKey(bt);
    bt.MessageEnd();
}

void PEM_DEREncode(BufferedTransformation& bt, const X509PublicKey& key)
{
    key.DEREncode(bt);
    bt.MessageEnd();
}

void PEM_DEREncode(BufferedTransformation& bt, const RSA::PrivateKey& key)
{
    return PEM_DEREncode(bt, dynamic_cast<const PKCS8PrivateKey&>(key));
}

template <class PUBLIC_KEY>
void PEM_SavePublicKey(BufferedTransformation& bt, const PUBLIC_KEY& key,
                       const secure_string& pre, const secure_string& post)
{
    PEM_SaveKey(bt, key, pre, post);
}

template <class PRIVATE_KEY>
void PEM_SavePrivateKey(BufferedTransformation& bt, const PRIVATE_KEY& key,
                        const secure_string& pre, const secure_string& post)
{
    PEM_SaveKey(bt, key, pre, post);
}

template <class KEY>
void PEM_SaveKey(BufferedTransformation& bt, const KEY& key,
                 const secure_string& pre, const secure_string& post)
{
    PEM_WriteLine(bt, pre);

    ByteQueue queue;
    PEM_DEREncode(queue, key);

    PEM_Base64Encode(queue, bt);

    PEM_WriteLine(bt, post);

    bt.MessageEnd();
}

template<class PRIVATE_KEY>
void PEM_SavePrivateKey(BufferedTransformation& bt, const PRIVATE_KEY& key,
                        RandomNumberGenerator& rng, const std::string& algorithm,
                        const char* password, size_t length,
                        const secure_string& pre, const secure_string& post)
{
    ByteQueue queue;

    PEM_WriteLine(queue, pre);

    // Proc-Type: 4,ENCRYPTED
    PEM_WriteLine(queue, PROC_TYPE_ENC);

    secure_string _key, _iv;
    member_ptr<StreamTransformation> stream;

    // After this executes, we have a StreamTransformation keyed and ready to go.
    PEM_CipherForAlgorithm(rng, algorithm, stream, _key, _iv, password, length);

    // Encode the IV. It gets written to the encapsulated header.
    HexEncoder hex;
    hex.Put(byte_ptr(_iv), _iv.size());
    hex.MessageEnd();

    secure_string encoded;
    encoded.resize(hex.MaxRetrievable());
    hex.Get(byte_ptr(encoded), encoded.size());

    // e.g., DEK-Info: AES-128-CBC,5E537774BCCD88B3E2F47FE294C93253
    secure_string dekinfo = "DEK-Info: ";
    dekinfo += algorithm.c_str();
    dekinfo += "," + encoded;

    // The extra newline separates the control fields from the encapsulated
    //   text (i.e, header from body). Its required by RFC 1421.
    PEM_WriteLine(queue, dekinfo);
    queue.Put(byte_ptr(EOL), EOL.size());

    ByteQueue temp;
    PEM_DEREncode(temp, key);

    PEM_EncryptAndBase64Encode(temp, queue, stream);

    PEM_WriteLine(queue, post);

    queue.TransferTo(bt);
    bt.MessageEnd();
}

void PEM_CipherForAlgorithm(RandomNumberGenerator& rng, std::string algorithm,
                            member_ptr<StreamTransformation>& stream,
                            secure_string& key, secure_string& iv,
                            const char* password, size_t length)
{
    unsigned int ksize=0, vsize=0;
    stream.release();

    secure_string alg; alg.reserve(algorithm.size());
    std::transform(algorithm.begin(), algorithm.end(),
                   std::back_inserter(alg), (int(*)(int))std::toupper);

    if (alg.empty())
        goto verify;  // verify throws

    if (alg[0] == 'A')
    {
        if (alg == "AES-256-CBC")
        {
            ksize = 32;
            vsize = 16;
            stream.reset(new CBC_Mode<AES>::Encryption);
        }
        else if (alg == "AES-192-CBC")
        {
            ksize = 24;
            vsize = 16;
            stream.reset(new CBC_Mode<AES>::Encryption);
        }
        else if (alg == "AES-128-CBC")
        {
            ksize = 16;
            vsize = 16;
            stream.reset(new CBC_Mode<AES>::Encryption);
        }
    }
    else if (alg[0] == 'C')
    {
        if (alg == "CAMELLIA-256-CBC")
        {
            ksize = 32;
            vsize = 16;
            stream.reset(new CBC_Mode<Camellia>::Encryption);
        }
        else if (alg == "CAMELLIA-192-CBC")
        {
            ksize = 24;
            vsize = 16;
            stream.reset(new CBC_Mode<Camellia>::Encryption);
        }
        else if (alg == "CAMELLIA-128-CBC")
        {
            ksize = 16;
            vsize = 16;
            stream.reset(new CBC_Mode<Camellia>::Encryption);
        }
    }
    else if (alg[0] == 'D')
    {
        if (alg == "DES-EDE3-CBC")
        {
            ksize = 24;
            vsize = 8;
            stream.reset(new CBC_Mode<DES_EDE3>::Encryption);
        }
        else if (alg == "DES-EDE2-CBC")
        {
            ksize = 16;
            vsize = 8;
            stream.reset(new CBC_Mode<DES_EDE2>::Encryption);
        }
        else if (alg == "DES-CBC")
        {
            ksize = 8;
            vsize = 8;
            stream.reset(new CBC_Mode<DES>::Encryption);
        }
    }
    else if (alg[0] == 'I')
    {
        if (alg == "IDEA-CBC")
        {
            ksize = 16;
            vsize = 8;
            stream.reset(new CBC_Mode<IDEA>::Encryption);
        }
    }

verify:

    // Verify a cipher was selected
    if (stream.get() == NULLPTR)
        throw NotImplemented(std::string("PEM_CipherForAlgorithm: '")
                             + algorithm.c_str() + "' is not implemented");

    const unsigned char* _pword = reinterpret_cast<const unsigned char*>(password);
    const size_t _plen = length;

    secure_string _key(ksize, '\0'), _iv(vsize, '\0'), _salt(vsize, '\0');

    // The IV pulls double duty. First, the first PKCS5_SALT_LEN bytes are used
    //   as the Salt in EVP_BytesToKey. Second, its used as the IV in the cipher.

    rng.GenerateBlock(byte_ptr(_iv), _iv.size());
    _salt = _iv;

    // MD5 is OpenSSL goodness. MD5, IV and Password are IN; KEY is OUT.
    // {NULL,0} parameters are the OUT IV. However, the original IV in
    // the PEM header is used; and not the derived IV.
    Weak::MD5 md5;
    int ret = OPENSSL_EVP_BytesToKey(md5, byte_ptr(_salt),
                  _pword, _plen, 1, byte_ptr(_key), _key.size(), NULL, 0);
    if (ret != static_cast<int>(ksize))
        throw Exception(Exception::OTHER_ERROR, "PEM_CipherForAlgorithm: OPENSSL_EVP_BytesToKey failed");

    SymmetricCipher* cipher = dynamic_cast<SymmetricCipher*>(stream.get());
    cipher->SetKeyWithIV(byte_ptr(_key), _key.size(), byte_ptr(_iv), _iv.size());

    _key.swap(key);
    _iv.swap(iv);
}

void PEM_Encrypt(BufferedTransformation& src, BufferedTransformation& dest,
                 member_ptr<StreamTransformation>& stream)
{
    StreamTransformationFilter filter(*stream, new Redirector(dest));
    src.TransferTo(filter);
    filter.MessageEnd();
}

void PEM_EncryptAndBase64Encode(BufferedTransformation& src, BufferedTransformation& dest,
                          member_ptr<StreamTransformation>& stream)
{
    ByteQueue temp;
    PEM_Encrypt(src, temp, stream);

    PEM_Base64Encode(temp, dest);
}

ANONYMOUS_NAMESPACE_END

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

NAMESPACE_BEGIN(CryptoPP)

using namespace CryptoPP::PEM;

void PEM_Save(BufferedTransformation& bt, const RSA::PublicKey& rsa)
{
    PEM_SavePublicKey(bt, rsa, PUBLIC_BEGIN, PUBLIC_END);
}

void PEM_Save(BufferedTransformation& bt, const RSA::PrivateKey& rsa)
{
    PEM_SavePrivateKey(bt, rsa, RSA_PRIVATE_BEGIN, RSA_PRIVATE_END);
}

void PEM_Save(BufferedTransformation& bt, const RSA::PrivateKey& rsa,
              RandomNumberGenerator& rng, const std::string& algorithm,
              const char* password, size_t length)
{
    PEM_SavePrivateKey(bt, rsa, rng, algorithm, password, length, RSA_PRIVATE_BEGIN, RSA_PRIVATE_END);
}

void PEM_Save(BufferedTransformation& bt, const DSA::PublicKey& dsa)
{
    PEM_SavePublicKey(bt, dsa, PUBLIC_BEGIN, PUBLIC_END);
}

void PEM_Save(BufferedTransformation& bt, const DSA::PrivateKey& dsa)
{
    PEM_SavePrivateKey(bt, dsa, DSA_PRIVATE_BEGIN, DSA_PRIVATE_END);
}

void PEM_Save(BufferedTransformation& bt, const DSA::PrivateKey& dsa,
              RandomNumberGenerator& rng, const std::string& algorithm,
              const char* password, size_t length)
{
    PEM_SavePrivateKey(bt, dsa, rng, algorithm, password, length, DSA_PRIVATE_BEGIN, DSA_PRIVATE_END);
}

void PEM_Save(BufferedTransformation& bt, const ElGamalKeys::PublicKey& key)
{
    PEM_SavePublicKey(bt, key, PUBLIC_BEGIN, PUBLIC_END);
}

void PEM_Save(BufferedTransformation& bt, const ElGamalKeys::PrivateKey& key)
{
    PEM_SavePrivateKey(bt, key, ELGAMAL_PRIVATE_BEGIN, ELGAMAL_PRIVATE_END);
}

void PEM_Save(BufferedTransformation& bt, const ElGamalKeys::PrivateKey& key,
              RandomNumberGenerator& rng, const std::string& algorithm,
              const char* password, size_t length)
{
    PEM_SavePrivateKey(bt, key, rng, algorithm, password, length, ELGAMAL_PRIVATE_BEGIN, ELGAMAL_PRIVATE_END);
}


void PEM_Save(BufferedTransformation& bt, const DL_GroupParameters_EC<ECP>& params)
{
    OID_State<DL_GroupParameters_EC<ECP> > state(params);
    PEM_SaveParams(bt, params, EC_PARAMETERS_BEGIN, EC_PARAMETERS_END);
}

void PEM_Save(BufferedTransformation& bt, const DL_GroupParameters_EC<EC2N>& params)
{
    OID_State<DL_GroupParameters_EC<EC2N> > state(params);
    PEM_SaveParams(bt, params, EC_PARAMETERS_BEGIN, EC_PARAMETERS_END);
}

void PEM_Save(BufferedTransformation& bt, const DL_PublicKey_EC<ECP>& ec)
{
    OID_State<DL_GroupParameters_EC<ECP> > state(ec.GetGroupParameters());
    PEM_SavePublicKey(bt, ec, PUBLIC_BEGIN, PUBLIC_END);
}

void PEM_Save(BufferedTransformation& bt, const DL_PrivateKey_EC<ECP>& ec)
{
    OID_State<DL_GroupParameters_EC<ECP> > state(ec.GetGroupParameters());
    PEM_SavePrivateKey(bt, ec, EC_PRIVATE_BEGIN, EC_PRIVATE_END);
}

void PEM_Save(BufferedTransformation& bt, const DL_PrivateKey_EC<ECP>& ec,
              RandomNumberGenerator& rng, const std::string& algorithm,
              const char* password, size_t length)
{
    OID_State<DL_GroupParameters_EC<ECP> > state(ec.GetGroupParameters());
    PEM_SavePrivateKey(bt, ec, rng, algorithm, password, length, EC_PRIVATE_BEGIN, EC_PRIVATE_END);
}

void PEM_Save(BufferedTransformation& bt, const DL_PublicKey_EC<EC2N>& ec)
{
    OID_State<DL_GroupParameters_EC<EC2N> > state(ec.GetGroupParameters());
    PEM_SavePublicKey(bt, ec, PUBLIC_BEGIN, PUBLIC_END);
}

void PEM_Save(BufferedTransformation& bt, const DL_PrivateKey_EC<EC2N>& ec)
{
    OID_State<DL_GroupParameters_EC<EC2N> > state(ec.GetGroupParameters());
    PEM_SavePrivateKey(bt, ec, EC_PRIVATE_BEGIN, EC_PRIVATE_END);
}

void PEM_Save(BufferedTransformation& bt, const DL_PrivateKey_EC<EC2N>& ec,
              RandomNumberGenerator& rng, const std::string& algorithm,
              const char* password, size_t length)
{
    OID_State<DL_GroupParameters_EC<EC2N> > state(ec.GetGroupParameters());
    PEM_SavePrivateKey(bt, ec, rng, algorithm, password, length, EC_PRIVATE_BEGIN, EC_PRIVATE_END);
}

void PEM_Save(BufferedTransformation& bt, const DL_Keys_ECDSA<ECP>::PrivateKey& ecdsa)
{
    PEM_Save(bt, dynamic_cast<const DL_PrivateKey_EC<ECP>&>(ecdsa));
}

void PEM_Save(BufferedTransformation& bt, DL_Keys_ECDSA<ECP>::PrivateKey& ecdsa,
              RandomNumberGenerator& rng, const std::string& algorithm,
              const char* password, size_t length)
{
    PEM_Save(bt, dynamic_cast<DL_PrivateKey_EC<ECP>&>(ecdsa), rng, algorithm, password, length);
}

void PEM_Save(BufferedTransformation& bt, const DL_GroupParameters_DSA& params)
{
    ByteQueue queue;

    PEM_WriteLine(queue, DSA_PARAMETERS_BEGIN);

    Base64Encoder encoder(new Redirector(queue), true, PEM_LINE_BREAK);
    params.Save(encoder);
    encoder.MessageEnd();

    PEM_WriteLine(queue, DSA_PARAMETERS_END);

    queue.TransferTo(bt);
    bt.MessageEnd();
}

void PEM_DH_Save(BufferedTransformation& bt, const Integer& p, const Integer& g)
{
    ByteQueue queue;

    PEM_WriteLine(queue, DH_PARAMETERS_BEGIN);

    Base64Encoder encoder(new Redirector(queue), true, PEM_LINE_BREAK);

    DERSequenceEncoder seq(encoder);
        p.BEREncode(seq);
        g.BEREncode(seq);
    seq.MessageEnd();

    encoder.MessageEnd();

    PEM_WriteLine(queue, DH_PARAMETERS_END);

    queue.TransferTo(bt);
    bt.MessageEnd();
}

void PEM_DH_Save(BufferedTransformation& bt, const Integer& p, const Integer& q, const Integer& g)
{
    ByteQueue queue;

    PEM_WriteLine(queue, DH_PARAMETERS_BEGIN);

    Base64Encoder encoder(new Redirector(queue), true, PEM_LINE_BREAK);

    DERSequenceEncoder seq(encoder);
        p.BEREncode(seq);
        q.BEREncode(seq);
        g.BEREncode(seq);
    seq.MessageEnd();

    encoder.MessageEnd();

    PEM_WriteLine(queue, DH_PARAMETERS_END);

    queue.TransferTo(bt);
    bt.MessageEnd();
}

NAMESPACE_END
