/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * cxx03Types.h
 *
 *  Created on: Apr 6, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef BINDINGS_CXX03_CXX03_CXX03TYPES_H_
#define BINDINGS_CXX03_CXX03_CXX03TYPES_H_

#include <complex>
#include <cstddef>
#include <string>
#include <vector>

namespace adios2
{
namespace cxx03
{

enum Mode
{
    Undefined,
    Write,
    Read,
    Append
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

#define ADIOS2_FOREACH_CXX03_TYPE_1ARG(MACRO)                                  \
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

#define ADIOS2_FOREACH_CXX03_ATTRIBUTE_TYPE_1ARG(MACRO)                        \
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

} // end namespace cxx03
} // end namespace adios2

#endif /* BINDINGS_CXX03_CXX03_CXX03TYPES_H_ */
