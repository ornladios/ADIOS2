/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#ifndef ADIOS2_HELPER_ADIOSKOKKOS_H_
#define ADIOS2_HELPER_ADIOSKOKKOS_H_

#include <cstddef>

namespace adios2
{
namespace helper
{

#ifdef _WIN32
#define ADIOS2_EXPORT __declspec(dllexport)
#else
#define ADIOS2_EXPORT __attribute__((visibility("default")))
#endif

/*
 * CUDA kernel for computing the min and max from a GPU buffer
 */
template <class T>
ADIOS2_EXPORT void KokkosMinMax(const T *values, const size_t size, T &min,
                                T &max);

/**
 * Wrapper around cudaMemcpy needed for isolating CUDA interface dependency
 */
ADIOS2_EXPORT
void KokkosMemcpyGPUToBuffer(char *dst, const char *GPUbuffer,
                             size_t byteCount);
ADIOS2_EXPORT
void KokkosMemcpyBufferToGPU(char *GPUbuffer, const char *src,
                             size_t byteCount);
ADIOS2_EXPORT
bool KokkosIsGPUBuffer(const void *ptr);

ADIOS2_EXPORT
void KokkosFinalize();
ADIOS2_EXPORT
void KokkosInit();
ADIOS2_EXPORT
bool KokkosIsInitialized();

} // helper
} // adios2

#endif /* ADIOS2_HELPER_ADIOSKOKKOS_H_ */
