/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosMpiHandshake.cpp
 */

#include "adiosMpiHandshake.h"
#include <iostream>

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
std::map<std::string, size_t> MpiHandshake::m_AppsForStreams;
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
                int appMasterRank =
                    reinterpret_cast<int *>(m_Buffer.data() + offset)[0];
                offset += sizeof(int);
                int appSize =
                    reinterpret_cast<int *>(m_Buffer.data() + offset)[0];
                offset += sizeof(int);
                std::string filename = m_Buffer.data() + offset;
                m_AppsSize[appMasterRank] = appSize;
                if (mode == 'w')
                {
                    auto &ranks = m_WritersMap[filename][appMasterRank];
                    bool existed = false;
                    for (const auto r : ranks)
                    {
                        if (r == rank)
                        {
                            existed = true;
                        }
                    }
                    if (not existed)
                    {
                        ranks.push_back(rank);
                    }
                }
                else if (mode == 'r')
                {
                    auto &ranks = m_ReadersMap[filename][appMasterRank];
                    bool existed = false;
                    for (const auto r : ranks)
                    {
                        if (r == rank)
                        {
                            existed = true;
                        }
                    }
                    if (not existed)
                    {
                        ranks.push_back(rank);
                    }
                }
            }
        }
    }
}

bool MpiHandshake::Check(const std::string &filename)
{
    Test();

    if (m_WritersMap[filename].size() + m_ReadersMap[filename].size() !=
        m_AppsForStreams[filename])
    {
        return false;
    }

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

void MpiHandshake::Start(const size_t maxStreamsPerApp,
                         const size_t maxFilenameLength,
                         const size_t appsForThisStream, const char mode,
                         const std::string &filename, MPI_Comm localComm)
{
    m_AppsForStreams[filename] = appsForThisStream;

    if (m_StreamID == 0)
    {
        MPI_Comm_size(MPI_COMM_WORLD, &m_WorldSize);
        MPI_Comm_rank(MPI_COMM_WORLD, &m_WorldRank);
        MPI_Comm_size(localComm, &m_LocalSize);
        MPI_Comm_rank(localComm, &m_LocalRank);
        m_MaxStreamsPerApp = maxStreamsPerApp;
        m_MaxFilenameLength = maxFilenameLength;
        m_ItemSize = maxFilenameLength + sizeof(char) + sizeof(int) * 2;

        if (m_LocalRank == 0)
        {
            m_LocalMasterRank = m_WorldRank;
        }

        MPI_Bcast(&m_LocalMasterRank, 1, MPI_INT, 0, localComm);

        m_SendRequests.resize(m_WorldSize);
        m_RecvRequests.resize(m_WorldSize);
        for (int i = 0; i < m_WorldSize; ++i)
        {
            m_SendRequests[i].resize(maxStreamsPerApp);
            m_RecvRequests[i].resize(maxStreamsPerApp);
        }

        size_t bufferSize = m_WorldSize * maxStreamsPerApp * m_ItemSize;
        m_Buffer.resize(bufferSize);

        for (int rank = 0; rank < m_WorldSize; ++rank)
        {
            for (size_t stream = 0; stream < maxStreamsPerApp; ++stream)
            {
                MPI_Irecv(m_Buffer.data() + PlaceInBuffer(stream, rank),
                          m_ItemSize, MPI_CHAR, rank, rank, MPI_COMM_WORLD,
                          &m_RecvRequests[rank][stream]);
            }
        }
    }

    if (filename.size() > maxFilenameLength)
    {
        throw(std::runtime_error("Filename too long"));
    }
    size_t offset = 0;
    std::vector<char> buffer(m_ItemSize);
    std::memcpy(buffer.data(), &mode, sizeof(char));
    offset += sizeof(char);
    std::memcpy(buffer.data() + offset, &m_LocalMasterRank, sizeof(int));
    offset += sizeof(int);
    std::memcpy(buffer.data() + offset, &m_LocalSize, sizeof(int));
    offset += sizeof(int);
    std::memcpy(buffer.data() + offset, filename.data(), filename.size());

    for (int i = 0; i < m_WorldSize; ++i)
    {
        MPI_Isend(buffer.data(), m_ItemSize, MPI_CHAR, i, m_WorldRank,
                  MPI_COMM_WORLD, &m_SendRequests[i][m_StreamID]);
    }

    ++m_StreamID;
}

void MpiHandshake::Wait(const std::string &filename)
{
    bool finished = false;
    while (not finished)
    {
        finished = Check(filename);
    }
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

void MpiHandshake::Finalize()
{
    --m_StreamID;
    if (m_StreamID == 0)
    {
        for (auto &rs : m_RecvRequests)
        {
            for (auto &r : rs)
            {
                MPI_Status status;
                int success;
                MPI_Test(&r, &success, &status);
                if (not success)
                {
                    MPI_Cancel(&r);
                }
            }
        }
        m_RecvRequests.clear();
        m_SendRequests.clear();
    }
}

void MpiHandshake::PrintMaps()
{
    for (int printRank = 0; printRank < m_WorldSize; ++printRank)
    {
        MPI_Barrier(MPI_COMM_WORLD);
        if (m_WorldRank == printRank)
        {
            std::cout << "For rank " << printRank << " ********************* "
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
