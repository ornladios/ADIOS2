/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * InlineWriter.tcc implementation of template functions with known type
 *
 *  Created on: Nov 16, 2018
 *      Author: Aron Helser aron.helser@kitware.com
 */
#ifndef ADIOS2_ENGINE_INLINEWRITER_TCC_
#define ADIOS2_ENGINE_INLINEWRITER_TCC_

#include "InlineWriter.h"

#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

template <class T>
void InlineWriter::PutSyncCommon(Variable<T> &variable,
                                 const typename Variable<T>::Info &blockInfo)
{
    auto &info = variable.m_BlocksInfo.back();
    info.BlockID = variable.m_BlocksInfo.size() - 1;
    // passed in blockInfo has current blockInfo.Data member.
    if (blockInfo.Shape.size() == 0 && blockInfo.Count.size() == 0 &&
        blockInfo.StepsCount == 1)
    {
        info.IsValue = true;
        info.Value = blockInfo.Data[0];
    }
    if (m_Verbosity == 5)
    {
        std::cout << "Inline Writer " << m_WriterRank << "     PutSync("
                  << variable.m_Name << ")\n";
    }
}

template <class T>
void InlineWriter::PutDeferredCommon(Variable<T> &variable, const T *data)
{
    variable.SetBlockInfo(data, CurrentStep());
    auto &info = variable.m_BlocksInfo.back();
    info.BlockID = variable.m_BlocksInfo.size() - 1;

    if (m_Verbosity == 5)
    {
        std::cout << "Inline Writer " << m_WriterRank << "     PutDeferred("
                  << variable.m_Name << ")\n";
    }
    m_NeedPerformPuts = true;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_INLINEWRITER_TCC_ */
