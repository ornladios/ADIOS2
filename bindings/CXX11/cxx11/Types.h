/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Types.h : provides type utilities for ADIOS2 C++11 bindings
 *
 *  Created on: Feb 11, 2019
 *      Author: Kai Germaschewski <kai.germaschewski@unh.edu>
 */

#ifndef ADIOS2_BINDINGS_CXX11_TYPES_H_
#define ADIOS2_BINDINGS_CXX11_TYPES_H_

namespace adios2
{

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

} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX11_TYPES_H_ */
