/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SscWriter.tcc
 *
 *  Created on: Nov 1, 2018
 *      Author: Jason Wang
 */

#ifndef ADIOS2_ENGINE_SSCWRITER_TCC_
#define ADIOS2_ENGINE_SSCWRITER_TCC_

#include "SscWriter.h"
#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

template <class T>
void SscWriter::PutSyncCommon(Variable<T> &variable, const T *data)
{
    TAU_SCOPED_TIMER_FUNC();
    PutDeferredCommon(variable, data);
    PerformPuts();
}

template <class T>
void SscWriter::PutDeferredCommon(Variable<T> &variable, const T *data)
{
    TAU_SCOPED_TIMER_FUNC();

    if (variable.m_Start.empty())
    {
        variable.m_Start.push_back(0);
    }
    if (variable.m_Count.empty())
    {
        variable.m_Count.push_back(1);
    }
    if (variable.m_Shape.empty())
    {
        variable.m_Shape.push_back(1);
    }

    variable.SetData(data);
    auto pattern = m_LocalWritePatternMap[variable.m_Name];
    if (ssc::AreSameDims(variable.m_Start, pattern.start) and
        ssc::AreSameDims(variable.m_Count, pattern.count) and
        ssc::AreSameDims(variable.m_Shape, pattern.shape))
    {
        std::memcpy(m_Buffer.data() + pattern.posStart, data, pattern.posCount);
    }
    else
    {
        throw std::runtime_error("ssc only accepts fixed IO pattern");
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_SSCWRITER_TCC_
