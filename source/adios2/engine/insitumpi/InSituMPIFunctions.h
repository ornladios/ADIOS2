/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * InSituMPIFunctions.h
 * common functions for in situ MPI Writer and Reader
 * It requires MPI
 *
 *  Created on: Dec 18, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#ifndef ADIOS2_ENGINE_INSITUMPIFUNCTIONS_H_
#define ADIOS2_ENGINE_INSITUMPIFUNCTIONS_H_

#include <mpi.h>

#include <string>
#include <vector>

namespace adios2
{

namespace insitumpi
{

// Message tags to distinguish from tags used in the application
// BaseTag and LastTag is intended for checking tag values only, not
// for tagging messages
enum MpiTags
{
    BaseTag = 27950,
    Connect,
    Step,
    FixedRemoteSchedule,
    MetadataLength,
    Metadata,
    NumReaderPerWriter,
    ReadScheduleLength,
    ReadSchedule,
    Data,
    ReadCompleted,
    LastTag
};

// Generate the list of the other processes in MPI_COMM_WORLD who are
// the partner in Write--Read. comm is 'our' communicator.
// name is the output/input name the Open() was called with.
// This is collective & blocking function and must be called both
// on the Writer and Reader side at the same time.
std::vector<int> FindPeers(const MPI_Comm comm, const std::string &name,
                           const bool amIWriter, const MPI_Comm commWorld);

// Select the peers that I need to communicate directly.
// There is no communication in this function.
std::vector<int> AssignPeers(const int rank, const int nproc,
                             const std::vector<int> &allPeers);

// This is collective & blocking function and must be called both
// on the Writer and Reader side at the same time.
// IAmSender is true on the writers, false on the readers.
// IAmWriterRoot is true only on one writer who will send the global metadata.
// return:
//    on Reader: global rank of writer root on the reader who is connected
//       to writer root, -1 everywhere else.
//    on Writer: 1 on a writer who is the first rank connected to a particular
//       reader, 0 everywhere else (i.e. one Writer selected for each Reader)
int ConnectDirectPeers(const MPI_Comm commWorld, const bool IAmSender,
                       const bool IAmWriterRoot, const int globalRank,
                       const std::vector<int> &peers);

// Wait for multiple MPI requests to complete and check errors
std::vector<MPI_Status> CompleteRequests(std::vector<MPI_Request> &requests,
                                         const bool IAmWriter,
                                         const int localRank);

} // end namespace insitumpi

} // end namespace adios2

#endif /* ADIOS2_ENGINE_INSITUMPIFUNCTIONS_H_ */
