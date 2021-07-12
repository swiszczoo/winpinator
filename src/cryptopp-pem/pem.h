// pem.h - PEM read and write routines.
//         Written and placed in the public domain by Jeffrey Walton

/// \file pem.h
/// \brief Functions to read and write PEM encoded objects
/// \details This is a library add-on. You must download and compile it
///  yourself. Also see <A HREF="http://www.cryptopp.com/wiki/PEM_Pack">PEM
///  Pack</A> on the Crypto++ wiki.

// Why Not Specialize Function Templates?
// http://www.gotw.ca/publications/mill17.htm

/////////////////////////////////////////////////////////////////////////////

#ifndef CRYPTOPP_PEM_H
#define CRYPTOPP_PEM_H

#include <cryptopp/pubkey.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/gfpcrypt.h>
#include "x509cert.h"
#include <cryptopp/elgamal.h>
#include <cryptopp/rsa.h>
#include <cryptopp/dsa.h>
#include <cryptopp/asn.h>

NAMESPACE_BEGIN(CryptoPP)

/// \brief Get the next PEM object
/// \param src the source BufferedTransformation
/// \param dest the destination BufferedTransformation
/// \returns true if an object was parsed, false otherwise
/// \details PEM_NextObject attempts to retrieve the next PEM encoded key or
///  parameter from <tt>src</tt> and transfers it to <tt>dest</tt>. If there
///  are multiple keys or parameters, then only the first is transferred.
/// \details If <tt>src</tt> is empty then PEM_NextObject() returns false.
///  If <tt>src</tt> holds a partial or malformed object is present then
///  InvalidDataFormat is thrown. The exception is used to distinguish
///  from an empty <tt>src</tt>, and the exception signals the caller to fix
///  <tt>src</tt>.
/// \details PEM_NextObject will parse an invalid object. For example, it
///  will parse a key or parameter with <tt>-----BEGIN FOO-----</tt> and
///  <tt>-----END BAR-----</tt>. The parser only looks for BEGIN and END
///  (and the dashes). The malformed input will be caught later when a
///  particular key or parameter is parsed.
/// \throws InvalidDataFormat
bool PEM_NextObject(BufferedTransformation& src, BufferedTransformation& dest);

/// \brief Recognized PEM types.
/// \details Many PEM types can be read and write, but not all of them.
/// \sa <A HREF="https://stackoverflow.com/questions/5355046">Where is the
///  PEM file format specified?</A>
enum PEM_Type {
    /// \brief Public key
    /// \details non-specific public key
    PEM_PUBLIC_KEY = 1,
    /// \brief Private key
    /// \details non-specific private key
    PEM_PRIVATE_KEY,
    /// \brief RSA public key
    PEM_RSA_PUBLIC_KEY,
    /// \brief RSA private key
    PEM_RSA_PRIVATE_KEY,
    /// \brief Encrypted RSA private key
    PEM_RSA_ENC_PRIVATE_KEY,
    /// \brief DSA public key
    PEM_DSA_PUBLIC_KEY,
    /// \brief DSA private key
    PEM_DSA_PRIVATE_KEY,
    /// \brief Encrypted DSA private key
    PEM_DSA_ENC_PRIVATE_KEY,
    /// \brief ElGamal public key
    PEM_ELGAMAL_PUBLIC_KEY,
    /// \brief ElGamal private key
    PEM_ELGAMAL_PRIVATE_KEY,
    /// \brief Encrypted ElGamal private key
    PEM_ELGAMAL_ENC_PRIVATE_KEY,
    /// \brief Elliptic curve public key
    /// \details non-specific elliptic curve public key
    PEM_EC_PUBLIC_KEY,
    /// \brief Elliptic curve private key
    /// \details non-specific elliptic curve private key
    PEM_EC_PRIVATE_KEY,
    /// \brief Encrypted elliptic curve private key
    /// \details non-specific encrypted elliptic curve private key
    PEM_EC_ENC_PRIVATE_KEY,
    /// \brief ECDSA public key
    PEM_ECDSA_PUBLIC_KEY,
    /// \brief ECDSA private key
    PEM_ECDSA_PRIVATE_KEY,
    /// \brief Encrypted ECDSA private key
    PEM_ENC_ECDSA_PRIVATE_KEY,
    /// \brief X25519 public key
    PEM_X25519_PUBLIC_KEY,
    /// \brief X25519 private key
    PEM_X25519_PRIVATE_KEY,
    /// \brief Encrypted X25519 private key
    PEM_X25519_ENC_PRIVATE_KEY,
    /// \brief Elliptic curve parameters
    PEM_EC_PARAMETERS,
    /// \brief Diffie-Hellman curve parameters
    PEM_DH_PARAMETERS,
    /// \brief DSA parameters
    PEM_DSA_PARAMETERS,
    /// \brief X.509 certificate
    PEM_X509_CERTIFICATE,
    /// \brief Certificate request
    PEM_REQ_CERTIFICATE,
    /// \brief Certificate
    PEM_CERTIFICATE,
    /// \brief Unsupported type
    PEM_UNSUPPORTED = 0xFFFFFFFF
};

/// \brief Determine the type of key or parameter
/// \param bt the source BufferedTransformation
/// \returns PEM_Type or PEM_UNSUPPORTED
PEM_Type PEM_GetType(const BufferedTransformation& bt);

/////////////////////////////////////////////////////////////////////////////

/// \brief Load a PEM encoded RSA public key
/// \param bt the source BufferedTransformation
/// \param key the RSA public key
/// \throws Exception on failure
void PEM_Load(BufferedTransformation& bt, RSA::PublicKey& key);

/// \brief Load a PEM encoded RSA private key
/// \param bt the source BufferedTransformation
/// \param key the RSA private key
/// \throws Exception on failure
void PEM_Load(BufferedTransformation& bt, RSA::PrivateKey& key);

/// \brief Load a PEM encoded RSA private key
/// \param bt the source BufferedTransformation
/// \param key the RSA private key
/// \param password pointer to the password buffer
/// \param length the size of the password buffer
/// \throws Exception on failure
void PEM_Load(BufferedTransformation& bt, RSA::PrivateKey& key,
              const char* password, size_t length);

/// \brief Load a PEM encoded DSA public key
/// \param bt the source BufferedTransformation
/// \param key the DSA public key
/// \throws Exception on failure
void PEM_Load(BufferedTransformation& bt, DSA::PublicKey& key);

/// \brief Load a PEM encoded DSA private key
/// \param bt the source BufferedTransformation
/// \param key the DSA private key
/// \throws Exception on failure
void PEM_Load(BufferedTransformation& bt, DSA::PrivateKey& key);

/// \brief Load a PEM encoded DSA private key
/// \param bt the source BufferedTransformation
/// \param key the DSA private key
/// \param password pointer to the password buffer
/// \param length the size of the password buffer
/// \throws Exception on failure
void PEM_Load(BufferedTransformation& bt, DSA::PrivateKey& key,
              const char* password, size_t length);

/// \brief Load a PEM encoded ElGamal public key
/// \param bt the source BufferedTransformation
/// \param key the ElGamal public key
/// \throws Exception on failure
void PEM_Load(BufferedTransformation& bt, ElGamalKeys::PublicKey& key);

/// \brief Load a PEM encoded ElGamal private key
/// \param bt the source BufferedTransformation
/// \param key the ElGamal private key
/// \throws Exception on failure
void PEM_Load(BufferedTransformation& bt, ElGamalKeys::PrivateKey& key);

/// \brief Load a PEM encoded ElGamal private key
/// \param bt the source BufferedTransformation
/// \param key the ElGamal private key
/// \param password pointer to the password buffer
/// \param length the size of the password buffer
/// \throws Exception on failure
void PEM_Load(BufferedTransformation& bt, ElGamalKeys::PrivateKey& key,
              const char* password, size_t length);

/// \brief Load a PEM encoded ECP public key
/// \param bt the source BufferedTransformation
/// \param key the ECP public key
/// \throws Exception on failure
void PEM_Load(BufferedTransformation& bt, DL_PublicKey_EC<ECP>& key);

/// \brief Load a PEM encoded ECP private key
/// \param bt the source BufferedTransformation
/// \param key the ECP private key
/// \throws Exception on failure
void PEM_Load(BufferedTransformation& bt, DL_PrivateKey_EC<ECP>& key);

/// \brief Load a PEM encoded ECP private key
/// \param bt the source BufferedTransformation
/// \param key the ECP private key
/// \param password pointer to the password buffer
/// \param length the size of the password buffer
/// \throws Exception on failure
void PEM_Load(BufferedTransformation& bt, DL_PrivateKey_EC<ECP>& key,
              const char* password, size_t length);

/// \brief Load a PEM encoded EC2N public key
/// \param bt the source BufferedTransformation
/// \param key the EC2N public key
/// \throws Exception on failure
void PEM_Load(BufferedTransformation& bt, DL_PublicKey_EC<EC2N>& key);

/// \brief Load a PEM encoded EC2N private key
/// \param bt the source BufferedTransformation
/// \param key the EC2N private key
/// \throws Exception on failure
void PEM_Load(BufferedTransformation& bt, DL_PrivateKey_EC<EC2N>& key);

/// \brief Load a PEM encoded EC2N private key
/// \param bt the source BufferedTransformation
/// \param key the EC2N private key
/// \param password pointer to the password buffer
/// \param length the size of the password buffer
/// \throws Exception on failure
void PEM_Load(BufferedTransformation& bt, DL_PrivateKey_EC<EC2N>& key,
              const char* password, size_t length);

/// \brief Load a PEM encoded ECDSA private key
/// \param bt the source BufferedTransformation
/// \param key the ECDSA private key
/// \throws Exception on failure
void PEM_Load(BufferedTransformation& bt, DL_Keys_ECDSA<ECP>::PrivateKey& key);

/// \brief Load a PEM encoded ECDSA private key
/// \param bt the source BufferedTransformation
/// \param key the ECDSA private key
/// \param password pointer to the password buffer
/// \param length the size of the password buffer
/// \throws Exception on failure
void PEM_Load(BufferedTransformation& bt, DL_Keys_ECDSA<ECP>::PrivateKey& key,
              const char* password, size_t length);

/// \brief Load a PEM encoded ECDSA public key
/// \param bt the source BufferedTransformation
/// \param key the ECDSA public key
/// \throws Exception on failure
void PEM_Load(BufferedTransformation& bt, DL_Keys_ECDSA<EC2N>::PrivateKey& key);

/// \brief Load a PEM encoded ECDSA private key
/// \param bt the source BufferedTransformation
/// \param key the ECDSA private key
/// \param password pointer to the password buffer
/// \param length the size of the password buffer
/// \throws Exception on failure
void PEM_Load(BufferedTransformation& bt, DL_Keys_ECDSA<EC2N>::PrivateKey& key,
              const char* password, size_t length);

/// \brief Load a PEM encoded DSA group parameters
/// \param bt the source BufferedTransformation
/// \param params the DSA group parameters
/// \throws Exception on failure
void PEM_Load(BufferedTransformation& bt, DL_GroupParameters_DSA& params);

/// \brief Load a PEM encoded ECP group parameters
/// \param bt the source BufferedTransformation
/// \param params the ECP group parameters
/// \throws Exception on failure
void PEM_Load(BufferedTransformation& bt, DL_GroupParameters_EC<ECP>& params);

/// \brief Load a PEM encoded EC2N group parameters
/// \param bt the source BufferedTransformation
/// \param params the EC2N group parameters
/// \throws Exception on failure
void PEM_Load(BufferedTransformation& bt, DL_GroupParameters_EC<EC2N>& params);

/// \brief Load a PEM encoded X.509 certificate
/// \param bt the source BufferedTransformation
/// \param cert the X.509 certificate
/// \throws Exception on failure
void PEM_Load(BufferedTransformation& bt, X509Certificate& cert);

/// \brief Load a PEM encoded Diffie-Hellman parameters
/// \param bt the source BufferedTransformation
/// \param p the prime modulus
/// \param g the group generator
/// \throws Exception on failure
void PEM_DH_Load(BufferedTransformation& bt, Integer& p, Integer& g);

/// \brief Load a PEM encoded Diffie-Hellman parameters
/// \param bt the source BufferedTransformation
/// \param p the prime modulus
/// \param q the subgroup order
/// \param g the group generator
/// \throws Exception on failure
void PEM_DH_Load(BufferedTransformation& bt, Integer& p, Integer& q, Integer& g);

/////////////////////////////////////////////////////////////////////////////

// Begin the Write routines. The write routines always write the "named curve"
// (i.e., the OID of secp256k1) rather than the domain parameters. This is
// because RFC 5915 specifies the format. In addition, OpenSSL cannot load and
// utilize an EC key with a non-named curve into a server. For encrpted private
// keys, the algorithm should be a value like `AES-128-CBC`. See pem_read.cpp
// and pem_write.cpp for the values that are recognized. On failure, any number
// of Crypto++ exceptions are thrown. No custom exceptions are thrown.

/// \brief Save a PEM encoded RSA public key
/// \param bt the destination BufferedTransformation
/// \param key the RSA public key
/// \throws Exception on failure
void PEM_Save(BufferedTransformation& bt, const RSA::PublicKey& key);

/// \brief Save a PEM encoded RSA private key
/// \param bt the destination BufferedTransformation
/// \param key the RSA private key
/// \throws Exception on failure
void PEM_Save(BufferedTransformation& bt, const RSA::PrivateKey& key);

/// \brief Save a PEM encoded RSA private key
/// \param bt the destination BufferedTransformation
/// \param rng a RandomNumberGenerator to produce an initialization vector
/// \param key the RSA private key
/// \param algorithm the encryption algorithm
/// \param password pointer to the password buffer
/// \param length the size of the password buffer
/// \details The algorithm should be a value like <tt>AES-128-CBC</tt>. See
///  <tt>pem_read.cpp</tt> and <tt>pem_write.cpp</tt> for the values that are
///  recognized.
/// \throws Exception on failure
void PEM_Save(BufferedTransformation& bt, const RSA::PrivateKey& key,
              RandomNumberGenerator& rng, const std::string& algorithm,
              const char* password, size_t length);

/// \brief Save a PEM encoded DSA public key
/// \param bt the destination BufferedTransformation
/// \param key the DSA public key
/// \throws Exception on failure
void PEM_Save(BufferedTransformation& bt, const DSA::PublicKey& key);

/// \brief Save a PEM encoded DSA private key
/// \param bt the destination BufferedTransformation
/// \param key the DSA private key
/// \throws Exception on failure
void PEM_Save(BufferedTransformation& bt, const DSA::PrivateKey& key);

/// \brief Save a PEM encoded DSA private key
/// \param bt the destination BufferedTransformation
/// \param rng a RandomNumberGenerator to produce an initialization vector
/// \param key the DSA private key
/// \param algorithm the encryption algorithm
/// \param password pointer to the password buffer
/// \param length the size of the password buffer
/// \details The algorithm should be a value like <tt>AES-128-CBC</tt>. See
///  <tt>pem_read.cpp</tt> and <tt>pem_write.cpp</tt> for the values that are
///  recognized.
/// \throws Exception on failure
void PEM_Save(BufferedTransformation& bt, const DSA::PrivateKey& key,
              RandomNumberGenerator& rng, const std::string& algorithm,
              const char* password, size_t length);

/// \brief Save a PEM encoded ElGamal public key
/// \param bt the destination BufferedTransformation
/// \param key the ElGamal public key
/// \throws Exception on failure
void PEM_Save(BufferedTransformation& bt, const ElGamalKeys::PublicKey& key);

/// \brief Save a PEM encoded ElGamal private key
/// \param bt the destination BufferedTransformation
/// \param key the ElGamal private key
/// \throws Exception on failure
void PEM_Save(BufferedTransformation& bt, const ElGamalKeys::PrivateKey& key);

/// \brief Save a PEM encoded ElGamal private key
/// \param bt the destination BufferedTransformation
/// \param rng a RandomNumberGenerator to produce an initialization vector
/// \param key the ElGamal private key
/// \param algorithm the encryption algorithm
/// \param password pointer to the password buffer
/// \param length the size of the password buffer
/// \details The algorithm should be a value like <tt>AES-128-CBC</tt>. See
///  <tt>pem_read.cpp</tt> and <tt>pem_write.cpp</tt> for the values that are
///  recognized.
/// \throws Exception on failure
void PEM_Save(BufferedTransformation& bt, const ElGamalKeys::PrivateKey& key,
              RandomNumberGenerator& rng, const std::string& algorithm,
              const char* password, size_t length);

/// \brief Save a PEM encoded ECP public key
/// \param bt the destination BufferedTransformation
/// \param key the ECP public key
/// \throws Exception on failure
void PEM_Save(BufferedTransformation& bt, const DL_PublicKey_EC<ECP>& key);

/// \brief Save a PEM encoded ECP private key
/// \param bt the destination BufferedTransformation
/// \param key the ECP private key
/// \throws Exception on failure
void PEM_Save(BufferedTransformation& bt, const DL_PrivateKey_EC<ECP>& key);

/// \brief Save a PEM encoded ECP private key
/// \param bt the destination BufferedTransformation
/// \param rng a RandomNumberGenerator to produce an initialization vector
/// \param key the ECP private key
/// \param algorithm the encryption algorithm
/// \param password pointer to the password buffer
/// \param length the size of the password buffer
/// \details The algorithm should be a value like <tt>AES-128-CBC</tt>. See
///  <tt>pem_read.cpp</tt> and <tt>pem_write.cpp</tt> for the values that are
///  recognized.
/// \details The "named curve" (i.e., the OID of secp256k1) is used rather
///  than the domain parameters. This is because RFC 5915 specifies the format.
///  In addition, OpenSSL cannot load and utilize an EC key with a non-named
///  curve into a server.
/// \throws Exception on failure
void PEM_Save(BufferedTransformation& bt, const DL_PrivateKey_EC<ECP>& key,
              RandomNumberGenerator& rng, const std::string& algorithm,
              const char* password, size_t length);

/// \brief Save a PEM encoded EC2N public key
/// \param bt the destination BufferedTransformation
/// \param key the EC2N public key
/// \throws Exception on failure
void PEM_Save(BufferedTransformation& bt, const DL_PublicKey_EC<EC2N>& key);

/// \brief Save a PEM encoded EC2N private key
/// \param bt the destination BufferedTransformation
/// \param key the EC2N private key
/// \throws Exception on failure
void PEM_Save(BufferedTransformation& bt, const DL_PrivateKey_EC<EC2N>& key);

/// \brief Save a PEM encoded EC2N private key
/// \param bt the destination BufferedTransformation
/// \param rng a RandomNumberGenerator to produce an initialization vector
/// \param key the EC2N private key
/// \param algorithm the encryption algorithm
/// \param password pointer to the password buffer
/// \param length the size of the password buffer
/// \details The algorithm should be a value like <tt>AES-128-CBC</tt>. See
///  <tt>pem_read.cpp</tt> and <tt>pem_write.cpp</tt> for the values that are
///  recognized.
/// \details The "named curve" (i.e., the OID of secp256k1) is used rather than
///  the domain parameters. This is because RFC 5915 specifies the format. In
///  addition, OpenSSL cannot load and utilize an EC key with a non-named curve
///  into a server.
/// \throws Exception on failure
void PEM_Save(BufferedTransformation& bt, const DL_PrivateKey_EC<EC2N>& key,
              RandomNumberGenerator& rng, const std::string& algorithm,
              const char* password, size_t length);

/// \brief Save a PEM encoded ECDSA private key
/// \param bt the destination BufferedTransformation
/// \param key the ECDSA private key
/// \throws Exception on failure
void PEM_Save(BufferedTransformation& bt, const DL_Keys_ECDSA<ECP>::PrivateKey& key);

/// \brief Save a PEM encoded ECDSA private key
/// \param bt the destination BufferedTransformation
/// \param rng a RandomNumberGenerator to produce an initialization vector
/// \param key the ECDSA private key
/// \param algorithm the encryption algorithm
/// \param password pointer to the password buffer
/// \param length the size of the password buffer
/// \details The algorithm should be a value like <tt>AES-128-CBC</tt>. See
///  <tt>pem_read.cpp</tt> and <tt>pem_write.cpp</tt> for the values that
///  are recognized.
/// \details The "named curve" (i.e., the OID of secp256k1) is used rather
///  than the domain parameters. This is because RFC 5915 specifies the format.
///  In addition, OpenSSL cannot load and utilize an EC key with a non-named
///  curve into a server.
/// \throws Exception on failure
void PEM_Save(BufferedTransformation& bt, const DL_Keys_ECDSA<ECP>::PrivateKey& key,
              RandomNumberGenerator& rng, const std::string& algorithm,
              const char* password, size_t length);

/// \brief Save a PEM encoded DSA group parameters
/// \param bt the destination BufferedTransformation
/// \param params the DSA group parameters
/// \throws Exception on failure
void PEM_Save(BufferedTransformation& bt, const DL_GroupParameters_DSA& params);

/// \brief Save a PEM encoded ECP group parameters
/// \param bt the destination BufferedTransformation
/// \param params the ECP group parameters
/// \throws Exception on failure
void PEM_Save(BufferedTransformation& bt, const DL_GroupParameters_EC<ECP>& params);

/// \brief Save a PEM encoded EC2N group parameters
/// \param bt the destination BufferedTransformation
/// \param params the EC2N group parameters
/// \throws Exception on failure
void PEM_Save(BufferedTransformation& bt, const DL_GroupParameters_EC<EC2N>& params);

/// \brief Save a PEM encoded X.509 certificate
/// \param bt the destination BufferedTransformation
/// \param cert the X.509 certificate
/// \throws Exception on failure
void PEM_Save(BufferedTransformation& bt, const X509Certificate& cert);

/// \brief Save a PEM encoded Diffie-Hellman parameters
/// \param bt the destination BufferedTransformation
/// \param p the prime modulus
/// \param g the group generator
/// \throws Exception on failure
void PEM_DH_Save(BufferedTransformation& bt, const Integer& p, const Integer& g);

/// \brief Save a PEM encoded Diffie-Hellman parameters
/// \param bt the destination BufferedTransformation
/// \param p the prime modulus
/// \param q the subgroup order
/// \param g the group generator
/// \throws Exception on failure
void PEM_DH_Save(BufferedTransformation& bt, const Integer& p, const Integer& q, const Integer& g);

NAMESPACE_END

#endif
