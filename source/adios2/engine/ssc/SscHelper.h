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
#include "nlohmann/json.hpp"
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

void BlockVecToJson(const BlockVec &input, nlohmann::json &output);
void AttributeMapToJson(IO &input, nlohmann::json &output);
void LocalJsonToGlobalJson(const std::vector<char> &input,
                           const size_t maxLocalSize, const int streamSize,
                           nlohmann::json &output);

void JsonToBlockVecVec(const nlohmann::json &input, BlockVecVec &output);
void JsonToBlockVecVec(const std::vector<char> &input, BlockVecVec &output);
void JsonToBlockVecVec(const std::string &input, BlockVecVec &output);

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
