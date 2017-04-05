/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Timer.h
 *
 *  Created on: Apr 4, 2017
 *      Author: wfg
 */

#ifndef TIMER_H_
#define TIMER_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <chrono>
#include <string>
/// \endcond

#include "core/Support.h"

namespace adios
{
namespace profiling
{

class Timer
{

public:
  const std::string m_Process;
  unsigned long long int m_ProcessTime = 0;

  /**
   * Timer object constructor using std::chrono class
   * @param process name of process to be measured
   * @param resolution time resolution (mus, ms, s, etc.) from Resolutions in
   * core/Support.h
   * @param debugMode true: additional checks and exceptions, false: no checks
   */
  Timer(const std::string process, const Support::Resolutions resolution,
        const bool debugMode = false);

  ~Timer();

  void SetInitialTime(); ///< set time to start counting for a process
  void SetTime(); ///< sets time to measure, it will add to overall process time
  std::string GetUnits() const;

private:
  const Support::Resolutions m_Resolution;
  const bool m_DebugMode = false;
  std::chrono::time_point<std::chrono::high_resolution_clock> m_InitialTime;
  std::chrono::time_point<std::chrono::high_resolution_clock> m_ElapsedTime;
  bool m_InitialTimeSet = false;
  long long int GetCurrentTime(); ///< called by SetTime
};

} // end namespace profiling
} // end namespace adios

#endif /* TIMER_H_ */
