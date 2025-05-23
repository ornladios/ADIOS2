/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressBigWhoop.h : wrapper to BigWhoop compression library
 *
 *  Created on: Tue Jan 26, 2024
 *      Author: Gregor Weiss gregor.weiss@hlrs.de
 */

#ifndef ADIOS2_OPERATOR_COMPRESS_COMPRESS_BIGWHOOP_H_
#define ADIOS2_OPERATOR_COMPRESS_COMPRESS_BIGWHOOP_H_

#include "adios2/core/Operator.h"

namespace adios2
{
namespace core
{
namespace compress
{

class CompressBigWhoop : public Operator
{

public:
    /**
     * Unique constructor
     */
    CompressBigWhoop(const Params &parameters);

    ~CompressBigWhoop() = default;

    /**
     * @param dataIn
     * @param blockStart
     * @param blockCount
     * @param type
     * @param bufferOut
     * @return size of compressed buffer
     */
    size_t Operate(const char *dataIn, const Dims &blockStart, const Dims &blockCount,
                   const DataType type, char *bufferOut) final;

    /**
     * @param bufferIn
     * @param sizeIn
     * @param dataOut
     * @return size of decompressed buffer
     */
    size_t InverseOperate(const char *bufferIn, const size_t sizeIn, char *dataOut) final;

    bool IsDataTypeValid(const DataType type) const final;

private:
    /**
     * Decompress function for V1 buffer. Do NOT remove even if the buffer
     * version is updated. Data might be still in legacy formats. This function
     * must be kept for backward compatibility.
     * @param bufferIn : compressed data buffer (V1 only)
     * @param sizeIn : number of bytes in bufferIn
     * @param dataOut : decompressed data buffer
     * @return : number of bytes in dataOut
     */
    size_t DecompressV1(const char *bufferIn, const size_t sizeIn, char *dataOut);

    std::string m_VersionInfo;
};

} // end namespace compress
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_TRANSFORM_COMPRESSION_COMPRESS_BIGWHOOP_H_ */
