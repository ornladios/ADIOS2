/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * py11types.h
 *
 *  Created on: Nov 7, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_PYTHON_PY11TYPES_H_
#define ADIOS2_BINDINGS_PYTHON_PY11TYPES_H_

#include <string>

namespace adios2
{
namespace py11
{

#define ADIOS2_FOREACH_PYTHON_TYPE_1ARG(MACRO)                                 \
    ADIOS2_FOREACH_STDTYPE_1ARG(MACRO)

#define ADIOS2_FOREACH_NUMPY_TYPE_1ARG(MACRO)                                  \
    ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(MACRO)

#define ADIOS2_FOREACH_NUMPY_ATTRIBUTE_TYPE_1ARG(MACRO)                        \
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(MACRO)

} // end namespace py11
} // end namespace adios2

#endif
