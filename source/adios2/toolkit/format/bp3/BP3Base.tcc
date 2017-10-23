/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP3Base.tcc
 *
 *  Created on: May 19, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP1_BP3BASE_TCC_
#define ADIOS2_TOOLKIT_FORMAT_BP1_BP3BASE_TCC_

#include "BP3Base.h"

#include <cmath> //std::min

#include "adios2/helper/adiosFunctions.h" //NextExponentialSize, CopyFromBuffer

namespace adios2
{
namespace format
{

// PROTECTED
template <>
int8_t BP3Base::GetDataType<std::string>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_string);
    return type;
}

template <>
int8_t BP3Base::GetDataType<char>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_byte);
    return type;
}

template <>
int8_t BP3Base::GetDataType<signed char>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_byte);
    return type;
}

template <>
int8_t BP3Base::GetDataType<short>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_short);
    return type;
}

template <>
int8_t BP3Base::GetDataType<int>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_integer);
    return type;
}

template <>
int8_t BP3Base::GetDataType<long int>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_long);
    return type;
}

template <>
int8_t BP3Base::GetDataType<long long int>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_long);
    return type;
}

template <>
int8_t BP3Base::GetDataType<unsigned char>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_unsigned_byte);
    return type;
}

template <>
int8_t BP3Base::GetDataType<unsigned short>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_unsigned_short);
    return type;
}

template <>
int8_t BP3Base::GetDataType<unsigned int>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_unsigned_integer);
    return type;
}

template <>
int8_t BP3Base::GetDataType<unsigned long int>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_unsigned_long);
    return type;
}

template <>
int8_t BP3Base::GetDataType<unsigned long long int>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_unsigned_long);
    return type;
}

template <>
int8_t BP3Base::GetDataType<float>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_real);
    return type;
}

template <>
int8_t BP3Base::GetDataType<double>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_double);
    return type;
}

template <>
int8_t BP3Base::GetDataType<long double>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_long_double);
    return type;
}

template <>
int8_t BP3Base::GetDataType<cfloat>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_complex);
    return type;
}

template <>
int8_t BP3Base::GetDataType<cdouble>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_double_complex);
    return type;
}

template <>
int8_t BP3Base::GetDataType<cldouble>() const noexcept
{
    const int8_t type = static_cast<const int8_t>(type_long_double_complex);
    return type;
}

template <class T>
BP3Base::Characteristics<T>
BP3Base::ReadElementIndexCharacteristics(const std::vector<char> &buffer,
                                         size_t &position,
                                         const bool untilTimeStep) const
{
    Characteristics<T> characteristics;
    characteristics.EntryCount = ReadValue<uint8_t>(buffer, position);
    characteristics.EntryLength = ReadValue<uint32_t>(buffer, position);

    const size_t start = position;
    size_t localPosition = 0;

    bool foundTimeStep = false;

    while (localPosition < characteristics.EntryLength)
    {
        const uint8_t id = ReadValue<uint8_t>(buffer, position);

        switch (id)
        {
        case (characteristic_time_index):
        {
            characteristics.Statistics.Step =
                ReadValue<uint32_t>(buffer, position);
            foundTimeStep = true;
            break;
        }

        case (characteristic_file_index):
        {
            characteristics.Statistics.FileIndex =
                ReadValue<uint32_t>(buffer, position);
            break;
        }

        case (characteristic_value):
        { // TODO make sure it's string or string array
            characteristics.Statistics.Min = ReadValue<T>(buffer, position);
            break;
        }

        case (characteristic_min):
        {
            characteristics.Statistics.Min = ReadValue<T>(buffer, position);
            break;
        }

        case (characteristic_max):
        {
            characteristics.Statistics.Max = ReadValue<T>(buffer, position);
            break;
        }

        case (characteristic_offset):
        {
            characteristics.Statistics.Offset =
                ReadValue<uint64_t>(buffer, position);
            break;
        }

        case (characteristic_payload_offset):
        {
            characteristics.Statistics.PayloadOffset =
                ReadValue<uint64_t>(buffer, position);
            break;
        }

        case (characteristic_dimensions):
        {
            const unsigned int dimensionsSize =
                static_cast<unsigned int>(ReadValue<uint8_t>(buffer, position));

            characteristics.Shape.reserve(dimensionsSize);
            characteristics.Start.reserve(dimensionsSize);
            characteristics.Count.reserve(dimensionsSize);
            position += 2; // skip length (not required)

            for (unsigned int d = 0; d < dimensionsSize; ++d)
            {
                characteristics.Count.push_back(
                    static_cast<size_t>(ReadValue<uint64_t>(buffer, position)));

                characteristics.Shape.push_back(
                    static_cast<size_t>(ReadValue<uint64_t>(buffer, position)));

                characteristics.Start.push_back(
                    static_cast<size_t>(ReadValue<uint64_t>(buffer, position)));
            }
            break;
        }
        // TODO: implement compression and BP1 Stats characteristics
        default:
        {
            throw std::invalid_argument("ERROR: characteristic ID " +
                                        std::to_string(id) +
                                        " not supported\n");
            break;
        }

        } // end switch

        if (untilTimeStep && foundTimeStep)
        {
            break;
        }

        localPosition = position - start;
    }

    return characteristics;
}

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BP1_BP3Base_TCC_ */
