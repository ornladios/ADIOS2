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
#include <map>
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

} // end namespace adios2

#endif /* ADIOS2_ADIOSBASETYPES_H_ */
