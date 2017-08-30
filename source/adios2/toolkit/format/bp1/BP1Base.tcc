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

#include "adios2/helper/adiosFunctions.h" //NextExponentialSize, CopyFromBuffer

namespace adios2
{
namespace format
{

template <class T>
BP1Base::ResizeResult BP1Base::ResizeBuffer(const Variable<T> &variable)
{
    size_t currentCapacity = m_Data.m_Buffer.capacity();
    size_t variableData =
        GetVariableIndexSize(variable) + variable.PayLoadSize();
    size_t requiredCapacity = variableData + m_Data.m_Position;

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
            m_Data.Resize(m_MaxBufferSize, " when resizing buffer to " +
                                               std::to_string(m_MaxBufferSize) +
                                               "bytes, in call to variable " +
                                               variable.m_Name + " Write");
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
            m_Data.Resize(nextSize, " when resizing buffer to " +
                                        std::to_string(nextSize) +
                                        "bytes, in call to variable " +
                                        variable.m_Name + " Write");
            result = ResizeResult::Success;
        }
    }

    return result;
}

// PROTECTED
template <>
int8_t BP1Base::GetDataType<std::string>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_string);
    return type;
}

template <>
int8_t BP1Base::GetDataType<char>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_byte);
    return type;
}

template <>
int8_t BP1Base::GetDataType<signed char>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_byte);
    return type;
}

template <>
int8_t BP1Base::GetDataType<short>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_short);
    return type;
}

template <>
int8_t BP1Base::GetDataType<int>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_integer);
    return type;
}

template <>
int8_t BP1Base::GetDataType<long int>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_long);
    return type;
}

template <>
int8_t BP1Base::GetDataType<long long int>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_long);
    return type;
}

template <>
int8_t BP1Base::GetDataType<unsigned char>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_unsigned_byte);
    return type;
}

template <>
int8_t BP1Base::GetDataType<unsigned short>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_unsigned_short);
    return type;
}

template <>
int8_t BP1Base::GetDataType<unsigned int>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_unsigned_integer);
    return type;
}

template <>
int8_t BP1Base::GetDataType<unsigned long int>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_unsigned_long);
    return type;
}

template <>
int8_t BP1Base::GetDataType<unsigned long long int>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_unsigned_long);
    return type;
}

template <>
int8_t BP1Base::GetDataType<float>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_real);
    return type;
}

template <>
int8_t BP1Base::GetDataType<double>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_double);
    return type;
}

template <>
int8_t BP1Base::GetDataType<long double>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_long_double);
    return type;
}

template <>
int8_t BP1Base::GetDataType<cfloat>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_complex);
    return type;
}

template <>
int8_t BP1Base::GetDataType<cdouble>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_double_complex);
    return type;
}

template <>
int8_t BP1Base::GetDataType<cldouble>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_long_double_complex);
    return type;
}

template <class T>
BP1Base::Characteristics<T>
BP1Base::ReadElementIndexCharacteristics(const std::vector<char> &buffer,
                                         size_t &position,
                                         const bool untilTimeStep) const
{
    Characteristics<T> characteristics;
    characteristics.Count = ReadValue<uint8_t>(buffer, position);
    characteristics.Length = ReadValue<uint32_t>(buffer, position);

    bool foundTimeStep = false;

    while (position < characteristics.Length + 5)
    {
        const uint8_t id = ReadValue<uint8_t>(buffer, position);

        switch (id)
        {
        case (characteristic_time_index):
            characteristics.Statistics.TimeStep =
                ReadValue<uint32_t>(buffer, position);
            foundTimeStep = true;
            break;

        case (characteristic_file_index):
            characteristics.Statistics.FileIndex =
                ReadValue<uint32_t>(buffer, position);
            break;

        case (characteristic_value):
            // TODO make sure it's string or string array
            characteristics.Statistics.Min = ReadValue<T>(buffer, position);
            break;

        case (characteristic_min):
            characteristics.Statistics.Min = ReadValue<T>(buffer, position);
            break;

        case (characteristic_max):
            characteristics.Statistics.Max = ReadValue<T>(buffer, position);
            break;

        case (characteristic_offset):
            characteristics.Statistics.Offset =
                ReadValue<uint64_t>(buffer, position);
            break;

        case (characteristic_payload_offset):
            characteristics.Statistics.PayloadOffset =
                ReadValue<uint64_t>(buffer, position);
            break;

        case (characteristic_dimensions):
            const uint8_t dimensionsCount =
                ReadValue<uint8_t>(buffer, position);
            characteristics.Dimensions.reserve(dimensionsCount * 3);

            ReadValue<uint16_t>(buffer, position); // length (not used)

            for (auto d = 0; d < dimensionsCount * 3; ++d)
            {
                characteristics.Dimensions.push_back(
                    ReadValue<uint64_t>(buffer, position));
            }
            break;
            // TODO: implement compression and BP1 Stats characteristics
        }

        if (untilTimeStep && foundTimeStep)
        {
            break;
        }
    }

    return characteristics;
}

// PRIVATE
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
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BP1_BP1BASE_TCC_ */
