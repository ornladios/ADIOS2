/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressBlosc.h
 *
 *  Created on: Jun 18, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_OPERATOR_COMPRESS_COMPRESSBLOSC_H_
#define ADIOS2_OPERATOR_COMPRESS_COMPRESSBLOSC_H_

#include <map>
#include <set>

#include "adios2/core/Operator.h"

namespace adios2
{
namespace core
{
namespace compress
{

class CompressBlosc : public Operator
{

public:
    /**
     * Unique constructor
     * @param debugMode
     */
    CompressBlosc(const Params &parameters, const bool debugMode);

    ~CompressBlosc() = default;

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
     * Decompression signature for legacy libraries that use void*
     * @param bufferIn
     * @param sizeIn
     * @param dataOut
     * @param dimensions
     * @param type
     * @return size of decompressed buffer in bytes
     */
    size_t Decompress(const void *bufferIn, const size_t sizeIn, void *dataOut,
                      const size_t sizeOut, Params &info) const final;

private:
    static const std::map<std::string, uint32_t> m_Shuffles;
    static const std::set<std::string> m_Compressors;
};

} // end namespace compress
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_OPERATOR_COMPRESS_COMPRESSBLOSC_H_ */
