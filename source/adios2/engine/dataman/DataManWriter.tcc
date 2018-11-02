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

#include "adios2/ADIOSMPI.h"
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

    if (m_Format == "dataman")
    {
        for (size_t i = 0; i < m_TransportChannels; ++i)
        {
            if (m_WorkflowMode == "subscribe")
            {
                m_DataManSerializer[i]->Put(variable, m_Name, CurrentStep(),
                                            m_MPIRank, "",
                                            m_IO.m_TransportsParameters[i]);
            }
            else
            {
                m_DataManSerializer[i]->Put(variable, m_Name, CurrentStep(),
                                            m_MPIRank, "",
                                            m_IO.m_TransportsParameters[i]);
            }
        }
    }
    else if (m_Format == "binary")
    {
    }
    else
    {
        throw(std::invalid_argument(
            "[DataManWriter::PutSyncCommon] invalid format " + m_Format));
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_DATAMAN_DATAMAN_WRITER_H_ */
