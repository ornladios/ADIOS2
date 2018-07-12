/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * cxx98Types.h : define public types under the adios2::cxx98 namespace
 *
 *  Created on: Apr 6, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_CXX98_CXX98_CXX98TYPES_H_
#define ADIOS2_BINDINGS_CXX98_CXX98_CXX98TYPES_H_

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

/** Variable shape type identifier, assigned automatically from the signature of
 *  DefineVariable */
enum ShapeID
{
    Unknown = -1,
    GlobalValue = 0, ///< single global value, common case
    GlobalArray = 1, ///< global (across MPI_Comm) array, common case
    JoinedArray = 2, ///< global array with a common (joinable) dimension
    LocalValue = 3,  ///< special case, local independent value
    LocalArray = 4   ///< special case, local independent array
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

// Making GetType a user facing function
template <class T>
inline std::string GetType() noexcept
{
    return "compound";
}

template <>
inline std::string GetType<void>() noexcept
{
    return "unknown";
}

template <>
inline std::string GetType<std::string>() noexcept
{
    return "string";
}

template <>
inline std::string GetType<char>() noexcept
{
    return "char";
}
template <>
inline std::string GetType<signed char>() noexcept
{
    return "signed char";
}
template <>
inline std::string GetType<unsigned char>() noexcept
{
    return "unsigned char";
}
template <>
inline std::string GetType<short>() noexcept
{
    return "short";
}
template <>
inline std::string GetType<unsigned short>() noexcept
{
    return "unsigned short";
}
template <>
inline std::string GetType<int>() noexcept
{
    return "int";
}
template <>
inline std::string GetType<unsigned int>() noexcept
{
    return "unsigned int";
}
template <>
inline std::string GetType<long int>() noexcept
{
    return "long int";
}
template <>
inline std::string GetType<unsigned long int>() noexcept
{
    return "unsigned long int";
}
template <>
inline std::string GetType<long long int>() noexcept
{
    return "long long int";
}
template <>
inline std::string GetType<unsigned long long int>() noexcept
{
    return "unsigned long long int";
}
template <>
inline std::string GetType<float>() noexcept
{
    return "float";
}
template <>
inline std::string GetType<double>() noexcept
{
    return "double";
}
template <>
inline std::string GetType<long double>() noexcept
{
    return "long double";
}
template <>
inline std::string GetType<std::complex<float>>() noexcept
{
    return "float complex";
}
template <>
inline std::string GetType<std::complex<double>>() noexcept
{
    return "double complex";
}
template <>
inline std::string GetType<std::complex<long double>>() noexcept
{
    return "long double complex";
}

} // end namespace cxx98
} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX98_CXX98_CXX98TYPES_H_ */
