/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP1Base.tcc
 *
 *  Created on: May 19, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP1_BP1BASE_TCC_
#define ADIOS2_TOOLKIT_FORMAT_BP1_BP1BASE_TCC_

#include "BP1Base.h"

namespace adios
{
namespace format
{

template <>
int8_t BP1Base::GetDataType<char>() const noexcept
{
    return type_byte;
}

template <>
int8_t BP1Base::GetDataType<short>() const noexcept
{
    return type_short;
}

template <>
int8_t BP1Base::GetDataType<int>() const noexcept
{
    return type_integer;
}

template <>
int8_t BP1Base::GetDataType<long int>() const noexcept
{
    return type_long;
}

template <>
int8_t BP1Base::GetDataType<long long int>() const noexcept
{
    return type_long;
}

template <>
int8_t BP1Base::GetDataType<unsigned char>() const noexcept
{
    return type_unsigned_byte;
}
template <>
int8_t BP1Base::GetDataType<unsigned short>() const noexcept
{
    return type_unsigned_short;
}
template <>
int8_t BP1Base::GetDataType<unsigned int>() const noexcept
{
    return type_unsigned_integer;
}

template <>
int8_t BP1Base::GetDataType<unsigned long int>() const noexcept
{
    return type_unsigned_long;
}

template <>
int8_t BP1Base::GetDataType<unsigned long long int>() const noexcept
{
    return type_unsigned_long;
}

template <>
int8_t BP1Base::GetDataType<float>() const noexcept
{
    return type_real;
}

template <>
int8_t BP1Base::GetDataType<double>() const noexcept
{
    return type_double;
}

template <>
int8_t BP1Base::GetDataType<long double>() const noexcept
{
    return type_long_double;
}

template <>
int8_t BP1Base::GetDataType<cfloat>() const noexcept
{
    return type_complex;
}

template <>
int8_t BP1Base::GetDataType<cdouble>() const noexcept
{
    return type_double_complex;
}

template <>
int8_t BP1Base::GetDataType<cldouble>() const noexcept
{
    return type_long_double_complex;
}

} // end namespace format
} // end namespace adios

#endif /* ADIOS2_TOOLKIT_FORMAT_BP1_BP1BASE_TCC_ */
