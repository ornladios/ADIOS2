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
#include "nlohmann/json.hpp"
#include <map>
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
    std::string type;
    Dims shape;
    Dims start;
    Dims count;
    Dims overlapStart;
    Dims overlapCount;
    size_t blockId;
    size_t bufferStart;
    size_t bufferCount;
};
using BlockVec = std::vector<BlockInfo>;
using BlockVecVec = std::vector<BlockVec>;
using RankPosMap = std::map<int, size_t>;

void PrintDims(const Dims &dims, const std::string &label = std::string());
void PrintBlockVec(const BlockVec &bv,
                   const std::string &label = std::string());
void PrintBlockVecVec(const BlockVecVec &bvv,
                      const std::string &label = std::string());

size_t GetTypeSize(const std::string &type);

size_t TotalDataSize(const Dims &dims, const std::string &type);
size_t TotalDataSize(const BlockVec &bv);

size_t TotalOverlapSize(const BlockVec &bv);
size_t TotalOverlapSize(const BlockVecVec &bvv);

void CalculateOverlap(BlockVecVec &globalPattern, BlockVec &localPattern);
void CalculatePosition(BlockVecVec &mapVec, RankPosMap &allOverlapRanks);
void CalculatePosition(BlockVecVec &writerMapVec, BlockVecVec &readerMapVec,
                       const int writerRank, RankPosMap &allOverlapRanks);

RankPosMap AllOverlapRanks(const BlockVecVec &mapVec);

BlockVecVec JsonToBlockVecVec(const nlohmann::json &input, const int size);
BlockVecVec JsonToBlockVecVec(const std::vector<char> &input, const int size);
BlockVecVec JsonToBlockVecVec(const std::string &input, const int size);

bool AreSameDims(const Dims &a, const Dims &b);

} // end namespace ssc
} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_SSCHELPER_H_
