/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SscHelper.cpp
 *
 *  Created on: Sep 30, 2019
 *      Author: Jason Wang
 */

#include "SscHelper.h"
#include "adios2/common/ADIOSMacros.h"
#include "adios2/helper/adiosType.h"
#include <iostream>
#include <numeric>

namespace adios2
{
namespace core
{
namespace engine
{
namespace ssc
{

size_t GetTypeSize(const std::string &type)
{
    if (type.empty())
    {
        throw(std::runtime_error("unknown data type"));
    }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>()) { return sizeof(T); }
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
    else { throw(std::runtime_error("unknown data type")); }
}

size_t TotalDataSize(const Dims &dims, const std::string &type)
{
    return std::accumulate(dims.begin(), dims.end(), GetTypeSize(type),
                           std::multiplies<size_t>());
}

size_t TotalDataSize(const BlockVec &bv)
{
    size_t s = 0;
    for (const auto &b : bv)
    {
        s += TotalDataSize(b.count, b.type);
    }
    return s;
}

size_t TotalOverlapSize(const BlockVec &bv)
{
    size_t s = 0;
    for (const auto &b : bv)
    {
        if (not b.overlapCount.empty())
        {
            s +=
                std::accumulate(b.overlapCount.begin(), b.overlapCount.end(),
                                GetTypeSize(b.type), std::multiplies<size_t>());
        }
    }
    return s;
}

size_t TotalOverlapSize(const BlockVecVec &bvv)
{
    size_t s = 0;
    for (const auto &bv : bvv)
    {
        s += TotalOverlapSize(bv);
    }
    return s;
}

void CalculateOverlap(BlockVecVec &globalVecVec, BlockVec &localVec)
{
    for (auto &rankBlockVec : globalVecVec)
    {
        for (auto &gBlock : rankBlockVec)
        {
            for (auto &lBlock : localVec)
            {
                if (lBlock.name == gBlock.name)
                {
                    if (gBlock.start.size() != gBlock.count.size() ||
                        lBlock.start.size() != lBlock.count.size() ||
                        gBlock.start.size() != lBlock.start.size())
                    {
                        continue;
                    }
                    gBlock.overlapStart.resize(gBlock.start.size());
                    gBlock.overlapCount.resize(gBlock.count.size());
                    for (size_t i = 0; i < gBlock.start.size(); ++i)
                    {
                        if (gBlock.start[i] + gBlock.count[i] <=
                                lBlock.start[i] or
                            lBlock.start[i] + lBlock.count[i] <=
                                gBlock.start[i])
                        {
                            gBlock.overlapStart.clear();
                            gBlock.overlapCount.clear();
                            break;
                        }
                        if (gBlock.start[i] < lBlock.start[i])
                        {
                            gBlock.overlapStart[i] = lBlock.start[i];
                        }
                        else
                        {
                            gBlock.overlapStart[i] = gBlock.start[i];
                        }
                        if (gBlock.start[i] + gBlock.count[i] <
                            lBlock.start[i] + lBlock.count[i])
                        {
                            gBlock.overlapCount[i] = gBlock.start[i] +
                                                     gBlock.count[i] -
                                                     gBlock.overlapStart[i];
                        }
                        else
                        {
                            gBlock.overlapCount[i] = lBlock.start[i] +
                                                     lBlock.count[i] -
                                                     gBlock.overlapStart[i];
                        }
                    }
                }
            }
        }
    }
}

RankPosMap AllOverlapRanks(const BlockVecVec &bvv)
{
    RankPosMap ret;
    int rank = 0;
    for (const auto &bv : bvv)
    {
        bool hasOverlap = false;
        for (const auto &b : bv)
        {
            if (not b.overlapCount.empty())
            {
                hasOverlap = true;
            }
        }
        if (hasOverlap)
        {
            ret[rank] = 0;
        }
        ++rank;
    }
    return ret;
}

void CalculatePosition(BlockVecVec &bvv, RankPosMap &allRanks)
{
    int rank = 0;
    size_t pos = 0;
    for (auto &bv : bvv)
    {
        bool hasOverlap = false;
        for (const auto r : allRanks)
        {
            if (r.first == rank)
            {
                hasOverlap = true;
                break;
            }
        }
        if (hasOverlap)
        {
            allRanks[rank] = pos;
            for (auto &b : bv)
            {
                b.bufferStart = pos;
                b.bufferCount = 0;
                if (not b.count.empty())
                {
                    b.bufferCount += std::accumulate(
                        b.count.begin(), b.count.end(), GetTypeSize(b.type),
                        std::multiplies<size_t>());
                    pos += b.bufferCount;
                }
            }
        }
        ++rank;
    }
}

void CalculatePosition(BlockVecVec &writerMapVec, BlockVecVec &readerMapVec,
                       const int writerRank, RankPosMap &allOverlapRanks)
{
    for (auto &rank : allOverlapRanks)
    {
        auto &readerRankMap = readerMapVec[rank.first];
        CalculateOverlap(writerMapVec, readerRankMap);
        auto currentReaderOverlapWriterRanks = AllOverlapRanks(writerMapVec);
        CalculatePosition(writerMapVec, currentReaderOverlapWriterRanks);
        allOverlapRanks[rank.first] =
            currentReaderOverlapWriterRanks[writerRank];
    }
}

BlockVecVec JsonToBlockVecVec(const nlohmann::json &input, const int size)
{
    BlockVecVec output(size);
    for (int i = 0; i < size; ++i)
    {
        auto &rankj = input[i];
        int varIndex = 0;
        for (auto it = rankj.begin(); it != rankj.end(); ++it)
        {
            output[i].emplace_back();
            auto &b = output[i].back();
            b.name = it.key();
            b.type = it.value()["T"].get<std::string>();
            b.start = it.value()["O"].get<Dims>();
            b.count = it.value()["C"].get<Dims>();
            b.shape = it.value()["S"].get<Dims>();
            b.blockId = varIndex;
            ++varIndex;
        }
    }
    return output;
}

BlockVecVec JsonToBlockVecVec(const std::string &input, const int size)
{
    nlohmann::json j;
    try
    {
        j = nlohmann::json::parse(input);
    }
    catch (...)
    {
        throw(std::runtime_error("corrupted json string"));
    }
    return JsonToBlockVecVec(j, size);
}

BlockVecVec JsonToBlockVecVec(const std::vector<char> &input, const int size)
{
    nlohmann::json j;
    try
    {
        j = nlohmann::json::parse(input);
    }
    catch (...)
    {
        throw(std::runtime_error("corrupted json char vector"));
    }
    return JsonToBlockVecVec(j, size);
}

bool AreSameDims(const Dims &a, const Dims &b)
{
    if (a.size() != b.size())
    {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i)
    {
        if (a[i] != b[i])
        {
            return false;
        }
    }
    return true;
}

void PrintDims(const Dims &dims, const std::string &label)
{
    std::cout << label;
    for (const auto &i : dims)
    {
        std::cout << i << ", ";
    }
    std::cout << std::endl;
}

void PrintBlockVec(const BlockVec &bv, const std::string &label)
{
    std::cout << label;
    for (const auto &i : bv)
    {
        std::cout << i.name << std::endl;
        std::cout << "    Type : " << i.type << std::endl;
        std::cout << "    ID : " << i.blockId << std::endl;
        PrintDims(i.shape, "    Shape : ");
        PrintDims(i.start, "    Start : ");
        PrintDims(i.count, "    Count : ");
        PrintDims(i.overlapStart, "    Overlap Start : ");
        PrintDims(i.overlapCount, "    Overlap Count : ");
        std::cout << "    Position Start : " << i.bufferStart << std::endl;
        std::cout << "    Position Count : " << i.bufferCount << std::endl;
    }
}

void PrintBlockVecVec(const BlockVecVec &bvv, const std::string &label)
{
    std::cout << label;
    size_t rank = 0;
    for (const auto &bv : bvv)
    {
        std::cout << "Rank " << rank << std::endl;
        for (const auto &i : bv)
        {
            std::cout << "    " << i.name << std::endl;
            std::cout << "        Type : " << i.type << std::endl;
            std::cout << "        ID : " << i.blockId << std::endl;
            PrintDims(i.shape, "        Shape : ");
            PrintDims(i.start, "        Start : ");
            PrintDims(i.count, "        Count : ");
            PrintDims(i.overlapStart, "        Overlap Start : ");
            PrintDims(i.overlapCount, "        Overlap Count : ");
            std::cout << "        Position Start : " << i.bufferStart
                      << std::endl;
            std::cout << "        Position Count : " << i.bufferCount
                      << std::endl;
        }
        ++rank;
    }
}

} // end namespace ssc
} // end namespace engine
} // end namespace core
} // end namespace adios2
