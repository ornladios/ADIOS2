/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressZSTD.cpp
 *
 *  Created on: March 4, 2025
 *      Author: Vicente Adolfo Bolea Sanchez
 */

#include "CompressZSTD.h"

#include <cmath>     //std::ceil
#include <ios>       //std::ios_base::failure
#include <stdexcept> //std::invalid_argument

extern "C" {
#include <zstd.h>
}

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace core
{
namespace compress
{

CompressZSTD::CompressZSTD(const Params &parameters)
: Operator("zstd", COMPRESS_ZSTD, "compress", parameters)
{
}

size_t CompressZSTD::Operate(const char *dataIn, const Dims &blockStart, const Dims &blockCount,
                             DataType type, char *bufferOut)
{
    const uint8_t bufferVersion = 1;
    unsigned int destOffset = 0;

    MakeCommonHeader(bufferOut, destOffset, bufferVersion);

    const size_t sizeIn = helper::GetTotalSize(blockCount, helper::GetDataTypeSize(type));
    const size_t batches = sizeIn / DefaultMaxFileBatchSize + 1;

    // ZSTD V1 metadata
    PutParameter(bufferOut, destOffset, sizeIn);
    PutParameter(bufferOut, destOffset, batches);
    // ZSTD V1 metadata end

    int compressionLevel = 1; // Default zstd compression level (1-22)
    int nThreads = 1;         // Default: single-threaded

    if (!m_Parameters.empty())
    {
        const std::string hint(" in call to CompressZSTD Compress " + ToString(type) + "\n");
        helper::SetParameterValueInt("compressionLevel", m_Parameters, compressionLevel, hint);
        helper::SetParameterValueInt("nThreads", m_Parameters, nThreads, hint);
        if (compressionLevel < 1 || compressionLevel > 22)
        {
            helper::Throw<std::invalid_argument>("Operator", "CompressZSTD", "Operate",
                                                 "compressionLevel must be an "
                                                 "integer between 1 (less "
                                                 "compression, less memory) and 22 "
                                                 "(more compression, more memory) inclusive, " +
                                                     hint);
        }
        if (nThreads < 1)
        {
            helper::Throw<std::invalid_argument>("Operator", "CompressZSTD", "Operate",
                                                 "nThreads must be greater than or equal to 1, " +
                                                     hint);
        }
    }

    unsigned int sourceOffset = 0;
    unsigned int batchInfoOffset = destOffset;
    destOffset += static_cast<unsigned int>(batches * 4 * sizeof(unsigned int));

    for (size_t b = 0; b < batches; ++b)
    {
        char *source = const_cast<char *>(dataIn) + sourceOffset;

        const size_t batchSize =
            (b == batches - 1) ? sizeIn % DefaultMaxFileBatchSize : DefaultMaxFileBatchSize;
        unsigned int sourceLen = static_cast<unsigned int>(batchSize);

        char *dest = bufferOut + destOffset;

        // Set up compression context based on nThreads
        ZSTD_CCtx *cctx = ZSTD_createCCtx();
        if (cctx == NULL)
        {
            helper::Throw<std::runtime_error>("Operator", "CompressZSTD", "Operate",
                                              "Failed to create ZSTD compression context");
        }

        // Set parameters for the context
        ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, compressionLevel);

        if (nThreads > 1)
        {
            ZSTD_CCtx_setParameter(cctx, ZSTD_c_nbWorkers, nThreads);
        }

        // Calculate maximum compressed size
        size_t maxDestSize = ZSTD_compressBound(sourceLen);

        // Check if buffer has enough space for compression
        if (maxDestSize > sourceLen)
        {
            helper::Throw<std::runtime_error>(
                "Operator", "CompressZSTD", "Operate",
                "Output buffer may not be large enough for compression");
        }

        // Compress data with context
        size_t destLen =
            ZSTD_compressCCtx(cctx, dest, maxDestSize, source, sourceLen, compressionLevel);

        // Free the context
        ZSTD_freeCCtx(cctx);

        // Check for errors
        if (ZSTD_isError(destLen))
        {
            CheckStatus(destLen,
                        "in call to ADIOS2 ZSTD Compress batch " + std::to_string(b) + "\n");
        }

        // ZSTD V1 metadata
        PutParameter(bufferOut, batchInfoOffset, sourceOffset);
        PutParameter(bufferOut, batchInfoOffset, sourceLen);
        PutParameter(bufferOut, batchInfoOffset, destOffset);
        PutParameter(bufferOut, batchInfoOffset, static_cast<unsigned int>(destLen));
        // ZSTD V1 metadata end

        sourceOffset += sourceLen;
        destOffset += static_cast<unsigned int>(destLen);
    }

    return destOffset;
}

size_t CompressZSTD::InverseOperate(const char *bufferIn, const size_t sizeIn, char *dataOut)
{
    size_t bufferInOffset = 1; // skip operator type
    const uint8_t bufferVersion = GetParameter<uint8_t>(bufferIn, bufferInOffset);
    bufferInOffset += 2; // skip two reserved bytes

    if (bufferVersion == 1)
    {
        // pass in the whole buffer as there is absolute positions saved in the
        // buffer to determine the offsets and lengths for batches
        return DecompressV1(bufferIn, sizeIn, dataOut);
    }
    else if (bufferVersion == 2)
    {
        // TODO: if a Version 2 ZSTD buffer is being implemented, put it here
        // and keep the DecompressV1 routine for backward compatibility
    }
    else
    {
        helper::Throw<std::runtime_error>("Operator", "CompressZSTD", "InverseOperate",
                                          "invalid ZSTD buffer version");
    }

    return 0;
}

bool CompressZSTD::IsDataTypeValid(const DataType type) const { return true; }

size_t CompressZSTD::DecompressV1(const char *bufferIn, const size_t sizeIn, char *dataOut)
{
    // Do NOT remove even if the buffer version is updated. Data might be still
    // in legacy formats. This function must be kept for backward compatibility.
    // If a newer buffer format is implemented, create another function, e.g.
    // DecompressV2 and keep this function for decompressing legacy data.

    size_t bufferInOffset = 4; // skip the first four bytes

    size_t sizeOut = GetParameter<size_t, size_t>(bufferIn, bufferInOffset);
    size_t batches = GetParameter<size_t, size_t>(bufferIn, bufferInOffset);

    size_t expectedSizeOut = 0;

    for (size_t b = 0; b < batches; ++b)
    {
        unsigned int destOffset = GetParameter<unsigned int>(bufferIn, bufferInOffset);

        bufferInOffset += sizeof(unsigned int);

        char *dest = dataOut + destOffset;

        const size_t batchSize =
            (b == batches - 1) ? sizeOut % DefaultMaxFileBatchSize : DefaultMaxFileBatchSize;

        unsigned int destLen = static_cast<unsigned int>(batchSize);

        unsigned int sourceOffset = GetParameter<unsigned int>(bufferIn, bufferInOffset);

        char *source = const_cast<char *>(bufferIn) + sourceOffset;

        unsigned int sourceLen = GetParameter<unsigned int>(bufferIn, bufferInOffset);

        // Create a decompression context (optional for simple cases but allows for advanced
        // settings)
        ZSTD_DCtx *dctx = ZSTD_createDCtx();
        if (dctx == NULL)
        {
            helper::Throw<std::runtime_error>("Operator", "CompressZSTD", "DecompressV1",
                                              "Failed to create ZSTD decompression context");
        }

        // Decompress the data
        size_t result = ZSTD_decompressDCtx(dctx, dest, destLen, source, sourceLen);

        // Free the context
        ZSTD_freeDCtx(dctx);

        // Check for errors
        if (ZSTD_isError(result))
        {
            CheckStatus(result,
                        "in call to ADIOS2 ZSTD Decompress batch " + std::to_string(b) + "\n");
        }

        expectedSizeOut += static_cast<size_t>(result);
    }

    if (expectedSizeOut != sizeOut)
    {
        helper::Throw<std::runtime_error>("Operator", "CompressZSTD", "DecompressV1",
                                          "corrupted ZSTD buffer");
    }

    return sizeOut;
}

void CompressZSTD::CheckStatus(const size_t status, const std::string hint) const
{
    if (ZSTD_isError(status))
    {
        helper::Throw<std::invalid_argument>("Operator", "CompressZSTD", "CheckStatus",
                                             std::string("ZSTD error: ") +
                                                 ZSTD_getErrorName(status) + " " + hint);
    }
}

} // end namespace compress
} // end namespace core
} // end namespace adios2