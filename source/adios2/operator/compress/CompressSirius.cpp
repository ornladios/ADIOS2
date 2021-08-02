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

CompressSirius::CompressSirius(const Params &parameters)
: Operator("sirius", parameters)
{
    int tiers;
    bool hasTiers = helper::GetParameter(parameters, "tiers", tiers);
    if (!hasTiers)
    {
        throw("sirius operator: must have parameter tiers");
    }
    m_TierBuffers.resize(tiers);
}

size_t CompressSirius::Compress(const void *dataIn, const Dims &dimensions,
                                const size_t elementSize, DataType varType,
                                void *bufferOut, const Params &params,
                                Params &info)
{
    size_t totalBytes = std::accumulate(dimensions.begin(), dimensions.end(),
                                        elementSize, std::multiplies<size_t>());

    size_t currentTierSize = totalBytes;

    if (m_CurrentTier == 0)
    {
        for (int i = 0; i < m_TierBuffers.size(); i++)
        {
            m_TierBuffers[i].resize(currentTierSize);
            std::memcpy(m_TierBuffers[i].data(), dataIn, currentTierSize);
            currentTierSize /= 2;
        }
    }

    std::memcpy(bufferOut, m_TierBuffers[m_CurrentTier].data(),
                m_TierBuffers[m_CurrentTier].size());

    m_CurrentTier++;
    m_CurrentTier %= m_TierBuffers.size();

    return 0;
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
