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
     * @param debugMode
     */
    CompressMGARD(const Params &parameters, const bool debugMode);

    ~CompressMGARD() = default;

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
    size_t Decompress(const void *bufferIn, const size_t sizeIn, void *dataOut,
                      const Dims &dimensions, const std::string varType,
                      const Params & /*parameters*/) const final;
};

} // end namespace compress
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_OPERATOR_COMPRESS_COMPRESSMGARD_H_ */
