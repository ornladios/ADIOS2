/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOSBaseTypes.h : fundamental type definitions used across ADIOS2.
 * This header is self-contained (no ADIOS internal dependencies) and
 * serves as the single source of truth for these types. It is used by
 * both internal ADIOS code (via ADIOSTypes.h) and the external plugin
 * operator interface.
 *
 *  Created on: Feb 10, 2026
 */

#ifndef ADIOS2_ADIOSBASETYPES_H_
#define ADIOS2_ADIOSBASETYPES_H_

#include <complex>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <numeric>
#include <string>
#include <vector>

namespace adios2
{

// Data types.
enum class DataType
{
    None,
    Int8,
    Int16,
    Int32,
    Int64,
    UInt8,
    UInt16,
    UInt32,
    UInt64,
    Float,
    Double,
    LongDouble,
    FloatComplex,
    DoubleComplex,
    String,
    Char,
    Struct
};

using Dims = std::vector<size_t>;
using Params = std::map<std::string, std::string>;
using vParams = std::vector<std::pair<std::string, Params>>;

/** Data accuracy **/

/* Error. Accuracy can be requested for reading data.
   norm: 0.0 = L2, inf() = Linf
   relative: true = relative error, false = absolute error
 */
struct Accuracy
{
    double error;
    double norm;
    bool relative;
};

/** Return the size in bytes of a single element of the given DataType. */
inline size_t GetDataTypeSize(const DataType type) noexcept
{
    switch (type)
    {
    case DataType::Int8:
    case DataType::UInt8:
    case DataType::Char:
        return 1;
    case DataType::Int16:
    case DataType::UInt16:
        return 2;
    case DataType::Int32:
    case DataType::UInt32:
        return 4;
    case DataType::Int64:
    case DataType::UInt64:
        return 8;
    case DataType::Float:
        return sizeof(float);
    case DataType::Double:
        return sizeof(double);
    case DataType::LongDouble:
        return sizeof(long double);
    case DataType::FloatComplex:
        return sizeof(std::complex<float>);
    case DataType::DoubleComplex:
        return sizeof(std::complex<double>);
    default:
        return 0;
    }
}

/** Return the total number of elements (product of dimensions) times elementSize. */
inline size_t GetTotalSize(const Dims &dimensions, const size_t elementSize = 1) noexcept
{
    return std::accumulate(dimensions.begin(), dimensions.end(), elementSize,
                           std::multiplies<size_t>());
}

} // end namespace adios2

#endif /* ADIOS2_ADIOSBASETYPES_H_ */
