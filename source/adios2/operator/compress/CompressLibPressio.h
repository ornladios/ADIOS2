/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressLibPressio.h : wrapper to LibPressio compression library
 *
 *  Created on: Tue Apr 13, 2021
 *      Author: Robert Underwood robertu@g.clemson.edu
 */

#ifndef ADIOS2_OPERATOR_COMPRESS_COMPRESS_LIBPRESSIO_H_
#define ADIOS2_OPERATOR_COMPRESS_COMPRESS_LIBPRESSIO_H_

#include "adios2/core/Operator.h"

namespace adios2
{
namespace core
{
namespace compress
{

class CompressLibPressio : public Operator
{

public:
    /**
     * Unique constructor
     */
    CompressLibPressio(const Params &parameters);

    ~CompressLibPressio() = default;

    /**
     * Compression signature for legacy libraries that use char*
     * @param dataIn
     * @param dimensions
     * @param type
     * @param bufferOut
     * @param parameters
     * @return size of compressed buffer in bytes
     */
    size_t Compress(const char *dataIn, const Dims &dimensions, DataType type,
                    char *bufferOut, const Params &parameters,
                    Params &info) final;

    /**
     * Wrapper around zfp decompression
     * @param bufferIn
     * @param sizeIn
     * @param dataOut
     * @param dimensions
     * @param type
     * @return size of decompressed data in dataOut
     */
    size_t Decompress(const char *bufferIn, const size_t sizeIn, char *dataOut,
                      const DataType type, const Dims &blockStart,
                      const Dims &blockCount, const Params &parameters,
                      Params &info) final;

    bool IsDataTypeValid(const DataType type) const final;
};

} // end namespace compress
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_TRANSFORM_COMPRESSION_COMPRESS_LIBPRESSIO_H_ */
