/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * InSituMPIReader.tcc
 *
 *  Created on: Dec 18, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#ifndef ADIOS2_ENGINE_INSITUMPIREADER_TCC_
#define ADIOS2_ENGINE_INSITUMPIREADER_TCC_

#include "InSituMPIReader.h"

#include <iostream>

namespace adios2
{

template <>
inline void InSituMPIReader::GetSyncCommon(Variable<std::string> &variable,
                                           std::string *data)
{
    variable.SetData(data);
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Reader " << m_ReaderRank << " GetSync("
                  << variable.m_Name << ")\n";
    }
    // FIXME: this call is only allowed for Global Values
}

template <class T>
inline void InSituMPIReader::GetSyncCommon(Variable<T> &variable, T *data)
{
    variable.SetData(data);
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Reader " << m_ReaderRank << " GetSync("
                  << variable.m_Name << ")\n";
    }
}

template <class T>
void InSituMPIReader::GetDeferredCommon(Variable<T> &variable, T *data)
{
    // returns immediately
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Reader " << m_ReaderRank << " GetDeferred("
                  << variable.m_Name << ")\n";
    }
    if (m_FixedSchedule && m_CurrentStep > 0)
    {
        // Create the async send for the variable now
        const SubFileInfoMap &sfim =
            m_BP3Deserializer.GetSubFileInfoMap(variable.m_Name);
        /* FIXME: this only works if there is only one block read for each
         * variable.
         * SubFileInfoMap contains ALL read schedules for the variable.
         * We should do this call per SubFileInfo that matches the request
         */
        AsyncRecvVariable(variable.m_Name, sfim);
    }
    else
    {
        /* FIXME: this call works if there is only one block read for each
         * variable.
         * SubFileInfoMap is created in this call which contains ALL read
         * schedules for the variable.
         */
        m_BP3Deserializer.GetDeferredVariable(variable, data);
        m_BP3Deserializer.m_PerformedGets = false;
    }
}

} // end namespace adios2

#endif // ADIOS2_ENGINE_INSITUMPIREADER_TCC_
