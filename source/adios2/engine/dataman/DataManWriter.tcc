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

namespace adios2
{

template <class T>
void DataManWriter::PutSyncCommon(Variable<T> &variable, const T *values)
{
    // here comes your magic at Writing now variable.m_UserValues has the
    // data
    // passed by the user
    // set variable
    variable.SetData(values);

    // This part will go away, this is just to monitor variables per rank

    if (variable.m_Count.empty())
    {
        variable.m_Count = variable.m_Shape;
    }
    if (variable.m_Start.empty())
    {
        variable.m_Start.assign(variable.m_Count.size(), 0);
    }

    json jmsg;
    jmsg["doid"] = m_Name;
    jmsg["var"] = variable.m_Name;
    jmsg["dtype"] = GetType<T>();
    jmsg["putshape"] = variable.m_Count;
    jmsg["varshape"] = variable.m_Shape;
    jmsg["offset"] = variable.m_Start;
    jmsg["timestep"] = 0;
    m_Man.put(values, jmsg);

    if (m_DoMonitor)
    {
        MPI_Barrier(m_MPIComm);
        std::cout << "I am hooked to the DataMan library\n";
        std::cout << "Variable " << variable.m_Name << "\n";
        std::cout << "putshape " << variable.m_Count.size() << "\n";
        std::cout << "varshape " << variable.m_Shape.size() << "\n";
        std::cout << "offset " << variable.m_Start.size() << "\n";

        int rank = 0, size = 1;
        MPI_Comm_size(m_MPIComm, &size);

        for (int i = 0; i < size; ++i)
        {
            if (i == rank)
            {
                std::cout << "Rank: " << i << "\n";
                std::cout << std::endl;
            }
            else
            {
                sleep(1);
            }
        }
        MPI_Barrier(m_MPIComm);
    }
}

} // end namespace adios

#endif /* ADIOS2_ENGINE_DATAMAN_DATAMAN_WRITER_H_ */
