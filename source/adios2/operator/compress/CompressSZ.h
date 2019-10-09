/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressSZ.h : wrapper to SZ compression library
 *
 *  Created on: Jul 24, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_OPERATOR_COMPRESS_COMPRESSSZ_H_
#define ADIOS2_OPERATOR_COMPRESS_COMPRESSSZ_H_

#include "adios2/core/Operator.h"

namespace adios2
{
namespace core
{
namespace compress
{

class CompressSZ : public Operator
{

public:
    /**
     * Unique constructor
     * @param debugMode
     */
    CompressSZ(const Params &parameters, const bool debugMode);

    ~CompressSZ() = default;

    size_t BufferMaxSize(const size_t sizeIn) const final;

    /**
     * Compression signature for legacy libraries that use void*
     * @param dataIn
     * @param dimensions
     * @param type
     * @param bufferOut
     * @param parameters
     * @return size of compressed buffer in bytes
     */
    size_t Compress(const void *dataIn, const Dims &dimensions,
                    const size_t elementSize, const std::string type,
                    void *bufferOut, const Params &parameters,
                    Params &info) const final;

    using Operator::Decompress;

    /**
     * Wrapper around zfp decompression
     * @param bufferIn
     * @param sizeIn
     * @param dataOut
     * @param dimensions
     * @param type
     * @return size of decompressed data in dataOut
     */
    size_t Decompress(const void *bufferIn, const size_t sizeIn, void *dataOut,
                      const Dims &dimensions, const std::string type,
                      const Params &parameters) const final;
};

} // end namespace compress
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_TRANSFORM_COMPRESSION_COMPRESSSZ_H_ */
