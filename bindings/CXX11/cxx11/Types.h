/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Types.h : provides type utilities for ADIOS2 C++11 bindings
 *
 *  Created on: Feb 11, 2019
 *      Author: Kai Germaschewski <kai.germaschewski@unh.edu>
 *              William F Godoy <godoywf@ornl.gov>
 */

#ifndef ADIOS2_BINDINGS_CXX11_TYPES_H_
#define ADIOS2_BINDINGS_CXX11_TYPES_H_

#include "Variable.h"

#include <vector>

/**
 * The ADIOS_FOREACH_TYPE_1ARG macros are similar to the _STDTYPE macros used in
 * core,
 * but use primitive C++ types rather than stdint based types. Only for use in
 * the CXX11 bindings.
 */
#define ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_TYPE_1ARG(MACRO)                    \
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
    MACRO(double)                                                              \
    MACRO(long double)

#define ADIOS2_FOREACH_PRIMITIVE_TYPE_1ARG(MACRO)                              \
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_TYPE_1ARG(MACRO)                        \
    MACRO(std::complex<float>)                                                 \
    MACRO(std::complex<double>)

#define ADIOS2_FOREACH_ATTRIBUTE_TYPE_1ARG(MACRO)                              \
    MACRO(std::string)                                                         \
    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_TYPE_1ARG(MACRO)

#define ADIOS2_FOREACH_TYPE_1ARG(MACRO)                                        \
    MACRO(std::string)                                                         \
    ADIOS2_FOREACH_PRIMITIVE_TYPE_1ARG(MACRO)

namespace adios2
{

template <class T>
std::string GetType() noexcept;

// LIMIT TEMPLATE TYPES FOR adios2::GetType
#define declare_template_instantiation(T)                                      \
                                                                               \
    extern template std::string GetType<T>() noexcept;

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace adios2
#endif /* ADIOS2_BINDINGS_CXX11_TYPES_H_ */
