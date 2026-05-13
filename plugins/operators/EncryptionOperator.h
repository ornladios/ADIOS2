/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENCRYPTIONOPERATOR_H_
#define ENCRYPTIONOPERATOR_H_

#include <memory>

#include "adios2/plugin/PluginOperatorInterface.h"

namespace adios2
{
namespace plugin
{

/** EncryptionOperator: libsodium-based encryption plugin.
 *
 * Mode is selected automatically. Setting any asymmetric env var or public-key
 * parameter activates asymmetric mode; otherwise symmetric mode is used.
 *
 * Symmetric (XSalsa20-Poly1305 / crypto_secretbox):
 *   Key lookup: ADIOS2_SECRET_KEY env (hex) → SecretKeyFile param → SecretKey param.
 *   SecretKeyFile generates a new key on first write if the file does not exist.
 *
 * Asymmetric (Curve25519 + XSalsa20-Poly1305 hybrid):
 *   Writer needs the public key; reader needs the secret key.
 *   Public key lookup:  ADIOS2_PUBLIC_KEY_FILE → ADIOS2_PUBLIC_KEY → PublicKeyFile → PublicKey
 *   Secret key lookup:  ADIOS2_ASYM_SECRET_KEY_FILE → ADIOS2_ASYM_SECRET_KEY
 *                       → SecretKeyFile → SecretKey
 *   If only the secret key is supplied, the public key is derived automatically.
 *
 * Use adios2_seal_keygen to generate Curve25519 key pairs.
 */
class EncryptionOperator : public PluginOperatorInterface
{
public:
    EncryptionOperator(const Params &parameters);
    virtual ~EncryptionOperator();

    size_t Operate(const char *dataIn, const Dims &blockStart, const Dims &blockCount,
                   const DataType type, char *bufferOut) override;

    size_t InverseOperate(const char *bufferIn, const size_t sizeIn, char *dataOut) override;

    bool IsDataTypeValid(const DataType type) const override;

    size_t GetEstimatedSize(const size_t ElemCount, const size_t ElemSize, const size_t ndims,
                            const size_t *dims) const override;

private:
    struct EncryptImpl;
    std::unique_ptr<EncryptImpl> Impl;
};

} // end namespace plugin
} // end namespace adios2

extern "C" {

adios2::plugin::EncryptionOperator *OperatorCreate(const adios2::Params &parameters);
void OperatorDestroy(adios2::plugin::EncryptionOperator *obj);
}

#endif /* ENCYRPTIONOPERATOR_H_ */
