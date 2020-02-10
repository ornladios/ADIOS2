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
                    if (gBlock.start.size() != gBlock.count.size() || lBlock.start.size() != lBlock.count.size() || gBlock.start.size() != lBlock.start.size())
                    {
                        continue;
                    }
                    gBlock.overlapStart.resize(gBlock.start.size());
                    gBlock.overlapCount.resize(gBlock.count.size());
                    for (size_t i = 0; i < gBlock.start.size(); ++i)
                    {
                        if (gBlock.start[i] + gBlock.count[i] <= lBlock.start[i] or lBlock.start[i] + lBlock.count[i] <= gBlock.start[i])
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
                        if (gBlock.start[i] + gBlock.count[i] < lBlock.start[i] + lBlock.count[i])
                        {
                            gBlock.overlapCount[i] = gBlock.start[i] + gBlock.count[i] - gBlock.overlapStart[i];
                        }
                        else
                        {
                            gBlock.overlapCount[i] = lBlock.start[i] + lBlock.count[i] - gBlock.overlapStart[i];
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
            ret[rank].first = 0;
        }
        ++rank;
    }
    return ret;
}

void JsonToBlockVecVec(const nlohmann::json &input, BlockVecVec &output)
{
    for (int i = 0; i < output.size(); ++i)
    {
        auto &rankj = input[i];
        output[i].clear();
        for (const auto &j : rankj)
        {
            output[i].emplace_back();
            auto &b = output[i].back();
            b.name = j["N"].get<std::string>();
            b.type = j["T"].get<std::string>();
            b.start = j["O"].get<Dims>();
            b.count = j["C"].get<Dims>();
            b.shape = j["S"].get<Dims>();
            b.bufferStart = j["X"].get<size_t>();
            b.bufferCount = j["Y"].get<size_t>();
        }
    }
}

void JsonToBlockVecVec(const std::string &input, BlockVecVec &output)
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
    JsonToBlockVecVec(j, output);
}

void JsonToBlockVecVec(const std::vector<char> &input, BlockVecVec &output)
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
    JsonToBlockVecVec(j, output);
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

void PrintBlock(const BlockInfo &b, const std::string &label)
{
    std::cout << label << std::endl;
    std::cout << b.name << std::endl;
    std::cout << "    Type : " << b.type << std::endl;
    PrintDims(b.shape, "    Shape : ");
    PrintDims(b.start, "    Start : ");
    PrintDims(b.count, "    Count : ");
    PrintDims(b.overlapStart, "    Overlap Start : ");
    PrintDims(b.overlapCount, "    Overlap Count : ");
    std::cout << "    Position Start : " << b.bufferStart << std::endl;
    std::cout << "    Position Count : " << b.bufferCount << std::endl;
}

void PrintBlockVec(const BlockVec &bv, const std::string &label)
{
    std::cout << label << std::endl;
    for (const auto &i : bv)
    {
        std::cout << i.name << std::endl;
        std::cout << "    Type : " << i.type << std::endl;
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
    std::cout << label << std::endl;
    size_t rank = 0;
    for (const auto &bv : bvv)
    {
        std::cout << "Rank " << rank << std::endl;
        for (const auto &i : bv)
        {
            std::cout << "    " << i.name << std::endl;
            std::cout << "        Type : " << i.type << std::endl;
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

void PrintRankPosMap(const RankPosMap &m, const std::string &label)
{
    std::cout << label << std::endl;
    for (const auto &rank : m)
    {
        std::cout << "Rank = " << rank.first
                  << ", bufferStart = " << rank.second.first
                  << ", bufferCount = " << rank.second.second << std::endl;
    }
}

} // end namespace ssc
} // end namespace engine
} // end namespace core
} // end namespace adios2
