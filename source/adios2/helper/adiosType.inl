/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosType.inl implementation of template functions in adiosType.h
 *
 *  Created on: May 17, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_HELPER_ADIOSTYPE_INL_
#define ADIOS2_HELPER_ADIOSTYPE_INL_
#ifndef ADIOS2_HELPER_ADIOSTYPE_H_
#error "Inline file should only be included from it's header, never on it's own"
#endif

#include "adios2/ADIOSMacros.h"

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
template <>
inline std::string GetType<std::complex<long double>>() noexcept
{
    return "long double complex";
}

#define define_impl(T, L)                                                      \
    template <>                                                                \
    inline std::string GetTypeDescription<T>() noexcept                        \
    {                                                                          \
        return ADIOS2_STRINGIFY(L);                                            \
    }
ADIOS2_FOREACH_TYPE_2ARGS(define_impl)
#undef define_impl

template <class T>
bool IsTypeAlias(
    const std::string type,
    const std::map<std::string, std::set<std::string>> &aliases) noexcept
{
    if (type == GetType<T>()) // is key itself
    {
        return true;
    }

    bool isAlias = false;
    if (aliases.at(GetType<T>()).count(type) == 1)
    {
        isAlias = true;
    }

    return isAlias;
}

} // end namespace adios

#endif /* ADIOS2_HELPER_ADIOSTYPE_INL_ */
