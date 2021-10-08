/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosCUDA.h CUDA functions used in the ADIOS framework
 *
 *  Created on: May 9, 2021
 *      Author: Ana Gainaru gainarua@ornl.gov
 */

#ifndef ADIOS2_HELPER_ADIOSCUDA_H_
#define ADIOS2_HELPER_ADIOSCUDA_H_

namespace adios2
{
namespace helper
{

/*
 * CUDA kernel for computing the min and max from a
 * GPU buffer
 */
template <class T>
void CUDAMinMax(const T *values, const size_t size, T &min, T &max);

} // helper
} // adios2

#endif /* ADIOS2_HELPER_ADIOSCUDA_H_ */
