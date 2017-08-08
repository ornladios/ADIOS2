/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP1Writer.tcc
 *
 *  Created on: Apr 11, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */
#ifndef ADIOS2_TOOLKIT_FORMAT_BP1_BP1WRITER_TCC_
#define ADIOS2_TOOLKIT_FORMAT_BP1_BP1WRITER_TCC_

#include "BP1Writer.h"

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace format
{

template <class T>
void BP1Writer::WriteVariableMetadata(const Variable<T> &variable) noexcept
{
    if (m_Profiler.IsActive)
    {
        m_Profiler.Timers.at("buffering").Resume();
    }

    Stats<typename TypeInfo<T>::ValueType> stats = GetStats(variable);

    stats.TimeIndex = m_MetadataSet.TimeStep;
    // Get new Index or point to existing index
    bool isNew = true; // flag to check if variable is new
    BP1Index &variableIndex =
        GetBP1Index(variable.m_Name, m_MetadataSet.VarsIndices, isNew);
    stats.MemberID = variableIndex.MemberID;

    // write metadata header in data and extract offsets
    stats.Offset = static_cast<uint64_t>(m_HeapBuffer.m_DataAbsolutePosition);
    WriteVariableMetadataInData(variable, stats);
    stats.PayloadOffset = m_HeapBuffer.m_DataAbsolutePosition;

    // write to metadata  index
    WriteVariableMetadataInIndex(variable, stats, isNew, variableIndex);

    ++m_MetadataSet.DataPGVarsCount;

    if (m_Profiler.IsActive)
    {
        m_Profiler.Timers.at("buffering").Pause();
    }
}

template <class T>
void BP1Writer::WriteVariablePayload(const Variable<T> &variable) noexcept
{
    if (m_Profiler.IsActive)
    {
        m_Profiler.Timers.at("buffering").Resume();
    }

    CopyToBufferThreads(m_HeapBuffer.m_Data, m_HeapBuffer.m_DataPosition,
                        variable.m_AppValues, variable.TotalSize(), m_Threads);

    m_HeapBuffer.m_DataAbsolutePosition += variable.PayLoadSize();

    if (m_Profiler.IsActive)
    {
        m_Profiler.Timers.at("buffering").Pause();
    }
}

// PRIVATE
template <class T>
size_t BP1Writer::WriteAttributeHeaderInData(const Attribute<T> &attribute,
                                             Stats<T> &stats) noexcept
{
    auto &buffer = m_HeapBuffer.m_Data;
    auto &position = m_HeapBuffer.m_DataPosition;

    // will go back to write length
    const size_t attributeLengthPosition = position;
    position += 4; // skip length

    CopyToBuffer(buffer, position, &stats.MemberID);
    WriteNameRecord(attribute.m_Name, buffer, position);
    position += 2; // skip path

    // TODO: attribute from Variable??
    constexpr int8_t no = 'n';
    CopyToBuffer(buffer, position, &no); // not associated with a Variable

    return attributeLengthPosition;
}

template <class T>
void BP1Writer::WriteAttributeLengthInData(
    const Attribute<T> &attribute, Stats<T> &stats,
    const size_t attributeLengthPosition) noexcept
{
    auto &buffer = m_HeapBuffer.m_Data;
    auto &position = m_HeapBuffer.m_DataPosition;

    // back to attribute length
    const uint32_t attributeLength =
        static_cast<const uint32_t>(position - attributeLengthPosition);
    size_t backPosition = attributeLengthPosition;
    CopyToBuffer(buffer, backPosition, &attributeLengthPosition);

    m_HeapBuffer.m_DataAbsolutePosition += position - attributeLengthPosition;
}

template <>
inline void
BP1Writer::WriteAttributeInData(const Attribute<std::string> &attribute,
                                Stats<std::string> &stats) noexcept
{
    const size_t attributeLengthPosition =
        WriteAttributeHeaderInData(attribute, stats);

    auto &buffer = m_HeapBuffer.m_Data;
    auto &position = m_HeapBuffer.m_DataPosition;

    uint8_t dataType = GetDataType<std::string>();
    if (!attribute.m_IsSingleValue)
    {
        dataType = type_string_array;
    }
    CopyToBuffer(buffer, position, &dataType);

    // here record payload offset
    stats.PayloadOffset = m_HeapBuffer.m_DataAbsolutePosition + position -
                          attributeLengthPosition;

    if (dataType == type_string)
    {
        const uint32_t dataSize =
            static_cast<const uint32_t>(attribute.m_DataSingleValue.size());
        CopyToBuffer(buffer, position, &dataSize);
        CopyToBuffer(buffer, position, attribute.m_DataSingleValue.data(),
                     attribute.m_DataSingleValue.size());
    }
    else if (dataType == type_string_array)
    {
        const uint32_t elements =
            static_cast<const uint32_t>(attribute.m_Elements);
        CopyToBuffer(buffer, position, &elements);

        for (size_t s = 0; s < attribute.m_Elements; ++s)
        {
            // include zero terminated
            const std::string element(attribute.m_DataArray[s] + '\0');

            const uint32_t elementSize =
                static_cast<const uint32_t>(element.size());

            CopyToBuffer(buffer, position, &elementSize);
            CopyToBuffer(buffer, position, element.data(), element.size());
        }
    }

    WriteAttributeLengthInData(attribute, stats, attributeLengthPosition);
}

template <class T>
void BP1Writer::WriteAttributeInData(const Attribute<T> &attribute,
                                     Stats<T> &stats) noexcept
{
    const size_t attributeLengthPosition =
        WriteAttributeHeaderInData(attribute, stats);

    auto &buffer = m_HeapBuffer.m_Data;
    auto &position = m_HeapBuffer.m_DataPosition;

    uint8_t dataType = GetDataType<T>();
    CopyToBuffer(buffer, position, &dataType);

    // here record payload offset
    stats.PayloadOffset = m_HeapBuffer.m_DataAbsolutePosition + position -
                          attributeLengthPosition;

    const uint32_t dataSize = attribute.m_Elements * sizeof(T);
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

    WriteAttributeLengthInData(attribute, stats, attributeLengthPosition);
}

template <>
inline void BP1Writer::WriteAttributeCharacteristicValueInIndex(
    uint8_t &characteristicsCounter, const Attribute<std::string> &attribute,
    std::vector<char> &buffer) noexcept
{
    uint8_t characteristicID = characteristic_value;

    InsertToBuffer(buffer, &characteristicID);

    if (attribute.m_IsSingleValue) // Single string
    {
        const uint16_t dataSize =
            static_cast<const uint16_t>(attribute.m_DataSingleValue.size());
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

            const uint16_t elementSize =
                static_cast<const uint16_t>(element.size());

            InsertToBuffer(buffer, &elementSize);
            InsertToBuffer(buffer, element.data(), element.size());
        }
    }
    ++characteristicsCounter;
}

template <class T>
void BP1Writer::WriteAttributeCharacteristicValueInIndex(
    uint8_t &characteristicsCounter, const Attribute<T> &attribute,
    std::vector<char> &buffer) noexcept
{
    uint8_t characteristicID = characteristic_value;

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
void BP1Writer::WriteAttributeInIndex(const Attribute<T> &attribute,
                                      const Stats<T> &stats) noexcept
{
    BP1Index index(stats.MemberID);
    auto &buffer = index.Buffer;

    buffer.insert(buffer.end(), 4, '\0'); // skip attribute length (4)
    InsertToBuffer(buffer, &stats.MemberID);
    buffer.insert(buffer.end(), 2, '\0'); // skip group name
    WriteNameRecord(attribute.m_Name, buffer);
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
    uint8_t characteristicID = characteristic_dimensions;
    InsertToBuffer(buffer, &characteristicID);
    constexpr uint8_t dimensions = 1;
    InsertToBuffer(buffer, &dimensions); // count
    constexpr uint16_t dimensionsLength = 24;
    InsertToBuffer(buffer, &dimensionsLength); // length
    WriteDimensionsRecord({attribute.m_Elements}, {}, {}, buffer);
    ++characteristicsCounter;

    // VALUE
    WriteAttributeCharacteristicValueInIndex(characteristicsCounter, attribute,
                                             buffer);

    // TIME Index
    WriteCharacteristicRecord(characteristic_time_index, characteristicsCounter,
                              stats.TimeIndex, buffer);

    const uint32_t rankU32 =
        static_cast<const uint32_t>(m_BP1Aggregator.m_RankMPI);
    WriteCharacteristicRecord(characteristic_file_index, characteristicsCounter,
                              rankU32, buffer);

    WriteCharacteristicRecord(characteristic_offset, characteristicsCounter,
                              stats.Offset, buffer);

    WriteCharacteristicRecord(characteristic_payload_offset,
                              characteristicsCounter, stats.PayloadOffset,
                              buffer);
    // END OF CHARACTERISTICS

    // Back to characteristics count and length
    size_t backPosition = characteristicsCountPosition;
    CopyToBuffer(buffer, backPosition, &characteristicsCounter); // count (1)

    // remove its own length (4) + characteristic counter (1)
    const uint32_t characteristicsLength = static_cast<const uint32_t>(
        buffer.size() - characteristicsCountPosition - 4 - 1);

    CopyToBuffer(buffer, backPosition, &characteristicsLength); // length

    // Finish characteristic count length
    m_MetadataSet.AttributesIndices.emplace(attribute.m_Name, index);
}

template <class T>
BP1Writer::Stats<typename TypeInfo<T>::ValueType>
BP1Writer::GetStats(const Variable<T> &variable) const noexcept
{
    Stats<typename TypeInfo<T>::ValueType> stats;
    const std::size_t valuesSize = variable.TotalSize();

    if (m_Verbosity == 0)
    {
        GetMinMaxThreads(variable.m_AppValues, valuesSize, stats.Min, stats.Max,
                         m_Threads);
    }
    return stats;
}

template <class T>
void BP1Writer::WriteVariableMetadataInData(
    const Variable<T> &variable,
    const Stats<typename TypeInfo<T>::ValueType> &stats) noexcept
{
    auto &buffer = m_HeapBuffer.m_Data;
    auto &position = m_HeapBuffer.m_DataPosition;

    // for writing length at the end
    const size_t varLengthPosition = position;
    position += 8; // skip var length (8)

    CopyToBuffer(buffer, position, &stats.MemberID);

    WriteNameRecord(variable.m_Name, buffer, position);
    position += 2; // skip path

    const uint8_t dataType = GetDataType<T>();
    CopyToBuffer(buffer, position, &dataType);

    constexpr char no = 'n'; // isDimension
    CopyToBuffer(buffer, position, &no);

    const uint8_t dimensions =
        static_cast<const uint8_t>(variable.m_Count.size());
    CopyToBuffer(buffer, position, &dimensions); // count

    // 27 is from 9 bytes for each: var y/n + local, var y/n + global dimension,
    // var y/n + global offset, changed for characteristic
    uint16_t dimensionsLength = 27 * dimensions;
    CopyToBuffer(buffer, position, &dimensionsLength); // length

    WriteDimensionsRecord(variable.m_Count, variable.m_Shape, variable.m_Start,
                          buffer, position);

    // CHARACTERISTICS
    WriteVariableCharacteristics(variable, stats, buffer, position);

    // Back to varLength including payload size
    // not need to remove its own size (8) from length from bpdump
    const uint64_t varLength = static_cast<const uint64_t>(
        position - varLengthPosition + variable.PayLoadSize());

    size_t backPosition = varLengthPosition;
    CopyToBuffer(buffer, backPosition, &varLength);

    m_HeapBuffer.m_DataAbsolutePosition += position - varLengthPosition;
}

template <class T>
void BP1Writer::WriteVariableMetadataInIndex(
    const Variable<T> &variable,
    const Stats<typename TypeInfo<T>::ValueType> &stats, const bool isNew,
    BP1Index &index) noexcept
{
    auto &buffer = index.Buffer;

    if (isNew) // write variable header
    {
        buffer.insert(buffer.end(), 4, '\0'); // skip var length (4)
        InsertToBuffer(buffer, &stats.MemberID);
        buffer.insert(buffer.end(), 2, '\0'); // skip group name
        WriteNameRecord(variable.m_Name, buffer);
        buffer.insert(buffer.end(), 2, '\0'); // skip path

        const std::uint8_t dataType = GetDataType<T>();
        InsertToBuffer(buffer, &dataType);

        // Characteristics Sets Count in Metadata
        index.Count = 1;
        InsertToBuffer(buffer, &index.Count);
    }
    else // update characteristics sets count
    {
        if (m_Verbosity == 0)
        {
            ++index.Count;
            size_t setsCountPosition = 15 + variable.m_Name.size();
            CopyToBuffer(buffer, setsCountPosition, &index.Count);
        }
    }

    WriteVariableCharacteristics(variable, stats, buffer);
}

template <class T>
void BP1Writer::WriteBoundsRecord(const bool isScalar, const Stats<T> &stats,
                                  std::uint8_t &characteristicsCounter,
                                  std::vector<char> &buffer) noexcept
{
    if (isScalar)
    {
        // stats.min = stats.max = value, need to test
        WriteCharacteristicRecord(characteristic_value, characteristicsCounter,
                                  stats.Min, buffer);
    }
    else
    {
        if (m_Verbosity == 0) // default verbose
        {
            WriteCharacteristicRecord(
                characteristic_min, characteristicsCounter, stats.Min, buffer);

            WriteCharacteristicRecord(
                characteristic_max, characteristicsCounter, stats.Max, buffer);
        }
    }
}

template <class T>
void BP1Writer::WriteBoundsRecord(const bool singleValue, const Stats<T> &stats,
                                  std::uint8_t &characteristicsCounter,
                                  std::vector<char> &buffer,
                                  size_t &position) noexcept
{
    if (singleValue)
    {
        // stats.min = stats.max = value, need to test
        WriteCharacteristicRecord(characteristic_value, characteristicsCounter,
                                  stats.Min, buffer, position);
    }
    else
    {
        if (m_Verbosity == 0) // default min and max only
        {
            WriteCharacteristicRecord(characteristic_min,
                                      characteristicsCounter, stats.Min, buffer,
                                      position);

            WriteCharacteristicRecord(characteristic_max,
                                      characteristicsCounter, stats.Max, buffer,
                                      position);
        }
    }
}

template <class T>
void BP1Writer::WriteCharacteristicRecord(const std::uint8_t characteristicID,
                                          std::uint8_t &characteristicsCounter,
                                          const T &value,
                                          std::vector<char> &buffer) noexcept
{
    const std::uint8_t id = characteristicID;
    InsertToBuffer(buffer, &id);
    InsertToBuffer(buffer, &value);
    ++characteristicsCounter;
}

template <class T>
void BP1Writer::WriteCharacteristicRecord(const uint8_t characteristicID,
                                          uint8_t &characteristicsCounter,
                                          const T &value,
                                          std::vector<char> &buffer,
                                          size_t &position) noexcept
{
    const std::uint8_t id = characteristicID;
    CopyToBuffer(buffer, position, &id);
    CopyToBuffer(buffer, position, &value);
    ++characteristicsCounter;
}

template <class T>
void BP1Writer::WriteVariableCharacteristics(
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
    uint8_t characteristicID = characteristic_dimensions;
    InsertToBuffer(buffer, &characteristicID);
    const uint8_t dimensions =
        static_cast<const uint8_t>(variable.m_Count.size());
    InsertToBuffer(buffer, &dimensions); // count
    const uint16_t dimensionsLength =
        static_cast<const uint16_t>(24 * dimensions);
    InsertToBuffer(buffer, &dimensionsLength); // length
    WriteDimensionsRecord(variable.m_Count, variable.m_Shape, variable.m_Start,
                          buffer);
    ++characteristicsCounter;

    WriteBoundsRecord(variable.m_SingleValue, stats, characteristicsCounter,
                      buffer);

    WriteCharacteristicRecord(characteristic_time_index, characteristicsCounter,
                              stats.TimeIndex, buffer);

    const uint32_t rankU32 =
        static_cast<const uint32_t>(m_BP1Aggregator.m_RankMPI);
    WriteCharacteristicRecord(characteristic_file_index, characteristicsCounter,
                              rankU32, buffer);

    WriteCharacteristicRecord(characteristic_offset, characteristicsCounter,
                              stats.Offset, buffer);

    WriteCharacteristicRecord(characteristic_payload_offset,
                              characteristicsCounter, stats.PayloadOffset,
                              buffer);
    // END OF CHARACTERISTICS

    // Back to characteristics count and length
    size_t backPosition = characteristicsCountPosition;
    CopyToBuffer(buffer, backPosition, &characteristicsCounter); // count (1)

    // remove its own length (4) + characteristic counter (1)
    const uint32_t characteristicsLength = static_cast<const uint32_t>(
        buffer.size() - characteristicsCountPosition - 4 - 1);

    CopyToBuffer(buffer, backPosition, &characteristicsLength); // length
}

template <class T>
void BP1Writer::WriteVariableCharacteristics(
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

    const uint8_t dimensions =
        static_cast<const uint8_t>(variable.m_Count.size());
    CopyToBuffer(buffer, position, &dimensions); // count
    const uint16_t dimensionsLength =
        static_cast<const uint16_t>(24 * dimensions);
    CopyToBuffer(buffer, position, &dimensionsLength); // length
    WriteDimensionsRecord(variable.m_Count, variable.m_Shape, variable.m_Start,
                          buffer, position, true); // isCharacteristic = true
    ++characteristicsCounter;

    // VALUE for SCALAR or STAT min, max for ARRAY
    WriteBoundsRecord(variable.m_SingleValue, stats, characteristicsCounter,
                      buffer, position);
    // END OF CHARACTERISTICS

    // Back to characteristics count and length
    size_t backPosition = characteristicsCountPosition;
    CopyToBuffer(buffer, backPosition, &characteristicsCounter);

    // remove its own length (4) + characteristic counter (1)
    const uint32_t characteristicsLength = static_cast<const uint32_t>(
        position - characteristicsCountPosition - 4 - 1);
    CopyToBuffer(buffer, backPosition, &characteristicsLength);
}

} // end namespace format
} // end namespace adios2

#endif // ADIOS2_TOOLKIT_FORMAT_BP1_BP1WRITER_TCC_
