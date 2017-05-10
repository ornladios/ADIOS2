/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOSTypes.h
 *
 *  Created on: Mar 23, 2017
 *      Author: pnb
 */

#ifndef ADIOS2_ADIOSTYPES_H_
#define ADIOS2_ADIOSTYPES_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <complex>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <string>
#include <type_traits>
#include <vector>
/// \endcond

#include "adios2/ADIOSConfig.h"

namespace adios
{

/** Variable shape type identifier */
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

enum class OpenMode
{
    Undefined,
    Write,
    Read,
    Append,
    ReadWrite,
    w,
    r,
    a,
    rw
};

typedef enum {
    GLOBAL_READERS = 2,
    ROUNDROBIN_READERS = 3,
    FIFO_READERS = 4,
    OPEN_ALL_STEPS = 5
} ReadMultiplexPattern;

typedef enum {
    NOWAITFORSTREAM = 0,
    WAITFORSTREAM = 1
} StreamOpenMode; // default: wait for stream

enum class TransformType
{
    bzip2,
    zfp
};

enum class TransportType
{
    File,
    WAN
};

enum class IOEngine
{
    Unknown,
    BPFileWriter, ///< produces bp files
    BPFileReader, ///< read bp files
    HDF5Writer,   ///<
    HDF5Reader,   ///<
    ADIOS1Writer,
    ADIOS1Reader,
    DataManWriter,
    DataManReader
};

typedef enum { NONBLOCKINGREAD = 0, BLOCKINGREAD = 1 } PerformReadMode;

typedef enum {
    APPEND = 0,
    UPDATE = 1, // writer advance modes
    NEXT_AVAILABLE = 2,
    LATEST_AVAILABLE = 3, // reader advance modes
} AdvanceMode;

enum class AdvanceStatus
{
    OK = 0,
    STEP_NOT_READY = 1,
    END_OF_STREAM = 2,
    OTHER_ERROR = 3
};

enum class TimeUnit
{
    MicroSeconds,
    MiliSeconds,
    Seconds,
    Minutes,
    Hours,
    mus,
    ms,
    s,
    m,
    h
};

/** Type of selection */
enum class SelectionType
{
    BoundingBox, ///< Contiguous block of data defined by offsets and counts per
                 /// dimension
    Points,      ///< List of individual points
    WriteBlock,  ///< Selection of an individual block written by a writer
                 /// process
    Auto         ///< Let the engine decide what to return
};

// adios defaults
const std::string DefaultFileLibrary("POSIX");
const std::string DefaultTimeUnit("mus");
constexpr TimeUnit DefaultTimeUnitEnum(TimeUnit::mus);
constexpr size_t DefaultBufferSize(16384); ///< in bytes

// adios alias values and types
constexpr bool DebugON = true;
constexpr bool DebugOFF = false;
constexpr size_t UnknownDim = 0;
constexpr size_t JoinedDim = std::numeric_limits<size_t>::max() - 1;
constexpr size_t LocalValueDim = std::numeric_limits<size_t>::max() - 2;
constexpr size_t IrregularDim = std::numeric_limits<size_t>::max() - 3;
constexpr bool ConstantDims = true;
constexpr bool ReadIn = true;

using std::size_t;

using Dims = std::vector<size_t>;
using Params = std::map<std::string, std::string>;

// Primitives
// using schar = signed char;
using std::int8_t;
using std::int16_t;
using std::int32_t;
using std::int64_t;

// using uchar = unsigned char;
using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;

// Complex
using cfloat = std::complex<float>;
using cdouble = std::complex<double>;
using cldouble = std::complex<long double>;

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

} // end namespace adios

#endif /* ADIOS2_ADIOSTYPES_H_ */
