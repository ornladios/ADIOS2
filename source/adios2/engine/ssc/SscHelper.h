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
        std::string type;
        size_t index;
    };
    using VarMap = std::map<std::string, VarInfo>;
    using VarMapVec = std::vector<VarMap>;

    void PrintDims(const Dims &dims, const std::string &label = std::string());
    void PrintVarMap(const VarMap& vm, const std::string &label = std::string());
    void PrintVarMapVec(const VarMapVec& vmv, const std::string &label = std::string());

} // end namespace ssc
} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_SSCHELPER_H_
