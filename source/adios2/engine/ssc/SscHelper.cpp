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
        if(type.empty())
        {
            throw(std::runtime_error("unknown data type"));
        }
#define declare_type(T)                                                        \
        else if (type == helper::GetType<T>())                                   \
        {                                                                          \
            return sizeof(T);\
        }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
        else {
            throw(std::runtime_error("unknown data type"));
        }
    }

    size_t TotalDataSize(const VarMapVec &vmv)
    {
        size_t s = 0;
        for(const auto &vm : vmv)
        {
            s += TotalDataSize(vm);
        }
        return s;
    }

    size_t TotalDataSize(const VarMap &vm)
    {
        size_t s = 0;
        for(const auto &v : vm)
        {
            if(not v.second.count.empty())
            {
                s += std::accumulate(v.second.count.begin(), v.second.count.end(), GetTypeSize(v.second.type), std::multiplies<size_t>());
            }
        }
        return s;
    }

    size_t TotalOverlapSize(const VarMapVec &vmv)
    {
        size_t s = 0;
        for(const auto &vm : vmv)
        {
            s += TotalOverlapSize(vm);
        }
        return s;
    }

    size_t TotalOverlapSize(const VarMap &vm)
    {
        size_t s = 0;
        for(const auto &v : vm)
        {
            if(not v.second.overlapCount.empty())
            {
                s += std::accumulate(v.second.overlapCount.begin(), v.second.overlapCount.end(), GetTypeSize(v.second.type), std::multiplies<size_t>());
            }
        }
        return s;
    }

    void CalculateOverlap(VarMapVec &mapVec, VarMap &singleMap)
    {
        for(auto &rankMap : mapVec)
        {
            for(auto &varPair : rankMap)
            {
                auto &ref1 = varPair.second;
                auto &ref2 = singleMap[varPair.first];
                if (ref1.start.size() != ref1.count.size() || ref2.start.size() != ref2.count.size() || ref1.start.size() != ref2.start.size())
                {
                    continue;
                }
                ref1.overlapStart.resize(ref1.start.size());
                ref1.overlapCount.resize(ref1.count.size());
                for (size_t i = 0; i < ref1.start.size(); ++i)
                {
                    if (ref1.start[i] + ref1.count[i] < ref2.start[i] or ref2.start[i] + ref2.count[i] < ref1.start[i])
                    {
                        ref1.overlapStart.clear();
                        ref1.overlapCount.clear();
                        break;
                    }
                    if (ref1.start[i] < ref2.start[i])
                    {
                        ref1.overlapStart[i] = ref2.start[i];
                    }
                    else
                    {
                        ref1.overlapStart[i] = ref1.start[i];
                    }
                    if (ref1.start[i] + ref1.count[i] < ref2.start[i] + ref2.count[i])
                    {
                        ref1.overlapCount[i] = ref1.start[i] + ref1.count[i] - ref1.overlapStart[i];
                    }
                    else
                    {
                        ref1.overlapCount[i] = ref2.start[i] + ref2.count[i] - ref1.overlapStart[i];
                    }
                }
            }
        }
    }

    void CalculatePosition(VarMapVec &mapVec)
    {
        size_t sumVec = 0;
        size_t tmp = 0;
        for(auto &rankMap : mapVec)
        {
            for(auto &varPair : rankMap)
            {
                auto &ref1 = varPair.second;
                ref1.posStart = sumVec;
                ref1.posCount = 0;
                if(not ref1.overlapCount.empty())
                {
                    ref1.posCount += std::accumulate(ref1.overlapCount.begin(), ref1.overlapCount.end(), GetTypeSize(ref1.type), std::multiplies<size_t>());
                    sumVec += ref1.posCount;
                }
            }
            ++tmp;
        }
    }

    void CalculatePosition(VarMapVec &writerMapVec, VarMapVec &readerMapVec, const int writerRank)
    {
        for(auto &readerRankMap : readerMapVec)
        {
            CalculateOverlap(writerMapVec, readerRankMap);
            CalculatePosition(writerMapVec);
            auto &writerRef = writerMapVec[writerRank];
            for(auto &writerVar : writerRef)
            {
                readerRankMap[writerVar.first].overlapStart = writerVar.second.overlapStart;
                readerRankMap[writerVar.first].overlapCount = writerVar.second.overlapCount;
                readerRankMap[writerVar.first].posStart = writerVar.second.posStart;
                readerRankMap[writerVar.first].posCount = writerVar.second.posCount;
            }
        }
    }

    VarMapVec JsonToVarMapVec(const nlohmann::json &input, const int size)
    {
        VarMapVec output(size);
        for(int i = 0; i<size; ++i)
        {
            auto &rankj = input[i];
            int varIndex = 0;
            for(auto it = rankj.begin(); it!=rankj.end(); ++it)
            {
                auto &v = output[i][it.key()];
                v.type = it.value()["T"].get<std::string>();
                v.start = it.value()["O"].get<Dims>();
                v.count = it.value()["C"].get<Dims>();
                v.shape = it.value()["S"].get<Dims>();
                v.id = varIndex;
                ++varIndex;
            }
        }
        return output;
    }

    VarMapVec JsonToVarMapVec(const std::string &input, const int size)
    {
        nlohmann::json j;
        try{
            j = nlohmann::json::parse(input);
        }
        catch(...)
        {
            throw(std::runtime_error("corrupted json string"));
        }
        return JsonToVarMapVec(j, size);
    }

    VarMapVec JsonToVarMapVec(const std::vector<char> &input, const int size)
    {
        nlohmann::json j;
        try{
            j = nlohmann::json::parse(input);
        }
        catch(...)
        {
            throw(std::runtime_error("corrupted json char vector"));
        }
        return JsonToVarMapVec(j, size);
    }

    void PrintDims(const Dims &dims, const std::string &label)
    {
        std::cout << label;
        for(const auto &i : dims)
        {
            std::cout << i << ", ";
        }
        std::cout << std::endl;
    }

    void PrintVarMap(const VarMap& vm, const std::string &label)
    {
        std::cout << label;
        for(const auto &i : vm)
        {
            std::cout << i.first << std::endl;
            std::cout << "    Type : " << i.second.type << std::endl;
            std::cout << "    ID : " << i.second.id << std::endl;
            PrintDims(i.second.shape, "    Shape : ");
            PrintDims(i.second.start, "    Start : ");
            PrintDims(i.second.count, "    Count : ");
            PrintDims(i.second.overlapStart, "    Overlap Start : ");
            PrintDims(i.second.overlapCount, "    Overlap Count : ");
        }
    }

    void PrintVarMapVec(const VarMapVec& vmv, const std::string &label)
    {
        std::cout << label;
        size_t rank=0;
        for(const auto &vm : vmv)
        {
            std::cout << "Rank " << rank << std::endl;
            for(const auto &i : vm)
            {
                std::cout << "    " << i.first << std::endl;
                std::cout << "        Type : " << i.second.type << std::endl;
                std::cout << "        ID : " << i.second.id << std::endl;
                PrintDims(i.second.shape, "        Shape : ");
                PrintDims(i.second.start, "        Start : ");
                PrintDims(i.second.count, "        Count : ");
                PrintDims(i.second.overlapStart, "        Overlap Start : ");
                PrintDims(i.second.overlapCount, "        Overlap Count : ");
                std::cout << "        Position Start : " << i.second.posStart << std::endl;
                std::cout << "        Position Count : " << i.second.posCount << std::endl;
            }
            ++rank;
        }
    }

} // end namespace ssc
} // end namespace engine
} // end namespace core
} // end namespace adios2
