/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SscHelper.h
 *
 *  Created on: Sep 30, 2019
 *      Author: Jason Wang
 */

#ifndef ADIOS2_ENGINE_SSCHELPER_H_
#define ADIOS2_ENGINE_SSCHELPER_H_

#include "adios2/common/ADIOSTypes.h"
#include "adios2/core/IO.h"
#include <mpi.h>
#include <unordered_map>
#include <vector>

namespace adios2
{
namespace core
{
namespace engine
{
namespace ssc
{

class Buffer
{
public:
    Buffer()
    {
        m_Buffer = malloc(1);
        m_Capacity = 1;
    }
    Buffer(size_t capacity)
    {
        m_Buffer = malloc(capacity);
        m_Capacity = capacity;
    }
    ~Buffer()
    {
        if (m_Buffer)
        {
            free(m_Buffer);
            m_Capacity = 0;
            m_Buffer = nullptr;
        }
    }
    void clear()
    {
        m_Buffer = realloc(m_Buffer, 1);
        m_Capacity = 1;
    }
    void reserve(size_t capacity)
    {
        m_Buffer = realloc(m_Buffer, capacity);
        m_Capacity = capacity;
    }
    template <typename T>
    T *data()
    {
        return reinterpret_cast<T *>(m_Buffer);
    }
    template <typename T>
    const T *data() const
    {
        return reinterpret_cast<const T *>(m_Buffer);
    }
    uint8_t *data() { return reinterpret_cast<uint8_t *>(m_Buffer); }
    const uint8_t *data() const
    {
        return reinterpret_cast<const uint8_t *>(m_Buffer);
    }
    size_t capacity() const { return m_Capacity; }
    uint8_t &operator[](const size_t i)
    {
        return *(reinterpret_cast<uint8_t *>(m_Buffer) + i);
    }
    const uint8_t &operator[](const size_t i) const
    {
        return *(reinterpret_cast<uint8_t *>(m_Buffer) + i);
    }

private:
    size_t m_Capacity = 0;
    void *m_Buffer = nullptr;
};

struct BlockInfo
{
    std::string name;
    DataType type;
    ShapeID shapeId;
    Dims shape;
    Dims start;
    Dims count;
    size_t bufferStart;
    size_t bufferCount;
    std::vector<char> value;
    void *data;
    bool performed;
};
using BlockVec = std::vector<BlockInfo>;
using BlockVecVec = std::vector<BlockVec>;
using RankPosMap = std::unordered_map<int, std::pair<size_t, size_t>>;
using MpiInfo = std::vector<std::vector<int>>;

void PrintDims(const Dims &dims, const std::string &label = std::string());
void PrintBlock(const BlockInfo &b, const std::string &label = std::string());
void PrintBlockVec(const BlockVec &bv,
                   const std::string &label = std::string());
void PrintBlockVecVec(const BlockVecVec &bvv,
                      const std::string &label = std::string());
void PrintRankPosMap(const RankPosMap &m,
                     const std::string &label = std::string());
void PrintMpiInfo(const MpiInfo &writersInfo, const MpiInfo &readersInfo);

size_t GetTypeSize(const std::string &type);

size_t TotalDataSize(const Dims &dims, DataType type, const ShapeID &shapeId);
size_t TotalDataSize(const BlockVec &bv);

RankPosMap CalculateOverlap(BlockVecVec &globalPattern,
                            const BlockVec &localPattern);

void SerializeVariables(const BlockVec &input, Buffer &output, const int rank);
void SerializeAttributes(IO &input, Buffer &output);
void Deserialize(const Buffer &input, BlockVecVec &output, IO &io,
                 const bool regVars, const bool regAttrs);
void AggregateMetadata(const Buffer &localBuffer, Buffer &globalBuffer,
                       MPI_Comm comm, const bool finalStep, const bool locked);
void BroadcastMetadata(Buffer &globalBuffer, const int root, MPI_Comm comm);

void MPI_Gatherv64OneSidedPush(
    const void *sendbuf, uint64_t sendcount, MPI_Datatype sendtype,
    void *recvbuf, const uint64_t *recvcounts, const uint64_t *displs,
    MPI_Datatype recvtype, int root, MPI_Comm comm,
    const int chunksize = std::numeric_limits<int>::max());
void MPI_Gatherv64OneSidedPull(
    const void *sendbuf, uint64_t sendcount, MPI_Datatype sendtype,
    void *recvbuf, const uint64_t *recvcounts, const uint64_t *displs,
    MPI_Datatype recvtype, int root, MPI_Comm comm,
    const int chunksize = std::numeric_limits<int>::max());
void MPI_Gatherv64(const void *sendbuf, uint64_t sendcount,
                   MPI_Datatype sendtype, void *recvbuf,
                   const uint64_t *recvcounts, const uint64_t *displs,
                   MPI_Datatype recvtype, int root, MPI_Comm comm,
                   const int chunksize = std::numeric_limits<int>::max());

bool AreSameDims(const Dims &a, const Dims &b);

} // end namespace ssc
} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_SSCHELPER_H_
