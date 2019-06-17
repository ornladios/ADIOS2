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
    Unknown,     ///< undefined shapeID
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
    Read    // reader advance mode
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

// Limit, using uint64_t to make it portable
constexpr uint64_t MaxU64 = std::numeric_limits<uint64_t>::max();
constexpr size_t MaxSizeT = std::numeric_limits<size_t>::max();
constexpr size_t DefaultSizeT = std::numeric_limits<size_t>::max();
constexpr size_t EngineCurrentStep = std::numeric_limits<size_t>::max();

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
constexpr uint64_t DefaultMaxBufferSize = MaxSizeT - 1;

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
constexpr uint64_t JoinedDim = MaxU64 - 1;
constexpr uint64_t LocalValueDim = MaxU64 - 2;
constexpr uint64_t IrregularDim = MaxU64 - 3;
constexpr bool ConstantDims = true;
constexpr bool end_step = true;
constexpr bool LocalValue = true;
constexpr bool GlobalValue = false;

using Dims = std::vector<size_t>;
using Params = std::map<std::string, std::string>;
using vParams = std::vector<std::pair<std::string, Params>>;
using Steps = size_t;

template <class T>
using Box = std::pair<T, T>;

/**
 * TypeInfo
 * used to map from primitive types to stdint-based types
 */
template <typename T, typename Enable = void>
struct TypeInfo;

/**
 * ToString
 * makes a string from an enum class like ShapeID etc, for debugging etc
 * It is also overloaded elsewhere to allow for a readable representation of
 * Variable, Attribute, etc.
 */

std::string ToString(ShapeID value);
std::string ToString(IOMode value);
std::string ToString(Mode value);
std::string ToString(ReadMultiplexPattern value);
std::string ToString(StreamOpenMode value);
std::string ToString(ReadMode value);
std::string ToString(StepMode value);
std::string ToString(StepStatus value);
std::string ToString(TimeUnit value);
std::string ToString(SelectionType value);

/**
 * os << [adios2_type] enables output of adios2 enums/classes directly
 * to output streams (e.g. std::cout), if ToString() can handle [adios2_type].
 */
template <typename T, typename Enable = decltype(ToString(std::declval<T>()))>
std::ostream &operator<<(std::ostream &os, const T &value);

namespace ops
{

// SZ PARAMETERS
#ifdef ADIOS2_HAVE_SZ

constexpr char SZ[] = "sz";

namespace sz
{
namespace key
{
constexpr char ACCURACY[] = "accuracy";
}
}

#endif

// ZFP PARAMETERS
#ifdef ADIOS2_HAVE_ZFP

constexpr char ZFP[] = "zfp";

namespace zfp
{
namespace key
{
constexpr char ACCURACY[] = "accuracy";
constexpr char RATE[] = "rate";
constexpr char PRECISION[] = "precision";
}
}
#endif

// MGARD PARAMETERS
#ifdef ADIOS2_HAVE_MGARD
constexpr char MGARD[] = "mgard";

namespace mgard
{
namespace key
{
constexpr char TOLERANCE[] = "tolerance";
constexpr char ACCURACY[] = "accuracy";
}
}
#endif

// PNG PARAMETERS
#ifdef ADIOS2_HAVE_PNG
constexpr char PNG[] = "png";

namespace png
{

namespace key
{
constexpr char COLOR_TYPE[] = "color_type";
constexpr char BIT_DEPTH[] = "bit_depth";
constexpr char COMPRESSION_LEVEL[] = "compression_level";
}

namespace value
{
constexpr char COLOR_TYPE_GRAY[] = "PNG_COLOR_TYPE_GRAY";
constexpr char COLOR_TYPE_GRAY_ALPHA[] = "PNG_COLOR_TYPE_GRAY_ALPHA";
constexpr char COLOR_TYPE_PALETTE[] = "PNG_COLOR_TYPE_PALETTE";
constexpr char COLOR_TYPE_RGB[] = "PNG_COLOR_TYPE_RGB";
constexpr char COLOR_TYPE_RGB_ALPHA[] = "PNG_COLOR_TYPE_RGB_ALPHA";
constexpr char COLOR_TYPE_RGBA[] = "PNG_COLOR_TYPE_RGBA";
constexpr char COLOR_TYPE_GA[] = "PNG_COLOR_TYPE_GA";

constexpr char BIT_DEPTH_1[] = "1";
constexpr char BIT_DEPTH_2[] = "2";
constexpr char BIT_DEPTH_3[] = "3";
constexpr char BIT_DEPTH_4[] = "4";
constexpr char BIT_DEPTH_5[] = "5";
constexpr char BIT_DEPTH_6[] = "6";
constexpr char BIT_DEPTH_7[] = "7";
constexpr char BIT_DEPTH_8[] = "8";
constexpr char BIT_DEPTH_9[] = "9";

constexpr char COMPRESSION_LEVEL_1[] = "1";
constexpr char COMPRESSION_LEVEL_2[] = "2";
constexpr char COMPRESSION_LEVEL_3[] = "3";
constexpr char COMPRESSION_LEVEL_4[] = "4";
constexpr char COMPRESSION_LEVEL_5[] = "5";
constexpr char COMPRESSION_LEVEL_6[] = "6";
constexpr char COMPRESSION_LEVEL_7[] = "7";
constexpr char COMPRESSION_LEVEL_8[] = "8";
constexpr char COMPRESSION_LEVEL_9[] = "9";

} // end namespace value

} // end namespace png
#endif

// BZIP2 PARAMETERS
#ifdef ADIOS2_HAVE_BZIP2

constexpr char BZIP2[] = "bzip2";
namespace bzip2
{

namespace key
{
constexpr char BLOCKSIZE100K[] = "blockSize100k";
}

namespace value
{
constexpr char BLOCKSIZE100K_1[] = "1";
constexpr char BLOCKSIZE100K_2[] = "2";
constexpr char BLOCKSIZE100K_3[] = "3";
constexpr char BLOCKSIZE100K_4[] = "4";
constexpr char BLOCKSIZE100K_5[] = "5";
constexpr char BLOCKSIZE100K_6[] = "6";
constexpr char BLOCKSIZE100K_7[] = "7";
constexpr char BLOCKSIZE100K_8[] = "8";
constexpr char BLOCKSIZE100K_9[] = "9";
} // end namespace value

} // end namespace bzip2
#endif

} // end namespace ops

} // end namespace adios2

#include "ADIOSTypes.inl"

#endif /* ADIOS2_ADIOSTYPES_H_ */
