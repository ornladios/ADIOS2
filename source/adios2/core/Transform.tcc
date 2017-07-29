/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Transform.tcc : specialization of template functions
 *
 *  Created on: Jul 25, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_TRANSFORM_TCC_
#define ADIOS2_CORE_TRANSFORM_TCC_

#include "Transform.h"

#include "adios2/ADIOSMacros.h"
#include "adios2/helper/adiosFunctions.h"

namespace adios2
{

#define declare_type(T)                                                        \
    template <>                                                                \
    size_t Transform::BufferMaxSize<T>(                                        \
        const T *dataIn, const Dims &dimensions, const Params &parameters)     \
        const                                                                  \
    {                                                                          \
        return DoBufferMaxSize(dataIn, dimensions, GetType<T>(), parameters);  \
    }
ADIOS2_FOREACH_ZFP_TYPE_1ARG(declare_type)
#undef declare_type

} // end namespace adios2

#endif // ADIOS2_CORE_TRANSFORM_TCC_
