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
    variable.m_Data = data;
    if (m_Verbosity == 5)
    {
        std::cout << "Staging Reader " << m_ReaderRank << "     GetSync("
                  << variable.m_Name << ")\n";
    }
}

template <class T>
inline void StagingReader::GetSyncCommon(Variable<T> &variable, T *data)
{
    variable.m_Data = data;
    if (m_Verbosity == 5)
    {
        std::cout << "Staging Reader " << m_ReaderRank << "     GetSync("
                  << variable.m_Name << ")\n";
    }
}

template <class T>
void StagingReader::GetDeferredCommon(Variable<T> &variable, T *data)
{
    // returns immediately
    if (m_Verbosity == 5)
    {
        std::cout << "Staging Reader " << m_ReaderRank << "     GetDeferred("
                  << variable.m_Name << ")\n";
    }
    m_NeedPerformGets = true;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_STAGINGREADER_TCC_
