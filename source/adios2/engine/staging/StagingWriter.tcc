/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * StagingWriter.tcc
 *
 *  Created on: Nov 1, 2018
 *      Author: Jason Wang
 */

#ifndef ADIOS2_ENGINE_STAGINGWRITER_TCC_
#define ADIOS2_ENGINE_STAGINGWRITER_TCC_

#include "StagingWriter.h"

#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

template <class T>
void StagingWriter::PutSyncCommon(Variable<T> &variable,
                                   const typename Variable<T>::Info &blockInfo)
{
    if (m_Verbosity == 5)
    {
        std::cout << "Staging Writer " << m_WriterRank << "     PutSync("
                  << variable.m_Name << ")\n";
    }
}

template <class T>
void StagingWriter::PutDeferredCommon(Variable<T> &variable, const T *data)
{
    variable.SetBlockInfo(data, CurrentStep());

    if (m_Verbosity == 5)
    {
        std::cout << "Staging Writer " << m_WriterRank << "     PutDeferred("
                  << variable.m_Name << ")\n";
    }
    m_NeedPerformPuts = true;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_STAGINGWRITER_TCC_
