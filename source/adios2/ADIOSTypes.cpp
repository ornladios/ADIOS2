
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

const DataType DataType::Unknown("unknown");
const DataType DataType::Compound("compound");
const DataType DataType::String("string");
const DataType DataType::Int8("signed char");
const DataType DataType::Int16("short");
const DataType DataType::Int32("int");
const DataType DataType::Int64("long long int");
const DataType DataType::UInt8("unsigned char");
const DataType DataType::UInt16("unsigned short");
const DataType DataType::UInt32("unsigned int");
const DataType DataType::UInt64("unsigned long long int");
const DataType DataType::Float("float");
const DataType DataType::Double("double");
const DataType DataType::LDouble("long double");
const DataType DataType::CFloat("float complex");
const DataType DataType::CDouble("double complex");

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
