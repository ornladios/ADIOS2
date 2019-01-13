/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * StagingReader.tcc
 *
 *  Created on: Nov 1, 2018
 *      Author: Jason Wang
 */

#ifndef ADIOS2_ENGINE_STAGINGREADER_TCC_
#define ADIOS2_ENGINE_STAGINGREADER_TCC_

#include "StagingReader.h"

#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

template <>
inline void StagingReader::GetSyncCommon(Variable<std::string> &variable,
                                         std::string *data)
{
    Log(5, "Staging Reader " + std::to_string(m_MpiRank) + " GetSync(" + variable.m_Name + ") start");

    GetDeferredCommon(variable, data);
    PerformGets();

    Log(5, "Staging Reader " + std::to_string(m_MpiRank) + " GetSync(" + variable.m_Name + ") end");
}

template <class T>
inline void StagingReader::GetSyncCommon(Variable<T> &variable, T *data)
{
    Log(5, "Staging Reader " + std::to_string(m_MpiRank) + " GetSync(" + variable.m_Name + ") start");

    GetDeferredCommon(variable, data);
    PerformGets();

    Log(5, "Staging Reader " + std::to_string(m_MpiRank) + " GetSync(" + variable.m_Name + ") end");
}

template <class T>
void StagingReader::GetDeferredCommon(Variable<T> &variable, T *data)
{
    Log(5, "Staging Reader " + std::to_string(m_MpiRank) + " GetDeferred(" + variable.m_Name + ") start");

    m_DataManSerializer.PutDeferredRequest(variable.m_Name, CurrentStep(),
                                           variable.m_Start, variable.m_Count,
                                           data);
    m_DeferredRequests.emplace_back();
    auto &req = m_DeferredRequests.back();
    req.variable = variable.m_Name;
    req.step = m_CurrentStep;
    req.start = variable.m_Start;
    req.count = variable.m_Count;
    req.data = data;
    req.type = helper::GetType<T>();

    Log(5, "Staging Reader " + std::to_string(m_MpiRank) + " GetDeferred(" + variable.m_Name + ") end");
}

template <typename T>
void StagingReader::CheckIOVariable(const std::string &name, const Dims &shape,
                                    const Dims &start, const Dims &count)
{
    auto v = m_IO.InquireVariable<T>(name);
    if (v == nullptr)
    {
        m_IO.DefineVariable<T>(name, shape, start, count);
    }
    else
    {
        if (v->m_Shape != shape)
        {
            v->SetShape(shape);
        }
        if (v->m_Start != start || v->m_Count != count)
        {
            v->SetSelection({start, count});
        }
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_STAGINGREADER_TCC_
