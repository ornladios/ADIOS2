/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressBlosc.cpp
 *
 *  Created on: Jun 18, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "CompressBlosc.h"

extern "C" {
#include <blosc.h>
}

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace core
{
namespace compress
{

const std::map<std::string, uint32_t> CompressBlosc::m_Shuffles = {
    {"BLOSC_NOSHUFFLE", BLOSC_NOSHUFFLE},
    {"BLOSC_SHUFFLE", BLOSC_SHUFFLE},
    {"BLOSC_BITSHUFFLE", BLOSC_BITSHUFFLE}};

const std::set<std::string> CompressBlosc::m_Compressors = {
    "blosclz", "lz4", "lz4hc", "snappy", "zlib", "zstd"};

CompressBlosc::CompressBlosc(const Params &parameters, const bool debugMode)
: Operator("blosc", parameters, debugMode)
{
}

size_t CompressBlosc::Compress(const void *dataIn, const Dims &dimensions,
                               const size_t elementSize, const std::string type,
                               void *bufferOut, const Params &parameters,
                               Params &info) const
{
    const size_t sizeIn =
        static_cast<size_t>(helper::GetTotalSize(dimensions) * elementSize);

    blosc_init();

    size_t threads = 1; // defaults
    int compressionLevel = 1;
    int doShuffle = BLOSC_SHUFFLE;
    std::string compressor = "blosclz";
    size_t blockSize = 0;

    for (const auto &itParameter : parameters)
    {
        const std::string key = itParameter.first;
        const std::string value = itParameter.second;

        if (key == "compression_level" || key == "clevel")
        {
            compressionLevel = static_cast<int>(helper::StringTo<int32_t>(
                value, m_DebugMode, "when setting Blosc clevel parameter\n"));
            if (m_DebugMode)
            {
                if (compressionLevel < 0 || compressionLevel > 9)
                {
                    throw std::invalid_argument(
                        "ERROR: compression_level must be an "
                        "integer between 0 (default: no compression) and 9 "
                        "(more compression, more memory) inclusive, in call to "
                        "ADIOS2 Blosc Compress\n");
                }
            }
        }
        else if (key == "doshuffle")
        {
            auto itShuffle = m_Shuffles.find(value);
            if (m_DebugMode)
            {
                if (itShuffle == m_Shuffles.end())
                {
                    throw std::invalid_argument(
                        "ERROR: invalid shuffle vale " + value +
                        " must be BLOSC_SHUFFLE, BLOSC_NOSHUFFLE or "
                        "BLOSC_BITSHUFFLE,  "
                        " in call to ADIOS2 Blosc Compress\n");
                }
            }
            doShuffle = itShuffle->second;
        }
        else if (key == "nthreads")
        {
            threads = static_cast<int>(helper::StringTo<int32_t>(
                value, m_DebugMode, "when setting Blosc nthreads parameter\n"));
        }
        else if (key == "compressor")
        {
            compressor = value;
            if (m_DebugMode && m_Compressors.count(compressor) == 0)
            {
                throw std::invalid_argument(
                    "ERROR: invalid compressor " + compressor +
                    " valid values: blosclz (default), lz4, lz4hc, snappy, "
                    "zlib, or ztsd, in call to ADIOS2 Blosc Compression\n");
            }
        }
        else if (key == "blocksize")
        {
            blockSize = static_cast<size_t>(helper::StringTo<uint64_t>(
                value, m_DebugMode,
                "when setting Blosc blocksize parameter\n"));
        }
    }

    const int result = blosc_set_compressor(compressor.c_str());
    if (m_DebugMode && result == -1)
    {
        throw std::invalid_argument("ERROR: invalid compressor " + compressor +
                                    " check if supported by blosc build, in "
                                    "call to ADIOS2 Blosc Compression\n");
    }

    blosc_set_nthreads(threads);
    blosc_set_blocksize(blockSize);

    const int compressedSize =
        blosc_compress(compressionLevel, doShuffle, elementSize, sizeIn, dataIn,
                       bufferOut, sizeIn);

    if (m_DebugMode && compressedSize <= 0)
    {
        throw std::invalid_argument(
            "ERROR: from blosc_compress return size: " +
            std::to_string(compressedSize) +
            ", check operator parameters, "
            " compression failed in ADIOS2 Blosc Compression\n");
    }

    blosc_destroy();
    return static_cast<size_t>(compressedSize);
}

size_t CompressBlosc::Decompress(const void *bufferIn, const size_t sizeIn,
                                 void *dataOut, const size_t sizeOut,
                                 Params &info) const
{
    blosc_init();
    const int decompressedSize = blosc_decompress(bufferIn, dataOut, sizeOut);
    blosc_destroy();
    return static_cast<size_t>(decompressedSize);
}

} // end namespace compress
} // end namespace core
} // end namespace adios2
