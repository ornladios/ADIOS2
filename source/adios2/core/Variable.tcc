/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Variable.tcc
 *
 *  Created on: May 1, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_VARIABLE_TCC_
#define ADIOS2_CORE_VARIABLE_TCC_

#include "Variable.h"

#include "adios2/ADIOSMacros.h"
#include "adios2/helper/adiosFunctions.h" //GetType<T>

namespace adios2
{

#define declare_type(T)                                                        \
                                                                               \
    template <>                                                                \
    Variable<T>::Variable(const std::string &name, const Dims &shape,          \
                          const Dims &start, const Dims &count,                \
                          const bool constantDims, const bool debugMode)       \
    : VariableBase(name, GetType<T>(), sizeof(T), shape, start, count,         \
                   constantDims, debugMode)                                    \
    {                                                                          \
    }                                                                          \
                                                                               \
    template <>                                                                \
    void Variable<T>::ApplyTransforms()                                        \
    {                                                                          \
    }

ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

} // end namespace adios2

#endif /* ADIOS2_CORE_VARIABLE_TCC_ */
