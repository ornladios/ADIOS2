/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOSMacros.h : a set of helper macros used by the CXX binding
 *
 */

#ifndef ADIOS2_ADIOSMACROS_CXX
#define ADIOS2_ADIOSMACROS_CXX

#ifdef ADIOS2_HAVE_KOKKOS
#include <Kokkos_Core.hpp>

#define ADIOS2_KOKKOS_MEMORY_SPACE(MACRO, MEMORY_SPACE)                        \
    MACRO(int32_t, MEMORY_SPACE)                                               \
    MACRO(uint32_t, MEMORY_SPACE)                                              \
    MACRO(int64_t, MEMORY_SPACE)                                               \
    MACRO(uint64_t, MEMORY_SPACE)                                              \
    MACRO(float, MEMORY_SPACE)                                                 \
    MACRO(double, MEMORY_SPACE)

#ifdef KOKKOS_ENABLE_CUDA
#define ADIOS2_FOREACH_KOKKOS_TYPE_2ARGS(MACRO)                                \
    ADIOS2_KOKKOS_MEMORY_SPACE(MACRO, Kokkos::HostSpace)                       \
    ADIOS2_KOKKOS_MEMORY_SPACE(MACRO, Kokkos::CudaSpace)                       \
    ADIOS2_KOKKOS_MEMORY_SPACE(MACRO, Kokkos::CudaHostPinnedSpace)             \
    ADIOS2_KOKKOS_MEMORY_SPACE(MACRO, Kokkos::CudaUVMSpace)

#else
#define ADIOS2_FOREACH_KOKKOS_TYPE_2ARGS(MACRO)                                \
    ADIOS2_KOKKOS_MEMORY_SPACE(MACRO, Kokkos::HostSpace)
#endif /* KOKKOS_ENABLE_CUDA */

#endif /* ADIOS2_HAVE_KOKKOS */

#endif /* ADIOS2_ADIOSMACROS_CXX */
