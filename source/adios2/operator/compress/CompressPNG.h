/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressPNG.h
 *
 *  Created on: Jun 10, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_OPERATOR_COMPRESS_COMPRESSPNG_H_
#define ADIOS2_OPERATOR_COMPRESS_COMPRESSPNG_H_

#include <set>

#include "adios2/core/Operator.h"

namespace adios2
{
namespace core
{
namespace compress
{

class CompressPNG : public Operator
{

public:
    /**
     * Unique constructor
     * @param debugMode
     */
    CompressPNG(const Params &parameters, const bool debugMode);

    ~CompressPNG() = default;

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
    /**
     * In debug mode, check status from PNG compression and decompression
     * functions
     * @param status returned by PNG library
     * @param hint extra exception information
     */
    void CheckStatus(const int status, const std::string hint) const;

    static const std::map<std::string, uint32_t> m_ColorTypes;
    static const std::map<std::string, std::set<uint32_t>> m_BitDepths;

    /** Used as a bridge to the callback function */
    struct DestInfo
    {
        char *BufferOut = nullptr;
        size_t Offset = 0;
    };
};

} // end namespace compress
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_OPERATOR_COMPRESS_COMPRESSPNG_H_ */
