/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosMPIFunctions.h : collection of MPI functions used across adios2
 *
 *  Created on: Jul 20, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_HELPER_ADIOSMPIFUNCTIONS_H_
#define ADIOS2_HELPER_ADIOSMPIFUNCTIONS_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
#include <vector>
/// \endcond

#include "adios2/common/ADIOSMPI.h"

namespace adios2
{
namespace helper
{

template <class T>
T BroadcastValue(const T &input, MPI_Comm mpiComm, const int rankSource = 0);

template <class T>
void BroadcastVector(std::vector<T> &vector, MPI_Comm mpiComm,
                     const int rankSource = 0);

template <class T>
T ReduceValues(const T source, MPI_Comm mpiComm, MPI_Op operation = MPI_SUM,
               const int rankDestination = 0);

/**
 * Gather a single source value from each ranks and forms a vector in
 * rankDestination
 * @param sourceSize input from each rank
 * @param mpiComm MPI communicator defining all ranks and size domain
 * @param rankDestination root, where all sizes are gathered in the returned
 * vector
 * @return in rankDestination: aggregated vector<T>, others: empty
 */
template <class T>
std::vector<T> GatherValues(const T source, MPI_Comm mpiComm,
                            const int rankDestination = 0);

/**
 * Perform AllGather for source value
 * @param source input
 * @param mpiComm MPI communicator defining all ranks and size domain
 * @return in all ranks: a vector with gathered source values ordered per rank
 */
template <class T>
std::vector<T> AllGatherValues(const T source, MPI_Comm mpiComm);

/**
 * Gather equal size arrays
 * @param source
 * @param sourceCount
 * @param destination
 * @param mpiComm
 * @param rankDestination
 */
template <class T>
void GatherArrays(const T *source, const size_t sourceCount, T *destination,
                  MPI_Comm mpiComm, const int rankDestination = 0);

/**
 * Perform AllGather for equal size arrays
 * @param source
 * @param sourceCount
 * @param destination
 * @param mpiComm
 */
template <class T>
void AllGatherArrays(const T *source, const size_t sourceCount, T *destination,
                     MPI_Comm mpiComm);

/**
 * Gather arrays of the same type into a destination (must be pre-allocated)
 * if countsSize == 1, calls MPI_Gather, otherwise calls MPI_Gatherv.
 * This function must be specialized for each MPI_Type.
 * @param source  input from each rank
 * @param counts  counts for each source
 * @param countsSize number of counts
 * @param destination resulting gathered buffer in rankDestination, unchaged in
 * others
 * @param mpiComm communicator establishing the domain
 * @param rankDestination rank in which arrays are gathered (root)
 */
template <class T>
void GathervArrays(const T *source, const size_t sourceCount,
                   const size_t *counts, const size_t countsSize,
                   T *destination, MPI_Comm mpiComm,
                   const int rankDestination = 0);

template <class T>
void GathervVectors(const std::vector<T> &in, std::vector<T> &out,
                    size_t &position, MPI_Comm mpiComm,
                    const int rankDestination = 0, const size_t extraSize = 0);

/**
 * Gets the displacements (offsets, start) for each
 * @param counts
 * @param countsSize
 * @return
 */
std::vector<int> GetGathervDisplacements(const size_t *counts,
                                         const size_t countsSize);

void CheckMPIReturn(const int value, const std::string hint);

std::string BroadcastFile(const std::string &fileName, MPI_Comm mpiComm,
                          const std::string hint = "",
                          const int rankSource = 0);

template <class T>
std::vector<MPI_Request> Isend64(const T *buffer, const size_t count,
                                 int destination, int tag, MPI_Comm mpiComm,
                                 const std::string &hint);

template <class T>
std::vector<MPI_Request> Irecv64(T *buffer, const size_t count, int source,
                                 int tag, MPI_Comm mpiComm,
                                 const std::string &hint);

} // end namespace helper
} // end namespace adios2

#include "adiosMPIFunctions.inl"

#endif /* ADIOS2_HELPER_ADIOMPIFUNCTIONS_H_ */
