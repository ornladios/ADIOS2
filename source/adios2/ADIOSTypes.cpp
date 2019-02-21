
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
}
