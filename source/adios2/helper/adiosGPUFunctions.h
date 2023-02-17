#ifndef ADIOS2_HELPER_ADIOSGPUFUNCTIONS_H_
#define ADIOS2_HELPER_ADIOSGPUFUNCTIONS_H_

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
ADIOS2_EXPORT void GPUMinMax(const T *values, const size_t size, T &min,
                             T &max);

/**
 * Wrapper around cudaMemcpy needed for isolating CUDA interface dependency
 */
ADIOS2_EXPORT
void MemcpyGPUToBuffer(char *dst, const char *GPUbuffer, size_t byteCount);
ADIOS2_EXPORT
void MemcpyBufferToGPU(char *GPUbuffer, const char *src, size_t byteCount);

ADIOS2_EXPORT
bool IsGPUBuffer(const void *ptr);

} // helper
} // adios2

#endif /* ADIOS2_HELPER_ADIOSGPUFUNCTIONS_H_ */
