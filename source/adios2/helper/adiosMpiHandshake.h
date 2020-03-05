/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosMpiHandshake.h
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
    static void Start(const size_t maxStreamsPerApp,
                      const size_t maxFilenameLength,
                      const size_t appsForThisStream, const char mode,
                      const std::string &filename, MPI_Comm localComm);
    static void Wait(const std::string &filename);
    static void Finalize();
    static const std::map<int, std::vector<int>> &
    GetWriterMap(const std::string &filename);
    static const std::map<int, std::vector<int>> &
    GetReaderMap(const std::string &filename);
    static void PrintMaps();

private:
    static void Test();
    static bool Check(const std::string &filename);
    static size_t PlaceInBuffer(const size_t stream, const int rank);
    static std::vector<char> m_Buffer;
    static std::vector<std::vector<MPI_Request>> m_SendRequests;
    static std::vector<std::vector<MPI_Request>> m_RecvRequests;
    static size_t m_MaxStreamsPerApp;
    static size_t m_MaxFilenameLength;
    static size_t m_ItemSize;
    static std::map<std::string, size_t> m_AppsForStreams;
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
