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

#include <cmath> //std::min

#include "adios2/helper/adiosMath.h" //NextExponentialSize

namespace adios2
{
namespace format
{

template <>
int8_t BP1Base::GetDataType<std::string>() const noexcept
{
    return type_string;
}

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

template <class T>
BP1Base::ResizeResult BP1Base::ResizeBuffer(const Variable<T> &variable)
{
    size_t currentCapacity = m_HeapBuffer.m_Data.capacity();
    size_t variableData =
        GetVariableIndexSize(variable) + variable.PayLoadSize();
    size_t requiredCapacity = variableData + m_HeapBuffer.m_DataPosition;

    ResizeResult result = ResizeResult::Unchanged;

    if (variableData > m_MaxBufferSize)
    {
        throw std::runtime_error(
            "ERROR: variable " + variable.m_Name + " data size: " +
            std::to_string(static_cast<float>(variableData) / (1024. * 1024.)) +
            " Mb is too large for adios2 bp MaxBufferSize=" +
            std::to_string(static_cast<float>(m_MaxBufferSize) /
                           (1024. * 1024.)) +
            "Mb, try increasing MaxBufferSize in call to IO SetParameters, in "
            "call to Write\n");
    }

    if (requiredCapacity <= currentCapacity)
    {
        // do nothing, unchanged is default
    }
    else if (requiredCapacity > m_MaxBufferSize)
    {
        if (currentCapacity < m_MaxBufferSize)
        {
            m_HeapBuffer.ResizeData(m_MaxBufferSize);
        }
        result = ResizeResult::Flush;
    }
    else // buffer must grow
    {
        if (currentCapacity < m_MaxBufferSize)
        {
            const size_t nextSize =
                std::min(m_MaxBufferSize,
                         NextExponentialSize(requiredCapacity, currentCapacity,
                                             m_GrowthFactor));
            m_HeapBuffer.ResizeData(nextSize);
            result = ResizeResult::Success;
        }
    }

    return result;
}

template <class T>
size_t BP1Base::GetVariableIndexSize(const Variable<T> &variable) const noexcept
{
    // size_t indexSize = varEntryLength + memberID + lengthGroupName +
    // groupName + lengthVariableName + lengthOfPath + path + datatype
    size_t indexSize = 23; // without characteristics
    indexSize += variable.m_Name.size();

    // characteristics 3 and 4, check variable number of dimensions
    const size_t dimensions = variable.m_Count.size();
    indexSize += 28 * dimensions; // 28 bytes per dimension
    indexSize += 1;               // id

    // characteristics, offset + payload offset in data
    indexSize += 2 * (1 + 8);
    // characteristic 0, if scalar add value, for now only allowing string
    if (dimensions == 1)
    {
        indexSize += sizeof(T);
        indexSize += 1; // id
        // must have an if here
        indexSize += 2 + variable.m_Name.size();
        indexSize += 1; // id
    }

    // characteristic statistics
    if (m_Verbosity == 0) // default, only min and max
    {
        indexSize += 2 * (sizeof(T) + 1);
        indexSize += 1 + 1; // id
    }

    return indexSize + 12; // extra 12 bytes in case of attributes
    // need to add transform characteristics
}

} // end namespace format
} // end namespace adios

#endif /* ADIOS2_TOOLKIT_FORMAT_BP1_BP1BASE_TCC_ */
