/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#ifndef ADIOS2_HELPER_ADIOSKokkos_CPP_
#define ADIOS2_HELPER_ADIOSKokkos_CPP_

#include "adiosKokkos.h"
#include "adios2/common/ADIOSMacros.h"

#include <Kokkos_Core.hpp>

namespace
{
template <class MemSpace>
void KokkosDeepCopy(const char *src, char *dst, size_t byteCount)
{
    Kokkos::View<const char *, MemSpace,
                 Kokkos::MemoryTraits<Kokkos::Unmanaged>>
        srcView(src, byteCount);
    Kokkos::View<char *, Kokkos::HostSpace,
                 Kokkos::MemoryTraits<Kokkos::Unmanaged>>
        dstView(dst, byteCount);
    Kokkos::deep_copy(dstView, srcView);
}

template <class T>
void KokkosMinMaxImpl(const T *data, const size_t size, T &min, T &max)
{
    Kokkos::parallel_reduce(size,
                            KOKKOS_LAMBDA(int i, T &lmax, T &lmin) {
                                if (lmax < data[i])
                                    lmax = data[i];
                                if (lmin > data[i])
                                    lmin = data[i];
                            },
                            Kokkos::Max<T>(max), Kokkos::Min<T>(min));
}

// types non supported on the device
void KokkosMinMaxImpl(const char * /*values*/, const size_t /*size*/,
                      char & /*min*/, char & /*max*/)
{
}
void KokkosMinMaxImpl(const std::complex<float> * /*values*/,
                      const size_t /*size*/, std::complex<float> & /*min*/,
                      std::complex<float> & /*max*/)
{
}
void KokkosMinMaxImpl(const std::complex<double> * /*values*/,
                      const size_t /*size*/, std::complex<double> & /*min*/,
                      std::complex<double> & /*max*/)
{
}

}

namespace adios2
{
namespace helper
{
void KokkosMemcpyGPUToBuffer(char *dst, const char *GPUbuffer, size_t byteCount)
{
#ifdef KOKKOS_ENABLE_CUDA
    KokkosDeepCopy<Kokkos::CudaSpace>(GPUbuffer, dst, byteCount);
#endif
#ifdef KOKKOS_ENABLE_HIP
    KokkosDeepCopy<Kokkos::Experimental::HIPSpace>(GPUbuffer, dst, byteCount);
#endif
}

void KokkosMemcpyBufferToGPU(char *GPUbuffer, const char *src, size_t byteCount)
{
#ifdef KOKKOS_ENABLE_CUDA
    KokkosDeepCopy<Kokkos::CudaSpace>(src, GPUbuffer, byteCount);
#endif
#ifdef KOKKOS_ENABLE_HIP
    KokkosDeepCopy<Kokkos::Experimental::HIPSpace>(src, GPUbuffer, byteCount);
#endif
}

bool KokkosIsGPUBuffer(const void *ptr)
{
#ifdef KOKKOS_ENABLE_CUDA
    cudaPointerAttributes attr;
    cudaPointerGetAttributes(&attr, ptr);
    if (attr.type == cudaMemoryTypeDevice)
    {
        return true;
    }
#endif
    return false;
}

void KokkosFinalize() { Kokkos::finalize(); }

void KokkosInit() { Kokkos::initialize(); }

bool KokkosIsInitialized() { return Kokkos::is_initialized(); }

template <class T>
void KokkosMinMax(const T *values, const size_t size, T &min, T &max)
{
    KokkosMinMaxImpl(values, size, min, max);
}

}
}

#define declare_type(T)                                                        \
    template void adios2::helper::KokkosMinMax(                                \
        const T *values, const size_t size, T &min, T &max);
ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_type)
#undef declare_type

#endif /* ADIOS2_HELPER_ADIOSKokkos_CPP_ */
