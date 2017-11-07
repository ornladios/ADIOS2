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

#include "adios2/ADIOSConfig.h"

namespace adios2
{

VariableCompound::VariableCompound(const std::string name,
                                   const size_t sizeOfStruct, const Dims shape,
                                   const Dims start, const Dims count,
                                   const bool constantDims,
                                   const bool debugMode)
: VariableBase(name, "compound", sizeOfStruct, shape, start, count,
               constantDims, debugMode)
{
}

#define declare_template_instantiation(T)                                      \
    template void VariableCompound::InsertMember<T>(const std::string,         \
                                                    const size_t);
ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace
