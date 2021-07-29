/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressSirius.h
 *
 *  Created on: Jul 28, 2021
 *      Author: Jason Wang jason.ruonan.wang@gmail.com
 */

#ifndef ADIOS2_OPERATOR_COMPRESS_COMPRESSSIRIUS_H_
#define ADIOS2_OPERATOR_COMPRESS_COMPRESSSIRIUS_H_

#include "adios2/core/Operator.h"

namespace adios2
{
namespace core
{
namespace compress
{

class CompressSirius : public Operator
{

public:
    CompressSirius(const Params &parameters);

    virtual ~CompressSirius();

    size_t Compress(const void *dataIn, const Dims &dimensions,
                    const size_t elementSize, DataType type, void *bufferOut,
                    const Params &parameters, Params &info) const final;

    size_t Decompress(const void *bufferIn, const size_t sizeIn, void *dataOut,
                      const Dims &dimensions, DataType type,
                      const Params &parameters) const final;

    bool IsDataTypeValid(const DataType type) const final;
};

} // end namespace compress
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_TRANSFORM_COMPRESSION_COMPRESSSZ_H_ */
