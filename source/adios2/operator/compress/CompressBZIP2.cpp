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

size_t CompressBZIP2::Compress(const char *dataIn, const Dims &dimensions,
                               DataType type, char *bufferOut,
                               const Params &parameters, Params & /*info*/)
{
    // defaults
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

    const size_t sizeIn =
        helper::GetTotalSize(dimensions, helper::GetDataTypeSize(type));

    const size_t batches = sizeIn / DefaultMaxFileBatchSize + 1;
    unsigned int destOffset = 0;
    unsigned int sourceOffset = 0;

    PutParameter(bufferOut, destOffset, sizeIn);
    PutParameter(bufferOut, destOffset, batches);
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

        const std::string bStr = std::to_string(b);

        CheckStatus(status, "in call to ADIOS2 BZIP2 Compress batch " + bStr);

        PutParameter(bufferOut, batchInfoOffset, sourceOffset);
        PutParameter(bufferOut, batchInfoOffset, sourceLen);
        PutParameter(bufferOut, batchInfoOffset, destOffset);
        PutParameter(bufferOut, batchInfoOffset, destLen);

        sourceOffset += sourceLen;
        destOffset += destLen;
    }

    return destOffset;
}

size_t CompressBZIP2::Decompress(const char *bufferIn, const size_t sizeIn,
                                 char *dataOut, const DataType /*type*/,
                                 const Dims & /*blockStart*/,
                                 const Dims & /*blockCount*/,
                                 const Params & /*parameters*/,
                                 Params & /*info*/)
{
    size_t sourceOffset = 0;

    size_t sizeOut = GetParameter<size_t>(bufferIn, sourceOffset);
    size_t batches = GetParameter<size_t>(bufferIn, sourceOffset);

    size_t batchInfoOffset = sourceOffset;
    sourceOffset += batches * 4 * sizeof(unsigned int);

    int small = 0;
    int verbosity = 0;

    size_t expectedSizeOut = 0;

    for (size_t b = 0; b < batches; ++b)
    {
        const std::string bStr = std::to_string(b);

        unsigned int destOffset =
            GetParameter<unsigned int>(bufferIn, batchInfoOffset);

        GetParameter<unsigned int>(bufferIn, batchInfoOffset);

        char *dest = dataOut + destOffset;

        const size_t batchSize = (b == batches - 1)
                                     ? sizeOut % DefaultMaxFileBatchSize
                                     : DefaultMaxFileBatchSize;

        unsigned int destLen = static_cast<unsigned int>(batchSize);

        unsigned int sourceOffset =
            GetParameter<unsigned int>(bufferIn, batchInfoOffset);

        char *source = const_cast<char *>(bufferIn) + sourceOffset;

        unsigned int sourceLen =
            GetParameter<unsigned int>(bufferIn, batchInfoOffset);

        int status = BZ2_bzBuffToBuffDecompress(dest, &destLen, source,
                                                sourceLen, small, verbosity);

        CheckStatus(status, "in call to ADIOS2 BZIP2 Decompress\n");

        expectedSizeOut += static_cast<size_t>(destLen);
    }

    return expectedSizeOut;
}

bool CompressBZIP2::IsDataTypeValid(const DataType type) const { return true; }

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
