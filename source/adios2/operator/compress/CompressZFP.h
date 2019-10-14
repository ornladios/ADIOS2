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
     * @param debugMode
     */
    CompressZFP(const Params &parameters, const bool debugMode);

    ~CompressZFP() = default;

    /**
     * Wrapper around zfp compression
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

private:
    /**
     * Returns Zfp supported zfp_type based on adios string type
     * @param type adios type as string, see GetType<T> in
     * helper/adiosType.inl
     * @return zfp_type
     */
    zfp_type GetZfpType(const std::string type) const;

    /**
     * Constructor Zfp zfp_field based on input information around the data
     * pointer
     * @param data
     * @param shape
     * @param type
     * @return zfp_field*
     */
    zfp_field *GetZFPField(const void *data, const Dims &shape,
                           const std::string type) const;

    zfp_stream *GetZFPStream(const Dims &dimensions, const std::string type,
                             const Params &parameters) const;

    size_t DoBufferMaxSize(const void *dataIn, const Dims &dimensions,
                           const std::string type,
                           const Params &parameters) const final;

    /**
     * In debug mode, check status from BZip compression and decompression
     * functions
     * @param status returned by BZip2 library
     * @param hint extra exception information
     */
    void CheckStatus(const int status, const std::string hint) const;
};

} // end namespace compress
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_TRANSFORM_COMPRESS_COMPRESSZFP_H_ */
