/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS_Types.h
 *
 *  Created on: Mar 23, 2017
 *      Author: pnb
 */

#ifndef ADIOS2_ADIOSTYPES_H_
#define ADIOS2_ADIOSTYPES_H_

#include <complex>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "adios2/ADIOSConfig.h"

namespace adios
{

// Alias the fixed sized typed into the adios namespace to make sure we're
// always using the right ones.
using std::size_t;

using std::int8_t;
using std::uint8_t;
using std::int16_t;
using std::uint16_t;
using std::int32_t;
using std::uint32_t;
using std::int64_t;
using std::uint64_t;

// Not sure if we're really use these ones but we'll round it out for
// completion
using real32_t = float;
using real64_t = double;
using complex32_t = std::complex<real32_t>;
using complex64_t = std::complex<real64_t>;

// Get a fixed width integer type from a size specification
template <size_t Bytes, bool Signed>
struct FixedWidthInt;

template <>
struct FixedWidthInt<1, true>
{
    using Type = std::int8_t;
};
template <>
struct FixedWidthInt<2, true>
{
    using Type = std::int16_t;
};
template <>
struct FixedWidthInt<4, true>
{
    using Type = std::int32_t;
};
template <>
struct FixedWidthInt<8, true>
{
    using Type = std::int64_t;
};
template <>
struct FixedWidthInt<1, false>
{
    using Type = std::uint8_t;
};
template <>
struct FixedWidthInt<2, false>
{
    using Type = std::uint16_t;
};
template <>
struct FixedWidthInt<4, false>
{
    using Type = std::uint32_t;
};
template <>
struct FixedWidthInt<8, false>
{
    using Type = std::uint64_t;
};

// Some core type information that may be useful at compile time
template <typename T, typename Enable = void>
struct TypeInfo;

template <typename T>
struct TypeInfo<T, typename std::enable_if<std::is_integral<T>::value>::type>
{
    using IOType =
        typename FixedWidthInt<sizeof(T), std::is_signed<T>::value>::Type;
    using ValueType = T;
};

template <typename T>
struct TypeInfo<T,
                typename std::enable_if<std::is_floating_point<T>::value>::type>
{
    using IOType = T;
    using ValueType = T;
};

template <typename T>
struct TypeInfo<T, typename std::enable_if<std::is_same<
                       T, std::complex<typename T::value_type>>::value>::type>
{
    using IOType = T;
    using ValueType = typename T::value_type;
};

const size_t UnknownDim = 0;
const size_t JoinedDim = SIZE_MAX;
const size_t VarDim = JoinedDim - 1;
const bool ConstantShape = true;

enum class Verbose
{
    ERROR = 0,
    WARN = 1,
    INFO = 2,
    DEBUG = 3
};

enum class IOMode
{
    INDEPENDENT = 0,
    COLLECTIVE = 1
};

} // end namespace adios

#endif /* ADIOS2_ADIOSTYPES_H_ */
