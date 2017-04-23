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

namespace adios
{

template <class T>
void DataManWriter::WriteVariableCommon(Variable<T> &variable, const T *values)
{
    // here comes your magic at Writing now variable.m_UserValues has the
    // data
    // passed by the user
    // set variable
    variable.m_AppValues = values;
    m_WrittenVariables.insert(variable.m_Name);

    // This part will go away, this is just to monitor variables per rank

    if (variable.m_GlobalDimensions.empty())
        variable.m_GlobalDimensions = variable.m_LocalDimensions;
    if (variable.m_Offsets.empty())
        variable.m_Offsets.assign(variable.m_LocalDimensions.size(), 0);

    json jmsg;
    jmsg["doid"] = m_Name;
    jmsg["var"] = variable.m_Name;
    jmsg["dtype"] = GetType<T>();
    jmsg["putshape"] = variable.m_LocalDimensions;
    jmsg["varshape"] = variable.m_GlobalDimensions;
    jmsg["offset"] = variable.m_Offsets;
    jmsg["timestep"] = 0;
    m_Man.put(values, jmsg);

    if (m_DoMonitor)
    {
        MPI_Barrier(m_MPIComm);
        std::cout << "I am hooked to the DataMan library\n";
        std::cout << "putshape " << variable.m_LocalDimensions.size()
                  << std::endl;
        std::cout << "varshape " << variable.m_GlobalDimensions.size()
                  << std::endl;
        std::cout << "offset " << variable.m_Offsets.size() << std::endl;
        for (int i = 0; i < m_SizeMPI; ++i)
        {
            if (i == m_RankMPI)
            {
                std::cout << "Rank: " << m_RankMPI << "\n";
                variable.Monitor(std::cout);
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
