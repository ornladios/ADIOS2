/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * EncryptionOperator.cpp
 *
 *  Created on: Dec 7, 2021
 *      Author: Caitlin Ross <caitlin.ross@kitware.com>
 */

#include "EncryptionOperator.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

#include <sodium.h>

namespace adios2
{
namespace plugin
{

/// Decode a hex string (2*len chars) into a byte buffer of length len.
static void HexDecode(const std::string &hex, unsigned char *out, size_t len)
{
    if (hex.size() != 2 * len)
    {
        throw std::runtime_error("EncryptionOperator: hex string has wrong length. Expected " +
                                 std::to_string(2 * len) + " chars, got " +
                                 std::to_string(hex.size()));
    }
    if (sodium_hex2bin(out, len, hex.c_str(), hex.size(), nullptr, nullptr, nullptr) != 0)
    {
        throw std::runtime_error("EncryptionOperator: invalid hex string");
    }
}

struct EncryptionOperator::EncryptImpl
{
    enum class Mode
    {
        Symmetric,
        Asymmetric
    };
    Mode EncryptionMode = Mode::Symmetric;

    // --- Symmetric mode ---
    std::string KeyFilename;
    unsigned char Key[crypto_secretbox_KEYBYTES] = {0};
    bool KeyValid = false;

    // --- Asymmetric mode ---
    unsigned char PublicKey[crypto_box_PUBLICKEYBYTES] = {0};
    unsigned char SecretKey[crypto_box_SECRETKEYBYTES] = {0};
    bool PublicKeyValid = false;
    bool SecretKeyValid = false;

    ~EncryptImpl()
    {
        sodium_munlock(Key, crypto_secretbox_KEYBYTES);
        sodium_munlock(PublicKey, crypto_box_PUBLICKEYBYTES);
        sodium_munlock(SecretKey, crypto_box_SECRETKEYBYTES);
    }

    // --- Symmetric helpers ---

    void GenerateOrReadKey()
    {
        // if the key file already exists, we'll use that key,
        // otherwise we'll generate one and write it out
        std::fstream keyFile(KeyFilename.c_str());
        if (keyFile)
        {
            keyFile.read(reinterpret_cast<char *>(&Key), crypto_secretbox_KEYBYTES);
            keyFile.close();
        }
        else
        {
            keyFile.open(KeyFilename.c_str(), std::fstream::out);
            if (!keyFile)
            {
                throw std::runtime_error("couldn't open file to write key");
            }
            crypto_secretbox_keygen(Key);
            keyFile.write(reinterpret_cast<char *>(&Key), crypto_secretbox_KEYBYTES);
            keyFile.close();
        }

        // lock the key to avoid swapping to disk
        if (sodium_mlock(Key, crypto_secretbox_KEYBYTES) == -1)
        {
            throw std::runtime_error("Unable to lock memory location of secret key,"
                                     " due to system limit on amount of memory that can be locked "
                                     "by a process.");
        }
        KeyValid = true;
    }

    // --- Asymmetric helpers ---

    void ReadPublicKey(const std::string &filename)
    {
        std::ifstream keyFile(filename, std::ios::binary);
        if (!keyFile)
        {
            throw std::runtime_error("EncryptionOperator: could not open public key file: " +
                                     filename);
        }
        keyFile.read(reinterpret_cast<char *>(PublicKey), crypto_box_PUBLICKEYBYTES);
        if (!keyFile)
        {
            throw std::runtime_error("EncryptionOperator: public key file is too short: " +
                                     filename);
        }
        keyFile.close();
        if (sodium_mlock(PublicKey, crypto_box_PUBLICKEYBYTES) == -1)
        {
            throw std::runtime_error("EncryptionOperator: unable to lock memory for public key");
        }
        PublicKeyValid = true;
    }

    void SetPublicKeyHex(const std::string &hex)
    {
        HexDecode(hex, PublicKey, crypto_box_PUBLICKEYBYTES);
        if (sodium_mlock(PublicKey, crypto_box_PUBLICKEYBYTES) == -1)
        {
            throw std::runtime_error("EncryptionOperator: unable to lock memory for public key");
        }
        PublicKeyValid = true;
    }

    void ReadSecretKey(const std::string &filename)
    {
        std::ifstream keyFile(filename, std::ios::binary);
        if (!keyFile)
        {
            throw std::runtime_error("EncryptionOperator: could not open secret key file: " +
                                     filename);
        }
        keyFile.read(reinterpret_cast<char *>(SecretKey), crypto_box_SECRETKEYBYTES);
        if (!keyFile)
        {
            throw std::runtime_error("EncryptionOperator: secret key file is too short: " +
                                     filename);
        }
        keyFile.close();
        if (sodium_mlock(SecretKey, crypto_box_SECRETKEYBYTES) == -1)
        {
            throw std::runtime_error("EncryptionOperator: unable to lock memory for secret key");
        }
        SecretKeyValid = true;
    }

    void SetSecretKeyHex(const std::string &hex)
    {
        HexDecode(hex, SecretKey, crypto_box_SECRETKEYBYTES);
        if (sodium_mlock(SecretKey, crypto_box_SECRETKEYBYTES) == -1)
        {
            throw std::runtime_error("EncryptionOperator: unable to lock memory for secret key");
        }
        SecretKeyValid = true;
    }
};

EncryptionOperator::EncryptionOperator(const Params &parameters)
: PluginOperatorInterface(parameters), Impl(new EncryptImpl)
{
    if (sodium_init() < 0)
    {
        throw std::runtime_error("libsodium could not be initialized");
    }

    // Detect mode: asymmetric when any public-key param is present.
    auto pkFileIt = m_Parameters.find("publickeyfile");
    auto pkHexIt = m_Parameters.find("publickey");
    bool hasAsymParam = (pkFileIt != m_Parameters.end() || pkHexIt != m_Parameters.end());

    if (hasAsymParam)
    {
        Impl->EncryptionMode = EncryptImpl::Mode::Asymmetric;

        // Load public key: file param takes priority, then hex param.
        if (pkFileIt != m_Parameters.end())
        {
            Impl->ReadPublicKey(pkFileIt->second);
        }
        else
        {
            Impl->SetPublicKeyHex(pkHexIt->second);
        }

        // Load secret key for decryption.
        // Lookup order: SecretKeyFile param, SecretKey param (hex),
        //   ADIOS2_SECRET_KEY_FILE env var, ADIOS2_SECRET_KEY env var (hex).
        auto skFileIt = m_Parameters.find("secretkeyfile");
        auto skHexIt = m_Parameters.find("secretkey");
        if (skFileIt != m_Parameters.end())
        {
            Impl->ReadSecretKey(skFileIt->second);
        }
        else if (skHexIt != m_Parameters.end())
        {
            Impl->SetSecretKeyHex(skHexIt->second);
        }
        else
        {
            const char *envSKFile = std::getenv("ADIOS2_SECRET_KEY_FILE");
            if (envSKFile && envSKFile[0] != '\0')
            {
                Impl->ReadSecretKey(std::string(envSKFile));
            }
            else
            {
                const char *envSKHex = std::getenv("ADIOS2_SECRET_KEY");
                if (envSKHex && envSKHex[0] != '\0')
                {
                    Impl->SetSecretKeyHex(std::string(envSKHex));
                }
            }
        }

        // If we have a secret key but no public key, derive the public key
        // from the secret key (Curve25519: pk = sk * basepoint).
        if (Impl->SecretKeyValid && !Impl->PublicKeyValid)
        {
            if (crypto_scalarmult_base(Impl->PublicKey, Impl->SecretKey) != 0)
            {
                throw std::runtime_error(
                    "EncryptionOperator: failed to derive public key from secret key");
            }
            if (sodium_mlock(Impl->PublicKey, crypto_box_PUBLICKEYBYTES) == -1)
            {
                throw std::runtime_error(
                    "EncryptionOperator: unable to lock memory for public key");
            }
            Impl->PublicKeyValid = true;
        }
    }
    else
    {
        // Symmetric mode: in the case "secretkeyfile" is found, so we know
        // the operator should be calling Operate(). If "secretkeyfile" is not
        // found, then the operator should be calling InverseOperate(), due to
        // ADIOS calling InverseOperate() not allowing Parameters to be passed.
        auto skFileIt = m_Parameters.find("secretkeyfile");
        if (skFileIt != m_Parameters.end())
        {
            Impl->KeyFilename = skFileIt->second;
            Impl->GenerateOrReadKey();
        }
    }
}

EncryptionOperator::~EncryptionOperator() {}

#if defined(__clang__)
#if __has_feature(memory_sanitizer)
// Memory Sanitizer has an issue with some libsodium calls.
__attribute__((no_sanitize("memory")))
#endif
#endif
size_t
EncryptionOperator::Operate(const char *dataIn, const Dims &blockStart, const Dims &blockCount,
                            const DataType type, char *bufferOut)
{
    if (Impl->EncryptionMode == EncryptImpl::Mode::Asymmetric)
    {
        if (!Impl->PublicKeyValid)
        {
            throw std::runtime_error("EncryptionOperator::Operate (asymmetric) was called, but"
                                     " a valid public key has not been loaded. "
                                     "Did you add the PublicKeyFile"
                                     " param when setting up the operator?");
        }

        size_t offset = 0;

        // 1. Write original data size.
        size_t sizeIn = helper::GetTotalSize(blockCount, helper::GetDataTypeSize(type));
        PutParameter(bufferOut, offset, sizeIn);

        // 2. Generate random session key and seal it with the public key.
        unsigned char sessionKey[crypto_secretbox_KEYBYTES];
        crypto_secretbox_keygen(sessionKey);

        const size_t sealedKeySize = crypto_box_SEALBYTES + crypto_secretbox_KEYBYTES;
        unsigned char *sealedKey = reinterpret_cast<unsigned char *>(bufferOut + offset);
        crypto_box_seal(sealedKey, sessionKey, crypto_secretbox_KEYBYTES, Impl->PublicKey);
        offset += sealedKeySize;

        // 3. Generate random nonce.
        unsigned char *nonce = reinterpret_cast<unsigned char *>(bufferOut + offset);
        randombytes_buf(nonce, crypto_secretbox_NONCEBYTES);
        offset += crypto_secretbox_NONCEBYTES;

        // 4. Encrypt data with session key.
        size_t cipherTextSize = sizeIn + crypto_secretbox_MACBYTES;
        unsigned char *cipherText = reinterpret_cast<unsigned char *>(bufferOut + offset);
        crypto_secretbox_easy(cipherText, reinterpret_cast<const unsigned char *>(dataIn), sizeIn,
                              nonce, sessionKey);
        offset += cipherTextSize;

        // 5. Wipe session key from memory.
        sodium_memzero(sessionKey, crypto_secretbox_KEYBYTES);

        return offset;
    }
    else
    {
        if (!Impl->KeyValid)
        {
            throw std::runtime_error("EncryptionOperator::Operate was called, but"
                                     " a valid secret key has not been generated. "
                                     "Did you add the SecretKeyFile"
                                     " param when setting up the operator?");
        }

        // offset for writing into bufferOut
        size_t offset = 0;

        // write any parameters we need to save for the InverseOperate() call
        // In this case, we just write out the size of the data
        size_t sizeIn = helper::GetTotalSize(blockCount, helper::GetDataTypeSize(type));
        PutParameter(bufferOut, offset, sizeIn);

        // create the nonce directly in the output buffer, since we'll need it for
        // decryption
        unsigned char *nonce = reinterpret_cast<unsigned char *>(bufferOut + offset);
        randombytes_buf(nonce, crypto_secretbox_NONCEBYTES);
        offset += crypto_secretbox_NONCEBYTES;

        // encrypt data directly into the output buffer
        size_t cipherTextSize = sizeIn + crypto_secretbox_MACBYTES;
        unsigned char *cipherText = reinterpret_cast<unsigned char *>(bufferOut + offset);
        crypto_secretbox_easy(cipherText, reinterpret_cast<const unsigned char *>(dataIn), sizeIn,
                              nonce, Impl->Key);
        offset += cipherTextSize;

        // need to return the size of data in the buffer
        return offset;
    }
}

#if defined(__clang__)
#if __has_feature(memory_sanitizer)
// Memory Sanitizer has an issue with some libsodium calls.
__attribute__((no_sanitize("memory")))
#endif
#endif
size_t
EncryptionOperator::InverseOperate(const char *bufferIn, const size_t sizeIn, char *dataOut)
{
    if (Impl->EncryptionMode == EncryptImpl::Mode::Asymmetric)
    {
        if (!Impl->PublicKeyValid || !Impl->SecretKeyValid)
        {
            throw std::runtime_error(
                "EncryptionOperator::InverseOperate (asymmetric) was called, but"
                " valid public and secret keys have not been loaded. "
                "Set PublicKeyFile/SecretKeyFile params, or set"
                " ADIOS2_SECRET_KEY_FILE or ADIOS2_SECRET_KEY env var.");
        }

        size_t offset = 0;

        // 1. Read original data size.
        const size_t dataBytes = GetParameter<size_t>(bufferIn, offset);

        // 2. Unseal the session key.
        const size_t sealedKeySize = crypto_box_SEALBYTES + crypto_secretbox_KEYBYTES;
        const unsigned char *sealedKey = reinterpret_cast<const unsigned char *>(bufferIn + offset);
        offset += sealedKeySize;

        unsigned char sessionKey[crypto_secretbox_KEYBYTES];
        if (crypto_box_seal_open(sessionKey, sealedKey, sealedKeySize, Impl->PublicKey,
                                 Impl->SecretKey) != 0)
        {
            throw std::runtime_error("EncryptionOperator: failed to unseal session key. "
                                     "Wrong key pair?");
        }

        // 3. Read nonce.
        const unsigned char *nonce = reinterpret_cast<const unsigned char *>(bufferIn + offset);
        offset += crypto_secretbox_NONCEBYTES;

        // 4. Decrypt data.
        size_t cipherTextSize = dataBytes + crypto_secretbox_MACBYTES;
        const unsigned char *cipherText =
            reinterpret_cast<const unsigned char *>(bufferIn + offset);
        offset += cipherTextSize;

        if (crypto_secretbox_open_easy(reinterpret_cast<unsigned char *>(dataOut), cipherText,
                                       cipherTextSize, nonce, sessionKey) != 0)
        {
            sodium_memzero(sessionKey, crypto_secretbox_KEYBYTES);
            throw std::runtime_error(
                "EncryptionOperator: asymmetric decryption failed, message forged!");
        }

        // 5. Wipe session key.
        sodium_memzero(sessionKey, crypto_secretbox_KEYBYTES);

        return dataBytes;
    }
    else
    {
        size_t offset = 0;

        // need to grab any parameter(s) we saved in Operate()
        const size_t dataBytes = GetParameter<size_t>(bufferIn, offset);

        // grab the nonce ptr
        const unsigned char *nonce = reinterpret_cast<const unsigned char *>(bufferIn + offset);
        offset += crypto_secretbox_NONCEBYTES;

        // grab the cipher text ptr
        size_t cipherTextSize = dataBytes + crypto_secretbox_MACBYTES;
        const unsigned char *cipherText =
            reinterpret_cast<const unsigned char *>(bufferIn + offset);
        offset += cipherTextSize;

        // decrypt directly into dataOut buffer
        if (crypto_secretbox_open_easy(reinterpret_cast<unsigned char *>(dataOut), cipherText,
                                       cipherTextSize, nonce, Impl->Key) != 0)
        {
            throw std::runtime_error("message forged!");
        }

        // return the size of the data
        return dataBytes;
    }
}

bool EncryptionOperator::IsDataTypeValid(const DataType type) const { return true; }

size_t EncryptionOperator::GetEstimatedSize(const size_t ElemCount, const size_t ElemSize,
                                            const size_t ndims, const size_t *dims) const
{
    size_t sizeIn = ElemCount * ElemSize;
    if (Impl->EncryptionMode == EncryptImpl::Mode::Asymmetric)
    {
        return (sizeof(size_t)                                       // Data size
                + (crypto_box_SEALBYTES + crypto_secretbox_KEYBYTES) // Sealed session key
                + crypto_secretbox_NONCEBYTES                        // Nonce
                + sizeIn                                             // Data
                + crypto_secretbox_MACBYTES                          // MAC
        );
    }
    else
    {
        return (sizeof(size_t)                // Data size
                + crypto_secretbox_NONCEBYTES // Nonce
                + sizeIn                      // Data
                + crypto_secretbox_MACBYTES   // MAC
        );
    }
}
} // end namespace plugin
} // end namespace adios2

extern "C" {

adios2::plugin::EncryptionOperator *OperatorCreate(const adios2::Params &parameters)
{
    return new adios2::plugin::EncryptionOperator(parameters);
}

void OperatorDestroy(adios2::plugin::EncryptionOperator *obj) { delete obj; }
}
