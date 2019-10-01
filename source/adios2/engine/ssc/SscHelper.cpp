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
            s += std::accumulate(v.second.count.begin(), v.second.count.end(), GetTypeSize(v.second.type), std::multiplies<size_t>());
        }
        return s;
    }

    Dims BufferPointers(const VarMapVec &vmv)
    {
        Dims pointers;
        size_t s = 0;
        for(const auto &vm : vmv)
        {
            pointers.push_back(s);
            s += TotalDataSize(vm);
        }
        return pointers;
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
            std::cout << "    Index : " << i.second.index << std::endl;
            PrintDims(i.second.shape, "    Shape : ");
            PrintDims(i.second.start, "    Start : ");
            PrintDims(i.second.count, "    Count : ");
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
                std::cout << "        Index : " << i.second.index << std::endl;
                PrintDims(i.second.shape, "        Shape : ");
                PrintDims(i.second.start, "        Start : ");
                PrintDims(i.second.count, "        Count : ");
            }
            ++rank;
        }
    }

} // end namespace ssc
} // end namespace engine
} // end namespace core
} // end namespace adios2
