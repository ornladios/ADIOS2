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
#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{
namespace ssc
{

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
