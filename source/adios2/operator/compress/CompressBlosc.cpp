/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressBlosc.cpp
 *
 *  Created on: Jun 18, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 *              Rene Widera r.widera@hzdr.de
 */

#include "CompressBlosc.h"

extern "C" {
#include <blosc.h>
}

#include "adios2/helper/adiosFunctions.h"

#include <algorithm>
#include <cassert>
#include <cstring>

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

CompressBlosc::CompressBlosc(const Params &parameters)
: Operator("blosc", parameters)
{
}

size_t CompressBlosc::BufferMaxSize(const size_t sizeIn) const
{
    const size_t maxInputPerChunk = BLOSC_MAX_BUFFERSIZE;
    const size_t numFullChunks = sizeIn / maxInputPerChunk;
    const size_t sizeLastChunk = sizeIn % maxInputPerChunk;

    const size_t maxOutputPerChunk = maxInputPerChunk + BLOSC_MAX_OVERHEAD;
    const size_t maxOutputLastChunk = sizeLastChunk + BLOSC_MAX_OVERHEAD;

    /* DataHeader is used to detect of old format which can only handle
     * BLOSC_MAX_BUFFERSIZE (<2GiB) or the new adios2 chunked blosc format is
     * used.
     */
    const size_t maxRquiredDataMem = maxOutputPerChunk * numFullChunks +
                                     maxOutputLastChunk + sizeof(DataHeader);

    return maxRquiredDataMem;
}

size_t CompressBlosc::Compress(const void *dataIn, const Dims &dimensions,
                               const size_t elementSize, DataType type,
                               void *bufferOut, const Params &parameters,
                               Params &info) const
{
    const size_t sizeIn =
        static_cast<size_t>(helper::GetTotalSize(dimensions) * elementSize);

    bool useMemcpy = false;
    /* input size under this bound would not compressed */
    size_t thresholdSize = 128;

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
                value, "when setting Blosc clevel parameter\n"));
            if (compressionLevel < 0 || compressionLevel > 9)
            {
                throw std::invalid_argument(
                    "ERROR: compression_level must be an "
                    "integer between 0 (default: no compression) and 9 "
                    "(more compression, more memory) inclusive, in call to "
                    "ADIOS2 Blosc Compress\n");
            }
        }
        else if (key == "doshuffle")
        {
            auto itShuffle = m_Shuffles.find(value);
            if (itShuffle == m_Shuffles.end())
            {
                throw std::invalid_argument(
                    "ERROR: invalid shuffle vale " + value +
                    " must be BLOSC_SHUFFLE, BLOSC_NOSHUFFLE or "
                    "BLOSC_BITSHUFFLE,  "
                    " in call to ADIOS2 Blosc Compress\n");
            }
            doShuffle = itShuffle->second;
        }
        else if (key == "nthreads")
        {
            threads = static_cast<int>(helper::StringTo<int32_t>(
                value, "when setting Blosc nthreads parameter\n"));
        }
        else if (key == "compressor")
        {
            compressor = value;
            if (m_Compressors.count(compressor) == 0)
            {
                throw std::invalid_argument(
                    "ERROR: invalid compressor " + compressor +
                    " valid values: blosclz (default), lz4, lz4hc, "
                    "snappy, "
                    "zlib, or zstd, in call to ADIOS2 Blosc Compression\n");
            }
        }
        else if (key == "blocksize")
        {
            blockSize = static_cast<size_t>(helper::StringTo<uint64_t>(
                value, "when setting Blosc blocksize parameter\n"));
        }
        else if (key == "threshold")
        {
            thresholdSize = static_cast<size_t>(helper::StringTo<uint64_t>(
                value, "when setting Blosc threshold parameter\n"));
            if (thresholdSize < 128u)
                thresholdSize = 128u;
        }
    }

    // write header to detect new compression format (set first 8 byte to zero)
    DataHeader *headerPtr = reinterpret_cast<DataHeader *>(bufferOut);

    // set default header
    *headerPtr = DataHeader{};

    const uint8_t *inputDataBuff = reinterpret_cast<const uint8_t *>(dataIn);

    int32_t typesize = elementSize;
    if (typesize > BLOSC_MAX_TYPESIZE)
        typesize = 1;

    uint8_t *outputBuff = reinterpret_cast<uint8_t *>(bufferOut);
    outputBuff += sizeof(DataHeader);

    size_t currentOutputSize = 0u;
    size_t inputOffset = 0u;

    if (sizeIn < thresholdSize)
    {
        /* disable compression */
        useMemcpy = true;
    }

    if (!useMemcpy)
    {
        const int result = blosc_set_compressor(compressor.c_str());
        if (result == -1)
        {
            throw std::invalid_argument(
                "ERROR: invalid compressor " + compressor +
                " check if supported by blosc build, in "
                "call to ADIOS2 Blosc Compression\n");
        }
        blosc_set_nthreads(threads);
        blosc_set_blocksize(blockSize);

        uint32_t chunk = 0;
        for (; inputOffset < sizeIn; ++chunk)
        {
            size_t inputChunkSize =
                std::min(sizeIn - inputOffset,
                         static_cast<size_t>(BLOSC_MAX_BUFFERSIZE));
            bloscSize_t maxIntputSize =
                static_cast<bloscSize_t>(inputChunkSize);

            bloscSize_t maxChunkSize = maxIntputSize + BLOSC_MAX_OVERHEAD;

            const uint8_t *in_ptr = inputDataBuff + inputOffset;
            uint8_t *out_ptr = outputBuff + currentOutputSize;

            bloscSize_t compressedChunkSize =
                blosc_compress(compressionLevel, doShuffle, typesize,
                               maxIntputSize, in_ptr, out_ptr, maxChunkSize);

            if (compressedChunkSize > 0)
                currentOutputSize += static_cast<size_t>(compressedChunkSize);
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
        std::memcpy(outputBuff, inputDataBuff, sizeIn);
        currentOutputSize = sizeIn;
        headerPtr->SetNumChunks(0u);
    }

    blosc_destroy();
    return currentOutputSize + sizeof(DataHeader);
}

size_t CompressBlosc::Decompress(const void *bufferIn, const size_t sizeIn,
                                 void *dataOut, const size_t sizeOut,
                                 Params &info) const
{
    assert(sizeIn >= sizeof(DataHeader));
    const bool isChunked =
        reinterpret_cast<const DataHeader *>(bufferIn)->IsChunked();

    size_t decompressedSize = 0u;
    if (isChunked)
        decompressedSize =
            DecompressChunkedFormat(bufferIn, sizeIn, dataOut, sizeOut, info);
    else
        decompressedSize =
            DecompressOldFormat(bufferIn, sizeIn, dataOut, sizeOut, info);

    return decompressedSize;
}

size_t CompressBlosc::DecompressChunkedFormat(const void *bufferIn,
                                              const size_t sizeIn,
                                              void *dataOut,
                                              const size_t sizeOut,
                                              Params &info) const
{
    const DataHeader *dataPtr = reinterpret_cast<const DataHeader *>(bufferIn);
    uint32_t num_chunks = dataPtr->GetNumChunks();
    size_t inputDataSize = sizeIn - sizeof(DataHeader);

    bool isCompressed = true;
    if (num_chunks == 0)
        isCompressed = false;

    size_t inputOffset = 0u;
    size_t currentOutputSize = 0u;

    const uint8_t *inputDataBuff =
        reinterpret_cast<const uint8_t *>(bufferIn) + sizeof(DataHeader);

    size_t uncompressedSize = sizeOut;

    if (isCompressed)
    {
        blosc_init();
        uint8_t *outputBuff = reinterpret_cast<uint8_t *>(dataOut);

        while (inputOffset < inputDataSize)
        {
            /* move over the size of the compressed data */
            const uint8_t *in_ptr = inputDataBuff + inputOffset;

            /** read the size of the compress block from the blosc meta data
             *
             * blosc meta data format (all little endian):
             *   - 1 byte blosc format version
             *   - 1 byte blosclz format version
             *   - 1 byte flags
             *   - 1 byte typesize
             *   - 4 byte uncompressed data size
             *   - 4 byte block size
             *   - 4 byte compressed data size
             *
             * we need only the compressed size ( source address + 12 byte)
             */
            bloscSize_t max_inputDataSize =
                *reinterpret_cast<const bloscSize_t *>(in_ptr + 12u);

            uint8_t *out_ptr = outputBuff + currentOutputSize;

            size_t outputChunkSize =
                std::min(uncompressedSize - currentOutputSize,
                         static_cast<size_t>(BLOSC_MAX_BUFFERSIZE));
            bloscSize_t max_output_size =
                static_cast<bloscSize_t>(outputChunkSize);

            bloscSize_t decompressdSize =
                blosc_decompress(in_ptr, out_ptr, max_output_size);

            if (decompressdSize > 0)
                currentOutputSize += static_cast<size_t>(decompressdSize);
            else
            {
                throw std::runtime_error(
                    "ERROR: ADIOS2 Blosc Decompress failed. Decompressed chunk "
                    "results in zero decompressed bytes.\n");
            }
            inputOffset += static_cast<size_t>(max_inputDataSize);
        }
        blosc_destroy();
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

size_t CompressBlosc::DecompressOldFormat(const void *bufferIn,
                                          const size_t sizeIn, void *dataOut,
                                          const size_t sizeOut,
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
