/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP1Aggregator.h
 *
 *  Created on: Mar 1, 2017
 *      Author: wfg
 */

#ifndef BP1AGGREGATOR_H_
#define BP1AGGREGATOR_H_

#include "ADIOS_MPI.h"

namespace adios
{
namespace format
{

/**
 * Does all MPI related spatial aggregation tasks
 */
class BP1Aggregator
{

public:
  MPI_Comm m_MPIComm = MPI_COMM_SELF; ///< MPI communicator from Engine
  int m_RankMPI = 0;                  ///< current MPI rank process
  int m_SizeMPI = 1;                  ///< current MPI processes size

  /**
   * Unique constructor
   * @param mpiComm coming from engine
   */
  BP1Aggregator(MPI_Comm mpiComm, const bool debugMode = false);

  ~BP1Aggregator();

  /**
   * Function that aggregates and writes (from rank = 0) profiling.log in python
   * dictionary format
   * @param rankLog contain rank profiling info to be aggregated
   */
  void WriteProfilingLog(const std::string fileName,
                         const std::string &rankLog);

private:
  const bool m_DebugMode = false;
};

} // end namespace format
} // end namespace adios

#endif /* BP1AGGREGATOR_H_ */
