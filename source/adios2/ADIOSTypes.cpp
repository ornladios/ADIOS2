
/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOSTypes.cpp : implementation of type-related functionality
 *
 *  Created on: Feb 20, 2019
 *      Author: Kai Germaschewski <kai.germaschewski@unh.edu>
 */

#include "ADIOSTypes.h"

namespace adios2
{

// The actual numbers below don't matter, but they've picked so that they could be unified with the
// enum adios2_type from adios2_c_type.h eventually
const DataType DataType::Unknown(-1);
const DataType DataType::Compound(14);
const DataType DataType::String(0);
const DataType DataType::Int8(5);
const DataType DataType::Int16(6);
const DataType DataType::Int32(7);
const DataType DataType::Int64(8);
const DataType DataType::UInt8(9);
const DataType DataType::UInt16(10);
const DataType DataType::UInt32(11);
const DataType DataType::UInt64(12);
const DataType DataType::Float(1);
const DataType DataType::Double(2);
const DataType DataType::LDouble(13);
const DataType DataType::CFloat(3);
const DataType DataType::CDouble(4);

DataType DataType::FromString(const std::string &type_string)
{
    DataType type = Unknown;

    if (false)
    {
    }
#define declare_type(T)                                                        \
    else if (type_string == ToString(Get<T>())) { type = Get<T>(); }
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    return type;
}

std::string DataType::ToString(const DataType &type)
{
    if (type == Unknown)
    {
        return "";
    }
    else if (type == Compound)
    {
        return "compound";
    }
    else if (type == String)
    {
        return "string";
    }
    else if (type == Int8)
    {
        return "signed char";
    }
    else if (type == Int16)
    {
        return "short";
    }
    else if (type == Int32)
    {
        return "int";
    }
    else if (type == Int64)
    {
        return "long long int";
    }
    else if (type == UInt8)
    {
        return "unsigned char";
    }
    else if (type == UInt16)
    {
        return "unsigned short";
    }
    else if (type == UInt32)
    {
        return "unsigned int";
    }
    else if (type == UInt64)
    {
        return "unsigned long long int";
    }
    else if (type == Float)
    {
        return "float";
    }
    else if (type == Double)
    {
        return "double";
    }
    else if (type == LDouble)
    {
        return "long double";
    }
    else if (type == CFloat)
    {
        return "float complex";
    }
    else if (type == CDouble)
    {
        return "double complex";
    }
    else
    {
        throw std::invalid_argument(
            "ERROR: invalid type in call to DataType::ToString\n");
    }
}
}
