/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressBlosc.h
 *
 *  Created on: Jun 18, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 *              Rene Widera r.widera@hzdr.de
 */

#ifndef ADIOS2_OPERATOR_COMPRESS_COMPRESSBLOSC_H_
#define ADIOS2_OPERATOR_COMPRESS_COMPRESSBLOSC_H_

#include <map>
#include <set>

#include "adios2/core/Operator.h"

namespace adios2
{
namespace core
{
namespace compress
{

class CompressBlosc : public Operator
{

public:
    /**
     * Unique constructor
     */
    CompressBlosc(const Params &parameters);

    ~CompressBlosc() = default;

    size_t BufferMaxSize(const size_t sizeIn) const final;

    /**
     * Compression signature for legacy libraries that use void*
     * @param dataIn
     * @param dimensions
     * @param type
     * @param bufferOut format will be: 'DataHeader ; (BloscCompressedChunk |
     * UncompressedData), [ BloscCompressedChunk, ...]'
     * @param parameters
     * @return size of compressed buffer in bytes
     */
    size_t Compress(const void *dataIn, const Dims &dimensions,
                    const size_t elementSize, DataType type, void *bufferOut,
                    const Params &parameters, Params &info) const final;

    /**
     * Decompression signature for legacy libraries that use void*
     * @param bufferIn
     * @param sizeIn
     * @param dataOut
     * @param dimensions
     * @param type
     * @return size of decompressed buffer in bytes
     */
    size_t Decompress(const void *bufferIn, const size_t sizeIn, void *dataOut,
                      const size_t sizeOut, Params &info) const final;

private:
    using bloscSize_t = int32_t;

    /** Decompress chunked data */
    size_t DecompressChunkedFormat(const void *bufferIn, const size_t sizeIn,
                                   void *dataOut, const size_t sizeOut,
                                   Params &info) const;

    /** Decompress data written before ADIOS2 supported large variables larger
     * 2GiB. */
    size_t DecompressOldFormat(const void *bufferIn, const size_t sizeIn,
                               void *dataOut, const size_t sizeOut,
                               Params &info) const;

    class __attribute__((packed)) DataHeader
    {
        /** compatible to the first 4 byte of blosc header
         *
         *   blosc meta data format (all little endian):
         *   - 1 byte blosc format version
         *   - 1 byte blosclz format version
         *   - 1 byte flags
         *   - 1 byte typesize
         *
         * If zero we writing the new adios blosc format which can handle more
         * than 2GiB data chunks.
         */
        uint32_t format = 0u;
        /** number of blosc chunks within the data blob
         *
         * If zero the data is not compressed and must be decompressed by using
         * 'memcpy'
         */
        uint32_t numberOfChunks = 0u;

    public:
        void SetNumChunks(const uint32_t numChunks)
        {
            numberOfChunks = numChunks;
        }
        uint32_t GetNumChunks() const { return numberOfChunks; }

        bool IsChunked() const { return format == 0; }
    };

    static const std::map<std::string, uint32_t> m_Shuffles;
    static const std::set<std::string> m_Compressors;
};

} // end namespace compress
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_OPERATOR_COMPRESS_COMPRESSBLOSC_H_ */
