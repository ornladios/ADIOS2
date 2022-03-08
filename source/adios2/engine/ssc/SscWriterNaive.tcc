/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SscWriterNaive.tcc
 *
 *  Created on: Mar 7, 2022
 *      Author: Jason Wang
 */

#ifndef ADIOS2_ENGINE_SSCWRITERNAIVE_TCC_
#define ADIOS2_ENGINE_SSCWRITERNAIVE_TCC_

#include "SscWriterNaive.h"

namespace adios2
{
namespace core
{
namespace engine
{
namespace ssc
{

template <>
void SscWriterNaive::PutDeferredCommon(Variable<std::string> &variable,
                                       const std::string *data)
{
    variable.SetData(data);
}

template <class T>
void SscWriterNaive::PutDeferredCommon(Variable<T> &variable, const T *data)
{

    if ((variable.m_ShapeID == ShapeID::GlobalValue ||
         variable.m_ShapeID == ShapeID::LocalValue ||
         variable.m_Type == DataType::String) &&
        m_WriterRank != 0)
    {
        return;
    }

    variable.SetData(data);

    Dims vStart = variable.m_Start;
    Dims vCount = variable.m_Count;
    Dims vShape = variable.m_Shape;

    if (m_IO.m_ArrayOrder != ArrayOrdering::RowMajor)
    {
        std::reverse(vStart.begin(), vStart.end());
        std::reverse(vCount.begin(), vCount.end());
        std::reverse(vShape.begin(), vShape.end());
    }
}
}
}
}
}

#endif // ADIOS2_ENGINE_SSCWRITERNAIVE_TCC_
