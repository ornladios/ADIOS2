/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosMemory.tcc
 *
 *  Created on: Jan 23, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_HELPER_ADIOSMEMORY_TCC_
#define ADIOS2_HELPER_ADIOSMEMORY_TCC_

#include "adiosMemory.h"

namespace adios2
{
namespace helper
{

#ifdef ADIOS2_HAVE_ENDIAN_REVERSE
template <>
void CopyEndianReverse(const char *src, const size_t payloadStride,
                       std::complex<float> *dest)
{
    std::reverse_copy(src, src + payloadStride, reinterpret_cast<char *>(dest));
    float *destF = reinterpret_cast<float *>(dest);
    std::reverse(destF, destF + payloadStride / sizeof(float));
}

template <>
void CopyEndianReverse(const char *src, const size_t payloadStride,
                       std::complex<double> *dest)
{
    std::reverse_copy(src, src + payloadStride, reinterpret_cast<char *>(dest));
    double *destF = reinterpret_cast<double *>(dest);
    std::reverse(destF, destF + payloadStride / sizeof(double));
}

template <class T>
inline void CopyEndianReverse(const char *src, const size_t payloadStride,
                              T *dest)
{
    if (sizeof(T) == 1)
    {
        std::copy(src, src + payloadStride, reinterpret_cast<char *>(dest));
        return;
    }

    std::reverse_copy(src, src + payloadStride, reinterpret_cast<char *>(dest));
    std::reverse(dest, dest + payloadStride / sizeof(T));
}
#endif

} // end namespace helper
} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOSMEMORY_TCC_ */
