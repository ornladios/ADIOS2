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
    Log(5,
        "SscWriter::PutSync(" + variable.m_Name + ") begin. Current step " +
            std::to_string(m_CurrentStep),
        true, true);
    if (m_CurrentStepActive)
    {
        PutDeferredCommon(variable, data);
        PerformPuts();
    }
    Log(5,
        "SscWriter::PutSync(" + variable.m_Name + ") end. Current step " +
            std::to_string(m_CurrentStep),
        true, true);
}

template <class T>
void SscWriter::PutDeferredCommon(Variable<T> &variable, const T *data)
{
    TAU_SCOPED_TIMER_FUNC();
    Log(5,
        "SscWriter::PutDeferred(" + variable.m_Name + ") start. Current step " +
            std::to_string(m_CurrentStep),
        true, true);

    if (m_CurrentStepActive)
    {
        for (const auto &op : variable.m_Operations)
        {
            std::lock_guard<std::mutex> l(m_CompressionParamsMutex);
            std::string opName = op.Op->m_Type;
            if (opName == "zfp" or opName == "bzip2" or opName == "sz")
            {
                m_CompressionParams[variable.m_Name]["CompressionMethod"] =
                    opName;
                for (const auto &p : op.Parameters)
                {
                    m_CompressionParams[variable.m_Name]
                                       [opName + ":" + p.first] = p.second;
                }
                break;
            }
        }

        if (variable.m_SingleValue)
        {
            variable.m_Shape = Dims(1, 1);
            variable.m_Start = Dims(1, 0);
            variable.m_Count = Dims(1, 1);
        }
        variable.SetData(data);
        m_DataManSerializer.PutVar(
            variable, m_Name, CurrentStep(), m_MpiRank,
            m_FullAddresses[rand() % m_FullAddresses.size()], Params());
    }

    Log(5,
        "SscWriter::PutDeferred(" + variable.m_Name + ") end. Current step " +
            std::to_string(m_CurrentStep),
        true, true);
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_SSCWRITER_TCC_
