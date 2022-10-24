/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressBlosc2.cpp
 *
 *  Created on: Jun 18, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 *              Rene Widera r.widera@hzdr.de
 */

#include "CompressBlosc2.h"

extern "C" {
#include <blosc2.h>
}

#include "adios2/helper/adiosFunctions.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>

namespace adios2
{
namespace core
{
namespace compress
{

const std::map<std::string, uint32_t> CompressBlosc2::m_Shuffles = {
    {"BLOSC_NOSHUFFLE", BLOSC_NOSHUFFLE},
    {"BLOSC_SHUFFLE", BLOSC_SHUFFLE},
    {"BLOSC_BITSHUFFLE", BLOSC_BITSHUFFLE}};

const std::set<std::string> CompressBlosc2::m_Compressors = {
    "blosclz", "lz4", "lz4hc", "snappy", "zlib", "zstd"};

CompressBlosc2::CompressBlosc2(const Params &parameters)
: Operator("blosc2", COMPRESS_BLOSC2, "compress", parameters)
{
}

size_t CompressBlosc2::Operate(const char *dataIn, const Dims &blockStart,
                               const Dims &blockCount, const DataType type,
                               char *bufferOut)
{
    size_t bufferOutOffset = 0;
    const uint8_t bufferVersion = 1;

    MakeCommonHeader(bufferOut, bufferOutOffset, bufferVersion);

    const size_t sizeIn =
        helper::GetTotalSize(blockCount, helper::GetDataTypeSize(type));

    // blosc2 V1 metadata
    PutParameter(bufferOut, bufferOutOffset, sizeIn);
    PutParameter(bufferOut, bufferOutOffset,
                 static_cast<uint8_t>(BLOSC2_VERSION_MAJOR));
    PutParameter(bufferOut, bufferOutOffset,
                 static_cast<uint8_t>(BLOSC2_VERSION_MINOR));
    PutParameter(bufferOut, bufferOutOffset,
                 static_cast<uint8_t>(BLOSC2_VERSION_RELEASE));
    // blosc2 V1 metadata end

    bool useMemcpy = false;

    // input size under this bound will not compress
    size_t thresholdSize = 128;

    blosc2_init();

    size_t threads = 1; // defaults
    int compressionLevel = 1;
    int doShuffle = BLOSC_SHUFFLE;
    std::string compressor = "blosclz";
    size_t blockSize = 0;

    for (const auto &itParameter : m_Parameters)
    {
        const std::string key = itParameter.first;
        const std::string value = itParameter.second;

        if (key == "compression_level" || key == "clevel")
        {
            compressionLevel = static_cast<int>(helper::StringTo<int32_t>(
                value, "when setting Blosc2 clevel parameter\n"));
            if (compressionLevel < 0 || compressionLevel > 9)
            {
                helper::Throw<std::invalid_argument>(
                    "Operator", "CompressBlosc2", "Operate",
                    "compression_level must be an integer between 0 (no "
                    "compression) and 9 (more compression, more memory "
                    "consumption) inclusive");
            }
        }
        else if (key == "doshuffle")
        {
            auto itShuffle = m_Shuffles.find(value);
            if (itShuffle == m_Shuffles.end())
            {
                helper::Throw<std::invalid_argument>(
                    "Operator", "CompressBlosc2", "Operate",
                    "Parameter doshuffle must be BLOSC_SHUFFLE, "
                    "BLOSC_NOSHUFFLE or BLOSC_BITSHUFFLE");
            }
            doShuffle = itShuffle->second;
        }
        else if (key == "nthreads")
        {
            threads = static_cast<int>(helper::StringTo<int32_t>(
                value, "when setting Blosc2 nthreads parameter\n"));
        }
        else if (key == "compressor")
        {
            compressor = value;
            if (m_Compressors.count(compressor) == 0)
            {
                helper::Throw<std::invalid_argument>(
                    "Operator", "CompressBlosc2", "Operate",
                    "Parameter compressor must be blosclz (default), lz4, "
                    "lz4hc, snappy, zlib, or zstd");
            }
        }
        else if (key == "blocksize")
        {
            blockSize = static_cast<size_t>(helper::StringTo<uint64_t>(
                value, "when setting Blosc2 blocksize parameter\n"));
        }
        else if (key == "threshold")
        {
            thresholdSize = static_cast<size_t>(helper::StringTo<uint64_t>(
                value, "when setting Blosc2 threshold parameter\n"));
            if (thresholdSize < 128u)
                thresholdSize = 128u;
        }
        else
        {
            helper::Log("Operator", "CompressBlosc2", "Operate",
                        "Unknown parameter keyword '" + key + "' with value '" +
                            value + "' passed to Blosc2 compression operator.",
                        helper::WARNING);
        }
    }

    // write header to detect new compression format (set first 8 byte to zero)
    DataHeader *headerPtr =
        reinterpret_cast<DataHeader *>(bufferOut + bufferOutOffset);

    // set default header
    *headerPtr = DataHeader{};
    bufferOutOffset += sizeof(DataHeader);

    int32_t typesize = helper::GetDataTypeSize(type);
    if (typesize > BLOSC_MAX_TYPESIZE)
        typesize = 1;

    size_t inputOffset = 0u;

    if (sizeIn < thresholdSize)
    {
        /* disable compression */
        useMemcpy = true;
    }

    if (!useMemcpy)
    {
        const int result = blosc1_set_compressor(compressor.c_str());
        if (result == -1)
        {
            helper::Throw<std::invalid_argument>(
                "Operator", "CompressBlosc2", "Operate",
                "blosc2 library linked does not support compressor " +
                    compressor);
        }
        blosc2_set_nthreads(threads);
        blosc1_set_blocksize(blockSize);

        uint32_t chunk = 0;
        for (; inputOffset < sizeIn; ++chunk)
        {
            size_t inputChunkSize =
                std::min(sizeIn - inputOffset,
                         static_cast<size_t>(BLOSC2_MAX_BUFFERSIZE));
            blosc2Size_t maxIntputSize =
                static_cast<blosc2Size_t>(inputChunkSize);

            blosc2Size_t maxChunkSize = maxIntputSize + BLOSC2_MAX_OVERHEAD;

            blosc2Size_t compressedChunkSize = blosc2_compress(
                compressionLevel, doShuffle, typesize, dataIn + inputOffset,
                maxIntputSize, bufferOut + bufferOutOffset, maxChunkSize);

            if (compressedChunkSize > 0)
                bufferOutOffset += static_cast<size_t>(compressedChunkSize);
            else
            {
                // something went wrong with the compression switch to memcopy
                useMemcpy = true;
                break;
            }
            /* add size to written output data */
            inputOffset += static_cast<size_t>(maxIntputSize);
        }

        if (!useMemcpy)
        {
            // validate that all bytes are compressed
            assert(inputOffset == sizeIn);
            headerPtr->SetNumChunks(chunk);
        }
    }

    if (useMemcpy)
    {
        std::memcpy(bufferOut + bufferOutOffset, dataIn + inputOffset, sizeIn);
        bufferOutOffset += sizeIn;
        headerPtr->SetNumChunks(0u);
    }

    blosc2_destroy();
    return bufferOutOffset;
}

size_t CompressBlosc2::InverseOperate(const char *bufferIn, const size_t sizeIn,
                                      char *dataOut)
{
    size_t bufferInOffset = 1; // skip operator type
    const uint8_t bufferVersion =
        GetParameter<uint8_t>(bufferIn, bufferInOffset);
    bufferInOffset += 2; // skip two reserved bytes

    if (bufferVersion == 1)
    {
        return DecompressV1(bufferIn + bufferInOffset, sizeIn - bufferInOffset,
                            dataOut);
    }
    else if (bufferVersion == 2)
    {
        // TODO: if a Version 2 blosc2 buffer is being implemented, put it here
        // and keep the DecompressV1 routine for backward compatibility
    }
    else
    {
        helper::Throw<std::runtime_error>("Operator", "CompressBlosc2",
                                          "InverseOperate",
                                          "invalid blosc2 buffer version");
    }

    return 0;
}

bool CompressBlosc2::IsDataTypeValid(const DataType type) const { return true; }

size_t CompressBlosc2::DecompressV1(const char *bufferIn, const size_t sizeIn,
                                    char *dataOut)
{
    // Do NOT remove even if the buffer version is updated. Data might be still
    // in lagacy formats. This function must be kept for backward compatibility.
    // If a newer buffer format is implemented, create another function, e.g.
    // DecompressV2 and keep this function for decompressing lagacy data.

    size_t bufferInOffset = 0;
    size_t sizeOut = GetParameter<size_t, size_t>(bufferIn, bufferInOffset);

    m_VersionInfo =
        " Data is compressed using BLOSC2 Version " +
        std::to_string(GetParameter<uint8_t>(bufferIn, bufferInOffset)) + "." +
        std::to_string(GetParameter<uint8_t>(bufferIn, bufferInOffset)) + "." +
        std::to_string(GetParameter<uint8_t>(bufferIn, bufferInOffset)) +
        ". Please make sure a compatible version is used for decompression.";

    if (sizeIn - bufferInOffset < sizeof(DataHeader))
    {
        helper::Throw<std::invalid_argument>(
            "Operator", "CompressBlosc2", "InverseOperate",
            "corrupted compressed buffer." + m_VersionInfo);
    }
    const bool isChunked =
        reinterpret_cast<const DataHeader *>(bufferIn + bufferInOffset)
            ->IsChunked();

    size_t decompressedSize = 0;
    if (isChunked)
    {
        decompressedSize =
            DecompressChunkedFormat(bufferIn + bufferInOffset,
                                    sizeIn - bufferInOffset, dataOut, sizeOut);
    }
    else
    {
        decompressedSize =
            DecompressOldFormat(bufferIn + bufferInOffset,
                                sizeIn - bufferInOffset, dataOut, sizeOut);
    }
    if (decompressedSize != sizeOut)
    {
        helper::Throw<std::runtime_error>("Operator", "CompressBlosc2",
                                          "DecompressV1", m_VersionInfo);
    }
    return sizeOut;
}

size_t CompressBlosc2::DecompressChunkedFormat(const char *bufferIn,
                                               const size_t sizeIn,
                                               char *dataOut,
                                               const size_t sizeOut) const
{
    const DataHeader *dataPtr = reinterpret_cast<const DataHeader *>(bufferIn);
    uint32_t num_chunks = dataPtr->GetNumChunks();
    size_t inputDataSize = sizeIn - sizeof(DataHeader);

    bool isCompressed = true;
    if (num_chunks == 0)
        isCompressed = false;

    size_t inputOffset = 0u;
    size_t currentOutputSize = 0u;

    const char *inputDataBuff = bufferIn + sizeof(DataHeader);

    size_t uncompressedSize = sizeOut;

    if (isCompressed)
    {
        blosc2_init();

        size_t threads = 1; // defaults
        for (const auto &itParameter : m_Parameters)
        {
            const std::string key = itParameter.first;
            const std::string value = itParameter.second;
            if (key == "nthreads")
            {
                threads = static_cast<int>(helper::StringTo<int32_t>(
                    value, "when setting Blosc2 nthreads parameter\n"));
            }
        }
        blosc2_set_nthreads(threads);

        while (inputOffset < inputDataSize)
        {
            /* move over the size of the compressed data */
            const char *in_ptr = inputDataBuff + inputOffset;

            /** read the size of the compress block from the blosc2 meta data
             *
             * blosc2 meta data format (all little endian):
             *   - 1 byte blosc2 format version
             *   - 1 byte blosclz format version
             *   - 1 byte flags
             *   - 1 byte typesize
             *   - 4 byte uncompressed data size
             *   - 4 byte block size
             *   - 4 byte compressed data size
             *
             * we need only the compressed size ( source address + 12 byte)
             */
            blosc2Size_t max_inputDataSize;
            std::memcpy(&max_inputDataSize, in_ptr + 12, sizeof(blosc2Size_t));

            char *out_ptr = dataOut + currentOutputSize;

            size_t outputChunkSize =
                std::min(uncompressedSize - currentOutputSize,
                         static_cast<size_t>(BLOSC2_MAX_BUFFERSIZE));
            blosc2Size_t max_output_size =
                static_cast<blosc2Size_t>(outputChunkSize);

            blosc2Size_t decompressdSize = blosc2_decompress(
                in_ptr, max_inputDataSize, out_ptr, max_output_size);

            if (decompressdSize > 0)
                currentOutputSize += static_cast<size_t>(decompressdSize);
            else
            {
                helper::Throw<std::runtime_error>(
                    "Operator", "CompressBlosc2", "DecompressChunkedFormat",
                    "blosc2 decompress failed with zero buffer size. " +
                        m_VersionInfo);
            }
            inputOffset += static_cast<size_t>(max_inputDataSize);
        }
        blosc2_destroy();
    }
    else
    {
        std::memcpy(dataOut, inputDataBuff, inputDataSize);
        currentOutputSize = inputDataSize;
        inputOffset += inputDataSize;
    }

    assert(currentOutputSize == uncompressedSize);
    assert(inputOffset == inputDataSize);

    return currentOutputSize;
}

size_t CompressBlosc2::DecompressOldFormat(const char *bufferIn,
                                           const size_t sizeIn, char *dataOut,
                                           const size_t sizeOut) const
{
    blosc2_init();
    size_t threads = 1; // defaults
    for (const auto &itParameter : m_Parameters)
    {
        const std::string key = itParameter.first;
        const std::string value = itParameter.second;
        if (key == "nthreads")
        {
            threads = static_cast<int>(helper::StringTo<int32_t>(
                value, "when setting Blosc2 nthreads parameter\n"));
        }
    }
    blosc2_set_nthreads(threads);
    const int decompressedSize =
        blosc2_decompress(bufferIn, sizeIn, dataOut, sizeOut);
    blosc2_destroy();
    return static_cast<size_t>(decompressedSize);
}

} // end namespace compress
} // end namespace core
} // end namespace adios2
