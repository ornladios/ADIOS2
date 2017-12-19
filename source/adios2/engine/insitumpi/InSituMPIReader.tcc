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
    m_NDeferredGets++;
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
    m_NDeferredGets++;
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
    m_NDeferredGets++;
}

} // end namespace adios2

#endif // ADIOS2_ENGINE_INSITUMPIREADER_TCC_
