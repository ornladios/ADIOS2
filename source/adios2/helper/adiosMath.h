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

namespace adios2
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

/**
 * Returns the appropriate size larger than requiredSize
 * @param requiredSize
 * @param currentSize
 * @param growthFactor larger than 1. (typically 1.5 or 2. )
 * @return next currentSize * growthFactor^n (n is a signed integer) larger than
 * requiredSize
 */
size_t NextExponentialSize(const size_t requiredSize, const size_t currentSize,
                           const float growthFactor) noexcept;

/**
 * Converts a start, count box into a [start, end[ box where end = start + count
 * @param start
 * @param count
 * @return [start, end[ box
 */
Box<Dims> StartEndBox(const Dims &start, const Dims &count) noexcept;

Box<Dims> StartCountBox(const Dims &start, const Dims &end) noexcept;

/**
 * Returns the intersection box { start, end } where end is inclusive from box1
 * and box2
 * @param box1 {start, end} input (end is exclusive)
 * @param box2 {start, end} input (end is exclusive)
 * @return empty if not interception, otherwise intersection box
 */
Box<Dims> IntersectionBox(const Box<Dims> &box1,
                          const Box<Dims> &box2) noexcept;

/**
 * Get a linear index for a point inside a localBox depending on data layout
 * @param localBox start and count
 * @param point inside box
 * @param isRowMajor
 * @param isZeroIndex
 * @return linear index for contiguous memory
 */
size_t LinearIndex(const Box<Dims> &localBox, const Dims &point,
                   const bool isRowMajor, const bool isZeroIndex) noexcept;

} // end namespace adios2

#include "adiosMath.inl"

#endif /* ADIOS2_HELPER_ADIOSMATH_H_ */
