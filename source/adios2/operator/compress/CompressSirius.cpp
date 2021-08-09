/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressSirius.cpp
 *
 *  Created on: Jul 28, 2021
 *      Author: Jason Wang jason.ruonan.wang@gmail.com
 */

#include "CompressSirius.h"
#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace core
{
namespace compress
{

int CompressSirius::m_CurrentTier = 0;
int CompressSirius::m_Tiers = 0;
std::vector<std::vector<char>> CompressSirius::m_TierBuffers;

CompressSirius::CompressSirius(const Params &parameters)
: Operator("sirius", parameters)
{
    helper::GetParameter(parameters, "tiers", m_Tiers);
    m_TierBuffers.resize(m_Tiers);
}

size_t CompressSirius::Compress(const void *dataIn, const Dims &dimensions,
                                const size_t elementSize, DataType varType,
                                void *bufferOut, const Params &params,
                                Params &info)
{
    size_t totalInputBytes =
        std::accumulate(dimensions.begin(), dimensions.end(), elementSize,
                        std::multiplies<size_t>());

    // if called from Tier 0 sub-engine, then compute tier buffers and put into
    // m_TierBuffers
    size_t bytesPerTier = totalInputBytes / m_Tiers;
    if (m_CurrentTier == 0)
    {
        for (int i = 0; i < m_TierBuffers.size(); i++)
        {
            m_TierBuffers[i].resize(bytesPerTier);
            std::memcpy(m_TierBuffers[i].data(),
                        reinterpret_cast<const char *>(dataIn) +
                            i * bytesPerTier,
                        bytesPerTier);
        }
    }

    // for all tiers' sub-engines, copy data from m_TierBuffers to output buffer
    std::memcpy(bufferOut, m_TierBuffers[m_CurrentTier].data(),
                m_TierBuffers[m_CurrentTier].size());

    m_CurrentTier++;
    m_CurrentTier %= m_Tiers;

    return bytesPerTier;
}

size_t CompressSirius::Decompress(const void *bufferIn, const size_t sizeIn,
                                  void *dataOut, const Dims &dimensions,
                                  DataType type, const Params &parameters)
{
    size_t outputBytes = std::accumulate(dimensions.begin(), dimensions.end(),
                                         helper::GetDataTypeSize(type),
                                         std::multiplies<size_t>());

    // decompress data and copy back to m_TierBuffers
    size_t bytesPerTier = outputBytes / m_Tiers;
    m_TierBuffers[m_CurrentTier].resize(bytesPerTier);
    std::memcpy(m_TierBuffers[m_CurrentTier].data(), bufferIn, bytesPerTier);

    // if called from the final tier, then merge all tier buffers and copy back
    // to dataOut
    size_t accumulatedBytes = 0;
    if (m_CurrentTier == m_Tiers - 1)
    {
        for (const auto &b : m_TierBuffers)
        {
            std::memcpy(reinterpret_cast<char *>(dataOut) + accumulatedBytes,
                        b.data(), b.size());
            accumulatedBytes += b.size();
        }
    }

    m_CurrentTier++;
    if (m_CurrentTier % m_Tiers == 0)
    {
        m_CurrentTier = 0;
    }

    return outputBytes;
}

bool CompressSirius::IsDataTypeValid(const DataType type) const
{
#define declare_type(T)                                                        \
    if (helper::GetDataType<T>() == type)                                      \
    {                                                                          \
        return true;                                                           \
    }
    ADIOS2_FOREACH_SIRIUS_TYPE_1ARG(declare_type)
#undef declare_type
    return false;
}

} // end namespace compress
} // end namespace core
} // end namespace adios2
