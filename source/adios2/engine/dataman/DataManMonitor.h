/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManCommon.h
 *
 *  Created on: Jun 2, 2020
 *      Author: Jason Wang
 */

#ifndef ADIOS2_ENGINE_DATAMAN_DATAMANMONITOR_H_
#define ADIOS2_ENGINE_DATAMAN_DATAMANMONITOR_H_

#include <chrono>
#include <queue>

namespace adios2
{
namespace core
{
namespace engine
{

class DataManMonitor
{
public:
    void BeginStep(size_t step);
    void EndStep(size_t step);
    void AddBytes(size_t bytes);

private:
    std::queue<std::chrono::time_point<std::chrono::system_clock>> m_Timers;
    std::chrono::time_point<std::chrono::system_clock> m_InitialTimer;
    std::queue<size_t> m_StepBytes;
    std::queue<size_t> m_TotalBytes;
    size_t m_AverageSteps = 10;

    bool m_Verbose = true;
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_DATAMAN_DATAMANMONITOR_H_ */
