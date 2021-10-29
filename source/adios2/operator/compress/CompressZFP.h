/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressZFP.h : wrapper to ZFP compression library
 * https://computation.llnl.gov/projects/floating-point-compression
 *
 *  Created on: Jul 25, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TRANSFORM_COMPRESS_COMPRESSZFP_H_
#define ADIOS2_TRANSFORM_COMPRESS_COMPRESSZFP_H_

extern "C" {
#include <zfp.h>
}

#include "adios2/core/Operator.h"

namespace adios2
{
namespace core
{
namespace compress
{

class CompressZFP : public Operator
{

public:
    /**
     * Unique constructor
     */
    CompressZFP(const Params &parameters);

    ~CompressZFP() = default;

    /**
     * @param dataIn
     * @param blockStart
     * @param blockCount
     * @param type
     * @param bufferOut
     * @param parameters
     * @return size of compressed buffer
     */
    size_t Operate(const char *dataIn, const Dims &blockStart,
                   const Dims &blockCount, const DataType type, char *bufferOut,
                   const Params &parameters) final;

    /**
     * @param bufferIn
     * @param sizeIn
     * @param dataOut
     * @return size of decompressed buffer
     */
    size_t InverseOperate(const char *bufferIn, const size_t sizeIn,
                          char *dataOut) final;

    bool IsDataTypeValid(const DataType type) const final;

private:
    /**
     * Returns Zfp supported zfp_type based on adios string type
     * @param type adios type as string, see GetDataType<T> in
     * helper/adiosType.inl
     * @return zfp_type
     */
    zfp_type GetZfpType(DataType type) const;

    /**
     * Constructor Zfp zfp_field based on input information around the data
     * pointer
     * @param data
     * @param shape
     * @param type
     * @return zfp_field*
     */
    zfp_field *GetZFPField(const char *data, const Dims &shape,
                           DataType type) const;

    zfp_stream *GetZFPStream(const Dims &dimensions, DataType type,
                             const Params &parameters) const;

    /**
     * Decompress function for V1 buffer. Do NOT remove even if the buffer
     * version is updated. Data might be still in lagacy formats. This function
     * must be kept for backward compatibility
     * @param bufferIn : compressed data buffer (V1 only)
     * @param sizeIn : number of bytes in bufferIn
     * @param dataOut : decompressed data buffer
     * @return : number of bytes in dataOut
     */
    size_t DecompressV1(const char *bufferIn, const size_t sizeIn,
                        char *dataOut);

    std::string m_VersionInfo;
};

} // end namespace compress
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_TRANSFORM_COMPRESS_COMPRESSZFP_H_ */
