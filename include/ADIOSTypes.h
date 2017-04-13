/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS_Types.h
 *
 *  Created on: Mar 23, 2017
 *      Author: pnb
 */

#ifndef ADIOS_TYPES_H_
#define ADIOS_TYPES_H_

#include <complex>
#include <cstddef>
#include <cstdint>

namespace adios
{

/** Use these values in Dims() when defining variables
 */
enum
{
    VARYING_DIMENSION = -1, //!< VARYING_DIMENSION
    LOCAL_VALUE = 0,        //!< LOCAL_VALUE
    GLOBAL_VALUE = 1        //!< GLOBAL_VALUE
};

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

} // end namespace adios

#endif /* ADIOS_TYPES_H_ */
