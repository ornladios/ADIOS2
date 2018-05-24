/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * cxx98Types.h
 *
 *  Created on: Apr 6, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef BINDINGS_CXX98_CXX98_CXX98TYPES_H_
#define BINDINGS_CXX98_CXX98_CXX98TYPES_H_

#include <complex>
#include <cstddef>
#include <string>
#include <vector>

namespace adios2
{
namespace cxx98
{

enum Mode
{
    Undefined = 0,
    Write = 1,
    Read = 2,
    Append = 3,

    Deferred = 4,
    Sync = 5
};

enum StepStatus
{
    OK,
    NotReady,
    EndOfStream,
    OtherError
};

enum StepMode
{
    AtEnd,
    Update, // writer advance mode
    NextAvailable,
    LatestAvailable // reader advance mode
};

typedef std::vector<std::size_t> Dims;

#define ADIOS2_FOREACH_CXX98_TYPE_1ARG(MACRO)                                  \
    MACRO(std::string)                                                         \
    MACRO(char)                                                                \
    MACRO(signed char)                                                         \
    MACRO(unsigned char)                                                       \
    MACRO(short)                                                               \
    MACRO(unsigned short)                                                      \
    MACRO(int)                                                                 \
    MACRO(unsigned int)                                                        \
    MACRO(long int)                                                            \
    MACRO(long long int)                                                       \
    MACRO(unsigned long int)                                                   \
    MACRO(unsigned long long int)                                              \
    MACRO(float)                                                               \
    MACRO(double)                                                              \
    MACRO(long double)                                                         \
    MACRO(std::complex<float>)                                                 \
    MACRO(std::complex<double>)

#define ADIOS2_FOREACH_CXX98_ATTRIBUTE_TYPE_1ARG(MACRO)                        \
    MACRO(std::string)                                                         \
    MACRO(char)                                                                \
    MACRO(signed char)                                                         \
    MACRO(unsigned char)                                                       \
    MACRO(short)                                                               \
    MACRO(unsigned short)                                                      \
    MACRO(int)                                                                 \
    MACRO(unsigned int)                                                        \
    MACRO(long int)                                                            \
    MACRO(unsigned long int)                                                   \
    MACRO(long long int)                                                       \
    MACRO(unsigned long long int)                                              \
    MACRO(float)                                                               \
    MACRO(double)

} // end namespace cxx98
} // end namespace adios2

#endif /* BINDINGS_CXX98_CXX98_CXX98TYPES_H_ */
