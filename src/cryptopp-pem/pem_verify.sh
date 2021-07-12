#!/usr/bin/env bash

# Script to verify the test keys written by pem_test.cxx

##################################
# prerequisites

# We need OpenSSL 1.0.2 or above
MODERN_OPENSSL=$(openssl version | grep -c -v -E '(OpenSSL 0.[0-9]|OpenSSL 1.0.0|OpenSSL 1.0.1)')

if [[ "$MODERN_OPENSSL" -eq 0 ]]; then
    echo "Please install OpenSSL 1.0.2 or above"
    exit 1
fi

#################
# pem_test program

echo
echo "Crypto++ reading and writing"

if [[ ! -x pem_test.exe ]]; then
    echo "Failed to find pem_test.exe"
    exit 1
fi

if ! ./pem_test.exe; then
    echo "Failed to execute pem_test.exe"
    exit 1
fi

#################
# RSA keys

# The RSA command returns 0 on success

echo
echo "OpenSSL reading"

if [[ -f rsa-pub.cryptopp.pem ]]; then
    echo "rsa-pub.cryptopp.pem:"
    if openssl rsa -in rsa-pub.cryptopp.pem -pubin -text -noout 1>/dev/null; then
        echo "  - OK"
    else
        echo "  - Failed to load rsa-pub.cryptopp.pem"
    fi
fi

if [[ -f rsa-pub.cryptopp.pem ]]; then
    echo "rsa-priv.cryptopp.pem:"
    if openssl rsa -in rsa-priv.cryptopp.pem -text -noout 1>/dev/null; then
        echo "  - OK"
    else
        echo "  - Failed to load rsa-priv.cryptopp.pem"
    fi
fi

if [[ -f rsa-enc-priv.cryptopp.pem ]]; then
    echo "rsa-enc-priv.cryptopp.pem:"
    if openssl rsa -in rsa-enc-priv.cryptopp.pem -passin pass:abcdefghijklmnopqrstuvwxyz -text -noout 1>/dev/null; then
        echo "  - OK"
    else
        echo "  - Failed to load rsa-enc-priv.cryptopp.pem"
    fi
fi

#################
# DSA keys

# The DSA command returns 0 on success

if [[ -f dsa-params.cryptopp.pem ]]; then
    echo "dsa-params.cryptopp.pem:"
    if openssl dsaparam -in dsa-params.cryptopp.pem -text -noout 1>/dev/null; then
        echo "  - OK"
    else
        echo "  - Failed to load dsa-params.cryptopp.pem (maybe false)"
    fi
fi

if [[ -f dsa-pub.cryptopp.pem ]]; then
    echo "dsa-pub.cryptopp.pem:"
    if openssl dsa -in dsa-pub.cryptopp.pem -pubin -text -noout 1>/dev/null; then
        echo "  - OK"
    else
        echo "  - Failed to load dsa-pub.cryptopp.pem (maybe false)"
    fi
fi

if [[ -f dsa-priv.cryptopp.pem ]]; then
    echo "dsa-priv.cryptopp.pem:"
    if openssl dsa -in dsa-priv.cryptopp.pem -text -noout 1>/dev/null; then
        echo "  - OK"
    else
        echo "  - Failed to load dsa-priv.cryptopp.pem (maybe false)"
    fi
fi

if [[ -f dsa-enc-priv.cryptopp.pem ]]; then
    echo "dsa-enc-priv.cryptopp.pem:"
    if openssl dsa -in dsa-enc-priv.cryptopp.pem -passin pass:abcdefghijklmnopqrstuvwxyz -text -noout 1>/dev/null; then
        echo "  - OK"
    else
        echo "  - Failed to load dsa-enc-priv.cryptopp.pem (maybe false)"
    fi
fi

#################
# EC keys

# The EC command returns 0 on success

if [[ -f ec-params.cryptopp.pem ]]; then
    echo "ec-params.cryptopp.pem:"
    if openssl ecparam -in ec-params.cryptopp.pem -text -noout 1>/dev/null; then
        echo "  - OK"
    else
        echo "  - Failed to load ec-params.cryptopp.pem"
    fi
fi

if [[ -f ec-pub.cryptopp.pem ]]; then
    echo "ec-pub.cryptopp.pem:"
    if openssl ec -in ec-pub.cryptopp.pem -pubin -text -noout 1>/dev/null; then
        echo "  - OK"
    else
        echo "  - Failed to load ec-pub.cryptopp.pem"
    fi
fi

if [[ -f ec-priv.cryptopp.pem ]]; then
    echo "ec-priv.cryptopp.pem:"
    if openssl ec -in ec-priv.cryptopp.pem -text -noout 1>/dev/null; then
        echo "  - OK"
    else
        echo "  - Failed to load ec-priv.cryptopp.pem"
    fi
fi

if [[ -f ec-enc-priv.cryptopp.pem ]]; then
    echo "ec-enc-priv.cryptopp.pem:"
    if openssl ec -in ec-enc-priv.cryptopp.pem -passin pass:abcdefghijklmnopqrstuvwxyz -text -noout 1>/dev/null; then
        echo "  - OK"
    else
        echo "  - Failed to load ec-enc-priv.cryptopp.pem"
    fi
fi

echo "Finished testing keys written by Crypto++"

exit 0
