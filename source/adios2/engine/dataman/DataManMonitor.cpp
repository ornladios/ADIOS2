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
    if (m_Timers.empty())
    {
        m_Timers.push(std::chrono::system_clock::now());
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
}

void DataManMonitor::EndStep(size_t step)
{
    m_Timers.push(std::chrono::system_clock::now());

    if (m_Timers.size() > m_AverageSteps)
    {
        m_Timers.pop();
    }
    if (m_TotalBytes.size() > m_AverageSteps)
    {
        m_TotalBytes.pop();
    }
    if (m_StepBytes.size() > m_AverageSteps)
    {
        m_StepBytes.pop();
    }

    if (m_Verbose)
    {
        double totalTime =
            std::chrono::duration_cast<std::chrono::microseconds>(
                (m_Timers.back() - m_InitialTimer))
                .count();
        double averageTime =
            std::chrono::duration_cast<std::chrono::microseconds>(
                (m_Timers.back() - m_Timers.front()))
                .count();
        double totalRate = m_TotalBytes.back() / totalTime;
        double averageRate =
            (m_TotalBytes.back() - m_TotalBytes.front()) / averageTime;

        std::cout << "Step " << step << ", Total MBs "
                  << m_TotalBytes.back() / 1000000 << ", Step MBs "
                  << m_StepBytes.back() / 1000000 << ", Total seconds "
                  << totalTime / 1000000 << ", " << m_Timers.size()
                  << " step seconds " << averageTime / 1000000
                  << ", Total MB/s " << totalRate << ", " << m_Timers.size()
                  << " step average MB/s " << averageRate << std::endl;
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
