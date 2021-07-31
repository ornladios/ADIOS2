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
    bool hasTiers = helper::GetParameter(parameters, "Tiers", tiers);
    if (!hasTiers)
    {
        throw("sirius operator: must have parameter Tiers");
    }
    m_TierBuffers.resize(tiers);
}

size_t CompressSirius::Compress(const void *dataIn, const Dims &dimensions,
                                const size_t elementSize, DataType varType,
                                void *bufferOut)
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
    if (helper::GetDataType<float>() == type)
    {
        return true;
    }
    return false;
}

} // end namespace compress
} // end namespace core
} // end namespace adios2
