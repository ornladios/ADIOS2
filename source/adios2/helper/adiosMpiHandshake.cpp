/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosMpiHandshake.cpp
 *
 *  Created on: Mar 1, 2020
 *      Author: Jason Wang
 */

#include "adiosMpiHandshake.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

namespace adios2
{
namespace helper
{

std::vector<char> MpiHandshake::m_Buffer;
std::vector<std::vector<MPI_Request>> MpiHandshake::m_SendRequests;
std::vector<std::vector<MPI_Request>> MpiHandshake::m_RecvRequests;
size_t MpiHandshake::m_MaxStreamsPerApp;
size_t MpiHandshake::m_MaxFilenameLength;
size_t MpiHandshake::m_ItemSize;
std::map<std::string, size_t> MpiHandshake::m_RendezvousAppCounts;
size_t MpiHandshake::m_StreamID = 0;
int MpiHandshake::m_WorldSize;
int MpiHandshake::m_WorldRank;
int MpiHandshake::m_LocalSize;
int MpiHandshake::m_LocalRank;
int MpiHandshake::m_LocalMasterRank;
std::map<std::string, std::map<int, std::vector<int>>>
    MpiHandshake::m_WritersMap;
std::map<std::string, std::map<int, std::vector<int>>>
    MpiHandshake::m_ReadersMap;
std::map<int, int> MpiHandshake::m_AppsSize;

size_t MpiHandshake::PlaceInBuffer(size_t stream, int rank)
{
    return rank * m_MaxStreamsPerApp * m_ItemSize + stream * m_ItemSize;
}

void MpiHandshake::Test()
{
    int success;
    MPI_Status status;

    for (int rank = 0; rank < m_WorldSize; ++rank)
    {
        for (size_t stream = 0; stream < m_MaxStreamsPerApp; ++stream)
        {
            MPI_Test(&m_RecvRequests[rank][stream], &success, &status);
            if (success)
            {
                size_t offset = PlaceInBuffer(stream, rank);
                char mode = m_Buffer[offset];
                offset += sizeof(char);
                int appMasterRank;
                std::memcpy(&appMasterRank, m_Buffer.data() + offset,
                            sizeof(appMasterRank));
                offset += sizeof(int);
                int appSize;
                std::memcpy(&appSize, m_Buffer.data() + offset,
                            sizeof(appSize));
                offset += sizeof(int);
                std::string filename = m_Buffer.data() + offset;
                m_AppsSize[appMasterRank] = appSize;
                if (mode == 'w')
                {
                    auto &ranks = m_WritersMap[filename][appMasterRank];
                    if (std::find(ranks.begin(), ranks.end(), rank) ==
                        ranks.end())
                    {
                        ranks.push_back(rank);
                    }
                }
                else if (mode == 'r')
                {
                    auto &ranks = m_ReadersMap[filename][appMasterRank];
                    if (std::find(ranks.begin(), ranks.end(), rank) ==
                        ranks.end())
                    {
                        ranks.push_back(rank);
                    }
                }
            }
        }
    }
}

bool MpiHandshake::Check(const std::string &filename, const bool verbose)
{
    Test();

    // check if RendezvousAppCount reached

    if (m_WritersMap[filename].size() + m_ReadersMap[filename].size() !=
        m_RendezvousAppCounts[filename])
    {
        if (verbose)
        {
            std::cout << "MpiHandshake Rank " << m_WorldRank << " Stream "
                      << filename << ": " << m_WritersMap[filename].size()
                      << " writers and " << m_ReadersMap[filename].size()
                      << " readers found out of "
                      << m_RendezvousAppCounts[filename]
                      << " total rendezvous apps" << std::endl;
        }
        return false;
    }

    // check if all ranks' info is received

    for (const auto &app : m_WritersMap[filename])
    {
        if (app.second.size() != m_AppsSize[app.first])
        {
            return false;
        }
    }

    for (const auto &app : m_ReadersMap[filename])
    {
        if (app.second.size() != m_AppsSize[app.first])
        {
            return false;
        }
    }

    return true;
}

void MpiHandshake::Handshake(const std::string &filename, const char mode,
                             const int timeoutSeconds,
                             const size_t maxStreamsPerApp,
                             const size_t maxFilenameLength,
                             const size_t rendezvousAppCountForStream,
                             MPI_Comm localComm)
{

    // initialize variables

    if (filename.size() > maxFilenameLength)
    {
        throw(std::runtime_error("Filename too long"));
    }

    MPI_Comm_size(MPI_COMM_WORLD, &m_WorldSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &m_WorldRank);
    MPI_Comm_size(localComm, &m_LocalSize);
    MPI_Comm_rank(localComm, &m_LocalRank);
    m_MaxStreamsPerApp = maxStreamsPerApp;
    m_MaxFilenameLength = maxFilenameLength;
    m_RendezvousAppCounts[filename] = rendezvousAppCountForStream;

    m_SendRequests.resize(m_WorldSize);
    m_RecvRequests.resize(m_WorldSize);
    for (int rank = 0; rank < m_WorldSize; ++rank)
    {
        m_SendRequests[rank].resize(maxStreamsPerApp);
        m_RecvRequests[rank].resize(maxStreamsPerApp);
    }

    m_ItemSize = maxFilenameLength + sizeof(char) + sizeof(int) * 2;
    m_Buffer.resize(m_WorldSize * maxStreamsPerApp * m_ItemSize);

    // broadcast local master rank's world rank to use as app ID

    if (m_LocalRank == 0)
    {
        m_LocalMasterRank = m_WorldRank;
    }
    MPI_Bcast(&m_LocalMasterRank, 1, MPI_INT, 0, localComm);

    // start receiving

    for (int rank = 0; rank < m_WorldSize; ++rank)
    {
        for (size_t stream = 0; stream < maxStreamsPerApp; ++stream)
        {
            MPI_Irecv(m_Buffer.data() + PlaceInBuffer(stream, rank),
                      static_cast<int>(m_ItemSize), MPI_CHAR, rank, rank,
                      MPI_COMM_WORLD, &m_RecvRequests[rank][stream]);
        }
    }

    // start sending

    size_t offset = 0;
    std::vector<char> buffer(m_ItemSize);
    std::memcpy(buffer.data(), &mode, sizeof(char));
    offset += sizeof(char);
    std::memcpy(buffer.data() + offset, &m_LocalMasterRank, sizeof(int));
    offset += sizeof(int);
    std::memcpy(buffer.data() + offset, &m_LocalSize, sizeof(int));
    offset += sizeof(int);
    std::memcpy(buffer.data() + offset, filename.data(), filename.size());

    for (int rank = 0; rank < m_WorldSize; ++rank)
    {
        MPI_Isend(buffer.data(), static_cast<int>(m_ItemSize), MPI_CHAR, rank,
                  m_WorldRank, MPI_COMM_WORLD,
                  &m_SendRequests[rank][m_StreamID]);
    }

    // wait and check if required RendezvousAppCount reached

    auto startTime = std::chrono::system_clock::now();
    while (!Check(filename, false))
    {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        auto nowTime = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
            nowTime - startTime);
        if (duration.count() > timeoutSeconds)
        {
            Check(filename, true);
            throw(std::runtime_error("Mpi handshake timeout on Rank" +
                                     std::to_string(m_WorldRank) +
                                     " for Stream " + filename));
        }
    }

    // clean up MPI requests

    for (auto &rs : m_RecvRequests)
    {
        for (auto &r : rs)
        {
            MPI_Status status;
            int success;
            MPI_Test(&r, &success, &status);
            if (!success)
            {
                MPI_Cancel(&r);
            }
        }
    }
    m_RecvRequests.clear();

    ++m_StreamID;
}

const std::map<int, std::vector<int>> &
MpiHandshake::GetWriterMap(const std::string &filename)
{
    return m_WritersMap[filename];
}
const std::map<int, std::vector<int>> &
MpiHandshake::GetReaderMap(const std::string &filename)
{
    return m_ReadersMap[filename];
}

void MpiHandshake::PrintMaps(const int printRank, const std::string &filename)
{
    if (m_WorldRank == printRank)
    {
        std::cout << "Printing MPI handshake map for Stream " << filename
                  << " from Rank " << printRank << std::endl;
        std::cout << "    Writers: " << std::endl;

        for (const auto &app : m_WritersMap[filename])
        {
            std::cout << "        App Master Rank " << app.first << std::endl;
            std::cout << "            ";
            for (const auto &rank : app.second)
            {
                std::cout << rank << ", ";
            }
            std::cout << std::endl;
        }

        std::cout << "    Readers: " << std::endl;
        for (const auto &app : m_ReadersMap[filename])
        {
            std::cout << "        App Master Rank " << app.first << std::endl;
            std::cout << "            ";
            for (const auto &rank : app.second)
            {
                std::cout << rank << ", ";
            }
            std::cout << std::endl;
        }
    }
}
void MpiHandshake::PrintMaps()
{
    for (int printRank = 0; printRank < m_WorldSize; ++printRank)
    {
        MPI_Barrier(MPI_COMM_WORLD);
        if (m_WorldRank == printRank)
        {
            std::cout << "For rank " << printRank
                      << "============================================"
                      << std::endl;
            std::cout << "Writers: " << std::endl;
            for (const auto &stream : m_WritersMap)
            {
                std::cout << "    Stream " << stream.first << std::endl;
                for (const auto &app : stream.second)
                {
                    std::cout << "        App Master Rank " << app.first
                              << std::endl;
                    std::cout << "            ";
                    for (const auto &rank : app.second)
                    {
                        std::cout << rank << ", ";
                    }
                    std::cout << std::endl;
                }
            }
            std::cout << "Readers: " << std::endl;
            for (const auto &stream : m_ReadersMap)
            {
                std::cout << "    Stream " << stream.first << std::endl;
                for (const auto &app : stream.second)
                {
                    std::cout << "        App Master Rank " << app.first
                              << std::endl;
                    std::cout << "            ";
                    for (const auto &rank : app.second)
                    {
                        std::cout << rank << ", ";
                    }
                    std::cout << std::endl;
                }
            }
        }
    }
}

} // end namespace helper
} // end namespace adios2
