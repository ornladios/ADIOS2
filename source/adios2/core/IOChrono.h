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

#include "adios2/ADIOSConfig.h"
#include "adios2/core/Timer.h"

namespace adios
{
namespace profiling
{

/**
 * Struct used to track
 */
struct IOChrono
{
    ///< one timer for each process (open, write, buffering, etc.)
    std::vector<Timer> Timers;

    ///< tracks bytes for buffering
    std::vector<unsigned long long int> TotalBytes;

    ///< flag to determine if profiling is used
    bool IsActive = false;
};

} // end namespace profiling
} // end namespace adios

#endif /* IOCHRONO_H_ */
