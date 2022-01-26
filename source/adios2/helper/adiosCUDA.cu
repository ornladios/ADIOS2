/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosCUDA.cpp
 *
 *  Created on: May 9, 2021
 *      Author: Ana Gainaru gainarua@ornl.gov
 */

#ifndef ADIOS2_HELPER_ADIOSCUDA_CU_
#define ADIOS2_HELPER_ADIOSCUDA_CU_

#include "adios2/common/ADIOSMacros.h"
#include <thrust/device_ptr.h>
#include <thrust/extrema.h>

#include "adiosCUDA.h"

namespace
{
template <class T>
void CUDAMinMaxImpl(const T *values, const size_t size, T &min, T &max)
{
    thrust::device_ptr<const T> dev_ptr(values);
    auto res = thrust::minmax_element(dev_ptr, dev_ptr + size);
    cudaMemcpy(&min, thrust::raw_pointer_cast(res.first), sizeof(T),
               cudaMemcpyDeviceToHost);
    cudaMemcpy(&max, thrust::raw_pointer_cast(res.second), sizeof(T),
               cudaMemcpyDeviceToHost);
}
// types non supported on the device
void CUDAMinMaxImpl(const long double * /*values*/, const size_t /*size*/,
                    long double & /*min*/, long double & /*max*/)
{
}
void CUDAMinMaxImpl(const std::complex<float> * /*values*/,
                    const size_t /*size*/, std::complex<float> & /*min*/,
                    std::complex<float> & /*max*/)
{
}
void CUDAMinMaxImpl(const std::complex<double> * /*values*/,
                    const size_t /*size*/, std::complex<double> & /*min*/,
                    std::complex<double> & /*max*/)
{
}
}

template <class T>
void adios2::helper::CUDAMinMax(const T *values, const size_t size, T &min,
                                T &max)
{
    CUDAMinMaxImpl(values, size, min, max);
}

#define declare_type(T)                                                        \
    template void adios2::helper::CUDAMinMax(                                  \
        const T *values, const size_t size, T &min, T &max);
ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_type)
#undef declare_type

#endif /* ADIOS2_HELPER_ADIOSCUDA_CU_ */
