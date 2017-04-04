/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * IOChrono.h
 *
 *  Created on: Mar 9, 2017
 *      Author: wfg
 */

#ifndef IOCHRONO_H_
#define IOCHRONO_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <vector>
/// \endcond

#include "packages/profiling/iochrono/Timer.h"

namespace adios
{
namespace profiling
{

/**
 * Struct used to track
 */
struct IOChrono
{
  std::vector<Timer>
      Timers; ///< one timer for each process (open, write, buffering, etc.)
  std::vector<unsigned long long int>
      TotalBytes;        ///< tracks bytes for buffering
  bool IsActive = false; ///< flag to determine if profiling is used
};

} // end namespace
} // end namespace

#endif /* IOCHRONO_H_ */
