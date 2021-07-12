// x509cert.cpp - X.509 certificate read and write routines for Crypto++.
//                Written and placed in the public domain by Jeffrey Walton
//                and Geoff Beier

///////////////////////////////////////////////////////////////////////////
// For documentation on the X509Certificate class, see
//   http://www.cryptopp.com/wiki/X509Certificate and
//   http://www.cryptopp.com/wiki/PEM_Pack
///////////////////////////////////////////////////////////////////////////

#include <cryptopp/cryptlib.h>
#include <cryptopp/secblock.h>
#include "x509cert.h"
#include <cryptopp/integer.h>
#include <cryptopp/files.h>
#include <cryptopp/oids.h>
#include <cryptopp/trap.h>

#include <cryptopp/rsa.h>
#include <cryptopp/dsa.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/xed25519.h>

// For printing
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>

// For Validate
#include <cryptopp/osrng.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

// Make these defines to avoid global objects
#define id_basicConstraints            (OID(2)+5+29+19)
#define id_authorityKeyIdentifier      (OID(2)+5+29+35)
#define id_msUserPrincipalName         (OID(1)+3+6+1+4+1+311+20+2+3)
#define id_subjectPublicKeyIdentifier  (OID(2)+5+29+14)
#define id_distinguishedName           (OID(2)+5+4+49)
#define id_commonName                  (OID(2)+5+4+3)
#define id_uniqueIdentifier            (OID(2)+5+4+45)
#define id_pkcsEmail                   (OID(1)+2+840+113549+1+9+1)
#define id_subjectAltName              (OID(2)+5+29+17)
#define id_netscapeServerName          (OID(2)+16+840+1+113730+1+12)
#define id_keyUsage                    (OID(2)+5+29+15)
#define id_extendedKeyUsage            (OID(2)+5+29+37)

ANONYMOUS_NAMESPACE_BEGIN

using namespace CryptoPP;

class SecByteBlockSink : public Bufferless<Sink>
{
public:
    SecByteBlockSink(SecByteBlock &block) : m_block(block) { }

    size_t Put2(const byte *inString, size_t length, int, bool)
    {
        if(!inString || !length) return length;

        size_t currentSize = m_block.size();
        m_block.Grow(currentSize+length);
        std::memcpy(m_block+currentSize, inString, length);

        return 0;
    }

private:
    SecByteBlock& m_block;
};

bool HasOptionalAttribute(const BufferedTransformation &bt, byte tag)
{
    byte b;
    if (bt.Peek(b) && b == tag)
        return true;
    return false;
}

inline bool IsRSAAlgorithm(const OID& alg)
{
    return alg == ASN1::rsaEncryption() ||  // rsaEncryption is most popular in spki
        (alg >= ASN1::rsaEncryption() && alg <= ASN1::sha512_256WithRSAEncryption());
}

inline bool IsDSAAlgorithm(const OID& alg)
{
    return alg == ASN1::id_dsa();
}

inline bool IsECDSAAlgorithm(const OID& alg)
{
    return alg == id_ecdsaWithSHA1 || alg == id_ecdsaWithSHA256 ||
        alg == id_ecdsaWithSHA384 || alg == id_ecdsaWithSHA512;
}

inline bool IsEd25519Algorithm(const OID& alg)
{
    return alg == ASN1::Ed25519();
}

inline bool IsECPrimeFieldAlgorithm(const OID& alg, const OID& field)
{
    if (alg != ASN1::id_ecPublicKey())
        return false;

    return field == ASN1::prime_field() ||
        (field >= ASN1::secp112r1() && field <= ASN1::secp521r1()) ||
        (field >= ASN1::secp192r1() && field <= ASN1::secp256r1()) ||  // not a typo
        (field >= ASN1::brainpoolP160r1() && field <= ASN1::brainpoolP512r1());
}

inline bool IsECBinaryFieldAlgorithm(const OID& alg, const OID& field)
{
    if (alg != ASN1::id_ecPublicKey())
        return false;

    return field == ASN1::characteristic_two_field();
}

ANONYMOUS_NAMESPACE_END

NAMESPACE_BEGIN(CryptoPP)

struct OidToName
{
    virtual ~OidToName() {};
    OidToName (const OID& o, const std::string& n) : oid(o), name(n) {}

    OID oid;
    std::string name;
};

struct OidToNameCompare
{
    bool operator() (const OidToName& first, const OidToName& second)
        { return (first.oid < second.oid); }
};

typedef std::vector<OidToName> OidToNameArray;

OidToNameArray GetOidToNameTable()
{
    // The names are mostly standard. Also see the various RFCs, and
    // X.520, Section 6, for a partial list of LDAP Names, and
    // https://www.iana.org/assignments/smi-numbers/smi-numbers.xhtml
    OidToNameArray table;
    table.reserve(96);

    table.push_back(OidToName(OID(1)+2+840+10045+2+1,   "ecPublicKey"));
    table.push_back(OidToName(OID(1)+2+840+10045+3+1+1, "secp192v1"));
    table.push_back(OidToName(OID(1)+2+840+10045+3+1+2, "secp192v2"));
    table.push_back(OidToName(OID(1)+2+840+10045+3+1+3, "secp192v3"));
    table.push_back(OidToName(id_secp256v1, "secp256v1"));

    table.push_back(OidToName(id_ecdsaWithSHA1,   "ecdsaWithSHA1"));
    table.push_back(OidToName(id_ecdsaWithSHA256, "ecdsaWithSHA256"));
    table.push_back(OidToName(id_ecdsaWithSHA384, "ecdsaWithSHA384"));
    table.push_back(OidToName(id_ecdsaWithSHA512, "ecdsaWithSHA512"));

    table.push_back(OidToName(OID(1)+3+132+0+33, "secp224r1"));
    table.push_back(OidToName(OID(1)+3+132+0+34, "secp384r1"));
    table.push_back(OidToName(OID(1)+3+132+0+35, "secp521r1"));

    table.push_back(OidToName(OID(1)+2+840+113549+1+1+1, "rsaEncryption"));
    table.push_back(OidToName(OID(1)+2+840+113549+1+1+2, "md2WithRSAEncryption"));
    table.push_back(OidToName(OID(1)+2+840+113549+1+1+3, "md4WithRSAEncryption"));
    table.push_back(OidToName(OID(1)+2+840+113549+1+1+4, "md5WithRSAEncryption"));
    table.push_back(OidToName(id_sha1WithRSASignature,   "sha1WithRSASignature"));
    table.push_back(OidToName(OID(1)+2+840+113549+1+1+6, "rsaOAEPEncryption"));
    table.push_back(OidToName(OID(1)+2+840+113549+1+1+7, "rsaAESOAEP"));
    table.push_back(OidToName(OID(1)+2+840+113549+1+1+10, "rsaSSAPSS"));
    table.push_back(OidToName(id_sha256WithRSAEncryption, "sha256WithRSAEncryption"));
    table.push_back(OidToName(id_sha384WithRSAEncryption, "sha384WithRSAEncryption"));
    table.push_back(OidToName(id_sha512WithRSAEncryption, "sha512WithRSAEncryption"));
    table.push_back(OidToName(OID(1)+2+840+113549+1+1+14, "sha224WithRSAEncryption"));
    table.push_back(OidToName(OID(1)+2+840+113549+1+1+15, "sha512-224WithRSAEncryption"));
    table.push_back(OidToName(OID(1)+2+840+113549+1+1+16, "sha512-256WithRSAEncryption"));

    table.push_back(OidToName(id_commonName,  "CN"));     // Common name
    table.push_back(OidToName(OID(2)+5+4+ 4,  "SN"));     // Surname
    table.push_back(OidToName(OID(2)+5+4+ 5,  "SERIALNUMBER"));  // Serial number
    table.push_back(OidToName(OID(2)+5+4+ 6,  "C"));      // Country
    table.push_back(OidToName(OID(2)+5+4+ 7,  "L"));      // Locality
    table.push_back(OidToName(OID(2)+5+4+ 8,  "ST"));     // State or province
    table.push_back(OidToName(OID(2)+5+4+ 9,  "STREET")); // Street address
    table.push_back(OidToName(OID(2)+5+4+10,  "O"));      // Organization
    table.push_back(OidToName(OID(2)+5+4+11,  "OU"));     // Organization unit
    table.push_back(OidToName(OID(2)+5+4+12,  "TITLE"));  // Title
    table.push_back(OidToName(OID(2)+5+4+13,  "DESCRIPTION"));    // Description
    table.push_back(OidToName(OID(2)+5+4+16,  "POSTALADDRESS"));  // Postal address
    table.push_back(OidToName(OID(2)+5+4+17,  "POSTALCODE"));     // Postal code
    table.push_back(OidToName(OID(2)+5+4+18,  "POSTOFFICEBOX"));  // Postal office box
    table.push_back(OidToName(OID(2)+5+4+20,  "TEL"));    // Phone number
    table.push_back(OidToName(OID(2)+5+4+23,  "FAX"));    // Fax number
    table.push_back(OidToName(OID(2)+5+4+35,   "USERPASSWORD"));        // User password
    table.push_back(OidToName(OID(2)+5+4+35+2, "ENCUSERPASSWORD"));     // Encrypted user password
    table.push_back(OidToName(OID(2)+5+4+36,   "USERCERTIFICATE"));     // User certificate
    table.push_back(OidToName(OID(2)+5+4+36+2, "ENCUSERCERTIFICATE"));  // Encrypted user certificate
    table.push_back(OidToName(OID(2)+5+4+37,   "CACERTIFICATE"));       // CA certificate
    table.push_back(OidToName(OID(2)+5+4+37+2, "ENCCACERTIFICATE"));    // Encrypted CA certificate
    table.push_back(OidToName(OID(2)+5+4+41,  "NAME"));   // Name
    table.push_back(OidToName(OID(2)+5+4+42,  "GN"));     // Given name
    table.push_back(OidToName(OID(2)+5+4+43,  "I"));      // Initials
    table.push_back(OidToName(OID(2)+5+4+44,  "GENERATION"));  // Generation qualifier+ Jr.+ Sr.+ etc
    table.push_back(OidToName(id_uniqueIdentifier, "UID"));    // X.500 Unique identifier
    table.push_back(OidToName(OID(2)+5+4+49,  "DN"));          // Distinguished name
    table.push_back(OidToName(OID(2)+5+4+51,  "HOUSE"));       // House identifier
    table.push_back(OidToName(OID(2)+5+4+65,  "PSEUDONYM"));   // Pseudonym
    table.push_back(OidToName(OID(2)+5+4+78,  "OID"));         // Object identifier
    table.push_back(OidToName(OID(2)+5+4+83,  "URI"));         // Uniform Resource Identifier
    table.push_back(OidToName(OID(2)+5+4+85,  "USERPWD"));     // URI user password
    table.push_back(OidToName(OID(2)+5+4+86,  "URN"));         // Uniform Resource Name
    table.push_back(OidToName(OID(2)+5+4+87,  "URL"));         // Uniform Resource Locator

    table.push_back(OidToName(id_subjectPublicKeyIdentifier, "SPKI")); // Subject public key identifier
    table.push_back(OidToName(id_keyUsage, "KU"));                     // Key use
    table.push_back(OidToName(id_subjectAltName, "SAN"));              // Subject alternate names
    table.push_back(OidToName(id_basicConstraints, "BC"));             // Basic constraints
    table.push_back(OidToName(OID(2)+5+29+30, "NC"));                  // Name constraints
    table.push_back(OidToName(id_authorityKeyIdentifier, "AKI"));      // Authority key identifier
    table.push_back(OidToName(id_extendedKeyUsage, "EKU"));            // Extended key use

    table.push_back(OidToName(id_netscapeServerName, "ssl-server-name"));   // Netscape server name
    table.push_back(OidToName(OID(2)+16+840+1+113730+1+13, "ns-comment"));  // Netscape comment

    table.push_back(OidToName(OID(0)+9+2342+19200300+100+1+1, "UID"));   // User Id
    table.push_back(OidToName(OID(0)+9+2342+19200300+100+1+25, "DC"));   // Domain component
    table.push_back(OidToName(id_pkcsEmail, "EMAIL"));          // Email address+ part of DN+ deprecated
    table.push_back(OidToName(id_msUserPrincipalName, "UPN"));  // Microsoft User Principal Name (UPN)
                                                                // Found in the SAN as [1] otherName

    std::sort(table.begin(), table.end(), OidToNameCompare());

    return table;
}

std::string OidToNameLookup(const OID& oid, const char *defaultName)
{
    static const OidToNameArray table = GetOidToNameTable();

    // Binary search
    size_t first  = 0;
    size_t last   = table.size() - 1;
    size_t middle = (first+last)/2;

    while (first <= last)
    {
        if (table[middle].oid < oid)
        {
            first = middle + 1;
        }
        else if (table[middle].oid == oid)
        {
            return table[middle].name;
        }
        else
            last = middle - 1;

        middle = (first + last)/2;
    }

    // Not found, return defaultName.
    if (defaultName != NULLPTR)
        return defaultName;

    std::ostringstream oss;
    oss << oid;
    return oss.str();
}

void RdnValue::BERDecode(BufferedTransformation &bt)
{
    BERSequenceDecoder seq(bt);

      m_oid.BERDecode(seq);

      byte b;
      if (seq.Peek(b) && ValidateTag(b))
      {
          m_tag = static_cast<ASNTag>(b);
          BERDecodeTextString(seq, m_value, b);
      }
      else
          BERDecodeError();

    seq.MessageEnd();
}

void RdnValue::DEREncode(BufferedTransformation &bt) const
{
    CRYPTOPP_UNUSED(bt);

    // TODO: Implement this function
    throw NotImplemented("RdnValue::DEREncode");
}

bool RdnValue::ValidateTag(byte tag) const
{
    if (tag == UTF8_STRING || tag == NUMERIC_STRING || tag == PRINTABLE_STRING ||
        tag == T61_STRING || tag == VIDEOTEXT_STRING || tag == IA5_STRING ||
        tag == VISIBLE_STRING || tag == GENERAL_STRING || tag == UNIVERSAL_STRING ||
        tag == BMP_STRING)
        return true;
    return false;
}

std::ostream& RdnValue::Print(std::ostream& out) const
{
    if (m_value.empty())
        { return out; }

    std::ostringstream oss;
    oss << OidToNameLookup(m_oid);
    oss << "=";
    oss << EncodeValue();

    return out << oss.str();
}

std::string RdnValue::EncodeValue() const
{
    if (m_value.empty())
        { return "\"\""; }

    bool quote = std::find(m_value.begin(), m_value.end(), byte(' ')) != m_value.end();

    std::string val;
    if (quote) val += "\"";
    val.append((const char*)ConstBytePtr(m_value), BytePtrSize(m_value));
    if (quote) val += "\"";

    return val;
}

void DateValue::BERDecode(BufferedTransformation &bt)
{
    byte b;
    if (bt.Peek(b) && ValidateTag(b))
    {
        m_tag = static_cast<ASNTag>(b);
        BERDecodeDate(bt, m_value, b);
    }
    else
        BERDecodeError();
}

void DateValue::DEREncode(BufferedTransformation &bt) const
{
    CRYPTOPP_UNUSED(bt);

    // TODO: Implement this function
    throw NotImplemented("DateValue::DEREncode");
}

bool DateValue::ValidateTag(byte tag) const
{
    if (tag == UTC_TIME || tag == GENERALIZED_TIME)
        return true;
    return false;
}

std::ostream& DateValue::Print(std::ostream& out) const
{
    if (m_value.empty())
        { return out; }

    return out << EncodeValue();
}

std::string DateValue::EncodeValue() const
{
    if (m_value.empty())
        { return ""; }

    return std::string((const char*)ConstBytePtr(m_value), BytePtrSize(m_value));
}

void ExtensionValue::BERDecode(BufferedTransformation &bt)
{
    BERSequenceDecoder seq(bt);
      m_oid.BERDecode(seq);

      m_critical = false;
      if (HasOptionalAttribute(seq, BOOLEAN))
      {
          word32 flag;
          BERDecodeUnsigned(seq, flag, BOOLEAN);
          m_critical = !!flag;
      }

      BERDecodeOctetString(seq, m_value);

    seq.MessageEnd();
}

void ExtensionValue::DEREncode(BufferedTransformation &bt) const
{
    CRYPTOPP_UNUSED(bt);

    // TODO: Implement this function
    throw NotImplemented("ExtensionValue::DEREncode");
}

bool ExtensionValue::ValidateTag(byte tag) const
{
    CRYPTOPP_UNUSED(tag);
    return true;
}

std::ostream& ExtensionValue::Print(std::ostream& out) const
{
    if (m_value.empty())
        { return out; }

    return out << EncodeValue();
}

std::string ExtensionValue::EncodeValue() const
{
    // TODO: Implement this function
    throw NotImplemented("ExtensionValue::EncodeValue");
    return "";
}

void KeyIdentifierValue::BERDecode(BufferedTransformation &bt)
{
    byte tag;
    if (!bt.Peek(tag) || !ValidateTag(tag))
        BERDecodeError();

    if (tag == (CONSTRUCTED | SEQUENCE))
    {
        // Authority key identifier
        BERSequenceDecoder seq(bt);
          if (HasOptionalAttribute(seq, CONTEXT_SPECIFIC|0))
          {
              BERSequenceDecoder dec(seq, CONTEXT_SPECIFIC|0);
                m_value.New(dec.MaxRetrievable());
                dec.Get(BytePtr(m_value), BytePtrSize(m_value));
              dec.MessageEnd();
          }
#if 0
          if (HasOptionalAttribute(seq, CONSTRUCTED|CONTEXT_SPECIFIC|1))
          {
              BERGeneralDecoder dec(seq, CONSTRUCTED|CONTEXT_SPECIFIC|1);
                RdnValueArray dn;
                while (! dec.EndReached())
                {
                    BERSetDecoder set(dec);
                      RdnValue value;
                      value.BERDecode(set);
                      dn.push_back(value);
                    set.MessageEnd();
                }
              dec.MessageEnd();
          }
#endif
          if (seq.EndReached() == false)
          {
              m_other.New(seq.MaxRetrievable());
              seq.Get(BytePtr(m_other), BytePtrSize(m_other));
          }
        seq.MessageEnd();

        m_type = KeyIdentifierValue::Hash;
        m_oid = id_authorityKeyIdentifier;
    }
    else if (tag == OCTET_STRING)
    {
        // Subject key identifier
        BERDecodeOctetString(bt, m_value);

        m_type = KeyIdentifierValue::Hash;
        m_oid = id_subjectPublicKeyIdentifier;
    }
    else
        BERDecodeError();
}

void KeyIdentifierValue::DEREncode(BufferedTransformation &bt) const
{
    CRYPTOPP_UNUSED(bt);

    // TODO: Implement this function
    throw NotImplemented("KeyIdentifierValue::DEREncode");
}

bool KeyIdentifierValue::ValidateTag(byte tag) const
{
    return (tag == (CONSTRUCTED | SEQUENCE)) || tag == OCTET_STRING;
}

std::ostream& KeyIdentifierValue::Print(std::ostream& out) const
{
    if (m_value.empty())
        { return out; }

    return out << EncodeValue();
}

std::string KeyIdentifierValue::EncodeValue() const
{
    std::string val;
    if (m_type == Hash)
    {
        val += "hash: ";

        HexEncoder encoder(new StringSink(val));
        encoder.Put(ConstBytePtr(m_value), BytePtrSize(m_value));
        encoder.MessageEnd();
    }
    else if (m_type == DnAndSn)
    {
        val += "name and serno: ";

        // TODO: finish this once we have a cert
        // val += "XXX, NNN";

        HexEncoder encoder(new StringSink(val));
        encoder.Put(ConstBytePtr(m_value), BytePtrSize(m_value));
        encoder.MessageEnd();
    }

    return val;
}

IdentityValue::IdentityValue(const SecByteBlock& value, IdentityEnum src)
    : m_value(value), m_src(src)
{
    if (m_src == otherName)
        { ConvertOtherName(); }
}

IdentityValue::IdentityValue(const std::string &value, IdentityEnum src)
    : m_value(ConstBytePtr(value), BytePtrSize(value)), m_src(src)
{
    if (m_src == otherName)
        { ConvertOtherName(); }
}

IdentityValue::IdentityValue(const OID& oid, const SecByteBlock& value, IdentityEnum src)
    : m_oid(oid), m_value(value), m_src(src)
{
    if (m_src == otherName)
        { ConvertOtherName(); }
}

IdentityValue::IdentityValue(const OID& oid, const std::string &value, IdentityEnum src)
    : m_oid(oid), m_value(ConstBytePtr(value), BytePtrSize(value)), m_src(src)
{
    if (m_src == otherName)
        { ConvertOtherName(); }
}

std::ostream& IdentityValue::Print(std::ostream& out) const
{
    if (m_value.empty())
        { return out; }

    std::ostringstream oss;
    oss << OidToNameLookup(m_oid);
    oss << ": ";
    oss << EncodeValue();

    return out << oss.str();
}

std::string IdentityValue::EncodeValue() const
{
    std::string val;

    switch (m_src)
    {
        case UniqueId:
        case SubjectPKI:
        {
            HexEncoder encoder(new StringSink(val));
            encoder.Put(ConstBytePtr(m_value), BytePtrSize(m_value));
            encoder.MessageEnd();
            break;
        }
        case iPAddress:
        {
            if (m_value.size() == 4)  // ipv4
            {
                std::ostringstream oss;
                for (size_t i=0; i<3; ++i)
                    oss << (unsigned int)m_value[i] << ".";
                oss << (unsigned int)m_value[3];
                val = oss.str();
            }
            else  // ipv6
            {
                HexEncoder encoder(new StringSink(val), true, 2, ":");
                encoder.Put(ConstBytePtr(m_value), BytePtrSize(m_value));
                encoder.MessageEnd();
            }
            break;
        }
        case registeredID:
        {
            OID oid;
            ArraySource store(ConstBytePtr(m_value), BytePtrSize(m_value), true);
            oid.BERDecode(store);

            std::ostringstream oss;
            oss << oid;
            val = oss.str();
            break;
        }
        default:
            val.resize(m_value.size());
            std::memcpy(BytePtr(val), ConstBytePtr(m_value), BytePtrSize(val));
    }

    return val;
}

// Micosoft PKI can include a User Principal Name in the otherName
// https://security.stackexchange.com/q/62746/29925. For the ASN.1
// see https://tools.ietf.org/html/rfc4556, Appendix A, Appendix C,
// and id-ms-san-sc-logon-upn.
void IdentityValue::ConvertOtherName()
{
    CRYPTOPP_ASSERT(m_src == otherName);
    if (m_src != otherName) { return; }

    // TODO: fix this when we get a test cert
    if (m_value[0] == OBJECT_IDENTIFIER)
    {
        SecByteBlock temp(m_value);
        ArraySource store(ConstBytePtr(temp), BytePtrSize(temp), true);
        OID oid; oid.BERDecode(store);

        const OID msUPN = id_msUserPrincipalName;
        if (oid == msUPN)  // Turn this object into a MS UPN
        {
            try
            {
                BERSequenceDecoder seq(store);
                  BERDecodeTextString(seq, m_value, UTF8_STRING);
                seq.MessageEnd();

                m_oid = msUPN;
                m_src = msOtherNameUPN;
            }
            catch (Exception& ex)
            {
                CRYPTOPP_UNUSED(ex);
                CRYPTOPP_ASSERT(0);
            }
        }
    }
}

KeyUsageValue::KeyUsageValue(const OID& oid)
    : m_oid(oid), m_usage(InvalidKeyUsage)
{
    if (oid == OID(1)+3+6+1+5+5+7+3+1)
        { m_usage = KeyUsageValue::serverAuth; }
    else if (oid == OID(1)+3+6+1+5+5+7+3+2)
        { m_usage = KeyUsageValue::clientAuth; }
    else if (oid == OID(1)+3+6+1+5+5+7+3+3)
        { m_usage = KeyUsageValue::codeSigning; }
    else if (oid == OID(1)+3+6+1+5+5+7+3+4)
        { m_usage = KeyUsageValue::emailProtection; }
    else if (oid == OID(1)+3+6+1+5+5+7+3+5)
        { m_usage = KeyUsageValue::ipsecEndSystem; }
    else if (oid == OID(1)+3+6+1+5+5+7+3+6)
        { m_usage = KeyUsageValue::ipsecTunnel; }
    else if (oid == OID(1)+3+6+1+5+5+7+3+7)
        { m_usage = KeyUsageValue::ipsecUser; }
    else if (oid == OID(1)+3+6+1+5+5+7+3+8)
        { m_usage = KeyUsageValue::timeStamping; }
    else if (oid == OID(1)+3+6+1+5+5+7+3+9)
        { m_usage = KeyUsageValue::OCSPSigning; }
    else if (oid == OID(1)+3+6+1+5+5+7+3+10)
        { m_usage = KeyUsageValue::dvcs; }
    else if (oid == OID(1)+3+6+1+5+5+7+3+13)
        { m_usage = KeyUsageValue::eapOverPPP; }
    else if (oid == OID(1)+3+6+1+5+5+7+3+14)
        { m_usage = KeyUsageValue::eapOverLAN; }
    else if (oid == OID(1)+3+6+1+5+5+7+3+17)
        { m_usage = KeyUsageValue::ipsecIKE; }
    else if (oid == OID(1)+3+6+1+5+5+7+3+20)
        { m_usage = KeyUsageValue::sipDomain; }
    else if (oid == OID(1)+3+6+1+5+5+7+3+21)
        { m_usage = KeyUsageValue::secureShellClient; }
    else if (oid == OID(1)+3+6+1+5+5+7+3+22)
        { m_usage = KeyUsageValue::secureShellServer; }
    else if (oid == OID(1)+3+6+1+5+5+7+3+27)
        { m_usage = KeyUsageValue::cmcCA; }
    else if (oid == OID(1)+3+6+1+5+5+7+3+28)
        { m_usage = KeyUsageValue::cmcRA; }
    else if (oid == OID(1)+3+6+1+5+5+7+3+29)
        { m_usage = KeyUsageValue::cmcArchive; }
}

KeyUsageValue::KeyUsageValue(const OID& oid, KeyUsageEnum usage)
    : m_oid(oid), m_usage(usage)
{
}

void KeyUsageValue::BERDecode(BufferedTransformation &bt)
{
    CRYPTOPP_UNUSED(bt);

    // TODO: Implement this function
    throw NotImplemented("KeyUsageValue::BERDecode");
}

void KeyUsageValue::DEREncode(BufferedTransformation &bt) const
{
    CRYPTOPP_UNUSED(bt);

    // TODO: Implement this function
    throw NotImplemented("KeyUsageValue::DEREncode");
}

std::ostream& KeyUsageValue::Print(std::ostream& out) const
{
    if (m_oid.Empty())
        { return out; }

    return out << EncodeValue();
}

std::string KeyUsageValue::EncodeValue() const
{
    std::string val;

    switch (m_usage)
    {
        case digitalSignature:
            val = "digitalSignature";
            break;

        case nonRepudiation:
            val = "nonRepudiation";
            break;

        case keyEncipherment:
            val = "keyEncipherment";
            break;

        case dataEncipherment:
            val = "dataEncipherment";
            break;

        case keyAgreement:
            val = "keyAgreement";
            break;

        case keyCertSign:
            val = "keyCertSign";
            break;

        case cRLSign:
            val = "cRLSign";
            break;

        case encipherOnly:
            val = "encipherOnly";
            break;

        case decipherOnly:
            val = "decipherOnly";
            break;

        case serverAuth:
            val = "serverAuth";
            break;

        case clientAuth:
            val = "clientAuth";
            break;

        case codeSigning:
            val = "codeSigning";
            break;

        case emailProtection:
            val = "emailProtection";
            break;

        case ipsecEndSystem:
            val = "ipsecEndSystem";
            break;

        case ipsecTunnel:
            val = "ipsecTunnel";
            break;

        case ipsecUser:
            val = "ipsecUser";
            break;

        case timeStamping:
            val = "timeStamping";
            break;

        case OCSPSigning:
            val = "OCSPSigning";
            break;

        case dvcs:
            val = "dvcs";
            break;

        case sbgpCertAAServerAuth:
            val = "sbgpCertAAServerAuth";
            break;

        case scvpResponder:
            val = "scvpResponder";
            break;

        case eapOverPPP:
            val = "eapOverPPP";
            break;

        case eapOverLAN:
            val = "eapOverLAN";
            break;

        case scvpServer:
            val = "scvpServer";
            break;

        case scvpClient:
            val = "scvpClient";
            break;

        case ipsecIKE:
            val = "ipsecIKE";
            break;

        case capwapAC:
            val = "capwapAC";
            break;

        case capwapWTP:
            val = "capwapWTP";
            break;

        case sipDomain:
            val = "sipDomain";
            break;

        case secureShellClient:
            val = "secureShellClient";
            break;

        case secureShellServer:
            val = "secureShellServer";
            break;

        case sendRouter:
            val = "sendRouter";
            break;

        case sendProxiedRouter:
            val = "sendProxiedRouter";
            break;

        case sendOwner:
            val = "sendOwner";
            break;

        case sendProxiedOwner:
            val = "sendProxiedOwner";
            break;

        case cmcCA:
            val = "cmcCA";
            break;

        case cmcRA:
            val = "cmcRA";
            break;

        case bgpsecRouter:
            val = "bgpsecRouter";
            break;

        case brandIndicatorforMessageIdentification:
            val = "brandIndicatorforMessageIdentification";
            break;

        default:
        {
            std::ostringstream oss;
            oss << m_oid;
            val = oss.str();

            break;
        }
    }

    return val;
}

void BasicConstraintValue::BERDecode(BufferedTransformation &bt)
{
    // BasicConstraints ::= SEQUENCE {
    //   cA                    BOOLEAN DEFAULT FALSE,
    //   pathLenConstraint     INTEGER (0..MAX) OPTIONAL
    // }

    BERSequenceDecoder seq(bt);
      if (HasOptionalAttribute(seq, BOOLEAN))
      {
          word32 flag;
          BERDecodeUnsigned(seq, flag, BOOLEAN);
          m_ca = !!flag;
      }
      if (HasOptionalAttribute(seq, INTEGER))
      {
          word32 len;
          BERDecodeUnsigned(seq, len, INTEGER);
          m_pathLen = len;
      }
    seq.MessageEnd();
}

void BasicConstraintValue::DEREncode(BufferedTransformation &bt) const
{
    CRYPTOPP_UNUSED(bt);

    // TODO: Implement this function
    throw NotImplemented("BasicConstraintValue::DEREncode");
}

// Null values so some accessors can return something
const SecByteBlock  X509Certificate::g_nullByteBlock;
const ExtensionValueArray X509Certificate::g_nullExtensions;

void X509Certificate::Save(BufferedTransformation &bt) const
{
    DEREncode(bt);
}

void X509Certificate::Load(BufferedTransformation &bt)
{
    BERDecode(bt);
}

void X509Certificate::Reset()
{
    m_origCertificate.New(0);
    m_certSignature.New(0);

    m_toBeSigned.release();
    m_keyUsage.release();
    m_identities.release();
    m_extensions.release();
    m_subjectUid.release();
    m_issuerUid.release();
    m_subjectPublicKey.release();
    m_subjectKeyIdentifier.release();
    m_authorityKeyIdentifier.release();

    m_subjectName.clear();
    m_issuerName.clear();

    m_certSignatureAlgortihm = OID();
    m_subjectSignatureAlgortihm = OID();
    m_notBefore = m_notAfter = DateValue();

    m_version = v1;
    m_serialNumber = 0;
}

void X509Certificate::SaveCertificateBytes(BufferedTransformation &bt)
{
    m_origCertificate.resize(bt.MaxRetrievable());
    bt.Peek(m_origCertificate, m_origCertificate.size());
}

bool X509Certificate::HasOptionalAttribute(const BufferedTransformation &bt, byte tag) const
{
    byte b = 0;
    if (bt.Peek(b) && b == tag)
        return true;
    return false;
}

const SecByteBlock& X509Certificate::GetToBeSigned() const
{
    if (m_toBeSigned.get() == NULLPTR)
    {
        m_toBeSigned.reset(new SecByteBlock);
        SecByteBlock &toBeSigned = *m_toBeSigned.get();

        ArraySource store(m_origCertificate, m_origCertificate.size(), true);
        BERSequenceDecoder cert(store);

          // This gyration determines how many octets are used for
          // tag and length under DER encoding. Then, len octets are
          // transferred to toBeSigned. If tbsCertificate is not DER
          // encoded, then this could break. Note the RFC requires
          // DER encoding, so it is not a problem in practice.
          size_t len = BERDecodePeekLength(cert);
          len += 1 /*SEQ*/ + DERLengthEncode(TheBitBucket(), len);

          toBeSigned.New(len);
          cert.Get(BytePtr(toBeSigned), BytePtrSize(toBeSigned));
          cert.SkipAll();  // Remaining octets are not needed.
        cert.MessageEnd();
    }

    return *m_toBeSigned.get();
}

// RFC 5280, Appendix A, pp. 112-116
void X509Certificate::BERDecode(BufferedTransformation &bt)
{
    // Clear old certificate data
    Reset();

    // Stash a copy of the certificate.
    SaveCertificateBytes(bt);

    BERSequenceDecoder certificate(bt);

      BERSequenceDecoder tbsCertificate(certificate);

        if (HasOptionalAttribute(tbsCertificate, CONTEXT_SPECIFIC|CONSTRUCTED|0))
            BERDecodeVersion(tbsCertificate, m_version);
        else
            m_version = v1;  // Default per RFC

        BERDecodeSerialNumber(tbsCertificate, m_serialNumber);

        BERDecodeSignatureAlgorithm(tbsCertificate, m_subjectSignatureAlgortihm);

        BERDecodeDistinguishedName(tbsCertificate, m_issuerName);

        BERDecodeValidity(tbsCertificate, m_notBefore, m_notAfter);

        BERDecodeDistinguishedName(tbsCertificate, m_subjectName);

        BERDecodeSubjectPublicKeyInfo(tbsCertificate, m_subjectPublicKey);

        if (m_version < v2 || tbsCertificate.EndReached())
            goto TBS_Done;

        // UniqueIdentifiers are v2
        if (HasOptionalAttribute(tbsCertificate, CONTEXT_SPECIFIC|CONSTRUCTED|1))
            BERDecodeIssuerUniqueId(tbsCertificate);

        if (tbsCertificate.EndReached())
            goto TBS_Done;

        if (HasOptionalAttribute(tbsCertificate, CONTEXT_SPECIFIC|CONSTRUCTED|2))
            BERDecodeSubjectUniqueId(tbsCertificate);

        if (m_version < v3 || tbsCertificate.EndReached())
            goto TBS_Done;

        // Extensions are v3
        if (HasOptionalAttribute(tbsCertificate, CONTEXT_SPECIFIC|CONSTRUCTED|3))
            BERDecodeExtensions(tbsCertificate);

    TBS_Done:

      tbsCertificate.MessageEnd();

      BERDecodeSignatureAlgorithm(certificate, m_certSignatureAlgortihm);

      BERDecodeSignature(certificate, m_certSignature);

    certificate.MessageEnd();
}

void X509Certificate::DEREncode(BufferedTransformation &bt) const
{
    CRYPTOPP_UNUSED(bt);

    // TODO: Implement this function
    throw NotImplemented("X509Certificate::DEREncode");
}

void X509Certificate::BERDecodeSignature(BufferedTransformation &bt, SecByteBlock &signature)
{
    word32 unused;
    BERDecodeBitString(bt, signature, unused);
    CRYPTOPP_ASSERT(unused == 0);

    // For ECDSA, convert from DER to P1363 so the signature is ready to use.
    const OID &algorithm = GetCertificateSignatureAlgorithm();
    if (IsECDSAAlgorithm(algorithm))
    {
        const X509PublicKey &publicKey = GetSubjectPublicKey();
        member_ptr<PK_Verifier> verifier(GetPK_VerifierObject(algorithm, publicKey));
        CRYPTOPP_ASSERT(verifier.get());

        size_t size = verifier->SignatureLength();
        SecByteBlock p1363Signature(size);

        // https://www.cryptopp.com/wiki/DSAConvertSignatureFormat
        size = DSAConvertSignatureFormat(
                    p1363Signature, p1363Signature.size(), DSA_P1363,
                    signature, signature.size(), DSA_DER);
        p1363Signature.resize(size);

        std::swap(signature, p1363Signature);
    }
}

void X509Certificate::BERDecodeIssuerUniqueId(BufferedTransformation &bt)
{
    CRYPTOPP_ASSERT(HasOptionalAttribute(bt, CONTEXT_SPECIFIC|CONSTRUCTED|1));

    m_issuerUid.reset(new SecByteBlock);
    SecByteBlock temp;

    BERGeneralDecoder seq(bt, CONTEXT_SPECIFIC|CONSTRUCTED|1);
      if (! seq.EndReached())  // Guard for a1:00
      {
          word32 unused;
          BERDecodeBitString(bt, temp, unused);
          CRYPTOPP_ASSERT(unused == 0);
      }
    seq.MessageEnd();

    std::swap(temp, *m_issuerUid.get());
}

void X509Certificate::BERDecodeSubjectUniqueId(BufferedTransformation &bt)
{
    CRYPTOPP_ASSERT(HasOptionalAttribute(bt, CONTEXT_SPECIFIC|CONSTRUCTED|2));

    m_subjectUid.reset(new SecByteBlock);
    SecByteBlock temp;

    BERGeneralDecoder seq(bt, CONTEXT_SPECIFIC|CONSTRUCTED|2);
      if (! seq.EndReached())  // Guard for a2:00
      {
          word32 unused;
          BERDecodeBitString(bt, temp, unused);
          CRYPTOPP_ASSERT(unused == 0);
      }
    seq.MessageEnd();

    std::swap(temp, *m_subjectUid.get());
}

void X509Certificate::BERDecodeExtensions(BufferedTransformation &bt)
{
    CRYPTOPP_ASSERT(HasOptionalAttribute(bt, CONTEXT_SPECIFIC|CONSTRUCTED|3));

    m_extensions.reset(new ExtensionValueArray);
    ExtensionValueArray temp;

    BERGeneralDecoder extensions(bt, CONTEXT_SPECIFIC|CONSTRUCTED|3);
      if (! extensions.EndReached())  // Guard for a3:00
      {
          BERSequenceDecoder seq(extensions);
            while (! seq.EndReached())
            {
                ExtensionValue value;
                value.BERDecode(seq);
                temp.push_back(value);
            }
          seq.MessageEnd();
      }

    extensions.MessageEnd();

    std::swap(temp, *m_extensions.get());
}

void X509Certificate::BERDecodeSubjectPublicKeyInfo(BufferedTransformation &bt, member_ptr<X509PublicKey>& publicKey)
{
    OID algorithm;  // Public key algorithm
    OID field;      // Field for elliptic curves

    // See the comments for BERDecodeSubjectPublicKeyInfo for
    // why we are not using m_subjectSignatureAlgortihm.
    GetSubjectPublicKeyInfoOids(bt, algorithm, field);

    if (IsRSAAlgorithm(algorithm))
        publicKey.reset(new RSA::PublicKey);
    else if (IsDSAAlgorithm(algorithm))
        publicKey.reset(new DSA::PublicKey);
    else if (IsEd25519Algorithm(algorithm))
        publicKey.reset(new ed25519PublicKey);
    else if (IsECPrimeFieldAlgorithm(algorithm, field))
        publicKey.reset(new DL_PublicKey_EC<ECP>);
    else if (IsECBinaryFieldAlgorithm(algorithm, field))
        publicKey.reset(new DL_PublicKey_EC<EC2N>);
    else
    {
        std::ostringstream oss;
        oss << "X509Certificate::BERDecodeSubjectPublicKeyInfo: ";
        if (field.Empty() == false) {
            oss << "Field " << field << " is not supported";
        } else {
            oss << "Algorithm " << algorithm << " is not supported";
        }
        throw NotImplemented(oss.str());
    }

    publicKey->Load(bt);

#if defined(PEM_KEY_OR_PARAMETER_VALIDATION) && !defined(NO_OS_DEPENDENCE)
    AutoSeededRandomPool prng;
    publicKey->Validate(prng, 3);
#endif
}

// BERDecodeSubjectPublicKeyInfo peeks at the subjectPublicKeyInfo because the
// information is less ambiguous. If we used subjectPublicKeyAlgorithm we would
// still need to peek because subjectPublicKeyAlgorithm lacks field information
// (prime vs. binary). We need a field to instantiate a key. For example,
// subjectPublicKeyAlgorithm==ecdsa_with_sha384() does not contain enough
// information to determine PublicKey_EC<ECP> or PublicKey_EC<EC2N>.
void X509Certificate::GetSubjectPublicKeyInfoOids(const BufferedTransformation &bt, OID& algorithm, OID& field) const
{
    try
    {
        // We need to read enough of the stream to determine the OIDs.
        ByteQueue temp;
        bt.CopyTo(temp, BERDecodePeekLength(bt));

        BERSequenceDecoder seq1(temp);
          BERSequenceDecoder seq2(seq1);
            algorithm.BERDecode(seq2);
            // EC Public Keys specify a field, also
            if (algorithm == ASN1::id_ecPublicKey())
                { field.BERDecode(seq2); }
            seq2.SkipAll();
          seq2.MessageEnd();
          seq1.SkipAll();
        seq1.MessageEnd();
    }
    catch (const BERDecodeErr&)
    {
    }
}

void X509Certificate::BERDecodeValidity(BufferedTransformation &bt, DateValue &notBefore, DateValue &notAfter)
{
    BERSequenceDecoder validitiy(bt);
      notBefore.BERDecode(validitiy);
      notAfter.BERDecode(validitiy);
    validitiy.MessageEnd();
}

void X509Certificate::BERDecodeDistinguishedName(BufferedTransformation &bt, RdnValueArray &rdnArray)
{
    // The name is a RDNSequence. It looks like:
    //   SEQUENCE {
    //     SET {
    //       SEQUENCE {
    //         OBJECT IDENTIFIER countryName (2 5 4 6)
    //         PrintableString 'US'
    //       }
    //     }
    //     SET {
    //       SEQUENCE {
    //         OBJECT IDENTIFIER stateOrProvinceName (2 5 4 8)
    //         UTF8String 'NY'
    //       }
    //     }
    //     ...
    //   }

    RdnValueArray temp;
    BERSequenceDecoder rdnSequence(bt);

      while (! rdnSequence.EndReached())
      {
          BERSetDecoder set(rdnSequence);
            RdnValue value;
            value.BERDecode(set);
            temp.push_back(value);
          set.MessageEnd();
      }

    rdnSequence.MessageEnd();

    std::swap(temp, rdnArray);
}

void X509Certificate::BERDecodeSignatureAlgorithm(BufferedTransformation &bt, OID &algorithm)
{
    BERSequenceDecoder seq(bt);
      algorithm.BERDecode(seq);
      bool parametersPresent = seq.EndReached() ? false : BERDecodeAlgorithmParameters(seq);
      // TODO: Figure out what to do here???
      CRYPTOPP_ASSERT(parametersPresent == false);
      CRYPTOPP_UNUSED(parametersPresent);
    seq.MessageEnd();
}

void X509Certificate::BERDecodeVersion(BufferedTransformation &bt, Version &version)
{
    CRYPTOPP_ASSERT(HasOptionalAttribute(bt, CONTEXT_SPECIFIC|CONSTRUCTED|0));

    word32 value = v1;  // Default per RFC
    BERGeneralDecoder seq(bt, CONTEXT_SPECIFIC|CONSTRUCTED|0);
      if (! seq.EndReached())  // Guard for a0:00
      {
          BERDecodeUnsigned<word32>(seq, value, INTEGER, 0, 2);  // check version
      }
    seq.MessageEnd();

    version = static_cast<Version>(value);
}

void X509Certificate::BERDecodeSerialNumber(BufferedTransformation &bt, Integer &serno)
{
    serno.BERDecode(bt);
}

bool X509Certificate::Validate(RandomNumberGenerator &rng, unsigned int level) const
{
    // TODO: add more tests
    bool pass = true, valid;

    valid = m_subjectPublicKey->Validate(rng, level);
    pass = valid && pass;
    CRYPTOPP_ASSERT(valid);

    if (IsSelfSigned() && level >= 1)
    {
        const X509PublicKey &publicKey = GetSubjectPublicKey();
        valid = ValidateSignature(rng, publicKey);
        pass = valid && pass;
        CRYPTOPP_ASSERT(valid);
    }

    return pass;
}

bool X509Certificate::ValidateSignature (RandomNumberGenerator &rng, const X509PublicKey &key) const
{
    CRYPTOPP_UNUSED(rng);

    const OID &algorithm = GetCertificateSignatureAlgorithm();
    member_ptr<PK_Verifier> verifier(GetPK_VerifierObject(algorithm, key));
    CRYPTOPP_ASSERT(verifier.get());

    const SecByteBlock &signature = GetCertificateSignature();
    const SecByteBlock &toBeSigned = GetToBeSigned();

    bool valid = verifier->VerifyMessage(toBeSigned, toBeSigned.size(), signature, signature.size());
    CRYPTOPP_ASSERT(valid);

    return valid;
}

// Get the verifier object for an algorithm and key.
PK_Verifier* X509Certificate::GetPK_VerifierObject(const OID &algorithm, const X509PublicKey &key) const
{
    member_ptr<PK_Verifier> verifier;

    if (algorithm == id_sha1WithRSASignature)
    {
        verifier.reset(new RSASS<PKCS1v15, SHA1>::Verifier(key));
    }
    else if (algorithm == id_sha256WithRSAEncryption)
    {
        verifier.reset(new RSASS<PKCS1v15, SHA256>::Verifier(key));
    }
    else if (algorithm == id_sha384WithRSAEncryption)
    {
        verifier.reset(new RSASS<PKCS1v15, SHA384>::Verifier(key));
    }
    else if (algorithm == id_sha512WithRSAEncryption)
    {
        verifier.reset(new RSASS<PKCS1v15, SHA512>::Verifier(key));
    }
    else if (algorithm == id_ecdsaWithSHA1)
    {
        verifier.reset(new ECDSA<ECP, SHA1>::Verifier(key));
    }
    else if (algorithm == id_ecdsaWithSHA256)
    {
        verifier.reset(new ECDSA<ECP, SHA256>::Verifier(key));
    }
    else if (algorithm == id_ecdsaWithSHA384)
    {
        verifier.reset(new ECDSA<ECP, SHA384>::Verifier(key));
    }
    else if (algorithm == id_ecdsaWithSHA512)
    {
        verifier.reset(new ECDSA<ECP, SHA512>::Verifier(key));
    }
    else
    {
        CRYPTOPP_ASSERT(0);
    }

    return verifier.release();
}

void X509Certificate::AssignFrom(const NameValuePairs &source)
{
    ConstByteArrayParameter val;
    if (source.GetValue("Certificate", val))
    {
        ArraySource certificate(val.begin(), val.size(), true);
        Load(certificate);
    }
}

bool X509Certificate::GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const
{
    if (std::strcmp(name, "Certificate") == 0)
    {
        this->ThrowIfTypeMismatch(name, typeid(ConstByteArrayParameter), valueType);
        reinterpret_cast<ConstByteArrayParameter*>(pValue)->Assign(m_origCertificate, m_origCertificate.size(), false);
        return true;
    }
    return false;
}

bool X509Certificate::FindExtension(const OID& oid, ExtensionValueArray::const_iterator& loc) const
{
    if (HasExtensions() == false)
    {
        // Set loc to something
        loc = g_nullExtensions.end();
        return false;
    }

    ExtensionValueArray::const_iterator first = m_extensions.get()->begin();
    ExtensionValueArray::const_iterator last = m_extensions.get()->end();

    while (first != last)
    {
        if (first->m_oid == oid) {
            loc = first;
            return true;
        }
        ++first;
    }
    loc = last;
    return false;
}

bool X509Certificate::HasIssuerUniqueId() const
{
    return m_issuerUid.get() != NULLPTR;
}

bool X509Certificate::HasSubjectUniqueId() const
{
    return m_subjectUid.get() != NULLPTR;
}

bool X509Certificate::HasExtensions() const
{
    return m_extensions.get() != NULLPTR;
}

bool X509Certificate::HasAuthorityKeyIdentifier() const
{
    if (HasExtensions() == false)
        { return false; }

    ExtensionValueArray::const_iterator unused;
    return FindExtension(id_authorityKeyIdentifier, unused);
}

bool X509Certificate::HasSubjectKeyIdentifier() const
{
    if (HasExtensions() == false)
        { return false; }

    ExtensionValueArray::const_iterator unused;
    return FindExtension(id_subjectPublicKeyIdentifier, unused);
}

bool X509Certificate::IsCertificateAuthority() const
{
    // BasicConstraints ::= SEQUENCE {
    //   cA                    BOOLEAN DEFAULT FALSE,
    //   pathLenConstraint     INTEGER (0..MAX) OPTIONAL
    // }

    ExtensionValueArray::const_iterator loc;
    if (FindExtension(id_basicConstraints, loc))
    {
        const ExtensionValue& ext = *loc;
        BasicConstraintValue basicConstraints(ext.m_critical);

        ArraySource store(ext.m_value, ext.m_value.size(), true);
        basicConstraints.BERDecode(store);

        return basicConstraints.m_ca;
    }

    return false;
}

bool X509Certificate::IsSelfSigned() const
{
    // IssuerUID and SubjectUID are optional
    if (HasIssuerUniqueId() && HasSubjectUniqueId() &&
        GetIssuerUniqueId() == GetSubjectUniqueId())
        return true;

    // AKI and SPKI are optional
    if (HasAuthorityKeyIdentifier() && HasSubjectKeyIdentifier() &&
        GetAuthorityKeyIdentifier() == GetSubjectKeyIdentifier())
        return true;

    // Distinguished Names
    bool same = false;
    if (m_issuerName.size() == m_subjectName.size())
    {
        RdnValueArray::const_iterator a = m_issuerName.begin();
        RdnValueArray::const_iterator b = m_subjectName.begin();
        RdnValueArray::const_iterator c = m_issuerName.end();

        same = true;
        while (a != c)
        {
            same = (a->m_value == b->m_value) && same;
            ++a; ++b;
        }
    }

    return same;
}

const KeyIdentifierValue& X509Certificate::GetAuthorityKeyIdentifier() const
{
    // OCTET STRING, encapsulates {
    //   SEQUENCE {
    //     [0]
    //       AE 22 75 36 0B F0 D2 37 CB D2 AB 5B 47 B7 9E B0
    //       ED 15 E5 9A
    //   }
    // }

    if (m_authorityKeyIdentifier.get() == NULLPTR)
    {
        m_authorityKeyIdentifier.reset(new KeyIdentifierValue);
        if (HasExtensions())
        {
            ExtensionValueArray::const_iterator loc;
            if (FindExtension(id_authorityKeyIdentifier, loc))
            {
                const ExtensionValue& ext = *loc;
                KeyIdentifierValue& identifier = *m_authorityKeyIdentifier.get();

                ArraySource store(ext.m_value, ext.m_value.size(), true);
                identifier.BERDecode(store);
            }
        }
    }

    return *m_authorityKeyIdentifier.get();
}

const KeyIdentifierValue& X509Certificate::GetSubjectKeyIdentifier() const
{
    // OCTET STRING, encapsulates {
    //   OCTET STRING
    //     AE 22 75 36 0B F0 D2 37 CB D2 AB 5B 47 B7 9E B0
    //     ED 15 E5 9A
    //   }
    // }

    if (m_subjectKeyIdentifier.get() == NULLPTR)
    {
        m_subjectKeyIdentifier.reset(new KeyIdentifierValue);
        if (HasExtensions())
        {
            ExtensionValueArray::const_iterator loc;
            if (FindExtension(id_subjectPublicKeyIdentifier, loc))
            {
                const ExtensionValue& ext = *loc;
                KeyIdentifierValue& identifier = *m_subjectKeyIdentifier.get();

                ArraySource store(ext.m_value, ext.m_value.size(), true);
                identifier.BERDecode(store);
            }
        }
    }

    return *m_subjectKeyIdentifier.get();
}

void X509Certificate::GetIdentitiesFromSubjectUniqueId(IdentityValueArray& identityArray) const
{
    if (HasSubjectUniqueId())
    {
        IdentityValue identity(*m_subjectUid.get(), IdentityValue::UniqueId);
        identityArray.push_back(identity);
    }
}

void X509Certificate::GetIdentitiesFromSubjectPublicKeyId(IdentityValueArray& identityArray) const
{
    const KeyIdentifierValue& subjectKeyIdentifier = GetSubjectKeyIdentifier();
    IdentityValue identity(id_subjectPublicKeyIdentifier, subjectKeyIdentifier.m_value, IdentityValue::SubjectPKI);
    identityArray.push_back(identity);
}

void X509Certificate::GetIdentitiesFromSubjectDistName(IdentityValueArray& identityArray) const
{
    // The full readable string
    {
        std::ostringstream oss;
        oss << GetSubjectDistinguishedName();
        const std::string id(oss.str());

        IdentityValue identity(id_distinguishedName, id, IdentityValue::SubjectDN);
        identityArray.push_back(identity);
    }

    // Get the CommonName separately
    {
        const RdnValueArray& rdnArray = GetSubjectDistinguishedName();
        RdnValueArray::const_iterator first = rdnArray.begin();
        RdnValueArray::const_iterator last = rdnArray.end();

        while (first != last)
        {
            if (first->m_oid == id_commonName)
            {
                IdentityValue identity(id_commonName, first->m_value, IdentityValue::SubjectCN);
                identityArray.push_back(identity);
                break;  // Only one common name
            }
            ++first;
        }
    }

    // Get the UniqueId separately
    {
        const RdnValueArray& rdnArray = GetSubjectDistinguishedName();
        RdnValueArray::const_iterator first = rdnArray.begin();
        RdnValueArray::const_iterator last = rdnArray.end();

        while (first != last)
        {
            if (first->m_oid == id_uniqueIdentifier)
            {
                IdentityValue identity(id_uniqueIdentifier, first->m_value, IdentityValue::SubjectUID);
                identityArray.push_back(identity);
                // Don't break due to multiple UniqueId's
            }
            ++first;
        }
    }

    // Get the PKCS#9 email separately
    {
        const RdnValueArray& rdnArray = GetSubjectDistinguishedName();
        RdnValueArray::const_iterator first = rdnArray.begin();
        RdnValueArray::const_iterator last = rdnArray.end();

        while (first != last)
        {
            if (first->m_oid == id_pkcsEmail)
            {
                IdentityValue identity(id_pkcsEmail, first->m_value, IdentityValue::SubjectEmail);
                identityArray.push_back(identity);
                // Don't break due to multiple emails
            }
            ++first;
        }
    }
}

void X509Certificate::GetIdentitiesFromSubjectAltName(IdentityValueArray& identityArray) const
{
    ExtensionValueArray::const_iterator loc;
    if (FindExtension(id_subjectAltName, loc))
    {
        const ExtensionValue& ext = *loc;
        ArraySource store(ext.m_value, ext.m_value.size(), true);

        BERSequenceDecoder seq(store);
          while (! seq.EndReached())
          {
              byte choice;
              if (! seq.Get(choice))
                  BERDecodeError();

              // GeneralName must be in range [0] otherName to [8] registeredID
              if (choice < 0x80 || choice > 0x88)
                  BERDecodeError();

              size_t len;
              if (! BERLengthDecode(seq, len))
                  BERDecodeError();

              SecByteBlock value(len);
              seq.Get(value, value.size());

              IdentityValue::IdentityEnum src;

              switch (choice)
              {
                  case 0x80:
                    src = IdentityValue::otherName;
                    break;

                  case 0x81:
                    src = IdentityValue::rfc822Name;
                    break;

                  case 0x82:
                    src = IdentityValue::dNSName;
                    break;

                  case 0x83:
                    src = IdentityValue::x400Address;
                    break;

                  case 0x84:
                    src = IdentityValue::directoryName;
                    break;

                  case 0x85:
                    src = IdentityValue::ediPartyName;
                    break;

                  case 0x86:
                    src = IdentityValue::uniformResourceIdentifier;
                    break;

                  case 0x87:
                    src = IdentityValue::iPAddress;
                    break;

                  case 0x88:
                    src = IdentityValue::registeredID;
                    break;

                  default:
                    src = IdentityValue::InvalidIdentityEnum;
                    break;
              }

              IdentityValue identity(id_subjectAltName, value, src);
              identityArray.push_back(identity);
          }
        seq.MessageEnd();
    }
}

void X509Certificate::GetIdentitiesFromNetscapeServer(IdentityValueArray& identityArray) const
{
    ExtensionValueArray::const_iterator loc;
    if (FindExtension(id_netscapeServerName, loc))
    {
        const ExtensionValue& ext = *loc;
        ArraySource store(ext.m_value, ext.m_value.size(), true);

        BERSequenceDecoder seq(store);

          SecByteBlock temp;
          temp.resize(seq.MaxRetrievable());
          seq.Get(BytePtr(temp), BytePtrSize(temp));

          IdentityValue identity(id_netscapeServerName, temp, IdentityValue::nsServer);
          identityArray.push_back(identity);

        seq.MessageEnd();
    }
}

const IdentityValueArray& X509Certificate::GetSubjectIdentities() const
{
    if (m_identities.get() == NULLPTR)
    {
        m_identities.reset(new IdentityValueArray);
        IdentityValueArray identities;

        GetIdentitiesFromSubjectUniqueId(identities);
        GetIdentitiesFromSubjectDistName(identities);
        GetIdentitiesFromSubjectPublicKeyId(identities);
        GetIdentitiesFromSubjectAltName(identities);
        GetIdentitiesFromNetscapeServer(identities);

        std::swap(*m_identities.get(), identities);
    }

    return *m_identities.get();
}

const KeyUsageValueArray& X509Certificate::GetSubjectKeyUsage() const
{
    if (m_keyUsage.get() == NULLPTR)
    {
        m_keyUsage.reset(new KeyUsageValueArray);
        KeyUsageValueArray keyUsages;

        ExtensionValueArray::const_iterator loc;
        if (FindExtension(id_keyUsage, loc))
        {
            const ExtensionValue& ext = *loc;
            ArraySource store(ConstBytePtr(ext.m_value), BytePtrSize(ext.m_value), true);

            SecByteBlock values;
            word32 unused;
            BERDecodeBitString(store, values, unused);

            // The bit string is one octet, with the bit mask blocked-left.
            CRYPTOPP_ASSERT(values.size() == 1);
            word32 mask = (values[0] >> unused);

            const KeyUsageValue::KeyUsageEnum usageEnum[] = {
                KeyUsageValue::digitalSignature,    // pos 0
                KeyUsageValue::nonRepudiation,
                KeyUsageValue::keyEncipherment,
                KeyUsageValue::dataEncipherment,
                KeyUsageValue::keyAgreement,
                KeyUsageValue::keyCertSign,
                KeyUsageValue::cRLSign,
                KeyUsageValue::encipherOnly,
                KeyUsageValue::decipherOnly
            };

            for (size_t i=0; i<COUNTOF(usageEnum); ++i)
            {
                if ((1 << i) & mask)
                {
                    KeyUsageValue ku(id_keyUsage, usageEnum[i]);
                    keyUsages.push_back(ku);
                }
            }
        }

        if (FindExtension(id_extendedKeyUsage, loc))
        {
            const ExtensionValue& ext = *loc;
            ArraySource store(ConstBytePtr(ext.m_value), BytePtrSize(ext.m_value), true);

            BERSequenceDecoder seq(store);

              while (! seq.EndReached())
              {
                OID oid;
                oid.BERDecode(seq);

                KeyUsageValue eku(oid);
                keyUsages.push_back(eku);
              }
            seq.MessageEnd();
        }

        std::swap(*m_keyUsage.get(), keyUsages);
    }

    return *m_keyUsage.get();
}

std::ostream& X509Certificate::Print(std::ostream& out) const
{
    std::ostringstream oss;
    std::ios_base::fmtflags base = out.flags() & std::ios_base::basefield;

    oss << "Version: " << GetVersion() << std::endl;
    oss << "Serial Number: " << "0x" << std::hex << GetSerialNumber() << std::endl;
    oss.setf(base, std::ios_base::basefield);  // reset basefield

    oss << "Not Before: " << GetNotBefore() << std::endl;
    oss << "Not After: " << GetNotAfter() << std::endl;

    oss << "Issuer DN: " << GetIssuerDistinguishedName() << std::endl;
    oss << "Subject DN: " << GetSubjectDistinguishedName() << std::endl;

    oss << "Authority KeyId: " << GetAuthorityKeyIdentifier() << std::endl;
    oss << "Subject KeyId: " << GetSubjectKeyIdentifier() << std::endl;

    oss << "Key Usage: " << GetSubjectKeyUsage() << std::endl;

    oss << "CA Certificate: " << IsCertificateAuthority() << ", ";
    oss << "Self Signed: " << IsSelfSigned() << std::endl;

    // Format signature
    std::string signature;
    const SecByteBlock& binarySignature = GetCertificateSignature();
    StringSource(binarySignature, binarySignature.size(), true, new HexEncoder(new StringSink(signature)));
    signature.resize(60); signature += "...";

    // Format tbs
    std::string toBeSigned;
    const SecByteBlock& binaryToBeSigned = GetToBeSigned();
    StringSource(binaryToBeSigned, binaryToBeSigned.size(), true, new HexEncoder(new StringSink(toBeSigned)));
    toBeSigned.resize(60); toBeSigned += "...";

    const OID& algorithm = GetCertificateSignatureAlgorithm();
    oss << "Signature Alg: " << OidToNameLookup(algorithm) << std::endl;
    oss << "To Be Signed: " << toBeSigned << std::endl;
    oss << "Signature: " << signature;

    return out << oss.str();
}

void X509Certificate::WriteCertificateBytes(BufferedTransformation &bt) const
{
    try
    {
        bt.Put(m_origCertificate, m_origCertificate.size());
    }
    catch(const Exception&)
    {
    }
}

NAMESPACE_END  // Cryptopp
