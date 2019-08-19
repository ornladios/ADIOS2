/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataMan.h
 *
 *  Created on: Jan 10, 2017
 *      Author: wfg
 */

#ifndef ADIOS2_ENGINE_DATAMAN_DATAMAN_WRITER_TCC_
#define ADIOS2_ENGINE_DATAMAN_DATAMAN_WRITER_TCC_

#include "DataManWriter.h"

#include "adios2/common/ADIOSMPI.h"
#include "adios2/helper/adiosFunctions.h" //GetType<T>

#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

template <class T>
void DataManWriter::PutSyncCommon(Variable<T> &variable, const T *values)
{
    PutDeferredCommon(variable, values);
    PerformPuts();
}

template <class T>
void DataManWriter::PutDeferredCommon(Variable<T> &variable, const T *values)
{

    variable.SetData(values);

    if (variable.m_Shape.empty())
    {
        variable.m_Shape = variable.m_Count;
    }
    if (variable.m_Count.empty())
    {
        variable.m_Count = variable.m_Shape;
    }
    if (variable.m_Start.empty())
    {
        variable.m_Start.assign(variable.m_Count.size(), 0);
    }

    if (m_IsRowMajor)
    {
        for (size_t i = 0; i < m_Channels; ++i)
        {
            m_DataManSerializer[i]->PutVar(variable, m_Name, CurrentStep(),
                                           m_MpiRank, "",
                                           m_IO.m_TransportsParameters[i]);
        }
    }
    else
    {
        if (m_ContiguousMajor)
        {
            Dims start = variable.m_Start;
            Dims count = variable.m_Count;
            Dims shape = variable.m_Shape;
            Dims memstart = variable.m_MemoryStart;
            Dims memcount = variable.m_MemoryCount;
            std::reverse(start.begin(), start.end());
            std::reverse(count.begin(), count.end());
            std::reverse(shape.begin(), shape.end());
            std::reverse(memstart.begin(), memstart.end());
            std::reverse(memcount.begin(), memcount.end());
            for (size_t i = 0; i < m_Channels; ++i)
            {
                m_DataManSerializer[i]->PutVar(
                    variable.m_Data, variable.m_Name, shape, start, count,
                    memstart, memcount, m_Name, CurrentStep(), m_MpiRank, "",
                    m_IO.m_TransportsParameters[i]);
            }
        }
        else
        {
            for (size_t i = 0; i < m_Channels; ++i)
            {
                m_DataManSerializer[i]->PutVar(variable, m_Name, CurrentStep(),
                                               m_MpiRank, "",
                                               m_IO.m_TransportsParameters[i]);
            }
        }
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_DATAMAN_DATAMAN_WRITER_H_ */
