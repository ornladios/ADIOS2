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
        AsyncRecvVariable(variable);
    }
    else
    {
        m_BP3Deserializer.GetDeferredVariable(variable, data);
        m_BP3Deserializer.m_PerformedGets = false;
    }
}

template <class T>
void InSituMPIReader::AsyncRecvVariable(Variable<T> &variable)
{
    const auto it = m_ReadScheduleMap.find(variable.m_Name);
    if (it != m_ReadScheduleMap.end())
    {
        SubFileInfoMap requests = it->second;

        // <writer, <steps, SubFileInfo>>
        for (const auto &subFileIndexPair : requests)
        {
            const size_t writerRank = subFileIndexPair.first; // writer
            // <steps, SubFileInfo>  but there is only one step
            for (const auto &stepPair : subFileIndexPair.second)
            {
                const std::vector<SubFileInfo> &sfis = stepPair.second;
                for (const auto &sfi : sfis)
                {
                    if (m_Verbosity == 5)
                    {
                        std::cout << "InSituMPI Reader " << m_ReaderRank
                                  << " async recv var = " << variable.m_Name
                                  << " from writer " << writerRank;
                        std::cout << " info = ";
                        insitumpi::PrintSubFileInfo(sfi);
                        std::cout << std::endl;
                    }
                }
                break; // there is only one step here
            }
        }
    }
}

} // end namespace adios2

#endif // ADIOS2_ENGINE_INSITUMPIREADER_TCC_
