/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP3Serializer.tcc
 *
 *  Created on: Apr 11, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP3_BP3Serializer_TCC_
#define ADIOS2_TOOLKIT_FORMAT_BP3_BP3Serializer_TCC_

#include "BP3Serializer.h"

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace format
{

template <class T>
inline void
BP3Serializer::PutVariableMetadata(const Variable<T> &variable) noexcept
{
    auto lf_SetOffset = [&](uint64_t &offset) {
        if (m_Aggregator.m_IsActive && !m_Aggregator.m_IsConsumer)
        {
            offset = static_cast<uint64_t>(m_Data.m_Position);
        }
        else
        {
            offset = static_cast<uint64_t>(m_Data.m_AbsolutePosition);
        }
    };

    ProfilerStart("buffering");

    Stats<typename TypeInfo<T>::ValueType> stats = GetStats<T>(variable);

    // Get new Index or point to existing index
    bool isNew = true; // flag to check if variable is new
    SerialElementIndex &variableIndex = GetSerialElementIndex(
        variable.m_Name, m_MetadataSet.VarsIndices, isNew);
    stats.MemberID = variableIndex.MemberID;

    lf_SetOffset(stats.Offset);
    PutVariableMetadataInData(variable, stats);
    lf_SetOffset(stats.PayloadOffset);

    // write to metadata  index
    PutVariableMetadataInIndex(variable, stats, isNew, variableIndex);
    ++m_MetadataSet.DataPGVarsCount;

    ProfilerStop("buffering");
}

template <class T>
inline void
BP3Serializer::PutVariablePayload(const Variable<T> &variable) noexcept
{
    ProfilerStart("buffering");
    PutPayloadInBuffer(variable);
    ProfilerStop("buffering");
}

// PRIVATE
template <class T>
size_t BP3Serializer::PutAttributeHeaderInData(const Attribute<T> &attribute,
                                               Stats<T> &stats) noexcept
{
    auto &buffer = m_Data.m_Buffer;
    auto &position = m_Data.m_Position;

    // will go back to write length
    const size_t attributeLengthPosition = position;
    position += 4; // skip length

    CopyToBuffer(buffer, position, &stats.MemberID);
    PutNameRecord(attribute.m_Name, buffer, position);
    position += 2; // skip path

    // TODO: attribute from Variable??
    constexpr int8_t no = 'n';
    CopyToBuffer(buffer, position, &no); // not associated with a Variable

    return attributeLengthPosition;
}

template <class T>
void BP3Serializer::PutAttributeLengthInData(
    const Attribute<T> &attribute, Stats<T> &stats,
    const size_t attributeLengthPosition) noexcept
{
    auto &buffer = m_Data.m_Buffer;
    auto &position = m_Data.m_Position;
    auto &absolutePosition = m_Data.m_AbsolutePosition;

    // back to attribute length
    size_t backPosition = attributeLengthPosition;
    CopyToBuffer(buffer, backPosition, &attributeLengthPosition);

    absolutePosition += position - attributeLengthPosition;
}

template <>
inline void
BP3Serializer::PutAttributeInData(const Attribute<std::string> &attribute,
                                  Stats<std::string> &stats) noexcept
{
    const size_t attributeLengthPosition =
        PutAttributeHeaderInData(attribute, stats);

    auto &buffer = m_Data.m_Buffer;
    auto &position = m_Data.m_Position;
    auto &absolutePosition = m_Data.m_AbsolutePosition;

    uint8_t dataType = GetDataType<std::string>();
    if (!attribute.m_IsSingleValue)
    {
        dataType = type_string_array;
    }
    CopyToBuffer(buffer, position, &dataType);

    // here record payload offset
    stats.PayloadOffset = absolutePosition + position - attributeLengthPosition;

    if (dataType == type_string)
    {
        const uint32_t dataSize =
            static_cast<uint32_t>(attribute.m_DataSingleValue.size());
        CopyToBuffer(buffer, position, &dataSize);
        CopyToBuffer(buffer, position, attribute.m_DataSingleValue.data(),
                     attribute.m_DataSingleValue.size());
    }
    else if (dataType == type_string_array)
    {
        const uint32_t elements = static_cast<uint32_t>(attribute.m_Elements);
        CopyToBuffer(buffer, position, &elements);

        for (size_t s = 0; s < attribute.m_Elements; ++s)
        {
            // include zero terminated
            const std::string element(attribute.m_DataArray[s] + '\0');

            const uint32_t elementSize = static_cast<uint32_t>(element.size());

            CopyToBuffer(buffer, position, &elementSize);
            CopyToBuffer(buffer, position, element.data(), element.size());
        }
    }

    PutAttributeLengthInData(attribute, stats, attributeLengthPosition);
}

template <class T>
void BP3Serializer::PutAttributeInData(const Attribute<T> &attribute,
                                       Stats<T> &stats) noexcept
{
    const size_t attributeLengthPosition =
        PutAttributeHeaderInData(attribute, stats);

    auto &buffer = m_Data.m_Buffer;
    auto &position = m_Data.m_Position;
    auto &absolutePosition = m_Data.m_AbsolutePosition;

    uint8_t dataType = GetDataType<T>();
    CopyToBuffer(buffer, position, &dataType);

    // here record payload offset
    stats.PayloadOffset = absolutePosition + position - attributeLengthPosition;

    const uint32_t dataSize =
        static_cast<uint32_t>(attribute.m_Elements * sizeof(T));
    CopyToBuffer(buffer, position, &dataSize);

    if (attribute.m_IsSingleValue) // single value
    {
        CopyToBuffer(buffer, position, &attribute.m_DataSingleValue);
    }
    else // array
    {
        CopyToBuffer(buffer, position, attribute.m_DataArray.data(),
                     attribute.m_Elements);
    }

    PutAttributeLengthInData(attribute, stats, attributeLengthPosition);
}

template <>
inline void BP3Serializer::PutAttributeCharacteristicValueInIndex(
    uint8_t &characteristicsCounter, const Attribute<std::string> &attribute,
    std::vector<char> &buffer) noexcept
{
    const uint8_t characteristicID =
        static_cast<uint8_t>(CharacteristicID::characteristic_value);

    InsertToBuffer(buffer, &characteristicID);

    if (attribute.m_IsSingleValue) // Single string
    {
        const uint16_t dataSize =
            static_cast<uint16_t>(attribute.m_DataSingleValue.size());
        InsertToBuffer(buffer, &dataSize);
        InsertToBuffer(buffer, attribute.m_DataSingleValue.data(),
                       attribute.m_DataSingleValue.size());
    }
    else // string array
    {
        for (size_t s = 0; s < attribute.m_Elements; ++s)
        {
            // without zero terminated character
            const std::string element(attribute.m_DataArray[s]);

            const uint16_t elementSize = static_cast<uint16_t>(element.size());

            InsertToBuffer(buffer, &elementSize);
            InsertToBuffer(buffer, element.data(), element.size());
        }
    }
    ++characteristicsCounter;
}

template <class T>
void BP3Serializer::PutAttributeCharacteristicValueInIndex(
    uint8_t &characteristicsCounter, const Attribute<T> &attribute,
    std::vector<char> &buffer) noexcept
{
    const uint8_t characteristicID = CharacteristicID::characteristic_value;

    InsertToBuffer(buffer, &characteristicID);

    if (attribute.m_IsSingleValue) // single value
    {
        InsertToBuffer(buffer, &attribute.m_DataSingleValue);
    }
    else // array
    {
        InsertToBuffer(buffer, attribute.m_DataArray.data(),
                       attribute.m_Elements);
    }
    ++characteristicsCounter;
}

template <class T>
void BP3Serializer::PutAttributeInIndex(const Attribute<T> &attribute,
                                        const Stats<T> &stats) noexcept
{
    SerialElementIndex index(stats.MemberID);
    auto &buffer = index.Buffer;

    buffer.insert(buffer.end(), 4, '\0'); // skip attribute length (4)
    InsertToBuffer(buffer, &stats.MemberID);
    buffer.insert(buffer.end(), 2, '\0'); // skip group name
    PutNameRecord(attribute.m_Name, buffer);
    buffer.insert(buffer.end(), 2, '\0'); // skip path

    uint8_t dataType = GetDataType<T>(); // dataType

    if (dataType == type_string && !attribute.m_IsSingleValue)
    {
        dataType = type_string_array;
    }

    InsertToBuffer(buffer, &dataType);

    // Characteristics Sets Count in Metadata
    index.Count = 1;
    InsertToBuffer(buffer, &index.Count);

    // START OF CHARACTERISTICS
    const size_t characteristicsCountPosition = buffer.size();
    // skip characteristics count(1) + length (4)
    buffer.insert(buffer.end(), 5, '\0');
    uint8_t characteristicsCounter = 0;

    // DIMENSIONS
    PutCharacteristicRecord(characteristic_time_index, characteristicsCounter,
                            stats.Step, buffer);

    PutCharacteristicRecord(characteristic_file_index, characteristicsCounter,
                            stats.FileIndex, buffer);

    uint8_t characteristicID = characteristic_dimensions;
    InsertToBuffer(buffer, &characteristicID);
    constexpr uint8_t dimensions = 1;
    InsertToBuffer(buffer, &dimensions); // count
    constexpr uint16_t dimensionsLength = 24;
    InsertToBuffer(buffer, &dimensionsLength); // length
    PutDimensionsRecord({attribute.m_Elements}, {}, {}, buffer);
    ++characteristicsCounter;

    // VALUE
    PutAttributeCharacteristicValueInIndex(characteristicsCounter, attribute,
                                           buffer);

    PutCharacteristicRecord(characteristic_offset, characteristicsCounter,
                            stats.Offset, buffer);

    PutCharacteristicRecord(characteristic_payload_offset,
                            characteristicsCounter, stats.PayloadOffset,
                            buffer);
    // END OF CHARACTERISTICS

    // Back to characteristics count and length
    size_t backPosition = characteristicsCountPosition;
    CopyToBuffer(buffer, backPosition, &characteristicsCounter); // count (1)

    // remove its own length (4) + characteristic counter (1)
    const uint32_t characteristicsLength = static_cast<uint32_t>(
        buffer.size() - characteristicsCountPosition - 4 - 1);

    CopyToBuffer(buffer, backPosition, &characteristicsLength); // length

    // Finish characteristic count length
    m_MetadataSet.AttributesIndices.emplace(attribute.m_Name, index);
}

template <>
inline BP3Serializer::Stats<typename TypeInfo<std::string>::ValueType>
BP3Serializer::GetStats(const Variable<std::string> &variable) noexcept
{
    Stats<typename TypeInfo<std::string>::ValueType> stats;
    stats.Step = m_MetadataSet.TimeStep;
    stats.FileIndex = GetFileIndex();
    return stats;
}

template <class T>
BP3Serializer::Stats<typename TypeInfo<T>::ValueType>
BP3Serializer::GetStats(const Variable<T> &variable) noexcept
{
    Stats<typename TypeInfo<T>::ValueType> stats;
    const std::size_t valuesSize = variable.TotalSize();

    if (m_Verbosity == 0)
    {
        ProfilerStart("minmax");
        GetMinMaxThreads(variable.GetData(), valuesSize, stats.Min, stats.Max,
                         m_Threads);
        ProfilerStop("minmax");
    }

    stats.Step = m_MetadataSet.TimeStep;
    stats.FileIndex = GetFileIndex();
    return stats;
}

template <class T>
void BP3Serializer::PutVariableMetadataInData(
    const Variable<T> &variable,
    const Stats<typename TypeInfo<T>::ValueType> &stats) noexcept
{
    auto &buffer = m_Data.m_Buffer;
    auto &position = m_Data.m_Position;
    auto &absolutePosition = m_Data.m_AbsolutePosition;

    // for writing length at the end
    const size_t varLengthPosition = position;
    position += 8; // skip var length (8)

    CopyToBuffer(buffer, position, &stats.MemberID);

    PutNameRecord(variable.m_Name, buffer, position);
    position += 2; // skip path

    const uint8_t dataType = GetDataType<T>();
    CopyToBuffer(buffer, position, &dataType);

    constexpr char no = 'n'; // isDimension
    CopyToBuffer(buffer, position, &no);

    const uint8_t dimensions = static_cast<uint8_t>(variable.m_Count.size());
    CopyToBuffer(buffer, position, &dimensions); // count

    // 27 is from 9 bytes for each: var y/n + local, var y/n + global dimension,
    // var y/n + global offset, changed for characteristic
    uint16_t dimensionsLength = 27 * dimensions;
    CopyToBuffer(buffer, position, &dimensionsLength); // length

    PutDimensionsRecord(variable.m_Count, variable.m_Shape, variable.m_Start,
                        buffer, position);

    // CHARACTERISTICS
    PutVariableCharacteristics(variable, stats, buffer, position);

    // Back to varLength including payload size
    // not need to remove its own size (8) from length from bpdump
    const uint64_t varLength = static_cast<uint64_t>(
        position - varLengthPosition + variable.PayloadSize());

    size_t backPosition = varLengthPosition;
    CopyToBuffer(buffer, backPosition, &varLength);

    absolutePosition += position - varLengthPosition;
}

template <>
inline void BP3Serializer::PutVariableMetadataInData(
    const Variable<std::string> &variable,
    const Stats<typename TypeInfo<std::string>::ValueType> &stats) noexcept
{
    auto &buffer = m_Data.m_Buffer;
    auto &position = m_Data.m_Position;
    auto &absolutePosition = m_Data.m_AbsolutePosition;

    // for writing length at the end
    const size_t varLengthPosition = position;
    position += 8; // skip var length (8)

    CopyToBuffer(buffer, position, &stats.MemberID);

    PutNameRecord(variable.m_Name, buffer, position);
    position += 2; // skip path

    const uint8_t dataType = GetDataType<std::string>();
    CopyToBuffer(buffer, position, &dataType);

    constexpr char no = 'n'; // isDimension
    CopyToBuffer(buffer, position, &no);

    const uint8_t dimensions = static_cast<uint8_t>(variable.m_Count.size());
    CopyToBuffer(buffer, position, &dimensions); // count

    uint16_t dimensionsLength = 27 * dimensions;
    CopyToBuffer(buffer, position, &dimensionsLength); // length

    PutDimensionsRecord(variable.m_Count, variable.m_Shape, variable.m_Start,
                        buffer, position);

    position += 5; // skipping characteristics

    // Back to varLength including payload size
    // not need to remove its own size (8) from length from bpdump
    const uint64_t varLength = static_cast<uint64_t>(
        position - varLengthPosition + variable.GetData()->size() + 2);

    size_t backPosition = varLengthPosition;
    CopyToBuffer(buffer, backPosition, &varLength);

    absolutePosition += position - varLengthPosition;
}

template <class T>
void BP3Serializer::PutVariableMetadataInIndex(
    const Variable<T> &variable,
    const Stats<typename TypeInfo<T>::ValueType> &stats, const bool isNew,
    SerialElementIndex &index) noexcept
{
    auto &buffer = index.Buffer;

    if (isNew) // write variable header
    {
        buffer.insert(buffer.end(), 4, '\0'); // skip var length (4)
        InsertToBuffer(buffer, &stats.MemberID);
        buffer.insert(buffer.end(), 2, '\0'); // skip group name
        PutNameRecord(variable.m_Name, buffer);
        buffer.insert(buffer.end(), 2, '\0'); // skip path

        const uint8_t dataType = GetDataType<T>();
        InsertToBuffer(buffer, &dataType);

        // Characteristics Sets Count in Metadata
        index.Count = 1;
        InsertToBuffer(buffer, &index.Count);

        // For updating absolute offsets in agreggation
        index.LastUpdatedPosition = buffer.size();
    }
    else // update characteristics sets count
    {
        if (m_Verbosity == 0)
        {
            ++index.Count;
            // fixed since group and path are not printed
            size_t setsCountPosition = 15 + variable.m_Name.size();
            CopyToBuffer(buffer, setsCountPosition, &index.Count);
        }
    }

    PutVariableCharacteristics(variable, stats, buffer);
}

template <class T>
void BP3Serializer::PutBoundsRecord(const bool isScalar, const Stats<T> &stats,
                                    uint8_t &characteristicsCounter,
                                    std::vector<char> &buffer) noexcept
{
    if (isScalar)
    {
        PutCharacteristicRecord(characteristic_value, characteristicsCounter,
                                stats.Min, buffer);
    }
    else
    {
        if (m_Verbosity == 0) // default verbose
        {
            PutCharacteristicRecord(characteristic_min, characteristicsCounter,
                                    stats.Min, buffer);

            PutCharacteristicRecord(characteristic_max, characteristicsCounter,
                                    stats.Max, buffer);
        }
    }
}

template <class T>
void BP3Serializer::PutBoundsRecord(const bool singleValue,
                                    const Stats<T> &stats,
                                    uint8_t &characteristicsCounter,
                                    std::vector<char> &buffer,
                                    size_t &position) noexcept
{
    if (singleValue)
    {
        const uint8_t id = characteristic_value;
        CopyToBuffer(buffer, position, &id);
        // special case required by bpdump
        const uint16_t length = sizeof(T);
        CopyToBuffer(buffer, position, &length);
        CopyToBuffer(buffer, position, &stats.Min);
        ++characteristicsCounter;
    }
    else
    {
        if (m_Verbosity == 0) // default min and max only
        {
            PutCharacteristicRecord(characteristic_min, characteristicsCounter,
                                    stats.Min, buffer, position);

            PutCharacteristicRecord(characteristic_max, characteristicsCounter,
                                    stats.Max, buffer, position);
        }
    }
}

template <class T>
void BP3Serializer::PutCharacteristicRecord(const uint8_t characteristicID,
                                            uint8_t &characteristicsCounter,
                                            const T &value,
                                            std::vector<char> &buffer) noexcept
{
    const uint8_t id = characteristicID;
    InsertToBuffer(buffer, &id);
    InsertToBuffer(buffer, &value);
    ++characteristicsCounter;
}

template <class T>
void BP3Serializer::PutCharacteristicRecord(const uint8_t characteristicID,
                                            uint8_t &characteristicsCounter,
                                            const T &value,
                                            std::vector<char> &buffer,
                                            size_t &position) noexcept
{
    const uint8_t id = characteristicID;
    CopyToBuffer(buffer, position, &id);
    CopyToBuffer(buffer, position, &value);
    ++characteristicsCounter;
}

template <>
inline void BP3Serializer::PutVariableCharacteristics(
    const Variable<std::string> &variable,
    const Stats<typename TypeInfo<std::string>::ValueType> &stats,
    std::vector<char> &buffer) noexcept
{
    const size_t characteristicsCountPosition = buffer.size();
    // skip characteristics count(1) + length (4)
    buffer.insert(buffer.end(), 5, '\0');
    uint8_t characteristicsCounter = 0;

    PutCharacteristicRecord(characteristic_time_index, characteristicsCounter,
                            stats.Step, buffer);

    PutCharacteristicRecord(characteristic_file_index, characteristicsCounter,
                            stats.FileIndex, buffer);

    uint8_t characteristicID = characteristic_value;
    InsertToBuffer(buffer, &characteristicID);
    PutNameRecord(*variable.GetData(), buffer);
    ++characteristicsCounter;

    // TODO: keep dimensions or not
    characteristicID = characteristic_dimensions;
    InsertToBuffer(buffer, &characteristicID);
    const uint8_t dimensions = static_cast<uint8_t>(variable.m_Count.size());
    InsertToBuffer(buffer, &dimensions); // count
    const uint16_t dimensionsLength = static_cast<uint16_t>(24 * dimensions);
    InsertToBuffer(buffer, &dimensionsLength); // length
    PutDimensionsRecord(variable.m_Count, variable.m_Shape, variable.m_Start,
                        buffer);
    ++characteristicsCounter;

    PutCharacteristicRecord(characteristic_offset, characteristicsCounter,
                            stats.Offset, buffer);

    PutCharacteristicRecord(characteristic_payload_offset,
                            characteristicsCounter, stats.PayloadOffset,
                            buffer);
    // END OF CHARACTERISTICS

    // Back to characteristics count and length
    size_t backPosition = characteristicsCountPosition;
    CopyToBuffer(buffer, backPosition, &characteristicsCounter); // count (1)

    // remove its own length (4) + characteristic counter (1)
    const uint32_t characteristicsLength = static_cast<uint32_t>(
        buffer.size() - characteristicsCountPosition - 4 - 1);

    CopyToBuffer(buffer, backPosition, &characteristicsLength); // length
}

template <class T>
void BP3Serializer::PutVariableCharacteristics(
    const Variable<T> &variable,
    const Stats<typename TypeInfo<T>::ValueType> &stats,
    std::vector<char> &buffer) noexcept
{
    // going back at the end
    const size_t characteristicsCountPosition = buffer.size();
    // skip characteristics count(1) + length (4)
    buffer.insert(buffer.end(), 5, '\0');
    uint8_t characteristicsCounter = 0;

    // DIMENSIONS
    PutCharacteristicRecord(characteristic_time_index, characteristicsCounter,
                            stats.Step, buffer);

    PutCharacteristicRecord(characteristic_file_index, characteristicsCounter,
                            stats.FileIndex, buffer);

    PutBoundsRecord(variable.m_SingleValue, stats, characteristicsCounter,
                    buffer);

    const uint8_t characteristicID = characteristic_dimensions;
    InsertToBuffer(buffer, &characteristicID);
    const uint8_t dimensions = static_cast<uint8_t>(variable.m_Count.size());
    InsertToBuffer(buffer, &dimensions); // count
    const uint16_t dimensionsLength = static_cast<uint16_t>(24 * dimensions);
    InsertToBuffer(buffer, &dimensionsLength); // length
    PutDimensionsRecord(variable.m_Count, variable.m_Shape, variable.m_Start,
                        buffer);
    ++characteristicsCounter;

    PutCharacteristicRecord(characteristic_offset, characteristicsCounter,
                            stats.Offset, buffer);

    PutCharacteristicRecord(characteristic_payload_offset,
                            characteristicsCounter, stats.PayloadOffset,
                            buffer);
    // END OF CHARACTERISTICS

    // Back to characteristics count and length
    size_t backPosition = characteristicsCountPosition;
    CopyToBuffer(buffer, backPosition, &characteristicsCounter); // count (1)

    // remove its own length (4) + characteristic counter (1)
    const uint32_t characteristicsLength = static_cast<uint32_t>(
        buffer.size() - characteristicsCountPosition - 4 - 1);

    CopyToBuffer(buffer, backPosition, &characteristicsLength); // length
}

template <class T>
void BP3Serializer::PutVariableCharacteristics(
    const Variable<T> &variable,
    const Stats<typename TypeInfo<T>::ValueType> &stats,
    std::vector<char> &buffer, size_t &position) noexcept
{
    // going back at the end
    const size_t characteristicsCountPosition = position;
    // skip characteristics count(1) + length (4)
    position += 5;
    uint8_t characteristicsCounter = 0;

    // DIMENSIONS
    uint8_t characteristicID = characteristic_dimensions;
    CopyToBuffer(buffer, position, &characteristicID);

    const uint8_t dimensions = static_cast<uint8_t>(variable.m_Count.size());
    CopyToBuffer(buffer, position, &dimensions); // count
    const uint16_t dimensionsLength = static_cast<uint16_t>(24 * dimensions);
    CopyToBuffer(buffer, position, &dimensionsLength); // length
    PutDimensionsRecord(variable.m_Count, variable.m_Shape, variable.m_Start,
                        buffer, position, true); // isCharacteristic = true
    ++characteristicsCounter;

    // VALUE for SCALAR or STAT min, max for ARRAY
    PutBoundsRecord(variable.m_SingleValue, stats, characteristicsCounter,
                    buffer, position);
    // END OF CHARACTERISTICS

    // Back to characteristics count and length
    size_t backPosition = characteristicsCountPosition;
    CopyToBuffer(buffer, backPosition, &characteristicsCounter);

    // remove its own length (4) + characteristic counter (1)
    const uint32_t characteristicsLength =
        static_cast<uint32_t>(position - characteristicsCountPosition - 4 - 1);
    CopyToBuffer(buffer, backPosition, &characteristicsLength);
}

template <>
inline void BP3Serializer::PutPayloadInBuffer(
    const Variable<std::string> &variable) noexcept
{
    PutNameRecord(*variable.GetData(), m_Data.m_Buffer, m_Data.m_Position);
    m_Data.m_AbsolutePosition += variable.GetData()->size() + 2;
}

template <class T>
void BP3Serializer::PutPayloadInBuffer(const Variable<T> &variable) noexcept
{
    ProfilerStart("memcpy");
    CopyToBufferThreads(m_Data.m_Buffer, m_Data.m_Position, variable.GetData(),
                        variable.TotalSize(), m_Threads);
    ProfilerStop("memcpy");
    m_Data.m_AbsolutePosition += variable.PayloadSize();
}

template <class T>
void BP3Serializer::UpdateIndexOffsetsCharacteristics(size_t &currentPosition,
                                                      const DataTypes dataType,
                                                      std::vector<char> &buffer)
{
    const uint8_t characteristicsCount =
        ReadValue<uint8_t>(buffer, currentPosition);

    const uint32_t characteristicsLength =
        ReadValue<uint32_t>(buffer, currentPosition);

    const size_t endPosition =
        currentPosition + static_cast<size_t>(characteristicsLength);

    while (currentPosition < endPosition)
    {
        const uint8_t id = ReadValue<uint8_t>(buffer, currentPosition);

        switch (id)
        {
        case (characteristic_time_index):
        {
            currentPosition += sizeof(uint32_t);
            break;
        }

        case (characteristic_file_index):
        {
            currentPosition += sizeof(uint32_t);
            break;
        }

        case (characteristic_value):
        {
            if (dataType == type_string)
            {
                // first get the length of the string
                const size_t length = static_cast<size_t>(
                    ReadValue<uint16_t>(buffer, currentPosition));

                currentPosition += length;
            }
            // using this function only for variables
            // TODO string array if string arrays are supported in the future
            else
            {
                currentPosition += sizeof(T);
            }

            break;
        }
        case (characteristic_min):
        {
            currentPosition += sizeof(typename TypeInfo<T>::ValueType);
            break;
        }
        case (characteristic_max):
        {
            currentPosition += sizeof(typename TypeInfo<T>::ValueType);
            break;
        }
        case (characteristic_offset):
        {
            const uint64_t currentOffset =
                ReadValue<uint64_t>(buffer, currentPosition);

            const uint64_t updatedOffset =
                currentOffset +
                static_cast<uint64_t>(m_Data.m_AbsolutePosition);

            currentPosition -= sizeof(uint64_t);
            CopyToBuffer(buffer, currentPosition, &updatedOffset);
            break;
        }
        case (characteristic_payload_offset):
        {
            const uint64_t currentPayloadOffset =
                ReadValue<uint64_t>(buffer, currentPosition);

            const uint64_t updatedPayloadOffset =
                currentPayloadOffset +
                static_cast<uint64_t>(m_Data.m_AbsolutePosition);

            currentPosition -= sizeof(uint64_t);
            CopyToBuffer(buffer, currentPosition, &updatedPayloadOffset);
            break;
        }
        case (characteristic_dimensions):
        {
            const size_t dimensionsSize = static_cast<size_t>(
                ReadValue<uint8_t>(buffer, currentPosition));

            currentPosition +=
                3 * sizeof(uint64_t) * dimensionsSize + 2; // 2 is for length
            break;
        }
        // TODO: implement operators
        default:
        {
            throw std::invalid_argument(
                "ERROR: characteristic ID " + std::to_string(id) +
                " not supported when updating offsets\n");
        }

        } // end id switch
    }     // end while
}

template <>
inline size_t BP3Serializer::GetAttributeSizeInData(
    const Attribute<std::string> &attribute) const noexcept
{
    // index header
    size_t size = 14 + attribute.m_Name.size() + 10;

    if (attribute.m_IsSingleValue)
    {
        size += 4 + attribute.m_DataSingleValue.size();
    }
    else
    {
        size += 4;
        for (const auto &dataString : attribute.m_DataArray)
        {
            size += 4 + dataString.size();
        }
    }
    return size;
}

template <class T>
size_t
BP3Serializer::GetAttributeSizeInData(const Attribute<T> &attribute) const
    noexcept
{
    size_t size = 14 + attribute.m_Name.size() + 10;
    size += 4 + sizeof(T) * attribute.m_Elements;
}

} // end namespace format
} // end namespace adios2

#endif // ADIOS2_TOOLKIT_FORMAT_BP3_BP3Serializer_TCC_
