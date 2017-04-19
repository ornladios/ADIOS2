/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Timer.cpp
 *
 *  Created on: Apr 4, 2017
 *      Author: wfg
 */

#include "core/Timer.h"

namespace adios
{
namespace profiling
{

Timer::Timer(const std::string process, const Support::Resolutions resolution,
             const bool debug)
: m_Process{process}, m_Resolution{resolution}, m_DebugMode{debug}
{
}

void Timer::SetInitialTime()
{
    m_InitialTime = std::chrono::high_resolution_clock::now();
    m_InitialTimeSet = true;
}

void Timer::SetTime()
{
    m_ElapsedTime = std::chrono::high_resolution_clock::now();
    m_ProcessTime += GetCurrentTime();
}

std::string Timer::GetUnits() const
{
    std::string units;
    if (m_Resolution == Support::Resolutions::mus)
        units = "mus";
    else if (m_Resolution == Support::Resolutions::ms)
        units = "ms";
    else if (m_Resolution == Support::Resolutions::s)
        units = "s";
    else if (m_Resolution == Support::Resolutions::m)
        units = "m";
    else if (m_Resolution == Support::Resolutions::h)
        units = "h";
    return units;
}

// PRIVATE

long long int Timer::GetCurrentTime()
{
    if (m_DebugMode == true)
    {
        if (m_InitialTimeSet == false)
            throw std::invalid_argument("ERROR: SetInitialTime() in process " +
                                        m_Process + " not called\n");
    }

    if (m_Resolution == Support::Resolutions::mus)
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(
                   m_ElapsedTime - m_InitialTime)
            .count();
    }
    else if (m_Resolution == Support::Resolutions::ms)
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   m_ElapsedTime - m_InitialTime)
            .count();
    }
    else if (m_Resolution == Support::Resolutions::s)
    {
        return std::chrono::duration_cast<std::chrono::seconds>(m_ElapsedTime -
                                                                m_InitialTime)
            .count();
    }
    else if (m_Resolution == Support::Resolutions::m)
    {
        return std::chrono::duration_cast<std::chrono::minutes>(m_ElapsedTime -
                                                                m_InitialTime)
            .count();
    }
    else if (m_Resolution == Support::Resolutions::h)
    {
        return std::chrono::duration_cast<std::chrono::hours>(m_ElapsedTime -
                                                              m_InitialTime)
            .count();
    }

    return -1; // failure
}

} // end namespace
} // end namespace
