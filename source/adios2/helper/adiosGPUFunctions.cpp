#include "adiosGPUFunctions.h"

#include "adios2/common/ADIOSMacros.h"

#ifdef ADIOS2_HAVE_CUDA
#include "adios2/helper/adiosCUDA.h"

namespace adios2
{
namespace helper
{

template <typename T>
void GPUMinMax(const T *values, const size_t size, T &min, T &max)
{
    CUDAMinMax(values, size, min, max);
}

void MemcpyGPUToBuffer(char *dst, const char *GPUbuffer, size_t byteCount)
{
    CUDAMemcpyGPUToBuffer(dst, GPUbuffer, byteCount);
}

void MemcpyBufferToGPU(char *GPUbuffer, const char *src, size_t byteCount)
{
    CUDAMemcpyBufferToGPU(GPUbuffer, src, byteCount);
}

bool IsGPUBuffer(const void *ptr) { return CUDAIsGPUBuffer(ptr); }

#define declare_type(T)                                                        \
    template void adios2::helper::GPUMinMax(                                   \
        const T *values, const size_t size, T &min, T &max);
ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_type)
#undef declare_type

} // helper
} // adios2

#elif defined(ADIOS2_HAVE_KOKKOS)

#include "adios2/helper/adiosKokkos.h"

namespace adios2
{
namespace helper
{

template <typename T>
void GPUMinMax(const T *values, const size_t size, T &min, T &max)
{
    KokkosMinMax(values, size, min, max);
}

void MemcpyGPUToBuffer(char *dst, const char *GPUbuffer, size_t byteCount)
{
    KokkosMemcpyGPUToBuffer(dst, GPUbuffer, byteCount);
}

void MemcpyBufferToGPU(char *GPUbuffer, const char *src, size_t byteCount)
{
    KokkosMemcpyBufferToGPU(GPUbuffer, src, byteCount);
}

bool IsGPUBuffer(const void *ptr) { return KokkosIsGPUBuffer(ptr); }

#define declare_type(T)                                                        \
    template void adios2::helper::GPUMinMax(                                   \
        const T *values, const size_t size, T &min, T &max);
ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_type)
#undef declare_type

} // helper
} // adios2

#endif
