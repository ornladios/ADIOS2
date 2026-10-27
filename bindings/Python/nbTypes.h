/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_BINDINGS_PYTHON_PY11TYPES_H_
#define ADIOS2_BINDINGS_PYTHON_PY11TYPES_H_

#include <adios2.h>

namespace adios2
{
namespace py11
{

#if ADIOS2_USE_MPI

/**
 * MPI4PY_Comm provides automatic conversion of Python mpi4py communicators to
 * the C++ MPI4PY_Comm type, which in itself implicitly converts to a MPI_Comm.
 *
 * The actual work is done by the caster in py11glue.cpp
 */
struct MPI4PY_Comm
{
    MPI_Comm comm;

    // allow implicit conversion to MPI_Comm
    operator MPI_Comm() { return comm; }
};

#endif

#define ADIOS2_FOREACH_PYTHON_TYPE_1ARG(MACRO) ADIOS2_FOREACH_STDTYPE_1ARG(MACRO)

#define ADIOS2_FOREACH_NUMPY_TYPE_1ARG(MACRO) ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(MACRO)

#define ADIOS2_FOREACH_NUMPY_ATTRIBUTE_TYPE_1ARG(MACRO) ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(MACRO)

/**
 * The list to walk when choosing a C++ type from a numpy dtype, as opposed to
 * the lists above, which choose a branch from an ADIOS DataType. numpy has no
 * "char" dtype; an array is always explicitly signed or unsigned. Since char
 * is signed on x86 and unsigned on aarch64 Linux, nb::dtype<char>() equals the
 * int8 dtype on one and the uint8 dtype on the other, so a char entry would
 * claim arrays of the wrong signedness. int8_t and uint8_t cover both byte
 * dtypes exactly, so char is left out here.
 */
#define ADIOS2_FOREACH_NUMPY_DTYPE_1ARG(MACRO)                                                     \
    MACRO(int8_t)                                                                                  \
    MACRO(int16_t)                                                                                 \
    MACRO(int32_t)                                                                                 \
    MACRO(int64_t)                                                                                 \
    MACRO(uint8_t)                                                                                 \
    MACRO(uint16_t)                                                                                \
    MACRO(uint32_t)                                                                                \
    MACRO(uint64_t)                                                                                \
    MACRO(float)                                                                                   \
    MACRO(double)                                                                                  \
    MACRO(long double)                                                                             \
    MACRO(std::complex<float>)                                                                     \
    MACRO(std::complex<double>)

} // end namespace py11
} // end namespace adios2

#endif
