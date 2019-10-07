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

#include <vector>
#include <map>
#include "adios2/common/ADIOSTypes.h"
#include "nlohmann/json.hpp"

namespace adios2
{
namespace core
{
namespace engine
{
namespace ssc
{
    struct VarInfo
    {
        Dims shape;
        Dims start;
        Dims count;
        Dims overlapStart;
        Dims overlapCount;
        std::string type;
        size_t id;
        size_t posStart;
        size_t posCount;
    };
    using VarMap = std::map<std::string, VarInfo>;
    using VarMapVec = std::vector<VarMap>;

    void PrintDims(const Dims &dims, const std::string &label = std::string());
    void PrintVarMap(const VarMap& vm, const std::string &label = std::string());
    void PrintVarMapVec(const VarMapVec& vmv, const std::string &label = std::string());

    size_t GetTypeSize(const std::string &type);

    size_t TotalDataSize(const VarMap &vm);
    size_t TotalDataSize(const VarMapVec &vmv);

    size_t TotalOverlapSize(const VarMap &vm);
    size_t TotalOverlapSize(const VarMapVec &vmv);

    void CalculateOverlap(VarMapVec &mapVec, VarMap &singleMap);
    void CalculatePosition(VarMapVec &mapVec);
    void CalculatePosition(VarMapVec &writerMapVec, VarMapVec &readerMapVec, const int writerRank);

    VarMapVec JsonToVarMapVec(const nlohmann::json &input, const int size);
    VarMapVec JsonToVarMapVec(const std::vector<char> &input, const int size);
    VarMapVec JsonToVarMapVec(const std::string &input, const int size);

} // end namespace ssc
} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_SSCHELPER_H_
