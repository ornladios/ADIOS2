/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * WdmReader.tcc
 *
 *  Created on: Nov 1, 2018
 *      Author: Jason Wang
 */

#ifndef ADIOS2_ENGINE_STAGINGREADER_TCC_
#define ADIOS2_ENGINE_STAGINGREADER_TCC_

#include "WdmReader.h"

#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

template <class T>
inline void WdmReader::GetSyncCommon(Variable<T> &variable, T *data)
{
    Log(5, "WdmReader::GetSync(" + variable.m_Name + ") begin", true, true);

    GetDeferredCommon(variable, data);
    PerformGets();

    Log(5, "WdmReader::GetSync(" + variable.m_Name + ") end", true, true);
}

template <class T>
void WdmReader::GetDeferredCommon(Variable<T> &variable, T *data)
{
    Log(5, "WdmReader::GetDeferred(" + variable.m_Name + ") begin", true, true);

    if(variable.m_SingleValue)
    {
        variable.m_Shape = Dims(1,1);
        variable.m_Start = Dims(1,0);
        variable.m_Count = Dims(1,1);
    }
    m_DataManSerializer.PutDeferredRequest(variable.m_Name, CurrentStep(),
                variable.m_Start, variable.m_Count, data);

    m_DeferredRequests.emplace_back();
    auto &req = m_DeferredRequests.back();
    req.variable = variable.m_Name;
    req.step = m_CurrentStep;
    req.start = variable.m_Start;
    req.count = variable.m_Count;
    req.data = data;
    req.type = helper::GetType<T>();

    Log(5, "WdmReader::GetDeferred(" + variable.m_Name + ") end", true, true);
}

template <typename T>
void WdmReader::CheckIOVariable(const std::string &name, const Dims &shape,
                                const Dims &start, const Dims &count)
{
    bool singleValue = false;
    if(shape.size() == 1 and start.size() ==1 and count.size()==1)
    {
        if(shape[0] == 1 and start[0] == 0 and count[0] == 1)
        {
            singleValue = true;
        }
    }
    auto v = m_IO.InquireVariable<T>(name);
    if (v == nullptr)
    {
        if(singleValue)
        {
            m_IO.DefineVariable<T>(name);
        }
        else
        {
            m_IO.DefineVariable<T>(name, shape, start, count);
        }
    }
    else
    {
        if(not singleValue)
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

}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_STAGINGREADER_TCC_
