/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS1Common.tcc
 *
 *  Created on: Jun 1, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 *              William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_INTEROP_ADIOS1_ADIOS1COMMON_TCC_
#define ADIOS2_TOOLKIT_INTEROP_ADIOS1_ADIOS1COMMON_TCC_

#include "ADIOS1Common.h"

#include "adios2/helper/adiosFunctions.h" //GetType

namespace adios2
{
namespace interop
{

template <>
enum ADIOS_DATATYPES ADIOS1Common::GetADIOS1Type<std::string>() const {
    return adios_string;
}

template <>
enum ADIOS_DATATYPES ADIOS1Common::GetADIOS1Type<char>() const {
    return adios_byte;
}

template <>
enum ADIOS_DATATYPES ADIOS1Common::GetADIOS1Type<signed char>() const {
    return adios_byte;
}

template <>
enum ADIOS_DATATYPES ADIOS1Common::GetADIOS1Type<unsigned char>() const {
    return adios_unsigned_byte;
}

template <>
enum ADIOS_DATATYPES ADIOS1Common::GetADIOS1Type<short>() const {
    return adios_short;
}

template <>
enum ADIOS_DATATYPES ADIOS1Common::GetADIOS1Type<unsigned short>() const {
    return adios_unsigned_short;
}

template <>
enum ADIOS_DATATYPES ADIOS1Common::GetADIOS1Type<int>() const {
    return adios_integer;
}

template <>
enum ADIOS_DATATYPES ADIOS1Common::GetADIOS1Type<unsigned int>() const {
    return adios_unsigned_integer;
}

template <>
enum ADIOS_DATATYPES ADIOS1Common::GetADIOS1Type<long int>() const {
    return adios_long;
}

template <>
enum ADIOS_DATATYPES ADIOS1Common::GetADIOS1Type<unsigned long int>() const {
    return adios_unsigned_long;
}

template <>
enum ADIOS_DATATYPES ADIOS1Common::GetADIOS1Type<long long int>() const {

    return adios_long;
}

template <>
enum ADIOS_DATATYPES
ADIOS1Common::GetADIOS1Type<unsigned long long int>() const {
    return adios_unsigned_long;
}

template <>
enum ADIOS_DATATYPES ADIOS1Common::GetADIOS1Type<float>() const {
    return adios_real;
}

template <>
enum ADIOS_DATATYPES ADIOS1Common::GetADIOS1Type<double>() const {
    return adios_double;
}

template <>
enum ADIOS_DATATYPES ADIOS1Common::GetADIOS1Type<long double>() const {
    return adios_unknown;
}

template <>
enum ADIOS_DATATYPES ADIOS1Common::GetADIOS1Type<cfloat>() const {
    return adios_complex;
}

template <>
enum ADIOS_DATATYPES ADIOS1Common::GetADIOS1Type<cdouble>() const {
    return adios_double_complex;
}

} // end namespace interop
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_INTEROP_ADIOS1_ADIOS1COMMON_TCC_ */
