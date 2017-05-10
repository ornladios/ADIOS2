/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosMath.h math functions used in the ADIOS framework
 *
 *  Created on: May 17, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_HELPER_ADIOSMATH_H_
#define ADIOS2_HELPER_ADIOSMATH_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <vector>
/// \endcond

#include "adios2/ADIOSTypes.h"

namespace adios
{
/**
 * Loops through a vector containing dimensions and returns the product of all
 * elements
 * @param dimensions input containing size on each dimension {Nx, Ny, Nz}
 * @return product of all dimensions Nx * Ny * Nz
 */
size_t GetTotalSize(const Dims &dimensions) noexcept;

/**
 * Gets the min and max from a values array of primitive types (not including
 * complex)
 * @param values input array
 * @param size of values array
 * @param min of values
 * @param max of values
 */
template <class T>
void GetMinMax(const T *values, const size_t size, T &min, T &max) noexcept;

/**
 * Version for complex types of GetMinMax, gets the "doughnut" range between min
 * and max modulus. Needed a different function as thread can't resolve the
 * overload of a GetMinMax with complex types
 * @param values array of complex numbers
 * @param size of the values array
 * @param min modulus from values
 * @param max modulus from values
 */
template <class T>
void GetMinMaxComplex(const std::complex<T> *values, const size_t size, T &min,
                      T &max) noexcept;

/**
 * Threaded version of GetMinMax.
 * Gets the min and max from a values array of primitive types (not including
 * complex) using threads
 * @param values input array of complex
 * @param size of values array
 * @param min of values
 * @param max of values
 * @param threads used for parallel computation
 */
template <class T>
void GetMinMaxThreads(const T *values, const size_t size, T &min, T &max,
                      const unsigned int threads = 1) noexcept;

/**
 * Overloaded version of GetMinMaxThreads for complex types
 * @param values input array of complex
 * @param size of values array
 * @param min of values
 * @param max of values
 * @param threads used for parallel computation
 */
template <class T>
void GetMinMaxThreads(const std::complex<T> *values, const size_t size, T &min,
                      T &max, const unsigned int threads = 1) noexcept;

/**
 * Check if index is within (inclusive) limits
 * lowerLimit <= index <= upperLimit
 * @param index input to be checked
 * @param upperLimit
 * @param lowerLimit
 * @return true index is within limits
 */
bool CheckIndexRange(const int index, const int upperLimit,
                     const int lowerLimit = 0) noexcept;

} // end namespace adios

#include "adiosMath.inl"

#endif /* ADIOS2_HELPER_ADIOSMATH_H_ */
