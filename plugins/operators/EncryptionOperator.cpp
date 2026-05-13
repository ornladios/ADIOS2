/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "EncryptionOperator.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>

#include <sodium.h>

namespace adios2
{
namespace plugin
{

// Wraps a C-string env var: returns nullopt when the pointer is null or the
// string is empty (checked via strlen, not just the first byte).
static std::optional<std::string_view> Env(const char *s)
{
    if (s != nullptr && std::strlen(s) != 0)
        return std::string_view(s);
    return std::nullopt;
}

static void HexDecode(const std::string &hex, unsigned char *out, size_t len)
{
    if (hex.size() != 2 * len)
        throw std::runtime_error("EncryptionOperator: hex string length " +
                                 std::to_string(hex.size()) + " expected " +
                                 std::to_string(2 * len));
    if (sodium_hex2bin(out, len, hex.c_str(), hex.size(), nullptr, nullptr, nullptr) != 0)
        throw std::runtime_error("EncryptionOperator: invalid hex string");
}

static void LoadKeyFromFile(unsigned char *buf, size_t len, bool &valid, const std::string &path,
                            const std::string &label)
{
    std::ifstream f(path, std::ios::binary);
    if (!f)
        throw std::runtime_error("EncryptionOperator: cannot open " + label + ": " + path);
    f.read(reinterpret_cast<char *>(buf), static_cast<std::streamsize>(len));
    if (!f)
        throw std::runtime_error("EncryptionOperator: " + label + " file too short: " + path);
    if (sodium_mlock(buf, len) == -1)
        throw std::runtime_error("EncryptionOperator: cannot lock memory for " + label);
    valid = true;
}

static void LoadKeyFromHex(unsigned char *buf, size_t len, bool &valid, const std::string &hex,
                           const std::string &label)
{
    HexDecode(hex, buf, len);
    if (sodium_mlock(buf, len) == -1)
        throw std::runtime_error("EncryptionOperator: cannot lock memory for " + label);
    valid = true;
}

// Sealed session key: ephemeral public key (32 B) + encrypted key (32 B) + MAC (16 B).
static constexpr size_t kSealedKeySize = crypto_box_SEALBYTES + crypto_secretbox_KEYBYTES;

struct EncryptionOperator::EncryptImpl
{
    enum class Mode
    {
        Symmetric,
        Asymmetric
    };
    Mode EncryptionMode = Mode::Symmetric;

    unsigned char Key[crypto_secretbox_KEYBYTES] = {};
    unsigned char PublicKey[crypto_box_PUBLICKEYBYTES] = {};
    unsigned char SecretKey[crypto_box_SECRETKEYBYTES] = {};
    bool KeyValid = false;
    bool PublicKeyValid = false;
    bool SecretKeyValid = false;

    ~EncryptImpl()
    {
        sodium_munlock(Key, crypto_secretbox_KEYBYTES);
        sodium_munlock(PublicKey, crypto_box_PUBLICKEYBYTES);
        sodium_munlock(SecretKey, crypto_box_SECRETKEYBYTES);
    }

    // Read or generate the symmetric key at `filename` (generates if absent).
    void GenerateOrReadKey(const std::string &filename)
    {
        std::fstream f(filename);
        if (f)
        {
            f.read(reinterpret_cast<char *>(Key), crypto_secretbox_KEYBYTES);
        }
        else
        {
            f.open(filename, std::fstream::out);
            if (!f)
                throw std::runtime_error("EncryptionOperator: cannot create key file: " + filename);
            crypto_secretbox_keygen(Key);
            f.write(reinterpret_cast<char *>(Key), crypto_secretbox_KEYBYTES);
        }
        if (sodium_mlock(Key, crypto_secretbox_KEYBYTES) == -1)
            throw std::runtime_error("EncryptionOperator: cannot lock memory for symmetric key");
        KeyValid = true;
    }
};

EncryptionOperator::EncryptionOperator(const Params &parameters)
: PluginOperatorInterface(parameters), Impl(new EncryptImpl)
{
    if (sodium_init() < 0)
        throw std::runtime_error("EncryptionOperator: libsodium initialization failed");

    const auto envPKFile = Env(std::getenv("ADIOS2_PUBLIC_KEY_FILE"));
    const auto envPKHex = Env(std::getenv("ADIOS2_PUBLIC_KEY"));
    const auto envASKFile = Env(std::getenv("ADIOS2_ASYM_SECRET_KEY_FILE"));
    const auto envASKHex = Env(std::getenv("ADIOS2_ASYM_SECRET_KEY"));
    const auto envSKHex = Env(std::getenv("ADIOS2_SECRET_KEY"));

    const auto end = m_Parameters.end();
    const auto pkFile = m_Parameters.find("publickeyfile");
    const auto pkHex = m_Parameters.find("publickey");
    const auto skFile = m_Parameters.find("secretkeyfile");
    const auto skHex = m_Parameters.find("secretkey");
    const auto modeIt = m_Parameters.find("mode");

    const bool hasAsymEnv = envPKFile || envPKHex || envASKFile || envASKHex;
    const bool isAsymmetric = hasAsymEnv || pkFile != end || pkHex != end ||
                              (modeIt != end && modeIt->second == "asymmetric");

    if (isAsymmetric)
    {
        Impl->EncryptionMode = EncryptImpl::Mode::Asymmetric;

        // Public key: env file → env hex → param file → param hex.
        if (envPKFile)
            LoadKeyFromFile(Impl->PublicKey, crypto_box_PUBLICKEYBYTES, Impl->PublicKeyValid,
                            std::string(*envPKFile), "public key");
        else if (envPKHex)
            LoadKeyFromHex(Impl->PublicKey, crypto_box_PUBLICKEYBYTES, Impl->PublicKeyValid,
                           std::string(*envPKHex), "public key");
        else if (pkFile != end)
            LoadKeyFromFile(Impl->PublicKey, crypto_box_PUBLICKEYBYTES, Impl->PublicKeyValid,
                            pkFile->second, "public key");
        else if (pkHex != end)
            LoadKeyFromHex(Impl->PublicKey, crypto_box_PUBLICKEYBYTES, Impl->PublicKeyValid,
                           pkHex->second, "public key");

        // Secret key: env file → env hex → param file → param hex.
        if (envASKFile)
            LoadKeyFromFile(Impl->SecretKey, crypto_box_SECRETKEYBYTES, Impl->SecretKeyValid,
                            std::string(*envASKFile), "secret key");
        else if (envASKHex)
            LoadKeyFromHex(Impl->SecretKey, crypto_box_SECRETKEYBYTES, Impl->SecretKeyValid,
                           std::string(*envASKHex), "secret key");
        else if (skFile != end)
            LoadKeyFromFile(Impl->SecretKey, crypto_box_SECRETKEYBYTES, Impl->SecretKeyValid,
                            skFile->second, "secret key");
        else if (skHex != end)
            LoadKeyFromHex(Impl->SecretKey, crypto_box_SECRETKEYBYTES, Impl->SecretKeyValid,
                           skHex->second, "secret key");

        // Derive public key from secret key when only the secret key was provided.
        if (Impl->SecretKeyValid && !Impl->PublicKeyValid)
        {
            if (crypto_scalarmult_base(Impl->PublicKey, Impl->SecretKey) != 0)
                throw std::runtime_error("EncryptionOperator: public key derivation failed");
            if (sodium_mlock(Impl->PublicKey, crypto_box_PUBLICKEYBYTES) == -1)
                throw std::runtime_error("EncryptionOperator: cannot lock memory for public key");
            Impl->PublicKeyValid = true;
        }
    }
    else
    {
        // Symmetric: env hex → param file (generates if absent) → param hex.
        if (envSKHex)
            LoadKeyFromHex(Impl->Key, crypto_secretbox_KEYBYTES, Impl->KeyValid,
                           std::string(*envSKHex), "symmetric key");
        else if (skFile != end)
            Impl->GenerateOrReadKey(skFile->second);
        else if (skHex != end)
            LoadKeyFromHex(Impl->Key, crypto_secretbox_KEYBYTES, Impl->KeyValid, skHex->second,
                           "symmetric key");
    }
}

EncryptionOperator::~EncryptionOperator() {}

#if defined(__clang__)
#if __has_feature(memory_sanitizer)
__attribute__((no_sanitize("memory")))
#endif
#endif
size_t
EncryptionOperator::Operate(const char *dataIn, const Dims &blockStart, const Dims &blockCount,
                            const DataType type, char *bufferOut)
{
    size_t offset = 0;
    const size_t sizeIn = GetTotalSize(blockCount, GetDataTypeSize(type));
    PutParameter(bufferOut, offset, sizeIn);

    if (Impl->EncryptionMode == EncryptImpl::Mode::Asymmetric)
    {
        if (!Impl->PublicKeyValid)
            throw std::runtime_error(
                "EncryptionOperator: no public key for encryption. "
                "Set ADIOS2_PUBLIC_KEY_FILE, ADIOS2_PUBLIC_KEY, or PublicKeyFile param.");

        unsigned char sessionKey[crypto_secretbox_KEYBYTES];
        crypto_secretbox_keygen(sessionKey);

        crypto_box_seal(reinterpret_cast<unsigned char *>(bufferOut + offset), sessionKey,
                        crypto_secretbox_KEYBYTES, Impl->PublicKey);
        offset += kSealedKeySize;

        auto *nonce = reinterpret_cast<unsigned char *>(bufferOut + offset);
        randombytes_buf(nonce, crypto_secretbox_NONCEBYTES);
        offset += crypto_secretbox_NONCEBYTES;

        crypto_secretbox_easy(reinterpret_cast<unsigned char *>(bufferOut + offset),
                              reinterpret_cast<const unsigned char *>(dataIn), sizeIn, nonce,
                              sessionKey);
        offset += sizeIn + crypto_secretbox_MACBYTES;
        sodium_memzero(sessionKey, crypto_secretbox_KEYBYTES);
    }
    else
    {
        if (!Impl->KeyValid)
            throw std::runtime_error("EncryptionOperator: no symmetric key for encryption. "
                                     "Set ADIOS2_SECRET_KEY or SecretKeyFile param.");

        auto *nonce = reinterpret_cast<unsigned char *>(bufferOut + offset);
        randombytes_buf(nonce, crypto_secretbox_NONCEBYTES);
        offset += crypto_secretbox_NONCEBYTES;

        crypto_secretbox_easy(reinterpret_cast<unsigned char *>(bufferOut + offset),
                              reinterpret_cast<const unsigned char *>(dataIn), sizeIn, nonce,
                              Impl->Key);
        offset += sizeIn + crypto_secretbox_MACBYTES;
    }

    return offset;
}

#if defined(__clang__)
#if __has_feature(memory_sanitizer)
__attribute__((no_sanitize("memory")))
#endif
#endif
size_t
EncryptionOperator::InverseOperate(const char *bufferIn, const size_t sizeIn, char *dataOut)
{
    size_t offset = 0;
    const size_t dataBytes = GetParameter<size_t>(bufferIn, offset);

    if (Impl->EncryptionMode == EncryptImpl::Mode::Asymmetric)
    {
        if (!Impl->PublicKeyValid || !Impl->SecretKeyValid)
            throw std::runtime_error("EncryptionOperator: no key pair for decryption. "
                                     "Set ADIOS2_ASYM_SECRET_KEY_FILE or ADIOS2_ASYM_SECRET_KEY.");

        const auto *sealedKey = reinterpret_cast<const unsigned char *>(bufferIn + offset);
        offset += kSealedKeySize;

        unsigned char sessionKey[crypto_secretbox_KEYBYTES];
        if (crypto_box_seal_open(sessionKey, sealedKey, kSealedKeySize, Impl->PublicKey,
                                 Impl->SecretKey) != 0)
            throw std::runtime_error("EncryptionOperator: failed to unseal session key. "
                                     "Wrong key pair?");

        const auto *nonce = reinterpret_cast<const unsigned char *>(bufferIn + offset);
        offset += crypto_secretbox_NONCEBYTES;

        const size_t cipherTextSize = dataBytes + crypto_secretbox_MACBYTES;
        if (crypto_secretbox_open_easy(reinterpret_cast<unsigned char *>(dataOut),
                                       reinterpret_cast<const unsigned char *>(bufferIn + offset),
                                       cipherTextSize, nonce, sessionKey) != 0)
        {
            sodium_memzero(sessionKey, crypto_secretbox_KEYBYTES);
            throw std::runtime_error("message forged!");
        }
        sodium_memzero(sessionKey, crypto_secretbox_KEYBYTES);
    }
    else
    {
        if (!Impl->KeyValid)
            throw std::runtime_error("EncryptionOperator: no symmetric key for decryption. "
                                     "Set ADIOS2_SECRET_KEY or SecretKeyFile param.");

        const auto *nonce = reinterpret_cast<const unsigned char *>(bufferIn + offset);
        offset += crypto_secretbox_NONCEBYTES;

        const size_t cipherTextSize = dataBytes + crypto_secretbox_MACBYTES;
        if (crypto_secretbox_open_easy(reinterpret_cast<unsigned char *>(dataOut),
                                       reinterpret_cast<const unsigned char *>(bufferIn + offset),
                                       cipherTextSize, nonce, Impl->Key) != 0)
            throw std::runtime_error("message forged!");
    }

    return dataBytes;
}

bool EncryptionOperator::IsDataTypeValid(const DataType type) const { return true; }

size_t EncryptionOperator::GetEstimatedSize(const size_t ElemCount, const size_t ElemSize,
                                            const size_t ndims, const size_t *dims) const
{
    const size_t sizeIn = ElemCount * ElemSize;
    if (Impl->EncryptionMode == EncryptImpl::Mode::Asymmetric)
    {
        return (sizeof(size_t)   // original data size
                + kSealedKeySize // sealed session key
                + crypto_secretbox_NONCEBYTES + sizeIn + crypto_secretbox_MACBYTES);
    }
    return (sizeof(size_t) + crypto_secretbox_NONCEBYTES + sizeIn + crypto_secretbox_MACBYTES);
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
