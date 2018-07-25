/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOSTypes.h : public header that contains "using/typedef" alias, defaults
 * and parameters options as enum classes
 *
 *  Created on: Mar 23, 2017
 *      Author: Chuck Atkins chuck.atkins@kitware.com
 *              Norbert Podhorszki pnorbert@ornl.gov
 *              William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_ADIOSTYPES_H_
#define ADIOS2_ADIOSTYPES_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <cstddef>
#include <cstdint>

#include <complex>
#include <limits>
#include <map>
#include <string>
#include <type_traits>
#include <utility> //std::pair
#include <vector>
/// \endcond

#include "adios2/ADIOSConfig.h"

namespace adios2
{

/** Variable shape type identifier, assigned automatically from the signature of
 *  DefineVariable */
enum class ShapeID
{
    GlobalValue, ///< single global value, common case
    GlobalArray, ///< global (across MPI_Comm) array, common case
    JoinedArray, ///< global array with a common (joinable) dimension
    LocalValue,  ///< special case, local independent value
    LocalArray   ///< special case, local independent array
};

/** Used to set IO class */
enum class IOMode
{
    Independent, ///< all I/O operations are independent per rank
    Collective   ///< expect collective I/O operations
};

/** OpenMode in IO Open */
enum class Mode
{
    Undefined,
    // open modes
    Write,
    Read,
    Append,
    // launch execution modes
    Sync,
    Deferred
};

enum class ReadMultiplexPattern
{
    GlobalReaders,
    RoundRobin,
    FirstInFirstOut,
    OpenAllSteps
};

enum class StreamOpenMode
{
    Wait,
    NoWait
};

enum class ReadMode
{
    NonBlocking,
    Blocking
};

enum class StepMode
{
    Append,
    Update, // writer advance mode
    NextAvailable,
    LatestAvailable // reader advance mode
};

enum class StepStatus
{
    OK,
    NotReady,
    EndOfStream,
    OtherError
};

enum class TimeUnit
{
    Microseconds,
    Milliseconds,
    Seconds,
    Minutes,
    Hours
};

/** Type of selection */
enum class SelectionType
{
    BoundingBox, ///< Contiguous block of data defined by offsets and counts
                 /// per dimension
    Points,      ///< List of individual points
    WriteBlock,  ///< Selection of an individual block written by a writer
                 /// process
    Auto         ///< Let the engine decide what to return
};

// Types
using std::size_t;

using std::int8_t;
using std::int16_t;
using std::int32_t;
using std::int64_t;

using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;

// Complex
using cfloat = std::complex<float>;
using cdouble = std::complex<double>;
using cldouble = std::complex<long double>;

// Limit
constexpr size_t MaxSizeT = std::numeric_limits<size_t>::max();

// adios defaults
#ifdef _WIN32
const std::string DefaultFileLibrary("fstream");
#else
const std::string DefaultFileLibrary("POSIX");
#endif
const std::string DefaultTimeUnit("Microseconds");
constexpr TimeUnit DefaultTimeUnitEnum(TimeUnit::Microseconds);

/** default initial bp buffer size, 16Kb, in bytes */
constexpr size_t DefaultInitialBufferSize = 16 * 1024;

/** default maximum bp buffer size, unlimited, in bytes.
 *  Needs to be studied for optimizing applications */
constexpr size_t DefaultMaxBufferSize = MaxSizeT - 1;

/** default buffer growth factor. Needs to be studied
 * for optimizing applications*/
constexpr float DefaultBufferGrowthFactor = 1.05f;

/** default size for writing/reading files using POSIX/fstream/stdio write
 *  2Gb - 100Kb (tolerance)*/
constexpr size_t DefaultMaxFileBatchSize = 2147381248;

constexpr char PathSeparator =
#ifdef _WIN32
    '\\';
#else
    '/';
#endif

// adios alias values and types
constexpr bool DebugON = true;
constexpr bool DebugOFF = false;
constexpr size_t UnknownDim = 0;
constexpr size_t JoinedDim = MaxSizeT - 1;
constexpr size_t LocalValueDim = MaxSizeT - 2;
constexpr size_t IrregularDim = MaxSizeT - 3;
constexpr bool ConstantDims = true;
constexpr bool endl = true;

using Dims = std::vector<size_t>;
using Params = std::map<std::string, std::string>;
using vParams = std::map<std::string, std::string>;
using Steps = size_t;

template <class T>
using Box = std::pair<T, T>;

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
struct TypeInfo
{
    using IOType = T;
    using ValueType = T;
};

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

template <typename T>
struct TypeInfo<
    T, typename std::enable_if<std::is_same<T, std::string>::value>::type>
{
    using IOType = T;
    using ValueType = T;
};

// Making GetType a user facing function
template <class T>
inline std::string GetType() noexcept
{
    return "compound";
}

template <>
inline std::string GetType<void>() noexcept
{
    return "unknown";
}

template <>
inline std::string GetType<std::string>() noexcept
{
    return "string";
}

template <>
inline std::string GetType<char>() noexcept
{
    return "char";
}
template <>
inline std::string GetType<signed char>() noexcept
{
    return "signed char";
}
template <>
inline std::string GetType<unsigned char>() noexcept
{
    return "unsigned char";
}
template <>
inline std::string GetType<short>() noexcept
{
    return "short";
}
template <>
inline std::string GetType<unsigned short>() noexcept
{
    return "unsigned short";
}
template <>
inline std::string GetType<int>() noexcept
{
    return "int";
}
template <>
inline std::string GetType<unsigned int>() noexcept
{
    return "unsigned int";
}
template <>
inline std::string GetType<long int>() noexcept
{
    return "long int";
}
template <>
inline std::string GetType<unsigned long int>() noexcept
{
    return "unsigned long int";
}
template <>
inline std::string GetType<long long int>() noexcept
{
    return "long long int";
}
template <>
inline std::string GetType<unsigned long long int>() noexcept
{
    return "unsigned long long int";
}
template <>
inline std::string GetType<float>() noexcept
{
    return "float";
}
template <>
inline std::string GetType<double>() noexcept
{
    return "double";
}
template <>
inline std::string GetType<long double>() noexcept
{
    return "long double";
}
template <>
inline std::string GetType<std::complex<float>>() noexcept
{
    return "float complex";
}
template <>
inline std::string GetType<std::complex<double>>() noexcept
{
    return "double complex";
}
template <>
inline std::string GetType<std::complex<long double>>() noexcept
{
    return "long double complex";
}

} // end namespace adios2

#endif /* ADIOS2_ADIOSTYPES_H_ */
