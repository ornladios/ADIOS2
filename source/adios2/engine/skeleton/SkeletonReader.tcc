/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_ENGINE_SKELETONREADER_TCC_
#define ADIOS2_ENGINE_SKELETONREADER_TCC_

#include "SkeletonReader.h"

#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

template <>
inline void SkeletonReader::GetSyncCommon(Variable<std::string> &variable, std::string *data)
{
    variable.m_Data = data;
    if (m_Verbosity == 5)
    {
        std::cout << "Skeleton Reader " << m_ReaderRank << "     GetSync(" << variable.m_Name
                  << ")\n";
    }
}

template <class T>
inline void SkeletonReader::GetSyncCommon(Variable<T> &variable, T *data)
{
    variable.m_Data = data;
    if (m_Verbosity == 5)
    {
        std::cout << "Skeleton Reader " << m_ReaderRank << "     GetSync(" << variable.m_Name
                  << ")\n";
    }
}

template <class T>
void SkeletonReader::GetDeferredCommon(Variable<T> &variable, T *data)
{
    // returns immediately
    if ((variable.m_ShapeID == ShapeID::GlobalValue) || (variable.m_ShapeID == ShapeID::LocalValue))
    {
        GetSyncCommon(variable, data);
    }
    else
    {
        if (m_Verbosity == 5)
        {
            std::cout << "Skeleton Reader " << m_ReaderRank << "     GetDeferred("
                      << variable.m_Name << ")\n";
        }
        m_NeedPerformGets = true;
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_SKELETONREADER_TCC_
