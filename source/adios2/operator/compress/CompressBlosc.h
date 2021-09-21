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

#if defined(_MSC_VER)
#define ADIOS2_CLASS_PACKED(name) __pragma(pack(push, 1)) class name
#define ADIOS2_CLASS_PACKED_SUFFIX __pragma(pack(pop))
#else
#define ADIOS2_CLASS_PACKED(name) class __attribute__((packed)) name
#define ADIOS2_CLASS_PACKED_SUFFIX
#endif

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
    size_t Compress(const char *dataIn, const Dims &dimensions, DataType type,
                    char *bufferOut, const Params &parameters,
                    Params &info) final;

    /**
     * Decompression signature for legacy libraries that use char*
     * @param bufferIn
     * @param sizeIn
     * @param dataOut
     * @param dimensions
     * @param type
     * @return size of decompressed buffer in bytes
     */
    size_t Decompress(const char *bufferIn, const size_t sizeIn, char *dataOut,
                      const DataType type, const Dims &blockStart,
                      const Dims &blockCount, const Params &parameters,
                      Params &info) final;

    bool IsDataTypeValid(const DataType type) const final;

private:
    using bloscSize_t = int32_t;

    /** Decompress chunked data */
    size_t DecompressChunkedFormat(const char *bufferIn, const size_t sizeIn,
                                   char *dataOut, const size_t sizeOut) const;

    /** Decompress data written before ADIOS2 supported large variables larger
     * 2GiB. */
    size_t DecompressOldFormat(const char *bufferIn, const size_t sizeIn,
                               char *dataOut, const size_t sizeOut) const;

    ADIOS2_CLASS_PACKED(DataHeader)
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
    }
    ADIOS2_CLASS_PACKED_SUFFIX;

    static const std::map<std::string, uint32_t> m_Shuffles;
    static const std::set<std::string> m_Compressors;
};

} // end namespace compress
} // end namespace core
} // end namespace adios2

#undef ADIOS2_CLASS_PACKED
#undef ADIOS2_CLASS_PACKED_SUFFIX

#endif /* ADIOS2_OPERATOR_COMPRESS_COMPRESSBLOSC_H_ */
