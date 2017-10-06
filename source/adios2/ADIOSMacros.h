/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOSMacros.h
 *   This contains a set of helper macros used internally
 */
#ifndef ADIOS2_ADIOSMACROS_H
#define ADIOS2_ADIOSMACROS_H

#include <string>

#include "adios2/ADIOSTypes.h"

#define ADIOS2_STRINGIFY_HELPER(X) #X
#define ADIOS2_STRINGIFY(X) ADIOS2_STRINGIFY_HELPER(X)

// The ADIOS_FOREACH_TYPE_1ARG macro assumes the given argument is a macro which
// takes a single argument that is a type and then inserts the given MACRO for
// each of the known primitive types
//
// An example of this might be to instantiate a template function for each
// known type.  For example:
//
//   template<typename T> int foo() { /* some implementation of foo */ }
//
//   #define instantiate_foo(T) template int foo<T>();
//   ADIOS_FOREACH_TYPE_1ARG(instantiate_foo)
//   #undef instantiate_foo
//
#define ADIOS2_FOREACH_TYPE_1ARG(MACRO)                                        \
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
    MACRO(std::complex<double>)                                                \
    MACRO(std::complex<long double>)

#define ADIOS2_FOREACH_PRIMITIVE_TYPE_1ARG(MACRO)                              \
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

#define ADIOS2_FOREACH_COMPLEX_TYPE_1ARG(MACRO)                                \
    MACRO(float)                                                               \
    MACRO(double)

#define ADIOS2_FOREACH_ZFP_TYPE_1ARG(MACRO)                                    \
    MACRO(int32_t)                                                             \
    MACRO(int64_t)                                                             \
    MACRO(float)                                                               \
    MACRO(double)

#define ADIOS2_FOREACH_ATTRIBUTE_TYPE_1ARG(MACRO)                              \
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
    MACRO(double)                                                              \
    MACRO(long double)

// The ADIOS_FOREACH_TYPE_2ARGS macro assumes the given argument is a macro
// which takes two arguments, the first being a type, and the second being a
// label for that type, i.e. std::complex<float> and CFloat.
//
// An example of this might be to define a virtual method for each known
// type, and since the method is virtual then it cannot be a template.
// For example:
//
//   #define declare_foo(T,L) virtual const T& foo ## L (std::string bar);
//   ADIOS_FOREACH_TYPE_2ARGS(declare_foo)
//   #undef declare_foo
//
//   is equivalent to:
//
//   virtual           char& foo_Char(std::string bar);
//   virtual unsigned  char& foo_UChar(std::string bar);
//   virtual          short& foo_Short(std::string bar);
//   virtual unsigned short& foo_UShort(std::string bar);
//   ...
//   virtual std::complex<long double>& foo_CLDouble(std::string bar);
//
#define ADIOS2_FOREACH_TYPE_2ARGS(MACRO)                                       \
    MACRO(char, Char)                                                          \
    MACRO(signed char, SChar)                                                  \
    MACRO(unsigned char, UChar)                                                \
    MACRO(short, Short)                                                        \
    MACRO(unsigned short, UShort)                                              \
    MACRO(int, Int)                                                            \
    MACRO(unsigned int, UInt)                                                  \
    MACRO(long int, LInt)                                                      \
    MACRO(long long int, LLInt)                                                \
    MACRO(unsigned long int, ULInt)                                            \
    MACRO(unsigned long long int, ULLInt)                                      \
    MACRO(float, Float)                                                        \
    MACRO(double, Double)                                                      \
    MACRO(long double, LDouble)                                                \
    MACRO(std::complex<float>, CFloat)                                         \
    MACRO(std::complex<double>, CDouble)                                       \
    MACRO(std::complex<long double>, CLDouble)

#define ADIOS2_FOREACH_PRIMITIVE_TYPE_2ARGS(MACRO)                             \
    MACRO(char, Char)                                                          \
    MACRO(signed char, SChar)                                                  \
    MACRO(unsigned char, UChar)                                                \
    MACRO(short, Short)                                                        \
    MACRO(unsigned short, UShort)                                              \
    MACRO(int, Int)                                                            \
    MACRO(unsigned int, UInt)                                                  \
    MACRO(long int, LInt)                                                      \
    MACRO(long long int, LLInt)                                                \
    MACRO(unsigned long int, ULInt)                                            \
    MACRO(unsigned long long int, ULLInt)                                      \
    MACRO(float, Float)                                                        \
    MACRO(double, Double)

#define ADIOS2_FOREACH_COMPLEX_TYPE_2ARGS(MACRO)                               \
    MACRO(std::complex<float>, CFloat)                                         \
    MACRO(std::complex<double>, CDouble)

#endif /* ADIOS2_ADIOSMACROS_H */
