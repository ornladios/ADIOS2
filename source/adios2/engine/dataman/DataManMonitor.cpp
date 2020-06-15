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

    m_StepBytes.push(0);

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

    if (m_StepTimers.size() > m_AverageSteps)
    {
        m_StepTimers.pop();
    }
    if (m_TotalBytes.size() > m_AverageSteps)
    {
        m_TotalBytes.pop();
    }
    if (m_StepBytes.size() > m_AverageSteps)
    {
        m_StepBytes.pop();
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
                  << static_cast<double>(m_StepBytes.back()) / 1000000.0
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

void DataManMonitor::BeginTransport(size_t step)
{
    std::lock_guard<std::mutex> l(m_TransportTimersMutex);
    m_TransportTimers.push({step, std::chrono::system_clock::now()});
}

void DataManMonitor::EndTransport()
{
    std::lock_guard<std::mutex> l(m_TransportTimersMutex);
    if (!m_TransportTimers.empty())
    {
        auto latency = std::chrono::duration_cast<std::chrono::microseconds>(
                           (std::chrono::system_clock::now() -
                            m_TransportTimers.back().second))
                           .count();
        if (m_Verbose)
        {
            std::lock_guard<std::mutex> l(m_PrintMutex);
            std::cout << "Step " << m_TransportTimers.back().first
                      << ", Latency milliseconds "
                      << static_cast<double>(latency) / 1000.0 << std::endl;
        }
        m_TransportTimers.pop();
    }
}

void DataManMonitor::AddBytes(size_t bytes)
{
    m_TotalBytes.back() += bytes;
    m_StepBytes.back() += bytes;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
