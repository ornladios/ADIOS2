/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressBZIP2.h : wrapper to BZIP2 compression library http://www.bzip.org/
 *
 *  Created on: Jul 24, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_OPERATOR_COMPRESS_COMPRESSBZIP2_H_
#define ADIOS2_OPERATOR_COMPRESS_COMPRESSBZIP2_H_

#include "adios2/core/Operator.h"

namespace adios2
{
namespace core
{
namespace compress
{

class CompressBZIP2 : public Operator
{

public:
    /**
     * Unique constructor
     */
    CompressBZIP2(const Params &parameters);

    ~CompressBZIP2() = default;

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
    /**
     * check status from BZip compression and decompression functions
     * @param status returned by BZip2 library
     * @param hint extra exception information
     */
    void CheckStatus(const int status, const std::string hint) const;
};

} // end namespace compress
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_TRANSFORM_COMPRESSION_COMPRESSBZIP2_H_ */
