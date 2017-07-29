/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressBZip2.cpp
 *
 *  Created on: Jul 24, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "CompressBZip2.h"

/// \cond EXCLUDE_FROM_DOXYGEN
#include <cmath>     //std::ceil
#include <ios>       //std::ios_base::failure
#include <stdexcept> //std::invalid_argument
/// \endcond

#include <bzlib.h>

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace transform
{

CompressBZip2::CompressBZip2(const bool debugMode)
: Transform("bzip2", debugMode)
{
}

size_t CompressBZip2::BufferMaxSize(const size_t sizeIn) const
{
    return static_cast<size_t>(std::ceil(1.1 * sizeIn) + 600);
}

size_t CompressBZip2::Compress(const void *dataIn, const Dims &dimensions,
                               const size_t elementSize, const std::string type,
                               void *bufferOut, const Params &parameters) const
{
    // defaults
    int blockSize100k = 1;
    int verbosity = 0;
    int workFactor = 0;

    if (!parameters.empty())
    {
        const std::string hint(" in call to CompressBZip2 Compress " + type +
                               "\n");
        SetParameterValueInt("BlockSize100K", parameters, blockSize100k,
                             m_DebugMode, hint);
        SetParameterValueInt("Verbosity", parameters, verbosity, m_DebugMode,
                             hint);
        SetParameterValueInt("WorkFactor", parameters, workFactor, m_DebugMode,
                             hint);
        if (m_DebugMode == true)
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
        static_cast<const size_t>(GetTotalSize(dimensions) * elementSize);
    // Build inputs to BZip2 compression function
    char *dest = const_cast<char *>(reinterpret_cast<const char *>(bufferOut));
    unsigned int destLen = static_cast<unsigned int>(BufferMaxSize(sizeIn));

    char *source = const_cast<char *>(reinterpret_cast<const char *>(dataIn));
    unsigned int sourceLen = static_cast<unsigned int>(sizeIn);

    int status = BZ2_bzBuffToBuffCompress(dest, &destLen, source, sourceLen,
                                          blockSize100k, verbosity, workFactor);

    if (m_DebugMode)
    {
        CheckStatus(status, "in call to CompressBZip2 Compress\n");
    }

    return static_cast<size_t>(destLen);
}

size_t CompressBZip2::Decompress(const void *bufferIn, const size_t sizeIn,
                                 void *dataOut, const size_t sizeOut) const
{
    // TODO: leave defaults at zero?
    int small = 0;
    int verbosity = 0;

    char *dest = reinterpret_cast<char *>(dataOut);
    unsigned int destLen = static_cast<unsigned int>(sizeOut);

    char *source = const_cast<char *>(reinterpret_cast<const char *>(bufferIn));
    unsigned int sourceLen = static_cast<unsigned int>(sizeIn);

    int status = BZ2_bzBuffToBuffDecompress(dest, &destLen, source, sourceLen,
                                            small, verbosity);

    if (m_DebugMode)
    {
        CheckStatus(status, "in call to CompressBZip2 Decompress\n");
    }

    return static_cast<size_t>(destLen);
}

void CompressBZip2::CheckStatus(const int status, const std::string hint) const
{
    switch (status)
    {

    case (BZ_CONFIG_ERROR):
        throw std::invalid_argument(
            "ERROR: BZ_CONFIG_ERROR, bzip2 library is not configured "
            "correctly" +
            hint);
        break;

    case (BZ_PARAM_ERROR):
        throw std::invalid_argument(
            "ERROR: BZ_PARAM_ERROR bufferOut stream might be null" + hint);
        break;

    case (BZ_MEM_ERROR):
        throw std::ios_base::failure(
            "ERROR: BZ_MEM_ERROR bzip2 detected insufficient memory " + hint);
        break;

    case (BZ_OUTBUFF_FULL):
        throw std::ios_base::failure("ERROR: BZ_OUTBUFF_FULL bzip2 detected "
                                     "size of compressed data is larger than "
                                     "destination length " +
                                     hint);
        break;

    // decompression
    case (BZ_DATA_ERROR):
        throw std::invalid_argument("ERROR: BZ_DATA_ERROR, bzip2 library "
                                    "detected integrity errors in compressed "
                                    "data " +
                                    hint);
        break;

    case (BZ_DATA_ERROR_MAGIC):
        throw std::invalid_argument("ERROR: BZ_DATA_ERROR_MAGIC, bzip2 library "
                                    "detected wrong magic numbers in "
                                    "compressed data " +
                                    hint);
        break;

    case (BZ_UNEXPECTED_EOF):
        throw std::invalid_argument("ERROR: BZ_UNEXPECTED_EOF, bzip2 library "
                                    "detected unexpected end of "
                                    "compressed data " +
                                    hint);
        break;
    }
}

} // end namespace transform
} // end namespace adios2
