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
#include <unordered_map>

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

    ~CompressSirius() = default;

    size_t Compress(const void *dataIn, const Dims &dimensions,
                    const size_t elementSize, DataType type, void *bufferOut,
                    const Params &params, Params &info) final;

    using Operator::Decompress;

    size_t Decompress(const void *bufferIn, const size_t sizeIn, void *dataOut,
                      const Dims &start, const Dims &count,
                      DataType type) final;

    bool IsDataTypeValid(const DataType type) const final;

    static bool m_CurrentReadFinished;

private:
    static int m_Tiers;

    // for compress
    static std::vector<std::vector<char>> m_TierBuffers;
    static int m_CurrentTier;

    // for decompress
    static std::vector<std::unordered_map<std::string, std::vector<char>>>
        m_TierBuffersMap;
    static std::unordered_map<std::string, int> m_CurrentTierMap;
};

} // end namespace compress
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_TRANSFORM_COMPRESSION_COMPRESSSZ_H_ */
