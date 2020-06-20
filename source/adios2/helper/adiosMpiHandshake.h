/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosMpiHandshake.h
 *
 *  Created on: Mar 1, 2020
 *      Author: Jason Wang
 */

#ifndef ADIOS2_HELPER_ADIOSMPIHANDSHAKE_H_
#define ADIOS2_HELPER_ADIOSMPIHANDSHAKE_H_

#include "adios2/common/ADIOSConfig.h"
#ifndef ADIOS2_HAVE_MPI
#error "Do not include adiosMpiHandshake.h without ADIOS2_HAVE_MPI."
#endif

#include <map>
#include <mpi.h>
#include <string>
#include <vector>

namespace adios2
{
namespace helper
{

class MpiHandshake
{
public:
    /**
     * Start the handshake operations and wait until the rendezvous conditions
     * are reached, or timeout.
     *
     * @param filename: name of the staging stream, must be within the length of
     * maxFilenameLength
     *
     * @param mode: 'r' or 'w', read or write
     *
     * @param timeoutSeconds: timeout for the handshake, will throw exception
     * when reaching this timeout
     *
     * @param maxStreamsPerApp: the maximum number of streams that all apps
     * sharing this MPI_COMM_WORLD can possibly open. It is required that this
     * number is consistent across all ranks. This is used for pre-allocating
     * the vectors holding MPI requests and must be specified correctly,
     * otherwise strange errors could occur. This class does not provide any
     * mechanism to check whether this number being passed is actually correct
     * or not accross all ranks, because implementing this logic for an
     * arbitrary communication pattern is overly expensive, if not impossible.
     *
     * @param maxFilenameLength: the maximum possible length of filename that
     * all apps sharing this MPI_COMM_WORLD could possibly define. It is
     * required that this number is consistent across all ranks. This is used
     * for pre-allocating the buffer for aggregating the global MPI information.
     * An exception will be thrown if any filename on any rank is found to be
     * longer than this.
     *
     * @param rendezvousAppCountForStream: the number of apps, including both
     * writers and readers, that will work on this stream. The function will
     * block until it receives the MPI handshake information from all these
     * apps, or until timeoutSeconds is passed.
     *
     * @param localComm: local MPI communicator for the app
     */
    static void Handshake(const std::string &filename, const char mode,
                          const int timeoutSeconds,
                          const size_t maxStreamsPerApp,
                          const size_t maxFilenameLength,
                          const size_t rendezvousAppCountForStream,
                          MPI_Comm localComm);

    /**
     * Get the writer map of all apps participating the stream.
     *
     * @param filename: name of the staging stream
     *
     * @return map of all writer apps participating the stream. Key is the world
     * rank of the master rank of the local communicator of a participating
     * writer app. Value is a vector of all world ranks of this writer app
     */
    static const std::map<int, std::vector<int>> &
    GetWriterMap(const std::string &filename);

    /**
     * Get the reader map of all apps participating the stream.
     *
     * @param filename: name of the staging stream
     *
     * @return map of all reader apps participating the stream. Key is the world
     * rank of the master rank of the local communicator of a participating
     * reader app. Value is a vector of all world ranks of this reader app
     */
    static const std::map<int, std::vector<int>> &
    GetReaderMap(const std::string &filename);

private:
    static void Test();
    static bool Check(const std::string &filename, const bool verbose);
    static size_t PlaceInBuffer(const size_t stream, const int rank);
    static void PrintMaps();
    static void PrintMaps(const int printRank, const std::string &filename);

    static std::vector<char> m_Buffer;
    static std::vector<std::vector<MPI_Request>> m_SendRequests;
    static std::vector<std::vector<MPI_Request>> m_RecvRequests;
    static size_t m_MaxStreamsPerApp;
    static size_t m_MaxFilenameLength;
    static size_t m_ItemSize;
    static std::map<std::string, size_t> m_RendezvousAppCounts;
    static size_t m_StreamID;
    static int m_WorldSize;
    static int m_WorldRank;
    static int m_LocalSize;
    static int m_LocalRank;
    static int m_LocalMasterRank;
    static std::map<std::string, std::map<int, std::vector<int>>> m_WritersMap;
    static std::map<std::string, std::map<int, std::vector<int>>> m_ReadersMap;
    static std::map<int, int> m_AppsSize;
};

} // end namespace helper
} // end namespace adios2

#endif // ADIOS2_HELPER_ADIOSMPIHANDSHAKE_H_
