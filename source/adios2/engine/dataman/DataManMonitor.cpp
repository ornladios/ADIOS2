/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManMonitor.cpp
 *
 *  Created on: Jun 2, 2020
 *      Author: Jason Wang
 */

#include "DataManMonitor.h"
#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

void DataManMonitor::SetAverageSteps(size_t step) { m_AverageSteps = step; }

void DataManMonitor::SetClockError(uint64_t roundLatency,
                                   uint64_t remoteTimeBase)
{
    m_ClockError = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count() -
                   remoteTimeBase - roundLatency / 2;
}

void DataManMonitor::BeginStep(size_t step)
{
    if (step == 0)
    {
        m_InitialTimer = std::chrono::system_clock::now();
    }
    if (m_StepTimers.empty())
    {
        m_StepTimers.push(std::chrono::system_clock::now());
    }

    m_StepBytes = 0;

    if (m_TotalBytes.empty())
    {
        m_TotalBytes.push(0);
    }
    else
    {
        m_TotalBytes.push(m_TotalBytes.back());
    }

    ++m_CurrentStep;
}

void DataManMonitor::EndStep(size_t step)
{
    m_StepTimers.push(std::chrono::system_clock::now());

    while (m_StepTimers.size() > m_AverageSteps)
    {
        m_StepTimers.pop();
    }
    while (m_TotalBytes.size() > m_AverageSteps)
    {
        m_TotalBytes.pop();
    }

    m_TotalTime = std::chrono::duration_cast<std::chrono::microseconds>(
                      (m_StepTimers.back() - m_InitialTimer))
                      .count();
    m_AverageTime = std::chrono::duration_cast<std::chrono::microseconds>(
                        (m_StepTimers.back() - m_StepTimers.front()))
                        .count();
    m_TotalRate = static_cast<double>(m_TotalBytes.back()) /
                  static_cast<double>(m_TotalTime);
    m_AverageRate =
        static_cast<double>(m_TotalBytes.back() - m_TotalBytes.front()) /
        static_cast<double>(m_AverageTime);
    if (step > 0)
    {
        m_DropRate = static_cast<double>((step - m_CurrentStep)) / step;
    }
    m_StepsPerSecond = step / m_TotalTime * 1000000;

    if (m_Verbose)
    {
        std::lock_guard<std::mutex> l(m_PrintMutex);
        std::cout << "Step " << step << ", Total MBs "
                  << static_cast<double>(m_TotalBytes.back()) / 1000000.0
                  << ", Step MBs "
                  << static_cast<double>(m_StepBytes) / 1000000.0
                  << ", Total seconds "
                  << static_cast<double>(m_TotalTime) / 1000000.0 << ", "
                  << m_StepTimers.size() << " step seconds "
                  << static_cast<double>(m_AverageTime) / 1000000.0
                  << ", Total MB/s " << m_TotalRate << ", "
                  << m_StepTimers.size() << " step average MB/s "
                  << m_AverageRate << ", Drop rate " << m_DropRate * 100 << "%"
                  << ", Steps per second " << m_StepsPerSecond << std::endl;
    }
}

void DataManMonitor::AddBytes(size_t bytes)
{
    m_TotalBytes.back() += bytes;
    m_StepBytes += bytes;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
