#!/usr/bin/env bash

# Script to create the test keys used pem_test.cxx

##################################
# prerequisites

CXX="${CXX:-c++}"
MAKEJOBS="${MAKEJOBS:-2}"

if ! command -v "${CXX}" 2>/dev/null; then
    echo "Please install a compiler like g++"
    exit 1
fi

if ! command -v openssl 2>/dev/null; then
    echo "Please install openssl package"
    exit 1
fi

if ! command -v curl 2>/dev/null; then
    echo "Please install curl package"
    exit 1
fi

if ! command -v perl 2>/dev/null; then
    echo "Please install perl package"
    exit 1
fi

# We need OpenSSL 1.0.2 or above
MODERN_OPENSSL=$(openssl version | grep -c -v -E '(OpenSSL 0.[0-9]|OpenSSL 1.0.0|OpenSSL 1.0.1)')

if [[ "$MODERN_OPENSSL" -eq 0 ]]; then
    echo "Please install OpenSSL 1.0.2 or above"
    exit 1
fi

##################################
# test program

echo "Compiling test program with ${CXX}"

rm -rf pem_test.exe pem_eol.exe

CXXFLAGS="-DDEBUG -g3 -O0 -Wall"

# Build crypto++ library if out of date.
if ! CXX="${CXX}" CXXFLAGS="${CXXFLAGS}" make -j ${MAKEJOBS}; then
    echo "Failed to build libcryptopp.a"
    exit 1
fi

# Build the test program
if ! ${CXX} ${CXXFLAGS} pem_test.cxx ./libcryptopp.a -o pem_test.exe; then
    echo "Failed to build pem_test.exe"
    exit 1
fi

# Build the RFC1421 EOL converter. OpenSSL no longer outputs well formed PEM.
if ! ${CXX} ${CXXFLAGS} pem_eol.cxx -o pem_eol.exe; then
    echo "Failed to build pem_test.exe"
    exit 1
fi

# Build the reproducer program in case test program crashes
# The reproducer loads badcert.der.
if [[ -e badcert.cxx ]]; then
    if ! ${CXX} ${CXXFLAGS} badcert.cxx ./libcryptopp.a -o badcert.exe; then
        echo "Failed to build badcert.exe"
        exit 1
    fi
fi

##################################
# test keys

echo "Generating OpenSSL keys"

# RSA private key, public key, and encrypted private key
openssl genrsa -out rsa-priv.pem 1024
openssl rsa -in rsa-priv.pem -out rsa-pub.pem -pubout
openssl rsa -in rsa-priv.pem -out rsa-enc-priv.pem -aes128 -passout pass:abcdefghijklmnopqrstuvwxyz

# DSA private key, public key, and encrypted private key
openssl dsaparam -out dsa-params.pem 1024
openssl gendsa -out dsa-priv.pem dsa-params.pem
openssl dsa -in dsa-priv.pem -out dsa-pub.pem -pubout
openssl dsa -in dsa-priv.pem -out dsa-enc-priv.pem -aes128 -passout pass:abcdefghijklmnopqrstuvwxyz

# EC private key, public key, and encrypted private key
openssl ecparam -out ec-params.pem -name secp256k1 -genkey
openssl ec -in ec-params.pem -out ec-priv.pem
openssl ec -in ec-priv.pem -out ec-pub.pem -pubout
openssl ec -in ec-priv.pem -out ec-enc-priv.pem -aes128 -passout pass:abcdefghijklmnopqrstuvwxyz

# Diffie-Hellman parameters
openssl dhparam -out dh-params.pem 512

# Make line endings CRLF per RFC 1421. OpenSSL no longer outputs well formed PEM.
./pem_eol.exe rsa-priv.pem rsa-pub.pem rsa-enc-priv.pem
./pem_eol.exe dsa-params.pem dsa-priv.pem dsa-pub.pem dsa-enc-priv.pem
./pem_eol.exe ec-params.pem ec-priv.pem ec-pub.pem ec-enc-priv.pem
./pem_eol.exe dh-params.pem

##################################
# malformed

# Only the '-----BEGIN PUBLIC KEY-----'
echo "-----BEGIN PUBLIC KEY-----" > rsa-short.pem

# Remove the last CRLF
perl -pe 'chop' < rsa-pub.pem | perl -pe 'chop' > rsa-trunc-1.pem

# Remove the last dash from encapsulation boundary (should throw)
perl -pe 'chop' < rsa-trunc-1.pem > rsa-trunc-2.pem

# Two keys in one file; missing CRLF between them
cat rsa-trunc-1.pem > rsa-concat.pem
cat rsa-pub.pem >> rsa-concat.pem

# Uses only CR (remove LF)
perl -pe 's/\n//g;' rsa-pub.pem > rsa-eol-cr.pem

# Uses only LF (remove CR)
perl -pe 's/\r//g;' rsa-pub.pem > rsa-eol-lf.pem

# No EOL (remove CR and LF)
perl -pe 's/\r//g;' -pe 's/\n//g;' rsa-pub.pem > rsa-eol-none.pem

echo "-----BEGIN FOO-----" > foobar.pem
head -c 180 /dev/urandom | base64 | fold -w 64 >> foobar.pem
echo "-----END BAR-----" >> foobar.pem

##################################
# Test Certificate

cat << EOF > ./example-com.conf
[ req ]
prompt              = no
default_bits        = 2048
default_keyfile     = server-key.pem
distinguished_name  = subject
req_extensions      = req_ext
x509_extensions     = x509_ext
string_mask         = utf8only

# CA/B requires a domain name in the Common Name
[ subject ]
countryName          = US
stateOrProvinceName  = NY
localityName         = New York
organizationName     = Example, LLC
commonName           = Example Company
emailAddress         = support@example.com

# A real server cert usually does not need clientAuth or secureShellServer
[ x509_ext ]
subjectKeyIdentifier    = hash
authorityKeyIdentifier  = keyid,issuer
basicConstraints        = critical,CA:FALSE
keyUsage                = digitalSignature
extendedKeyUsage        = serverAuth, clientAuth
subjectAltName          = @alternate_names
nsComment               = "OpenSSL Generated Certificate"

# A real server cert usually does not need clientAuth or secureShellServer
[ req_ext ]
subjectKeyIdentifier    = hash
basicConstraints        = critical,CA:FALSE
keyUsage                = digitalSignature
extendedKeyUsage        = serverAuth, clientAuth
subjectAltName          = @alternate_names
nsComment               = "OpenSSL Generated Certificate"

# A real server cert should not have email addresses
# The CA/B BR forbids IP addresses
[ alternate_names ]
DNS.1  = example.com
DNS.2  = www.example.com
DNS.3  = mail.example.com
DNS.4  = ftp.example.com
IP.1   = 127.0.0.1
IP.2   = ::1
email.1  = webmaster@example.com
email.2  = ftpmaster@example.com
email.3  = hostmaster@example.com
EOF

# And create the cert
openssl req -config example-com.conf -new -x509 -sha256 -newkey rsa:2048 -nodes \
    -keyout example-com.key.pem -days 365 -out example-com.cert.pem

# Convert to ASN.1/DER
openssl x509 -in example-com.cert.pem -inform PEM -out example-com.cert.der -outform DER

# View PEM cert with 'openssl x509 -in example-com.cert.pem -inform PEM -text -noout'
# View DER cert with 'dumpasn1 example-com.cert.der'

##################################
# Mozilla cacert.pem

if [ ! -e "cacert.pem" ]; then
    curl -L -s -o cacert.pem https://curl.se/ca/cacert.pem
fi

##################################
# Google roots.pem

if [ ! -e "roots.pem" ]; then
    curl -L -s -o roots.pem https://pki.goog/roots.pem
fi

##################################
# Crypto++ website

echo -e "GET / HTTP/1.1\r\nHost: www.cryptopp.com\r\n\r\n" | openssl s_client -showcerts -servername www.cryptopp.com -connect www.cryptopp.com:443 2>/dev/null | openssl x509 > www-cryptopp-com.cert.pem
