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
#include <mutex>
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
    void SetAverageSteps(size_t steps);
    void SetClockError(uint64_t roundLatency, uint64_t remoteTimeBase);

private:
    using TimePoint = std::chrono::time_point<std::chrono::system_clock>;
    TimePoint m_InitialTimer;
    std::queue<TimePoint> m_StepTimers;
    std::queue<size_t> m_TotalBytes;
    size_t m_StepBytes;

    std::mutex m_PrintMutex;

    size_t m_AverageSteps = 50;
    int64_t m_CurrentStep = -1;
    uint64_t m_ClockError = 0;

    double m_TotalTime = 0;
    double m_AverageTime = 0;
    double m_TotalRate = 0;
    double m_AverageRate = 0;
    double m_DropRate = 0;
    double m_StepsPerSecond = 0;

    bool m_Verbose = true;
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_DATAMAN_DATAMANMONITOR_H_ */
