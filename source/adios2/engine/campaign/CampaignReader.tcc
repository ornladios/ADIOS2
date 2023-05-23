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

template <class T>
inline Variable<T> CampaignReader::DuplicateVariable(Variable<T> *variable,
                                                     IO &io, std::string &name,
                                                     Engine *e,
                                                     VarInternalInfo &vii)
{
    auto &v = io.DefineVariable<T>(name, variable->Shape());
    v.m_AvailableStepsCount = variable->GetAvailableStepsCount();
    v.m_AvailableStepsStart = variable->GetAvailableStepsStart();
    v.m_ShapeID = variable->m_ShapeID;
    v.m_SingleValue = variable->m_SingleValue;
    v.m_ReadAsJoined = variable->m_ReadAsJoined;
    v.m_ReadAsLocalValue = variable->m_ReadAsLocalValue;
    v.m_RandomAccess = variable->m_RandomAccess;
    v.m_MemSpace = variable->m_MemSpace;
    v.m_JoinedDimPos = variable->m_JoinedDimPos;
    v.m_AvailableStepBlockIndexOffsets =
        variable->m_AvailableStepBlockIndexOffsets;
    v.m_AvailableShapes = variable->m_AvailableShapes;
    v.m_Engine = e;
    vii.originalVar = static_cast<void *>(variable);
    m_VarInternalInfo.emplace(name, vii);
    return v;
}

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
