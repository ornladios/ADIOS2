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
    variable.SetData(data);
    auto saved = m_LocalWritePatternMap[variable.m_Name];
    if(ssc::AreSameDims(variable.m_Start, saved.start) and ssc::AreSameDims(variable.m_Count, saved.count) and ssc::AreSameDims(variable.m_Shape, saved.shape))
    {
        std::memcpy(m_Buffer.data() + saved.posStart, data, saved.posCount);
    }
    else
    {
        throw std::runtime_error("ssc only accepts fixed IO pattern");
    }
}

template <class T>
void SscWriter::PutDeferredCommon(Variable<T> &variable, const T *data)
{
    TAU_SCOPED_TIMER_FUNC();
    PutSyncCommon(variable, data);
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_SSCWRITER_TCC_
