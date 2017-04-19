/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOSMacros.h
 *   This contains a set of helper macros used internally
 */
#ifndef ADIOSMACROS_H
#define ADIOSMACROS_H

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
#define ADIOS_FOREACH_TYPE_1ARG(MACRO)                                         \
    MACRO(char)                                                                \
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
    MACRO(long double)                                                         \
    MACRO(std::complex<float>)                                                 \
    MACRO(std::complex<double>)                                                \
    MACRO(std::complex<long double>)

#define ADIOS_FOREACH_PRIMITIVE_TYPE_1ARG(MACRO)                               \
    MACRO(char)                                                                \
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

#define ADIOS_FOREACH_COMPLEX_TYPE_1ARG(MACRO)                                 \
    MACRO(float)                                                               \
    MACRO(double)                                                              \
    MACRO(long double)

#endif // ADIOSMACROS_H
