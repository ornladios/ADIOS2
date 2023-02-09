/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#ifndef ADIOS2_HELPER_ADIOSCUDA_H_
#define ADIOS2_HELPER_ADIOSCUDA_H_

namespace adios2
{
namespace helper
{
/*
 * CUDA kernel for computing the min and max from a GPU buffer
 */
template <class T>
void GPUMinMax(const T *values, const size_t size, T &min, T &max);

/**
 * Wrapper around cudaMemcpy needed for isolating CUDA interface dependency
 */
void MemcpyGPUToBuffer(char *dst, const char *GPUbuffer, size_t byteCount);
void MemcpyBufferToGPU(char *GPUbuffer, const char *src, size_t byteCount);

bool IsGPUbuffer(const void *ptr);

} // helper
} // adios2

#endif /* ADIOS2_HELPER_ADIOSCUDA_H_ */
