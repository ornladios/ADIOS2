/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * VariableCompound.cpp
 *
 *  Created on: May 16, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "VariableCompound.h"
#include "VariableCompound.tcc"

#include "adios2/common/ADIOSConfig.h"

namespace adios2
{
namespace core
{

VariableCompound::VariableCompound(const std::string &name,
                                   const size_t structSize, const Dims &shape,
                                   const Dims &start, const Dims &count,
                                   const bool constantDims,
                                   const bool debugMode)
: VariableBase(name, "compound", structSize, shape, start, count, constantDims,
               debugMode)
{
}

#define declare_template_instantiation(T)                                      \
    template void VariableCompound::InsertMember<T>(const std::string &,       \
                                                    const size_t);
ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace core
} // end namespace adios2
