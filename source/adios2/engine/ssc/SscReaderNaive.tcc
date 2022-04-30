/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SscReaderNaive.tcc
 *
 *  Created on: Mar 7, 2022
 *      Author: Jason Wang
 */

#ifndef ADIOS2_ENGINE_SSCREADERNAIVE_TCC_
#define ADIOS2_ENGINE_SSCREADERNAIVE_TCC_

#include "SscReaderNaive.h"
#include "adios2/helper/adiosMemory.h"

namespace adios2
{
namespace core
{
namespace engine
{
namespace ssc
{

template <typename T>
std::vector<typename Variable<T>::BPInfo>
SscReaderNaive::BlocksInfoCommon(const Variable<T> &variable,
                                 const size_t step) const
{
    std::vector<typename Variable<T>::BPInfo> ret;
    return ret;
}

}
}
}
}

#endif // ADIOS2_ENGINE_SSCREADERNAIVE_TCC_
