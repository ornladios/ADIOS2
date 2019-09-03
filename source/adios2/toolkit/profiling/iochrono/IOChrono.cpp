/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * IOChrono.cpp
 *
 *  Created on: Sep 6, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "IOChrono.h"

namespace adios2
{
namespace profiling
{

void IOChrono::Start(const std::string process) noexcept
{
    if (m_IsActive)
    {
        m_Timers.at(process).Resume();
    }
}

void IOChrono::Stop(const std::string process)
{
    if (m_IsActive)
    {
        m_Timers.at(process).Pause();
    }
}

} // end namespace profiling
} // end namespace adios2
