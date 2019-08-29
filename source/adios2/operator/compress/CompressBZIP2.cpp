/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressBZIP2.cpp
 *
 *  Created on: Jul 24, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "CompressBZIP2.h"

#include <cmath>     //std::ceil
#include <ios>       //std::ios_base::failure
#include <stdexcept> //std::invalid_argument

extern "C" {
#include <bzlib.h>
}

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace core
{
namespace compress
{

CompressBZIP2::CompressBZIP2(const Params &parameters, const bool debugMode)
: Operator("bzip2", parameters, debugMode)
{
}

size_t CompressBZIP2::BufferMaxSize(const size_t sizeIn) const
{
    return static_cast<size_t>(std::ceil(1.1 * sizeIn) + 600);
}

size_t CompressBZIP2::Compress(const void *dataIn, const Dims &dimensions,
                               const size_t elementSize, const std::string type,
                               void *bufferOut, const Params &parameters,
                               Params &info) const
{
    // defaults
    int blockSize100k = 1;
    int verbosity = 0;
    int workFactor = 0;

    if (!parameters.empty())
    {
        const std::string hint(" in call to CompressBZIP2 Compress " + type +
                               "\n");
        helper::SetParameterValueInt("blockSize100k", parameters, blockSize100k,
                                     m_DebugMode, hint);
        helper::SetParameterValueInt("verbosity", parameters, verbosity,
                                     m_DebugMode, hint);
        helper::SetParameterValueInt("workFactor", parameters, workFactor,
                                     m_DebugMode, hint);
        if (m_DebugMode)
        {

            if (blockSize100k < 1 || blockSize100k > 9)
            {
                throw std::invalid_argument(
                    "ERROR: BlockSize100K must be an "
                    "integer between 1 (less "
                    "compression, less memory) and 9 "
                    "(more compression, more memory) inclusive, " +
                    hint);
            }
        }
    }

    const size_t sizeIn =
        static_cast<size_t>(helper::GetTotalSize(dimensions) * elementSize);

    const size_t batches = sizeIn / DefaultMaxFileBatchSize + 1;
    info["batches"] = std::to_string(batches);

    unsigned int destOffset = 0;
    unsigned int sourceOffset = 0;

    for (size_t b = 0; b < batches; ++b)
    {
        char *source =
            const_cast<char *>(reinterpret_cast<const char *>(dataIn)) +
            sourceOffset;

        const size_t batchSize = (b == batches - 1)
                                     ? sizeIn % DefaultMaxFileBatchSize
                                     : DefaultMaxFileBatchSize;
        unsigned int sourceLen = static_cast<unsigned int>(batchSize);

        char *dest = reinterpret_cast<char *>(bufferOut) + destOffset;
        unsigned int destLen = sourceLen;

        int status =
            BZ2_bzBuffToBuffCompress(dest, &destLen, source, sourceLen,
                                     blockSize100k, verbosity, workFactor);

        const std::string bStr = std::to_string(b);
        if (m_DebugMode)
        {
            CheckStatus(status,
                        "in call to ADIOS2 BZIP2 Compress batch " + bStr);
        }

        // record metadata info
        info["OriginalOffset_" + bStr] = std::to_string(sourceOffset);
        info["OriginalSize_" + bStr] = std::to_string(sourceLen);
        info["CompressedOffset_" + bStr] = std::to_string(destOffset);
        info["CompressedSize_" + bStr] = std::to_string(destLen);

        sourceOffset += sourceLen;
        destOffset += destLen;
    }

    return destOffset;
}

size_t CompressBZIP2::Decompress(const void *bufferIn, const size_t sizeIn,
                                 void *dataOut, const size_t sizeOut,
                                 Params &info) const
{
    // TODO: leave defaults at zero?
    int small = 0;
    int verbosity = 0;

    auto itBatches = info.find("batches");
    const size_t batches =
        (itBatches == info.end())
            ? 1
            : static_cast<size_t>(helper::StringTo<uint32_t>(
                  itBatches->second, m_DebugMode,
                  "when extracting batches in ADIOS2 BZIP2 Decompress"));

    size_t expectedSizeOut = 0;

    for (size_t b = 0; b < batches; ++b)
    {
        const std::string bStr = std::to_string(b);

        const size_t destOffset =
            static_cast<size_t>(helper::StringTo<uint32_t>(
                info.at("OriginalOffset_" + bStr), m_DebugMode,
                "when extracting batches in ADIOS2 BZIP2 Decompress"));

        char *dest = reinterpret_cast<char *>(dataOut) + destOffset;

        const size_t batchSize = (b == batches - 1)
                                     ? sizeOut % DefaultMaxFileBatchSize
                                     : DefaultMaxFileBatchSize;
        unsigned int destLen = static_cast<unsigned int>(batchSize);

        const size_t sourceOffset =
            static_cast<size_t>(helper::StringTo<uint32_t>(
                info.at("CompressedOffset_" + bStr), m_DebugMode,
                "when extracting batches in ADIOS2 BZIP2 Decompress"));
        char *source =
            const_cast<char *>(reinterpret_cast<const char *>(bufferIn)) +
            sourceOffset;

        unsigned int sourceLen =
            static_cast<unsigned int>(helper::StringTo<uint32_t>(
                info.at("CompressedSize_" + bStr), m_DebugMode,
                "when extracting batches in ADIOS2 BZIP2 Decompress"));

        int status = BZ2_bzBuffToBuffDecompress(dest, &destLen, source,
                                                sourceLen, small, verbosity);
        if (m_DebugMode)
        {
            CheckStatus(status, "in call to ADIOS2 BZIP2 Decompress\n");
            // TODO verify size of decompressed buffer
        }
        expectedSizeOut += static_cast<size_t>(destLen);
    }

    return expectedSizeOut;
}

void CompressBZIP2::CheckStatus(const int status, const std::string hint) const
{
    switch (status)
    {
    case (BZ_CONFIG_ERROR):
        throw std::invalid_argument(
            "ERROR: BZ_CONFIG_ERROR, BZIP2 library is not configured "
            "correctly" +
            hint);

    case (BZ_PARAM_ERROR):
        throw std::invalid_argument(
            "ERROR: BZ_PARAM_ERROR bufferOut stream might be null" + hint);

    case (BZ_MEM_ERROR):
        throw std::ios_base::failure(
            "ERROR: BZ_MEM_ERROR BZIP2 detected insufficient memory " + hint);

    case (BZ_OUTBUFF_FULL):
        throw std::ios_base::failure("ERROR: BZ_OUTBUFF_FULL BZIP2 detected "
                                     "size of compressed data is larger than "
                                     "destination length " +
                                     hint);

    // decompression
    case (BZ_DATA_ERROR):
        throw std::invalid_argument("ERROR: BZ_DATA_ERROR, BZIP2 library "
                                    "detected integrity errors in compressed "
                                    "data " +
                                    hint);

    case (BZ_DATA_ERROR_MAGIC):
        throw std::invalid_argument("ERROR: BZ_DATA_ERROR_MAGIC, BZIP2 library "
                                    "detected wrong magic numbers in "
                                    "compressed data " +
                                    hint);

    case (BZ_UNEXPECTED_EOF):
        throw std::invalid_argument("ERROR: BZ_UNEXPECTED_EOF, BZIP2 library "
                                    "detected unexpected end of "
                                    "compressed data " +
                                    hint);
    default:
        break;
    }
}

} // end namespace compress
} // end namespace core
} // end namespace adios2
