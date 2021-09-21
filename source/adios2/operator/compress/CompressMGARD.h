/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressMGARD.h :
 *
 *  Created on: Aug 3, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_OPERATOR_COMPRESS_COMPRESSMGARD_H_
#define ADIOS2_OPERATOR_COMPRESS_COMPRESSMGARD_H_

#include "adios2/core/Operator.h"

namespace adios2
{
namespace core
{
namespace compress
{

class CompressMGARD : public Operator
{

public:
    /**
     * Unique constructor
     */
    CompressMGARD(const Params &parameters);

    ~CompressMGARD() = default;

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
     *
     * @param bufferIn
     * @param sizeIn
     * @param dataOut
     * @param dimensions
     * @param varType
     * @param
     * @return
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

#endif /* ADIOS2_OPERATOR_COMPRESS_COMPRESSMGARD_H_ */
