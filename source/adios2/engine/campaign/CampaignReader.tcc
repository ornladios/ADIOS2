/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CampaignReader.tcc
 *
 *  Created on: May 15, 2023
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#ifndef ADIOS2_ENGINE_CAMPAIGNREADER_TCC_
#define ADIOS2_ENGINE_CAMPAIGNREADER_TCC_

#include "CampaignReader.h"

#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

template <>
inline void CampaignReader::GetSyncCommon(Variable<std::string> &variable,
                                          std::string *data)
{
    variable.m_Data = data;
    if (m_Verbosity == 5)
    {
        std::cout << "Skeleton Reader " << m_ReaderRank << "     GetSync("
                  << variable.m_Name << ")\n";
    }
}

template <class T>
inline void CampaignReader::GetSyncCommon(Variable<T> &variable, T *data)
{
    variable.m_Data = data;
    if (m_Verbosity == 5)
    {
        std::cout << "Skeleton Reader " << m_ReaderRank << "     GetSync("
                  << variable.m_Name << ")\n";
    }
}

template <class T>
void CampaignReader::GetDeferredCommon(Variable<T> &variable, T *data)
{
    // returns immediately
    if (m_Verbosity == 5)
    {
        std::cout << "Skeleton Reader " << m_ReaderRank << "     GetDeferred("
                  << variable.m_Name << ")\n";
    }
    m_NeedPerformGets = true;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_CAMPAIGNREADER_TCC_
