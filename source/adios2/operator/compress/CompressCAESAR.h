/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_OPERATOR_COMPRESS_COMPRESS_CAESAR_H_
#define ADIOS2_OPERATOR_COMPRESS_COMPRESS_CAESAR_H_
#include "adios2/core/Operator.h"
namespace adios2
{
namespace core
{
namespace compress
{

class CompressCAESAR : public Operator
{
public:
    /**
     * @param dataIn
     * @param blockStart
     * @param blockCount
     * @param type
     * @param bufferOut
     * @return size of compressed buffer
     */
    CompressCAESAR(const Params &parameters);
    ~CompressCAESAR() override = default;

    size_t Operate(const char *dataIn, const Dims &blockStart, const Dims &blockCount,
                   const DataType type, char *bufferOut) final;

    size_t InverseOperate(const char *bufferIn, const size_t sizeIn, char *dataOut) final;
    bool IsDataTypeValid(const DataType type) const final;
    size_t GetEstimatedSize(const size_t ElemCount, const size_t ElemSize, const size_t ndims,
                            const size_t *dims) const override;

private:
    /**
     * Decompress function for V1 buffer. Do NOT remove even if the buffer
     * version is updated. Data might be still in lagacy formats. This function
     * must be kept for backward compatibility
     * @param bufferIn : compressed data buffer (V1 only)
     * @param sizeIn : number of bytes in bufferIn
     * @param dataOut : decompressed data buffer
     * @return : number of bytes in dataOut
     */
    size_t DecompressV1(const char *bufferIn, const size_t sizeIn, char *dataOut);
    std::string m_VersionInfo;
};

} // namespace compress
} // namespace core
} // namespace adios2

#endif // ADIOS2_OPERATOR_COMPRESS_COMPRESS_CAESAR_H_
