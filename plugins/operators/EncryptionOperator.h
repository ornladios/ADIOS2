/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * EncryptionOperator.h
 *
 *  Created on: Dec 7, 2021
 *      Author: Caitlin Ross <caitlin.ross@kitware.com>
 */

#ifndef ENCRYPTIONOPERATOR_H_
#define ENCRYPTIONOPERATOR_H_

#include <memory>

#include "adios2/common/ADIOSTypes.h"
#include "adios2/operator/plugin/PluginOperatorInterface.h"

namespace adios2
{
namespace plugin
{

/** EncryptionOperator that uses libsodium. The encryption mode is determined
 * automatically by the parameters supplied.
 *
 * --- Symmetric mode (crypto_secretbox) ---
 * Activated when 'SecretKeyFile' is provided without any public-key params.
 * Both writer and reader must share the same secret key file.
 *
 *   SecretKeyFile  - path to the secret key file; if the file does not exist
 *                    a new key is generated and written there.
 *
 * --- Asymmetric mode (hybrid crypto_box_seal) ---
 * Activated when 'PublicKeyFile' or 'PublicKey' is provided. A random session
 * key is sealed with the recipient's Curve25519 public key; bulk data is
 * encrypted with that session key via crypto_secretbox. Only the public key
 * is needed for writing; both keys are needed for reading.
 *
 *   PublicKeyFile  - path to a 32-byte Curve25519 public key file
 *   PublicKey      - hex-encoded public key (64 hex chars)
 *   SecretKeyFile  - path to a 32-byte secret key file (reading only)
 *   SecretKey      - hex-encoded secret key (64 hex chars, reading only)
 *
 * Secret key lookup order (asymmetric): SecretKeyFile param, SecretKey param,
 *   ADIOS2_SECRET_KEY_FILE env var, ADIOS2_SECRET_KEY env var.
 * When only the secret key is supplied, the public key is derived automatically
 * via Curve25519 (sk * basepoint).
 *
 * Use adios2_seal_keygen to generate asymmetric key pairs.
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
