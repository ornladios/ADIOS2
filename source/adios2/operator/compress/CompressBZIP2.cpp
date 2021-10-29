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

CompressBZIP2::CompressBZIP2(const Params &parameters)
: Operator("bzip2", parameters)
{
}

size_t CompressBZIP2::Operate(const char *dataIn, const Dims &blockStart,
                              const Dims &blockCount, DataType type,
                              char *bufferOut, const Params &parameters)
{

    const uint8_t bufferVersion = 1;
    unsigned int destOffset = 0;

    // Universal operator metadata
    PutParameter(bufferOut, destOffset, OperatorType::BZIP2);
    PutParameter(bufferOut, destOffset, bufferVersion);
    PutParameter(bufferOut, destOffset, static_cast<uint16_t>(0));
    // Universal operator metadata end

    const size_t sizeIn =
        helper::GetTotalSize(blockCount, helper::GetDataTypeSize(type));
    const size_t batches = sizeIn / DefaultMaxFileBatchSize + 1;

    // bzip2 V1 metadata
    PutParameter(bufferOut, destOffset, sizeIn);
    PutParameter(bufferOut, destOffset, batches);
    // bzip2 V1 metadata end

    int blockSize100k = 1;
    int verbosity = 0;
    int workFactor = 0;
    if (!parameters.empty())
    {
        const std::string hint(" in call to CompressBZIP2 Compress " +
                               ToString(type) + "\n");
        helper::SetParameterValueInt("blockSize100k", parameters, blockSize100k,
                                     hint);
        helper::SetParameterValueInt("verbosity", parameters, verbosity, hint);
        helper::SetParameterValueInt("workFactor", parameters, workFactor,
                                     hint);
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

    unsigned int sourceOffset = 0;
    unsigned int batchInfoOffset = destOffset;
    destOffset += batches * 4 * sizeof(unsigned int);

    for (size_t b = 0; b < batches; ++b)
    {
        char *source = const_cast<char *>(dataIn) + sourceOffset;

        const size_t batchSize = (b == batches - 1)
                                     ? sizeIn % DefaultMaxFileBatchSize
                                     : DefaultMaxFileBatchSize;
        unsigned int sourceLen = static_cast<unsigned int>(batchSize);

        char *dest = bufferOut + destOffset;
        unsigned int destLen = sourceLen;

        int status =
            BZ2_bzBuffToBuffCompress(dest, &destLen, source, sourceLen,
                                     blockSize100k, verbosity, workFactor);

        CheckStatus(status, "in call to ADIOS2 BZIP2 Compress batch " +
                                std::to_string(b) + "\n");

        // bzip2 V1 metadata
        PutParameter(bufferOut, batchInfoOffset, sourceOffset);
        PutParameter(bufferOut, batchInfoOffset, sourceLen);
        PutParameter(bufferOut, batchInfoOffset, destOffset);
        PutParameter(bufferOut, batchInfoOffset, destLen);
        // bzip2 V1 metadata end

        sourceOffset += sourceLen;
        destOffset += destLen;
    }

    return destOffset;
}

size_t CompressBZIP2::InverseOperate(const char *bufferIn, const size_t sizeIn,
                                     char *dataOut)
{
    size_t bufferInOffset = 1; // skip operator type
    const uint8_t bufferVersion =
        GetParameter<uint8_t>(bufferIn, bufferInOffset);
    bufferInOffset += 2; // skip two reserved bytes

    if (bufferVersion == 1)
    {
        // pass in the whole buffer as there is absolute positions saved in the
        // buffer to determine the offsets and lengths for batches
        return DecompressV1(bufferIn, sizeIn, dataOut);
    }
    else if (bufferVersion == 2)
    {
        // TODO: if a Version 2 bzip2 buffer is being implemented, put it here
        // and keep the DecompressV1 routine for backward compatibility
    }
    else
    {
        throw("unknown bzip2 buffer version");
    }

    return 0;
}

bool CompressBZIP2::IsDataTypeValid(const DataType type) const { return true; }

size_t CompressBZIP2::DecompressV1(const char *bufferIn, const size_t sizeIn,
                                   char *dataOut)
{
    // Do NOT remove even if the buffer version is updated. Data might be still
    // in lagacy formats. This function must be kept for backward compatibility.
    // If a newer buffer format is implemented, create another function, e.g.
    // DecompressV2 and keep this function for decompressing lagacy data.

    size_t bufferInOffset = 4; // skip the first four bytes

    size_t sizeOut = GetParameter<size_t, size_t>(bufferIn, bufferInOffset);
    size_t batches = GetParameter<size_t, size_t>(bufferIn, bufferInOffset);

    int small = 0;
    int verbosity = 0;

    size_t expectedSizeOut = 0;

    for (size_t b = 0; b < batches; ++b)
    {
        unsigned int destOffset =
            GetParameter<unsigned int>(bufferIn, bufferInOffset);

        bufferInOffset += sizeof(unsigned int);

        char *dest = dataOut + destOffset;

        const size_t batchSize = (b == batches - 1)
                                     ? sizeOut % DefaultMaxFileBatchSize
                                     : DefaultMaxFileBatchSize;

        unsigned int destLen = static_cast<unsigned int>(batchSize);

        unsigned int sourceOffset =
            GetParameter<unsigned int>(bufferIn, bufferInOffset);

        char *source = const_cast<char *>(bufferIn) + sourceOffset;

        unsigned int sourceLen =
            GetParameter<unsigned int>(bufferIn, bufferInOffset);

        int status = BZ2_bzBuffToBuffDecompress(dest, &destLen, source,
                                                sourceLen, small, verbosity);

        CheckStatus(status, "in call to ADIOS2 BZIP2 Decompress batch " +
                                std::to_string(b) + "\n");

        expectedSizeOut += static_cast<size_t>(destLen);
    }

    if (expectedSizeOut != sizeOut)
    {
        throw("corrupted bzip2 buffer");
    }

    return sizeOut;
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
