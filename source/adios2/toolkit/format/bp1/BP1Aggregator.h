/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP1Aggregator.h
 *
 *  Created on: Mar 1, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP1_BP1AGGREGATOR_H_
#define ADIOS2_TOOLKIT_FORMAT_BP1_BP1AGGREGATOR_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
/// \endcond

#include "adios2/ADIOSConfig.h"
#include "adios2/ADIOSMPICommOnly.h"

namespace adios2
{
namespace format
{

/** Does all MPI aggregation tasks */
class BP1Aggregator
{

public:
    MPI_Comm m_MPIComm;  ///< MPI communicator from Engine
    int m_RankMPI = 0;   ///< current MPI rank process
    int m_SizeMPI = 1;   ///< current MPI processes size
    int m_Processes = 1; ///< number of aggregated MPI processes

    /**
     * Unique constructor
     * @param mpiComm coming from engine
     */
    BP1Aggregator(MPI_Comm mpiComm, const bool debugMode = false);

    ~BP1Aggregator() = default;

    /**
     * Function that aggregates and writes (from rank = 0) profiling.log in
     * python dictionary format
     * @param rankLog contain rank profiling info to be aggregated
     */
    std::string GetGlobalProfilingLog(const std::string &rankLog);

private:
    const bool m_DebugMode = false;
};

} // end namespace format
} // end namespace adios

#endif /* ADIOS2_UTILITIES_FORMAT_BP1_BP1AGGREGATOR_H_ */
