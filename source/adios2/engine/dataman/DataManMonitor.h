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
    void BeginTransport(size_t step);
    void EndTransport();
    void AddBytes(size_t bytes);

private:
    using TimePoint = std::chrono::time_point<std::chrono::system_clock>;
    std::queue<TimePoint> m_StepTimers;
    TimePoint m_InitialTimer;
    std::queue<size_t> m_StepBytes;
    std::queue<size_t> m_TotalBytes;

    std::queue<std::pair<size_t, TimePoint>> m_TransportTimers;
    std::mutex m_TransportTimersMutex;

    std::mutex m_PrintMutex;

    size_t m_AverageSteps = 10;
    int64_t m_CurrentStep = -1;

    double m_TotalTime;
    double m_AverageTime;
    double m_TotalRate;
    double m_AverageRate;
    double m_DropRate;
    double m_StepsPerSecond;

    bool m_Verbose = true;
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_DATAMAN_DATAMANMONITOR_H_ */
