/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * WdmWriter.tcc
 *
 *  Created on: Nov 1, 2018
 *      Author: Jason Wang
 */

#ifndef ADIOS2_ENGINE_STAGINGWRITER_TCC_
#define ADIOS2_ENGINE_STAGINGWRITER_TCC_

#include "WdmWriter.h"

#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

template <class T>
void WdmWriter::PutSyncCommon(Variable<T> &variable, const T *data)
{
    Log(5,
        "WdmWriter::PutSync(" + variable.m_Name + ") begin. Current step " +
            std::to_string(m_CurrentStep),
        true, true);
    PutDeferredCommon(variable, data);
    PerformPuts();
    Log(5,
        "WdmWriter::PutSync(" + variable.m_Name + ") end. Current step " +
            std::to_string(m_CurrentStep),
        true, true);
}

template <class T>
void WdmWriter::PutDeferredCommon(Variable<T> &variable, const T *data)
{
    Log(5,
        "WdmWriter::PutDeferred(" + variable.m_Name + ") start. Current step " +
            std::to_string(m_CurrentStep),
        true, true);
    if (variable.m_SingleValue)
    {
        variable.m_Shape = Dims(1, 1);
        variable.m_Start = Dims(1, 0);
        variable.m_Count = Dims(1, 1);
    }
    variable.SetData(data);
    m_DataManSerializer.PutVar(variable, m_Name, CurrentStep(), m_MpiRank,
                               m_FullAddresses[rand() % m_FullAddresses.size()],
                               Params());
    Log(5,
        "WdmWriter::PutDeferred(" + variable.m_Name + ") end. Current step " +
            std::to_string(m_CurrentStep),
        true, true);
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_STAGINGWRITER_TCC_
