/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP3Serializer.tcc
 *
 *  Created on: Apr 11, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP3_BP3SERIALIZER_TCC_
#define ADIOS2_TOOLKIT_FORMAT_BP3_BP3SERIALIZER_TCC_

#include "BP3Serializer.h"

#include <algorithm> // std::all_of, std::fill_n

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace format
{

template <class T>
void BP3Serializer::PutVariableMetadata(
    const core::Variable<T> &variable,
    const typename core::Variable<T>::Info &blockInfo,
    const bool sourceRowMajor, typename core::Variable<T>::Span *span) noexcept
{
    ProfilerStart("buffering");

    Stats<T> stats =
        GetBPStats<T>(variable.m_SingleValue, blockInfo, sourceRowMajor);

    // Get new Index or point to existing index
    bool isNew = true; // flag to check if variable is new
    SerialElementIndex &variableIndex = GetSerialElementIndex(
        variable.m_Name, m_MetadataSet.VarsIndices, isNew);
    stats.MemberID = variableIndex.MemberID;

    SetDataOffset(stats.Offset);
    PutVariableMetadataInData(variable, blockInfo, stats);
    SetDataOffset(stats.PayloadOffset);
    if (span != nullptr)
    {
        span->m_PayloadPosition = m_Data.m_Position;
    }

    // write to metadata  index
    PutVariableMetadataInIndex(variable, blockInfo, stats, isNew, variableIndex,
                               span);
    ++m_MetadataSet.DataPGVarsCount;

    ProfilerStop("buffering");
}

template <class T>
inline void BP3Serializer::PutVariablePayload(
    const core::Variable<T> &variable,
    const typename core::Variable<T>::Info &blockInfo,
    const bool sourceRowMajor, typename core::Variable<T>::Span *span) noexcept
{
    ProfilerStart("buffering");
    if (span != nullptr)
    {
        const size_t blockSize = helper::GetTotalSize(blockInfo.Count);
        if (span->m_Value != T{})
        {
            T *itBegin = reinterpret_cast<T *>(m_Data.m_Buffer.data() +
                                               m_Data.m_Position);

            // TODO: does std::fill_n have a bug in gcc or due to optimizations
            // this is impossible? This seg faults in Release mode only . Even
            // RelWithDebInfo works, replacing with explicit loop below using
            // access operator []
            // std::fill_n(itBegin, blockSize, span->m_Value);

            for (auto i = 0; i < blockSize; ++i)
            {
                itBegin[i] = span->m_Value;
            }
        }

        m_Data.m_Position += blockSize * sizeof(T);
        m_Data.m_AbsolutePosition += blockSize * sizeof(T);
        ProfilerStop("buffering");
        return;
    }

    if (blockInfo.Operations.empty())
    {
        PutPayloadInBuffer(variable, blockInfo, sourceRowMajor);
    }
    else
    {
        PutOperationPayloadInBuffer(variable, blockInfo);
    }

    ProfilerStop("buffering");
}

template <class T>
void BP3Serializer::PutSpanMetadata(
    const core::Variable<T> &variable,
    const typename core::Variable<T>::Span &span) noexcept
{
    if (m_StatsLevel == 0)
    {
        // Get Min/Max from populated data
        ProfilerStart("minmax");
        T min, max;
        helper::GetMinMaxThreads(span.Data(), span.Size(), min, max, m_Threads);
        ProfilerStop("minmax");

        // Put min/max in variable index
        SerialElementIndex &variableIndex =
            m_MetadataSet.VarsIndices.at(variable.m_Name);
        auto &buffer = variableIndex.Buffer;

        const size_t minPosition = span.m_MinMaxMetadataPositions.first;
        const size_t maxPosition = span.m_MinMaxMetadataPositions.second;
        std::copy(&min, &min + 1,
                  reinterpret_cast<T *>(buffer.data() + minPosition));
        std::copy(&max, &max + 1,
                  reinterpret_cast<T *>(buffer.data() + maxPosition));
    }
}

// PRIVATE
template <class T>
size_t
BP3Serializer::PutAttributeHeaderInData(const core::Attribute<T> &attribute,
                                        Stats<T> &stats) noexcept
{
    auto &buffer = m_Data.m_Buffer;
    auto &position = m_Data.m_Position;

    // will go back to write length
    const size_t attributeLengthPosition = position;
    position += 4; // skip length

    helper::CopyToBuffer(buffer, position, &stats.MemberID);
    PutNameRecord(attribute.m_Name, buffer, position);
    position += 2; // skip path

    constexpr int8_t no = 'n';
    helper::CopyToBuffer(buffer, position,
                         &no); // not associated with a Variable

    return attributeLengthPosition;
}

template <class T>
void BP3Serializer::PutAttributeLengthInData(
    const core::Attribute<T> &attribute, Stats<T> &stats,
    const size_t attributeLengthPosition) noexcept
{
    auto &buffer = m_Data.m_Buffer;
    auto &position = m_Data.m_Position;
    auto &absolutePosition = m_Data.m_AbsolutePosition;

    // back to attribute length
    size_t backPosition = attributeLengthPosition;
    uint32_t len = static_cast<uint32_t>(position - attributeLengthPosition);
    helper::CopyToBuffer(buffer, backPosition, &len);

    absolutePosition += position - attributeLengthPosition;
}

template <>
inline void
BP3Serializer::PutAttributeInData(const core::Attribute<std::string> &attribute,
                                  Stats<std::string> &stats) noexcept
{
    const size_t attributeLengthPosition =
        PutAttributeHeaderInData(attribute, stats);

    auto &buffer = m_Data.m_Buffer;
    auto &position = m_Data.m_Position;
    auto &absolutePosition = m_Data.m_AbsolutePosition;

    uint8_t dataType = TypeTraits<std::string>::type_enum;
    if (!attribute.m_IsSingleValue)
    {
        dataType = type_string_array;
    }
    helper::CopyToBuffer(buffer, position, &dataType);

    // here record payload offset
    stats.PayloadOffset = absolutePosition + position - attributeLengthPosition;

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

    PutAttributeLengthInData(attribute, stats, attributeLengthPosition);
}

template <class T>
void BP3Serializer::PutAttributeInData(const core::Attribute<T> &attribute,
                                       Stats<T> &stats) noexcept
{
    const size_t attributeLengthPosition =
        PutAttributeHeaderInData(attribute, stats);

    auto &buffer = m_Data.m_Buffer;
    auto &position = m_Data.m_Position;
    auto &absolutePosition = m_Data.m_AbsolutePosition;

    uint8_t dataType = TypeTraits<T>::type_enum;
    helper::CopyToBuffer(buffer, position, &dataType);

    // here record payload offset
    stats.PayloadOffset = absolutePosition + position - attributeLengthPosition;

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

    PutAttributeLengthInData(attribute, stats, attributeLengthPosition);
}

template <>
inline void BP3Serializer::PutAttributeCharacteristicValueInIndex(
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
void BP3Serializer::PutAttributeCharacteristicValueInIndex(
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
void BP3Serializer::PutAttributeInIndex(const core::Attribute<T> &attribute,
                                        const Stats<T> &stats) noexcept
{
    SerialElementIndex index(stats.MemberID);
    auto &buffer = index.Buffer;

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
inline BP3Serializer::Stats<std::string> BP3Serializer::GetBPStats(
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
BP3Serializer::Stats<T>
BP3Serializer::GetBPStats(const bool singleValue,
                          const typename core::Variable<T>::Info &blockInfo,
                          const bool isRowMajor) noexcept
{
    Stats<T> stats;
    stats.Step = m_MetadataSet.TimeStep;
    stats.FileIndex = GetFileIndex();

    // added to support span
    if (blockInfo.Data == nullptr)
    {
        stats.Min = {};
        stats.Max = {};
        return stats;
    }

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
void BP3Serializer::PutVariableMetadataInData(
    const core::Variable<T> &variable,
    const typename core::Variable<T>::Info &blockInfo,
    const Stats<T> &stats) noexcept
{
    auto &buffer = m_Data.m_Buffer;
    auto &position = m_Data.m_Position;
    auto &absolutePosition = m_Data.m_AbsolutePosition;

    // for writing length at the end
    const size_t varLengthPosition = position;
    position += 8; // skip var length (8)

    helper::CopyToBuffer(buffer, position, &stats.MemberID);

    PutNameRecord(variable.m_Name, buffer, position);
    position += 2; // skip path

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
    PutVariableCharacteristics(variable, blockInfo, stats, buffer, position);

    // Back to varLength including payload size
    // not need to remove its own size (8) from length from bpdump
    const uint64_t varLength = static_cast<uint64_t>(
        position - varLengthPosition +
        helper::PayloadSize(blockInfo.Data, blockInfo.Count));

    size_t backPosition = varLengthPosition;
    helper::CopyToBuffer(buffer, backPosition, &varLength);

    absolutePosition += position - varLengthPosition;
}

template <>
inline void BP3Serializer::PutVariableMetadataInData(
    const core::Variable<std::string> &variable,
    const typename core::Variable<std::string>::Info &blockInfo,
    const Stats<std::string> &stats) noexcept
{
    auto &buffer = m_Data.m_Buffer;
    auto &position = m_Data.m_Position;
    auto &absolutePosition = m_Data.m_AbsolutePosition;

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

    // Back to varLength including payload size
    // not need to remove its own size (8) from length from bpdump
    const uint64_t varLength = static_cast<uint64_t>(
        position - varLengthPosition +
        helper::PayloadSize(blockInfo.Data, blockInfo.Count));

    size_t backPosition = varLengthPosition;
    helper::CopyToBuffer(buffer, backPosition, &varLength);

    absolutePosition += position - varLengthPosition;
}

template <class T>
void BP3Serializer::PutVariableMetadataInIndex(
    const core::Variable<T> &variable,
    const typename core::Variable<T>::Info &blockInfo, const Stats<T> &stats,
    const bool isNew, SerialElementIndex &index,
    typename core::Variable<T>::Span *span) noexcept
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

    PutVariableCharacteristics(variable, blockInfo, stats, buffer, span);
}

template <class T>
void BP3Serializer::PutBoundsRecord(const bool singleValue,
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
void BP3Serializer::PutBoundsRecord(const bool singleValue,
                                    const Stats<T> &stats,
                                    uint8_t &characteristicsCounter,
                                    std::vector<char> &buffer,
                                    size_t &position) noexcept
{
    if (singleValue)
    {
        const uint8_t id = characteristic_value;
        helper::CopyToBuffer(buffer, position, &id);
        // special case required by bpdump
        const uint16_t length = sizeof(T);
        helper::CopyToBuffer(buffer, position, &length);
        helper::CopyToBuffer(buffer, position, &stats.Min);
        ++characteristicsCounter;
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
void BP3Serializer::PutCharacteristicRecord(const uint8_t characteristicID,
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
void BP3Serializer::PutCharacteristicRecord(const uint8_t characteristicID,
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
inline void BP3Serializer::PutVariableCharacteristics(
    const core::Variable<std::string> &variable,
    const core::Variable<std::string>::Info &blockInfo,
    const Stats<std::string> &stats, std::vector<char> &buffer,
    typename core::Variable<std::string>::Span * /*span*/) noexcept
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
void BP3Serializer::PutVariableCharacteristics(
    const core::Variable<T> &variable,
    const typename core::Variable<T>::Info &blockInfo, const Stats<T> &stats,
    std::vector<char> &buffer, typename core::Variable<T>::Span *span) noexcept
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

    if (blockInfo.Data != nullptr || span != nullptr)
    {
        if (m_StatsLevel == 0 && span != nullptr)
        {
            span->m_MinMaxMetadataPositions.first = buffer.size() + 1;
            span->m_MinMaxMetadataPositions.second =
                buffer.size() + 2 + sizeof(T);
        }

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
void BP3Serializer::PutVariableCharacteristics(
    const core::Variable<T> &variable,
    const typename core::Variable<T>::Info &blockInfo, const Stats<T> &stats,
    std::vector<char> &buffer, size_t &position) noexcept
{
    // going back at the end
    const size_t characteristicsCountPosition = position;
    // skip characteristics count(1) + length (4)
    position += 5;
    uint8_t characteristicsCounter = 0;

    // DIMENSIONS
    uint8_t characteristicID = characteristic_dimensions;
    helper::CopyToBuffer(buffer, position, &characteristicID);

    const uint8_t dimensions = static_cast<uint8_t>(blockInfo.Count.size());
    helper::CopyToBuffer(buffer, position, &dimensions); // count
    const uint16_t dimensionsLength = static_cast<uint16_t>(24 * dimensions);
    helper::CopyToBuffer(buffer, position, &dimensionsLength); // length
    PutDimensionsRecord(blockInfo.Count, blockInfo.Shape, blockInfo.Start,
                        buffer, position, true);
    ++characteristicsCounter;

    // VALUE for SCALAR or STAT min, max for ARRAY
    if (blockInfo.Data != nullptr)
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
inline void BP3Serializer::PutPayloadInBuffer(
    const core::Variable<std::string> &variable,
    const typename core::Variable<std::string>::Info &blockInfo,
    const bool /* sourceRowMajor*/) noexcept
{
    PutNameRecord(*blockInfo.Data, m_Data.m_Buffer, m_Data.m_Position);
    m_Data.m_AbsolutePosition += blockInfo.Data->size() + 2;
}

template <class T>
void BP3Serializer::PutPayloadInBuffer(
    const core::Variable<T> &variable,
    const typename core::Variable<T>::Info &blockInfo,
    const bool sourceRowMajor) noexcept
{
    const size_t blockSize = helper::GetTotalSize(blockInfo.Count);
    ProfilerStart("memcpy");
    if (!blockInfo.MemoryStart.empty())
    {
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
void BP3Serializer::UpdateIndexOffsetsCharacteristics(size_t &currentPosition,
                                                      const DataTypes dataType,
                                                      std::vector<char> &buffer)
{
    const bool isLittleEndian = helper::IsLittleEndian();
    const uint8_t characteristicsCount =
        helper::ReadValue<uint8_t>(buffer, currentPosition, isLittleEndian);

    const uint32_t characteristicsLength =
        helper::ReadValue<uint32_t>(buffer, currentPosition, isLittleEndian);

    const size_t endPosition =
        currentPosition + static_cast<size_t>(characteristicsLength);

    while (currentPosition < endPosition)
    {
        const uint8_t id =
            helper::ReadValue<uint8_t>(buffer, currentPosition, isLittleEndian);

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

                    helper::ReadValue<uint16_t>(buffer, currentPosition,
                                                isLittleEndian));

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
            const uint64_t currentOffset = helper::ReadValue<uint64_t>(
                buffer, currentPosition, isLittleEndian);

            const uint64_t updatedOffset =
                currentOffset +
                static_cast<uint64_t>(m_Data.m_AbsolutePosition);

            currentPosition -= sizeof(uint64_t);
            helper::CopyToBuffer(buffer, currentPosition, &updatedOffset);
            break;
        }
        case (characteristic_payload_offset):
        {
            const uint64_t currentPayloadOffset = helper::ReadValue<uint64_t>(
                buffer, currentPosition, isLittleEndian);

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

                helper::ReadValue<uint8_t>(buffer, currentPosition,
                                           isLittleEndian));

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
BP3Serializer::GetAttributeSizeInData(const core::Attribute<T> &attribute) const
    noexcept
{
    size_t size = 14 + attribute.m_Name.size() + 10;
    size += 4 + sizeof(T) * attribute.m_Elements;
    return size;
}

// operations related functions
template <class T>
void BP3Serializer::PutCharacteristicOperation(
    const core::Variable<T> &variable,
    const typename core::Variable<T>::Info &blockInfo,
    std::vector<char> &buffer) noexcept
{
    // TODO: we only take the first operation for now
    const std::map<size_t, std::shared_ptr<BPOperation>> bpOperations =
        SetBPOperations<T>(blockInfo.Operations);

    const size_t operationIndex = bpOperations.begin()->first;
    std::shared_ptr<BPOperation> bpOperation = bpOperations.begin()->second;

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
    bpOperation->SetMetadata(variable, blockInfo, operation, buffer);
}

template <class T>
void BP3Serializer::PutOperationPayloadInBuffer(
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

#endif // ADIOS2_TOOLKIT_FORMAT_BP3_BP3SERIALIZER_TCC_
