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

std::unordered_map<std::string, int> CompressSirius::m_CurrentTierMap;
std::vector<std::unordered_map<std::string, std::vector<char>>>
    CompressSirius::m_TierBuffersMap;
int CompressSirius::m_CurrentTier;
std::vector<std::vector<char>> CompressSirius::m_TierBuffers;
int CompressSirius::m_Tiers = 0;
bool CompressSirius::m_CurrentReadFinished = false;

CompressSirius::CompressSirius(const Params &parameters)
: Operator("sirius", parameters)
{
    helper::GetParameter(parameters, "Tiers", m_Tiers);
    m_TierBuffersMap.resize(m_Tiers);
    m_TierBuffers.resize(m_Tiers);
}

size_t CompressSirius::Compress(const char *dataIn, const Dims &blockStart,
                                const Dims &blockCount, DataType varType,
                                char *bufferOut, const Params &params,
                                Params &info)
{
    size_t totalInputBytes = std::accumulate(
        blockCount.begin(), blockCount.end(), helper::GetDataTypeSize(varType),
        std::multiplies<size_t>());

    // if called from Tier 0 sub-engine, then compute tier buffers and put into
    // m_TierBuffers
    size_t bytesPerTier = totalInputBytes / m_Tiers;
    if (m_CurrentTier == 0)
    {
        for (size_t i = 0; i < m_TierBuffers.size(); i++)
        {
            m_TierBuffers[i].resize(bytesPerTier);
            std::memcpy(m_TierBuffers[i].data(), dataIn + i * bytesPerTier,
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

size_t CompressSirius::Decompress(const char *bufferIn, const size_t sizeIn,
                                  char *dataOut, const DataType type,
                                  const Dims &blockStart,
                                  const Dims &blockCount,
                                  const Params &parameters, Params &info)
{
    const size_t outputBytes = std::accumulate(
        blockCount.begin(), blockCount.end(), helper::GetDataTypeSize(type),
        std::multiplies<size_t>());

    std::string blockId =
        helper::DimsToString(blockStart) + helper::DimsToString(blockCount);

    // decompress data and copy back to m_TierBuffers
    size_t bytesPerTier = outputBytes / m_Tiers;
    auto &currentBuffer = m_TierBuffersMap[m_CurrentTierMap[blockId]][blockId];
    auto &currentTier = m_CurrentTierMap[blockId];
    currentBuffer.resize(bytesPerTier);
    std::memcpy(currentBuffer.data(), bufferIn, bytesPerTier);

    // if called from the final tier, then merge all tier buffers and copy back
    // to dataOut
    size_t accumulatedBytes = 0;
    // TODO: it currently only copies output data back when the final tier is
    // read. However, the real Sirius algorithm should instead decide when to
    // copy back decompressed data based on required acuracy level. Once it's
    // done, it should set m_CurrentReadFinished to true to inform the MHS
    // engine that the current read is finished so that it won't read the next
    // tier.
    if (currentTier == m_Tiers - 1)
    {
        for (auto &bmap : m_TierBuffersMap)
        {
            auto &b = bmap[blockId];
            std::memcpy(dataOut + accumulatedBytes, b.data(), b.size());
            accumulatedBytes += b.size();
        }
        // set m_CurrentReadFinished to true if after the current call, the
        // required acuracy is already satisfied, so that the MHS engine knows
        // it shouldn't continue reading the next tier.
        m_CurrentReadFinished = true;
    }

    // set m_CurrentReadFinished to false if the current tier does not satisfy
    // the required acuracy, so the MHS engine will read the next tier.
    m_CurrentReadFinished = false;

    currentTier++;
    if (currentTier % m_Tiers == 0)
    {
        currentTier = 0;
    }

    if (currentTier == 0)
    {
        return outputBytes;
    }
    else
    {
        return 0;
    }
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
