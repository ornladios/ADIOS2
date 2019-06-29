/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP4Serializer.tcc
 *
 *  Created on: Aug 1, 2018
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP4_BP4SERIALIZER_TCC_
#define ADIOS2_TOOLKIT_FORMAT_BP4_BP4SERIALIZER_TCC_

#include "BP4Serializer.h"

#include <algorithm> // std::all_of

#include "adios2/helper/adiosFunctions.h"

#include <iostream>
#include <stddef.h>

namespace adios2
{
namespace format
{

template <class T>
inline void BP4Serializer::PutVariableMetadata(
    const core::Variable<T> &variable,
    const typename core::Variable<T>::Info &blockInfo,
    const bool sourceRowMajor) noexcept
{
    auto lf_SetOffset = [&](uint64_t &offset) {
        if (m_Aggregator.m_IsActive && !m_Aggregator.m_IsConsumer)
        {
            offset = static_cast<uint64_t>(m_Data.m_Position);
        }
        else
        {
            offset = static_cast<uint64_t>(m_Data.m_AbsolutePosition +
                                           m_PreDataFileLength);
        }
    };

    ProfilerStart("buffering");

    Stats<T> stats =
        GetBPStats<T>(variable.m_SingleValue, blockInfo, sourceRowMajor);

    // Get new Index or point to existing index
    bool isNew = true; // flag to check if variable is new
    SerialElementIndex &variableIndex = GetSerialElementIndex(
        variable.m_Name, m_MetadataSet.VarsIndices, isNew);
    variableIndex.Valid =
        true; // flag to indicate this variable is put at current step
    stats.MemberID = variableIndex.MemberID;

    /*const size_t startingPos = m_Data.m_Position;*/
    lf_SetOffset(stats.Offset);
    PutVariableMetadataInData(variable, blockInfo, stats);
    lf_SetOffset(stats.PayloadOffset);

    /*
    std::cout << " -- Var name=" << variable.m_Name
              << " buffer startPos = " << std::to_string(startingPos)
              << " offset = " << std::to_string(stats.Offset)
              << " payload offset = " << std::to_string(stats.PayloadOffset)
              << " position = " << std::to_string(m_Data.m_Position)
              << " abs.position = " << std::to_string(m_Data.m_AbsolutePosition)
              << std::endl;
    */

    // write to metadata  index
    PutVariableMetadataInIndex(variable, blockInfo, stats, isNew,
                               variableIndex);
    ++m_MetadataSet.DataPGVarsCount;

    ProfilerStop("buffering");
}

template <class T>
inline void BP4Serializer::PutVariablePayload(
    const core::Variable<T> &variable,
    const typename core::Variable<T>::Info &blockInfo,
    const bool sourceRowMajor) noexcept
{
    ProfilerStart("buffering");
    const size_t startingPos = m_Data.m_Position;
    if (blockInfo.Operations.empty())
    {
        PutPayloadInBuffer(variable, blockInfo, sourceRowMajor);
    }
    else
    {
        PutOperationPayloadInBuffer(variable, blockInfo);
    }

    /*std::cout << " -- Var payload name=" << variable.m_Name
              << " startPos = " << std::to_string(startingPos)
              << " end position = " << std::to_string(m_Data.m_Position)
              << " end abs.position = "
              << std::to_string(m_Data.m_AbsolutePosition) << " value[0] = "
              << std::to_string(
                     *reinterpret_cast<double *>(&m_Data.m_Buffer[startingPos]))
              << " value[last] = "
              << std::to_string(*reinterpret_cast<double *>(
                     m_Data.m_Buffer.data() + m_Data.m_Position - 8))
              << " 4 bytes ahead = '"
              << std::string(m_Data.m_Buffer.data() + startingPos - 4, 4) << "'"
              << std::endl;*/

    ProfilerStop("buffering");
}

// PRIVATE
template <class T>
size_t
BP4Serializer::PutAttributeHeaderInData(const core::Attribute<T> &attribute,
                                        Stats<T> &stats, const char *headerID,
                                        const size_t headerIDLength) noexcept
{
    auto &buffer = m_Data.m_Buffer;
    auto &position = m_Data.m_Position;

    helper::CopyToBuffer(buffer, position, headerID, headerIDLength);

    // will go back to write length
    const size_t attributeLengthPosition = position;
    position += 4; // skip length

    helper::CopyToBuffer(buffer, position, &stats.MemberID);
    PutNameRecord(attribute.m_Name, buffer, position);
    position += 2; // skip path

    // TODO: attribute from Variable??
    constexpr int8_t no = 'n';
    helper::CopyToBuffer(buffer, position,
                         &no); // not associated with a Variable

    return attributeLengthPosition;
}

template <class T>
void BP4Serializer::PutAttributeLengthInData(
    const core::Attribute<T> &attribute, Stats<T> &stats,
    const size_t attributeLengthPosition) noexcept
{
    auto &buffer = m_Data.m_Buffer;
    auto &position = m_Data.m_Position;

    // back to attribute length
    size_t backPosition = attributeLengthPosition;
    uint32_t len = static_cast<uint32_t>(position - attributeLengthPosition);
    helper::CopyToBuffer(buffer, backPosition, &len);
}

template <>
inline void
BP4Serializer::PutAttributeInData(const core::Attribute<std::string> &attribute,
                                  Stats<std::string> &stats) noexcept
{
    auto &buffer = m_Data.m_Buffer;
    auto &position = m_Data.m_Position;
    auto &absolutePosition = m_Data.m_AbsolutePosition;

    const size_t mdBeginPosition = position;

    // write a block identifier [AMD
    const char amd[] = "[AMD"; // no \0
    const size_t attributeLengthPosition =
        PutAttributeHeaderInData(attribute, stats, amd, sizeof(amd) - 1);

    uint8_t dataType = TypeTraits<std::string>::type_enum;
    if (!attribute.m_IsSingleValue)
    {
        dataType = type_string_array;
    }
    helper::CopyToBuffer(buffer, position, &dataType);

    // here record payload offset
    stats.PayloadOffset =
        absolutePosition + position - mdBeginPosition + m_PreDataFileLength;

    if (dataType == type_string)
    {
        const uint32_t dataSize =
            static_cast<uint32_t>(attribute.m_DataSingleValue.size());
        helper::CopyToBuffer(buffer, position, &dataSize);
        helper::CopyToBuffer(buffer, position,
                             attribute.m_DataSingleValue.data(),
                             attribute.m_DataSingleValue.size());
    }
    else if (dataType == type_string_array)
    {
        const uint32_t elements = static_cast<uint32_t>(attribute.m_Elements);
        helper::CopyToBuffer(buffer, position, &elements);

        for (size_t s = 0; s < attribute.m_Elements; ++s)
        {
            // include zero terminated
            const std::string element(attribute.m_DataArray[s] + '\0');

            const uint32_t elementSize = static_cast<uint32_t>(element.size());

            helper::CopyToBuffer(buffer, position, &elementSize);
            helper::CopyToBuffer(buffer, position, element.data(),
                                 element.size());
        }
    }

    // write a block identifier AMD]
    const char amdend[] = "AMD]"; // no \0
    helper::CopyToBuffer(buffer, position, amdend, sizeof(amdend) - 1);

    PutAttributeLengthInData(attribute, stats, attributeLengthPosition);
    absolutePosition += position - mdBeginPosition;
}

template <class T>
void BP4Serializer::PutAttributeInData(const core::Attribute<T> &attribute,
                                       Stats<T> &stats) noexcept
{
    auto &buffer = m_Data.m_Buffer;
    auto &position = m_Data.m_Position;
    auto &absolutePosition = m_Data.m_AbsolutePosition;

    const size_t mdBeginPosition = position;

    // write a block identifier [AMD
    const char amd[] = "[AMD"; // no \0
    const size_t attributeLengthPosition =
        PutAttributeHeaderInData(attribute, stats, amd, sizeof(amd) - 1);

    uint8_t dataType = TypeTraits<T>::type_enum;
    helper::CopyToBuffer(buffer, position, &dataType);

    // here record payload offset
    stats.PayloadOffset =
        absolutePosition + position - mdBeginPosition + m_PreDataFileLength;

    const uint32_t dataSize =
        static_cast<uint32_t>(attribute.m_Elements * sizeof(T));
    helper::CopyToBuffer(buffer, position, &dataSize);

    if (attribute.m_IsSingleValue) // single value
    {
        helper::CopyToBuffer(buffer, position, &attribute.m_DataSingleValue);
    }
    else // array
    {
        helper::CopyToBuffer(buffer, position, attribute.m_DataArray.data(),
                             attribute.m_Elements);
    }

    // write a block identifier AMD]
    const char amdend[] = "AMD]"; // no \0
    helper::CopyToBuffer(buffer, position, amdend, sizeof(amdend) - 1);

    PutAttributeLengthInData(attribute, stats, attributeLengthPosition);
    absolutePosition += position - mdBeginPosition;
}

template <>
inline void BP4Serializer::PutAttributeCharacteristicValueInIndex(
    uint8_t &characteristicsCounter,
    const core::Attribute<std::string> &attribute,
    std::vector<char> &buffer) noexcept
{
    const uint8_t characteristicID =
        static_cast<uint8_t>(CharacteristicID::characteristic_value);

    helper::InsertToBuffer(buffer, &characteristicID);

    if (attribute.m_IsSingleValue) // Single string
    {
        const uint16_t dataSize =
            static_cast<uint16_t>(attribute.m_DataSingleValue.size());
        helper::InsertToBuffer(buffer, &dataSize);
        helper::InsertToBuffer(buffer, attribute.m_DataSingleValue.data(),
                               attribute.m_DataSingleValue.size());
    }
    else // string array
    {
        for (size_t s = 0; s < attribute.m_Elements; ++s)
        {
            // without zero terminated character
            const std::string element(attribute.m_DataArray[s]);

            const uint16_t elementSize = static_cast<uint16_t>(element.size());

            helper::InsertToBuffer(buffer, &elementSize);
            helper::InsertToBuffer(buffer, element.data(), element.size());
        }
    }
    ++characteristicsCounter;
}

template <class T>
void BP4Serializer::PutAttributeCharacteristicValueInIndex(
    uint8_t &characteristicsCounter, const core::Attribute<T> &attribute,
    std::vector<char> &buffer) noexcept
{
    const uint8_t characteristicID = CharacteristicID::characteristic_value;

    helper::InsertToBuffer(buffer, &characteristicID);

    if (attribute.m_IsSingleValue) // single value
    {
        helper::InsertToBuffer(buffer, &attribute.m_DataSingleValue);
    }
    else // array
    {
        helper::InsertToBuffer(buffer, attribute.m_DataArray.data(),
                               attribute.m_Elements);
    }
    ++characteristicsCounter;
}

template <class T>
void BP4Serializer::PutAttributeInIndex(const core::Attribute<T> &attribute,
                                        const Stats<T> &stats) noexcept
{
    SerialElementIndex index(stats.MemberID);
    auto &buffer = index.Buffer;

    index.Valid = true; // when the attribute is put, set this flag to true

    buffer.insert(buffer.end(), 4, '\0'); // skip attribute length (4)
    helper::InsertToBuffer(buffer, &stats.MemberID);
    buffer.insert(buffer.end(), 2, '\0'); // skip group name
    PutNameRecord(attribute.m_Name, buffer);
    buffer.insert(buffer.end(), 2, '\0'); // skip path

    uint8_t dataType = TypeTraits<T>::type_enum; // dataType

    if (dataType == type_string && !attribute.m_IsSingleValue)
    {
        dataType = type_string_array;
    }

    helper::InsertToBuffer(buffer, &dataType);

    // Characteristics Sets Count in Metadata
    index.Count = 1;
    helper::InsertToBuffer(buffer, &index.Count);

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
    helper::InsertToBuffer(buffer, &characteristicID);
    constexpr uint8_t dimensions = 1;
    helper::InsertToBuffer(buffer, &dimensions); // count
    constexpr uint16_t dimensionsLength = 24;
    helper::InsertToBuffer(buffer, &dimensionsLength); // length
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
    helper::CopyToBuffer(buffer, backPosition,
                         &characteristicsCounter); // count (1)

    // remove its own length (4) + characteristic counter (1)
    const uint32_t characteristicsLength = static_cast<uint32_t>(
        buffer.size() - characteristicsCountPosition - 4 - 1);

    helper::CopyToBuffer(buffer, backPosition,
                         &characteristicsLength); // length

    // Remember this attribute and its serialized piece
    m_MetadataSet.AttributesIndices.emplace(attribute.m_Name, index);
    m_SerializedAttributes.emplace(attribute.m_Name);
}

template <>
inline BP4Serializer::Stats<std::string> BP4Serializer::GetBPStats(
    const bool /*singleValue*/,
    const typename core::Variable<std::string>::Info & /*blockInfo*/,
    const bool /*isRowMajor*/) noexcept
{
    Stats<std::string> stats;
    stats.Step = m_MetadataSet.TimeStep;
    stats.FileIndex = GetFileIndex();
    return stats;
}

template <class T>
BP4Serializer::Stats<T>
BP4Serializer::GetBPStats(const bool singleValue,
                          const typename core::Variable<T>::Info &blockInfo,
                          const bool isRowMajor) noexcept
{
    Stats<T> stats;
    stats.Step = m_MetadataSet.TimeStep;
    stats.FileIndex = GetFileIndex();

    if (singleValue)
    {
        stats.Value = *blockInfo.Data;
        stats.Min = stats.Value;
        stats.Max = stats.Value;
        return stats;
    }

    if (m_StatsLevel == 0)
    {
        ProfilerStart("minmax");
        if (blockInfo.MemoryStart.empty())
        {
            const std::size_t valuesSize =
                helper::GetTotalSize(blockInfo.Count);
            helper::GetMinMaxThreads(blockInfo.Data, valuesSize, stats.Min,
                                     stats.Max, m_Threads);
        }
        else // non-contiguous memory min/max
        {
            helper::GetMinMaxSelection(blockInfo.Data, blockInfo.MemoryCount,
                                       blockInfo.MemoryStart, blockInfo.Count,
                                       isRowMajor, stats.Min, stats.Max);
        }
        ProfilerStop("minmax");
    }

    return stats;
}

template <class T>
void BP4Serializer::PutVariableMetadataInData(
    const core::Variable<T> &variable,
    const typename core::Variable<T>::Info &blockInfo,
    const Stats<T> &stats) noexcept
{
    auto &buffer = m_Data.m_Buffer;
    auto &position = m_Data.m_Position;
    auto &absolutePosition = m_Data.m_AbsolutePosition;

    const size_t mdBeginPosition = position;

    // write a block identifier [VMD
    const char vmd[] = "[VMD"; //  don't write \0!
    helper::CopyToBuffer(buffer, position, vmd, sizeof(vmd) - 1);

    // for writing length at the end
    const size_t varLengthPosition = position;
    position += 8; // skip var length (8)

    helper::CopyToBuffer(buffer, position, &stats.MemberID);

    PutNameRecord(variable.m_Name, buffer, position);
    // path is empty now, write a 0 length to skip it
    const uint16_t zero16 = 0;
    helper::CopyToBuffer(buffer, position, &zero16);

    const uint8_t dataType = TypeTraits<T>::type_enum;
    helper::CopyToBuffer(buffer, position, &dataType);

    constexpr char no = 'n'; // isDimension
    helper::CopyToBuffer(buffer, position, &no);

    const uint8_t dimensions = static_cast<uint8_t>(variable.m_Count.size());
    helper::CopyToBuffer(buffer, position, &dimensions); // count

    // 27 is from 9 bytes for each: var y/n + local, var y/n + global dimension,
    // var y/n + global offset, changed for characteristic
    uint16_t dimensionsLength = 27 * dimensions;
    helper::CopyToBuffer(buffer, position, &dimensionsLength); // length

    PutDimensionsRecord(variable.m_Count, variable.m_Shape, variable.m_Start,
                        buffer, position);

    // CHARACTERISTICS
    PutVariableCharacteristicsInData(variable, blockInfo, stats, buffer,
                                     position);

    // pad metadata so that data will fall on aligned position in memory
    // write a padding plus block identifier VMD]
    // format: length in 1 byte + padding characters + VMD]
    // we would write at minimum 5 bytes, byte for length + "VMD]"
    // hence the +5 in the calculation below
    size_t padSize = rand() % 32;
    // helper::PaddingToAlignPointer(buffer.data() + position + 5);

    const char vmdEnd[] = "                                VMD]";
    unsigned char vmdEndLen = static_cast<unsigned char>(padSize + 4);
    // starting position in vmdEnd from where we copy to buffer
    // we don't copy the \0 from vmdEnd !
    const char *ptr = vmdEnd + (sizeof(vmdEnd) - 1 - vmdEndLen);
    /*std::cout << " -- Pad metadata with " << std::to_string(padSize)
              << " bytes. var = " << variable.m_Name << " rank = " << m_RankMPI
              << " position = " << std::to_string(position) << " pad string = '"
              << ptr << "'"
              << std::to_string(reinterpret_cast<std::uintptr_t>(buffer.data()))
              << std::endl;*/
    helper::CopyToBuffer(buffer, position, &vmdEndLen, 1);
    helper::CopyToBuffer(buffer, position, ptr, vmdEndLen);

    // Back to varLength including payload size
    // including the closing padding but NOT the opening [VMD
    const uint64_t varLength = static_cast<uint64_t>(
        position - varLengthPosition +
        helper::PayloadSize(blockInfo.Data, blockInfo.Count));

    size_t backPosition = varLengthPosition;
    helper::CopyToBuffer(buffer, backPosition, &varLength);

    absolutePosition += position - mdBeginPosition;
}

template <>
inline void BP4Serializer::PutVariableMetadataInData(
    const core::Variable<std::string> &variable,
    const typename core::Variable<std::string>::Info &blockInfo,
    const Stats<std::string> &stats) noexcept
{
    auto &buffer = m_Data.m_Buffer;
    auto &position = m_Data.m_Position;
    auto &absolutePosition = m_Data.m_AbsolutePosition;

    const size_t mdBeginPosition = position;

    // write a block identifier [VMD
    const char vmd[] = "[VMD"; // no \0
    helper::CopyToBuffer(buffer, position, vmd, sizeof(vmd) - 1);

    // for writing length at the end
    const size_t varLengthPosition = position;
    position += 8; // skip var length (8)

    helper::CopyToBuffer(buffer, position, &stats.MemberID);

    PutNameRecord(variable.m_Name, buffer, position);
    position += 2; // skip path

    const uint8_t dataType = TypeTraits<std::string>::type_enum;
    helper::CopyToBuffer(buffer, position, &dataType);

    constexpr char no = 'n'; // is dimension is deprecated
    helper::CopyToBuffer(buffer, position, &no);

    const uint8_t dimensions = static_cast<uint8_t>(blockInfo.Count.size());
    helper::CopyToBuffer(buffer, position, &dimensions); // count

    uint16_t dimensionsLength = 27 * dimensions;
    helper::CopyToBuffer(buffer, position, &dimensionsLength); // length

    PutDimensionsRecord(blockInfo.Count, blockInfo.Shape, blockInfo.Start,
                        buffer, position);

    position += 5; // skipping characteristics

    // write a block identifier VMD]
    // byte 4 and then four characters (as len+str without terminating 0)
    const char vmdend[] = "\4VMD]"; // no \0
    helper::CopyToBuffer(buffer, position, vmdend, sizeof(vmdend) - 1);

    // Back to varLength including payload size
    // including the closing padding but NOT the opening [VMD
    const uint64_t varLength = static_cast<uint64_t>(
        position - varLengthPosition +
        helper::PayloadSize(blockInfo.Data, blockInfo.Count));

    size_t backPosition = varLengthPosition;
    helper::CopyToBuffer(buffer, backPosition, &varLength);

    absolutePosition += position - mdBeginPosition;
}

template <class T>
void BP4Serializer::PutVariableMetadataInIndex(
    const core::Variable<T> &variable,
    const typename core::Variable<T>::Info &blockInfo, const Stats<T> &stats,
    const bool isNew, SerialElementIndex &index) noexcept
{
    auto &buffer = index.Buffer;

    if (isNew) // write variable header
    {
        buffer.insert(buffer.end(), 4, '\0'); // skip var length (4)
        helper::InsertToBuffer(buffer, &stats.MemberID);
        buffer.insert(buffer.end(), 2, '\0'); // skip group name
        PutNameRecord(variable.m_Name, buffer);
        buffer.insert(buffer.end(), 2, '\0'); // skip path

        const uint8_t dataType = TypeTraits<T>::type_enum;
        helper::InsertToBuffer(buffer, &dataType);

        // Characteristics Sets Count in Metadata
        index.Count = 1;
        helper::InsertToBuffer(buffer, &index.Count);

        // For updating absolute offsets in agreggation
        index.LastUpdatedPosition = buffer.size();
    }
    else // update characteristics sets count
    {
        if (m_StatsLevel == 0)
        {
            ++index.Count;
            // fixed since group and path are not printed
            size_t setsCountPosition = 15 + variable.m_Name.size();
            helper::CopyToBuffer(buffer, setsCountPosition, &index.Count);
        }
    }

    PutVariableCharacteristics(variable, blockInfo, stats, buffer);
}

template <class T>
void BP4Serializer::PutBoundsRecord(const bool singleValue,
                                    const Stats<T> &stats,
                                    uint8_t &characteristicsCounter,
                                    std::vector<char> &buffer) noexcept
{
    if (singleValue)
    {
        PutCharacteristicRecord(characteristic_value, characteristicsCounter,
                                stats.Min, buffer);
    }
    else
    {
        if (m_StatsLevel == 0) // default verbose
        {
            PutCharacteristicRecord(characteristic_min, characteristicsCounter,
                                    stats.Min, buffer);

            PutCharacteristicRecord(characteristic_max, characteristicsCounter,
                                    stats.Max, buffer);
        }
    }
}

template <class T>
void BP4Serializer::PutBoundsRecord(const bool singleValue,
                                    const Stats<T> &stats,
                                    uint8_t &characteristicsCounter,
                                    std::vector<char> &buffer,
                                    size_t &position) noexcept
{
    if (singleValue)
    {
        PutCharacteristicRecord(characteristic_value, characteristicsCounter,
                                stats.Min, buffer, position);
    }
    else
    {
        if (m_StatsLevel == 0) // default min and max only
        {
            PutCharacteristicRecord(characteristic_min, characteristicsCounter,
                                    stats.Min, buffer, position);

            PutCharacteristicRecord(characteristic_max, characteristicsCounter,
                                    stats.Max, buffer, position);
        }
    }
}

template <class T>
void BP4Serializer::PutCharacteristicRecord(const uint8_t characteristicID,
                                            uint8_t &characteristicsCounter,
                                            const T &value,
                                            std::vector<char> &buffer) noexcept
{
    const uint8_t id = characteristicID;
    helper::InsertToBuffer(buffer, &id);
    helper::InsertToBuffer(buffer, &value);
    ++characteristicsCounter;
}

template <class T>
void BP4Serializer::PutCharacteristicRecord(const uint8_t characteristicID,
                                            uint8_t &characteristicsCounter,
                                            const T &value,
                                            std::vector<char> &buffer,
                                            size_t &position) noexcept
{
    const uint8_t id = characteristicID;
    helper::CopyToBuffer(buffer, position, &id);
    helper::CopyToBuffer(buffer, position, &value);
    ++characteristicsCounter;
}

template <>
inline void BP4Serializer::PutVariableCharacteristics(
    const core::Variable<std::string> &variable,
    const core::Variable<std::string>::Info &blockInfo,
    const Stats<std::string> &stats, std::vector<char> &buffer) noexcept
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
    helper::InsertToBuffer(buffer, &characteristicID);
    PutNameRecord(*blockInfo.Data, buffer);
    ++characteristicsCounter;

    characteristicID = characteristic_dimensions;
    helper::InsertToBuffer(buffer, &characteristicID);
    const uint8_t dimensions = static_cast<uint8_t>(blockInfo.Count.size());
    helper::InsertToBuffer(buffer, &dimensions); // count
    const uint16_t dimensionsLength = static_cast<uint16_t>(24 * dimensions);
    helper::InsertToBuffer(buffer, &dimensionsLength); // length
    PutDimensionsRecord(blockInfo.Count, blockInfo.Shape, blockInfo.Start,
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
    helper::CopyToBuffer(buffer, backPosition,
                         &characteristicsCounter); // count (1)

    // remove its own length (4) + characteristic counter (1)
    const uint32_t characteristicsLength = static_cast<uint32_t>(
        buffer.size() - characteristicsCountPosition - 4 - 1);

    helper::CopyToBuffer(buffer, backPosition,
                         &characteristicsLength); // length
}

template <class T>
void BP4Serializer::PutVariableCharacteristics(
    const core::Variable<T> &variable,
    const typename core::Variable<T>::Info &blockInfo, const Stats<T> &stats,
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

    if (blockInfo.Data != nullptr)
    {
        PutBoundsRecord(variable.m_SingleValue, stats, characteristicsCounter,
                        buffer);
    }

    uint8_t characteristicID = characteristic_dimensions;
    helper::InsertToBuffer(buffer, &characteristicID);
    const uint8_t dimensions = static_cast<uint8_t>(blockInfo.Count.size());
    helper::InsertToBuffer(buffer, &dimensions); // count
    const uint16_t dimensionsLength = static_cast<uint16_t>(24 * dimensions);
    helper::InsertToBuffer(buffer, &dimensionsLength); // length
    PutDimensionsRecord(blockInfo.Count, blockInfo.Shape, blockInfo.Start,
                        buffer);
    ++characteristicsCounter;

    PutCharacteristicRecord(characteristic_offset, characteristicsCounter,
                            stats.Offset, buffer);

    PutCharacteristicRecord(characteristic_payload_offset,
                            characteristicsCounter, stats.PayloadOffset,
                            buffer);

    if (blockInfo.Operations.size())
    {
        const bool isZeroCount =
            std::all_of(blockInfo.Count.begin(), blockInfo.Count.end(),
                        [](const size_t i) { return i == 0; });

        // do not compress if count dimensions are all zero
        if (!isZeroCount)
        {
            characteristicID = characteristic_transform_type;
            helper::InsertToBuffer(buffer, &characteristicID);
            PutCharacteristicOperation(variable, blockInfo, buffer);
            ++characteristicsCounter;
        }
    }

    // END OF CHARACTERISTICS

    // Back to characteristics count and length
    size_t backPosition = characteristicsCountPosition;
    helper::CopyToBuffer(buffer, backPosition,
                         &characteristicsCounter); // count (1)

    // remove its own length (4) + characteristic counter (1)
    const uint32_t characteristicsLength = static_cast<uint32_t>(
        buffer.size() - characteristicsCountPosition - 4 - 1);

    helper::CopyToBuffer(buffer, backPosition,
                         &characteristicsLength); // length
}

template <class T>
void BP4Serializer::PutVariableCharacteristicsInData(
    const core::Variable<T> &variable,
    const typename core::Variable<T>::Info &blockInfo, const Stats<T> &stats,
    std::vector<char> &buffer, size_t &position) noexcept
{
    // going back at the end
    const size_t characteristicsCountPosition = position;
    // skip characteristics count(1) + length (4)
    position += 5;
    uint8_t characteristicsCounter = 0;

    // Write STAT min, max characteristics for an ARRAY
    // VALUE variable data is not written into characteristics
    // in the data file (only in metadata file in other function)
    if (blockInfo.Data != nullptr && !variable.m_SingleValue)
    {
        PutBoundsRecord(variable.m_SingleValue, stats, characteristicsCounter,
                        buffer, position);
    }
    // END OF CHARACTERISTICS

    // Back to characteristics count and length
    size_t backPosition = characteristicsCountPosition;
    helper::CopyToBuffer(buffer, backPosition, &characteristicsCounter);

    // remove its own length (4) + characteristic counter (1)
    const uint32_t characteristicsLength =
        static_cast<uint32_t>(position - characteristicsCountPosition - 4 - 1);
    helper::CopyToBuffer(buffer, backPosition, &characteristicsLength);
}

template <>
inline void BP4Serializer::PutPayloadInBuffer(
    const core::Variable<std::string> &variable,
    const typename core::Variable<std::string>::Info &blockInfo,
    const bool /* sourceRowMajor*/) noexcept
{
    PutNameRecord(*blockInfo.Data, m_Data.m_Buffer, m_Data.m_Position);
    m_Data.m_AbsolutePosition += blockInfo.Data->size() + 2;
}

template <class T>
void BP4Serializer::PutPayloadInBuffer(
    const core::Variable<T> &variable,
    const typename core::Variable<T>::Info &blockInfo,
    const bool sourceRowMajor) noexcept
{
    const size_t blockSize = helper::GetTotalSize(blockInfo.Count);
    ProfilerStart("memcpy");
    if (!blockInfo.MemoryStart.empty())
    {
        // TODO make it a BP4Serializer function
        helper::CopyMemoryBlock(
            reinterpret_cast<T *>(m_Data.m_Buffer.data() + m_Data.m_Position),
            blockInfo.Start, blockInfo.Count, sourceRowMajor, blockInfo.Data,
            blockInfo.Start, blockInfo.Count, sourceRowMajor, false, Dims(),
            Dims(), blockInfo.MemoryStart, blockInfo.MemoryCount);
        m_Data.m_Position += blockSize * sizeof(T);
    }
    else
    {
        helper::CopyToBufferThreads(m_Data.m_Buffer, m_Data.m_Position,
                                    blockInfo.Data, blockSize, m_Threads);
    }
    ProfilerStop("memcpy");
    m_Data.m_AbsolutePosition += blockSize * sizeof(T); // payload size
}

template <class T>
void BP4Serializer::UpdateIndexOffsetsCharacteristics(size_t &currentPosition,
                                                      const DataTypes dataType,
                                                      std::vector<char> &buffer)
{
    const uint8_t characteristicsCount =
        helper::ReadValue<uint8_t>(buffer, currentPosition);

    const uint32_t characteristicsLength =
        helper::ReadValue<uint32_t>(buffer, currentPosition);

    const size_t endPosition =
        currentPosition + static_cast<size_t>(characteristicsLength);

    while (currentPosition < endPosition)
    {
        const uint8_t id = helper::ReadValue<uint8_t>(buffer, currentPosition);

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
                    helper::ReadValue<uint16_t>(buffer, currentPosition));

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
            currentPosition += sizeof(T);
            break;
        }
        case (characteristic_max):
        {
            currentPosition += sizeof(T);
            break;
        }
        case (characteristic_offset):
        {
            const uint64_t currentOffset =
                helper::ReadValue<uint64_t>(buffer, currentPosition);

            const uint64_t updatedOffset =
                currentOffset +
                static_cast<uint64_t>(m_Data.m_AbsolutePosition);

            currentPosition -= sizeof(uint64_t);
            helper::CopyToBuffer(buffer, currentPosition, &updatedOffset);
            break;
        }
        case (characteristic_payload_offset):
        {
            const uint64_t currentPayloadOffset =
                helper::ReadValue<uint64_t>(buffer, currentPosition);

            const uint64_t updatedPayloadOffset =
                currentPayloadOffset +
                static_cast<uint64_t>(m_Data.m_AbsolutePosition);

            currentPosition -= sizeof(uint64_t);
            helper::CopyToBuffer(buffer, currentPosition,
                                 &updatedPayloadOffset);
            break;
        }
        case (characteristic_dimensions):
        {
            const size_t dimensionsSize = static_cast<size_t>(
                helper::ReadValue<uint8_t>(buffer, currentPosition));

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
inline size_t BP4Serializer::GetAttributeSizeInData(
    const core::Attribute<std::string> &attribute) const noexcept
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
BP4Serializer::GetAttributeSizeInData(const core::Attribute<T> &attribute) const
    noexcept
{
    size_t size = 14 + attribute.m_Name.size() + 10;
    size += 4 + sizeof(T) * attribute.m_Elements;
    return size;
}

// operations related functions
template <class T>
void BP4Serializer::PutCharacteristicOperation(
    const core::Variable<T> &variable,
    const typename core::Variable<T>::Info &blockInfo,
    std::vector<char> &buffer) noexcept
{
    // TODO: we only take the first operation for now
    const std::map<size_t, std::shared_ptr<BPOperation>> bpOperations =
        SetBPOperations<T>(blockInfo.Operations);

    const size_t operationIndex = bpOperations.begin()->first;
    std::shared_ptr<BPOperation> bp4Operation = bpOperations.begin()->second;

    auto &operation = blockInfo.Operations[operationIndex];

    const std::string type = operation.Op->m_Type;
    const uint8_t typeLength = static_cast<uint8_t>(type.size());
    helper::InsertToBuffer(buffer, &typeLength);
    helper::InsertToBuffer(buffer, type.c_str(), type.size());

    // pre-transform type
    const uint8_t dataType = TypeTraits<T>::type_enum;
    helper::InsertToBuffer(buffer, &dataType);
    // pre-transform dimensions
    const uint8_t dimensions = static_cast<uint8_t>(blockInfo.Count.size());
    helper::InsertToBuffer(buffer, &dimensions); // count
    const uint16_t dimensionsLength = static_cast<uint16_t>(24 * dimensions);
    helper::InsertToBuffer(buffer, &dimensionsLength); // length
    PutDimensionsRecord(blockInfo.Count, blockInfo.Shape, blockInfo.Start,
                        buffer);
    // here put the metadata info depending on operation
    bp4Operation->SetMetadata(variable, blockInfo, operation, buffer);
}

template <class T>
void BP4Serializer::PutOperationPayloadInBuffer(
    const core::Variable<T> &variable,
    const typename core::Variable<T>::Info &blockInfo)
{
    // TODO: we only take the first operation for now
    const std::map<size_t, std::shared_ptr<BPOperation>> bpOperations =
        SetBPOperations<T>(blockInfo.Operations);

    const size_t operationIndex = bpOperations.begin()->first;
    const std::shared_ptr<BPOperation> bpOperation =
        bpOperations.begin()->second;

    bpOperation->SetData(variable, blockInfo,
                         blockInfo.Operations[operationIndex], m_Data);

    // update metadata
    bool isFound = false;
    SerialElementIndex &variableIndex = GetSerialElementIndex(
        variable.m_Name, m_MetadataSet.VarsIndices, isFound);
    bpOperation->UpdateMetadata(variable, blockInfo,
                                blockInfo.Operations[operationIndex],
                                variableIndex.Buffer);
}

} // end namespace format
} // end namespace adios2

#endif // ADIOS2_TOOLKIT_FORMAT_BP4_BP4SERIALIZER_TCC_
