/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SscReaderNaive.tcc
 *
 *  Created on: Mar 7, 2022
 *      Author: Jason Wang
 */

#ifndef ADIOS2_ENGINE_SSCREADERNAIVE_TCC_
#define ADIOS2_ENGINE_SSCREADERNAIVE_TCC_

#include "SscReaderNaive.h"
#include "adios2/helper/adiosMemory.h"

namespace adios2
{
namespace core
{
namespace engine
{
namespace ssc
{

template <typename T>
std::vector<typename Variable<T>::BPInfo>
SscReaderNaive::BlocksInfoCommon(const Variable<T> &variable,
                                 const size_t step) const
{
    std::vector<typename Variable<T>::BPInfo> ret;

    for (const auto &v : m_BlockMap[variable.m_Name])
    {
        ret.emplace_back();
        auto &b = ret.back();
        b.Start = v.start;
        b.Count = v.count;
        b.Shape = v.shape;
        b.Step = m_CurrentStep;
        b.StepsStart = m_CurrentStep;
        b.StepsCount = 1;
        if (m_IO.m_ArrayOrder != ArrayOrdering::RowMajor)
        {
            std::reverse(b.Start.begin(), b.Start.end());
            std::reverse(b.Count.begin(), b.Count.end());
            std::reverse(b.Shape.begin(), b.Shape.end());
        }
        if (v.shapeId == ShapeID::GlobalValue ||
            v.shapeId == ShapeID::LocalValue)
        {
            b.IsValue = true;
            if (m_CurrentStep == 0 || m_WriterDefinitionsLocked == false ||
                m_ReaderSelectionsLocked == false)
            {
                std::memcpy(reinterpret_cast<char *>(&b.Value), v.value.data(),
                            v.value.size());
            }
            else
            {
                std::memcpy(reinterpret_cast<char *>(&b.Value),
                            m_Buffer.data() + v.bufferStart, v.bufferCount);
            }
        }
    }

    return ret;
}

}
}
}
}

#endif // ADIOS2_ENGINE_SSCREADERNAIVE_TCC_
