/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Timer.h
 *
 *  Created on: Apr 4, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_PROFILING_IOCHRONO_TIMER_H_
#define ADIOS2_TOOLKIT_PROFILING_IOCHRONO_TIMER_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <chrono>
#include <string>
/// \endcond

#include "adios2/common/ADIOSConfig.h"
#include "adios2/common/ADIOSTypes.h"

namespace adios2
{
namespace profiling
{

class Timer
{

public:
    /** process name */
    const std::string m_Process;

    /** process elapsed time */
    int64_t m_ProcessTime = 0;

    /** time unit for elapsed time from ADIOSTypes.h */
    const TimeUnit m_TimeUnit;

    /** creation timedate from std::ctime */
    std::string m_LocalTimeDate;

    /**
     * Timer object constructor using std::chrono class
     * @param process name of process to be measured
     * @param timeUnit (mus, ms, s, etc.) from ADIOSTypes.h TimeUnit
     * @param debugMode true: additional exception checks (recommended)
     */
    Timer(const std::string process, const TimeUnit timeUnit,
          const bool debugMode = false);

    ~Timer() = default;

    /** sets timer active to start counting */
    void Resume() noexcept;

    /**
     * Pauses timer (set to inactive)
     * @throws std::invalid_argument if Resume not previously called
     */
    void Pause();

    /** Returns TimeUnit as a short std::string  */
    std::string GetShortUnits() const noexcept;

private:
    /** true: extra exceptions */
    const bool m_DebugMode = false;

    /** Set at Resume */
    std::chrono::time_point<std::chrono::high_resolution_clock> m_InitialTime;

    /** Set at Pause */
    std::chrono::time_point<std::chrono::high_resolution_clock> m_ElapsedTime;

    /** Checks if m_InitialTime is set, timer is running */
    bool m_InitialTimeSet = false;

    /** called by Pause to get time between Pause and Resume */
    int64_t GetElapsedTime();
};

} // end namespace profiling
} // end namespace adios

#endif /* ADIOS2_CORE_TIMER_H_ */
