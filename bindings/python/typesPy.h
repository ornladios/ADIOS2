/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * typesPy.h
 *
 *  Created on: Nov 7, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef BINDINGS_PYTHON_TYPESPY_H_
#define BINDINGS_PYTHON_TYPESPY_H_

namespace adios2
{

#define ADIOS2_FOREACH_NUMPY_TYPE_1ARG(MACRO)                                  \
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

#define ADIOS2_FOREACH_NUMPY_ATTRIBUTE_TYPE_1ARG(MACRO)                        \
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
    MACRO(long double)

} // end namespace adios2

#endif /* BINDINGS_PYTHON_TYPESPY_H_ */
