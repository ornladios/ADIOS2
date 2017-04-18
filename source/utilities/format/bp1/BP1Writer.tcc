/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP1Writer.tcc
 *
 *  Created on: Apr 11, 2017
 *      Author: wfg
 */
#ifndef BP1WRITER_TCC_
#define BP1WRITER_TCC_

#include "BP1Writer.h"

namespace adios
{
namespace format
{

// PUBLIC
template <class T>
std::size_t BP1Writer::GetVariableIndexSize(const Variable<T> &variable) const
    noexcept
{
    // size_t indexSize = varEntryLength + memberID + lengthGroupName +
    // groupName + lengthVariableName + lengthOfPath + path + datatype
    std::size_t indexSize = 23; // without characteristics
    indexSize += variable.m_Name.size();

    // characteristics 3 and 4, check variable number of dimensions
    const std::size_t dimensions =
        variable.DimensionsSize(); // commas in CSV + 1
    indexSize += 28 * dimensions;  // 28 bytes per dimension
    indexSize += 1;                // id

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

template <class T>
void BP1Writer::WriteVariableMetadata(const Variable<T> &variable,
                                      capsule::STLVector &heap,
                                      BP1MetadataSet &metadataSet) const
    noexcept
{
    Stats<typename TypeInfo<T>::ValueType> stats = GetStats(variable);

    stats.TimeIndex = metadataSet.TimeStep;
    // Get new Index or point to existing index
    bool isNew = true; // flag to check if variable is new
    BP1Index &varIndex =
        GetBP1Index(variable.m_Name, metadataSet.VarsIndices, isNew);
    stats.MemberID = varIndex.MemberID;

    // write metadata header in data and extract offsets
    stats.Offset = heap.m_DataAbsolutePosition;
    WriteVariableMetadataInData(variable, stats, heap);
    stats.PayloadOffset = heap.m_DataAbsolutePosition;

    // write to metadata  index
    WriteVariableMetadataInIndex(variable, stats, isNew, varIndex);

    ++metadataSet.DataPGVarsCount;
}

template <class T>
void BP1Writer::WriteVariablePayload(const Variable<T> &variable,
                                     capsule::STLVector &heap,
                                     const unsigned int nthreads) const noexcept
{
    // EXPENSIVE part, might want to use threads if large, serial for now
    InsertToBuffer(heap.m_Data, variable.m_AppValues, variable.TotalSize());
    heap.m_DataAbsolutePosition += variable.PayLoadSize();
}

// PRIVATE
template <class T>
BP1Writer::Stats<typename TypeInfo<T>::ValueType>
BP1Writer::GetStats(const Variable<T> &variable) const noexcept
{
    Stats<typename TypeInfo<T>::ValueType> stats;
    const std::size_t valuesSize = variable.TotalSize();

    if (m_Verbosity == 0)
    {
        if (valuesSize >= 10000000) // ten million? this needs actual results
                                    // //we can make decisions for threads
            // based on valuesSize
            GetMinMax(variable.m_AppValues, valuesSize, stats.Min, stats.Max,
                      m_Threads); // here we can add cores from constructor
        else
            GetMinMax(variable.m_AppValues, valuesSize, stats.Min, stats.Max);
    }
    return stats;
}

template <class T>
void BP1Writer::WriteBoundsRecord(const bool isScalar, const Stats<T> &stats,
                                  std::vector<char> &buffer,
                                  std::uint8_t &characteristicsCounter,
                                  const bool addLength) const noexcept
{
    if (isScalar == true)
    {
        WriteCharacteristicRecord(characteristic_value, stats.Min, buffer,
                                  characteristicsCounter,
                                  addLength); // stats.min = stats.max = value
        return;
    }

    if (m_Verbosity == 0) // default verbose
    {
        WriteCharacteristicRecord(characteristic_min, stats.Min, buffer,
                                  characteristicsCounter, addLength);
        WriteCharacteristicRecord(characteristic_max, stats.Max, buffer,
                                  characteristicsCounter, addLength);
    }
}

template <class T>
void BP1Writer::WriteCharacteristicRecord(const std::uint8_t characteristicID,
                                          const T &value,
                                          std::vector<char> &buffer,
                                          std::uint8_t &characteristicsCounter,
                                          const bool addLength) const noexcept
{
    const std::uint8_t id = characteristicID;
    InsertToBuffer(buffer, &id);

    if (addLength == true)
    {
        const std::uint16_t lengthOfCharacteristic = sizeof(T); // id
        InsertToBuffer(buffer, &lengthOfCharacteristic);
    }

    InsertToBuffer(buffer, &value);
    ++characteristicsCounter;
}

template <class T>
void BP1Writer::WriteVariableMetadataInData(
    const Variable<T> &variable,
    const Stats<typename TypeInfo<T>::ValueType> &stats,
    capsule::STLVector &heap) const noexcept
{
    auto &buffer = heap.m_Data;

    // for writing length at the end
    const std::size_t varLengthPosition = buffer.size();

    buffer.insert(buffer.end(), 8, 0);              // skip var length (8)
    InsertToBuffer(buffer, &stats.MemberID);        // memberID
    WriteNameRecord(variable.m_Name, buffer);       // variable name
    buffer.insert(buffer.end(), 2, 0);              // skip path
    const std::uint8_t dataType = GetDataType<T>(); // dataType
    InsertToBuffer(buffer, &dataType);
    constexpr char no = 'n'; // isDimension
    InsertToBuffer(buffer, &no);

    // write variable dimensions
    const std::uint8_t dimensions = variable.m_LocalDimensions.size();
    InsertToBuffer(buffer, &dimensions); // count

    // 27 is from 9 bytes for each: var y/n + local, var y/n + global dimension,
    // var y/n + global offset, changed for characteristic
    std::uint16_t dimensionsLength = 27 * dimensions;
    InsertToBuffer(buffer, &dimensionsLength); // length
    WriteDimensionsRecord(buffer, variable.m_LocalDimensions,
                          variable.m_GlobalDimensions, variable.m_Offsets, 18,
                          true);

    // CHARACTERISTICS
    WriteVariableCharacteristics(variable, stats, buffer, true);

    // Back to varLength including payload size
    const std::uint64_t varLength = buffer.size() - varLengthPosition +
                                    variable.PayLoadSize() -
                                    8; // remove its own size

    CopyToBufferPosition(buffer, varLengthPosition, &varLength); // length

    heap.m_DataAbsolutePosition +=
        buffer.size() - varLengthPosition; // update absolute position to be
                                           // used as payload position
}

template <class T>
void BP1Writer::WriteVariableMetadataInIndex(
    const Variable<T> &variable,
    const Stats<typename TypeInfo<T>::ValueType> &stats, const bool isNew,
    BP1Index &index) const noexcept
{
    auto &buffer = index.Buffer;

    if (isNew == true) // write variable header (might be shared with
                       // attributes index)
    {
        buffer.insert(buffer.end(), 4, 0); // skip var length (4)
        InsertToBuffer(buffer, &stats.MemberID);
        buffer.insert(buffer.end(), 2, 0); // skip group name
        WriteNameRecord(variable.m_Name, buffer);
        buffer.insert(buffer.end(), 2, 0); // skip path

        const std::uint8_t dataType = GetDataType<T>();
        InsertToBuffer(buffer, &dataType);

        // Characteristics Sets Count in Metadata
        index.Count = 1;
        InsertToBuffer(buffer, &index.Count);
    }
    else // update characteristics sets count
    {
        const std::size_t characteristicsSetsCountPosition =
            15 + variable.m_Name.size();
        ++index.Count;
        CopyToBufferPosition(buffer, characteristicsSetsCountPosition,
                             &index.Count); // test
    }

    WriteVariableCharacteristics(variable, stats, buffer);
}

template <class T>
void BP1Writer::WriteVariableCharacteristics(
    const Variable<T> &variable,
    const Stats<typename TypeInfo<T>::ValueType> &stats,
    std::vector<char> &buffer, const bool addLength) const noexcept
{
    const std::size_t characteristicsCountPosition =
        buffer.size(); // very important to track as writer is going back to
                       // this position
    buffer.insert(buffer.end(), 5,
                  0); // skip characteristics count(1) + length (4)
    std::uint8_t characteristicsCounter = 0;

    // DIMENSIONS
    std::uint8_t characteristicID = characteristic_dimensions;
    InsertToBuffer(buffer, &characteristicID);
    const std::uint8_t dimensions = variable.m_LocalDimensions.size();

    if (addLength == true)
    {
        const std::int16_t lengthOfDimensionsCharacteristic =
            24 * dimensions + 3; // 24 = 3 local, global, offset x 8 bytes/each
        InsertToBuffer(buffer, &lengthOfDimensionsCharacteristic);
    }

    InsertToBuffer(buffer, &dimensions); // count
    const std::uint16_t dimensionsLength = 24 * dimensions;
    InsertToBuffer(buffer, &dimensionsLength); // length
    WriteDimensionsRecord(buffer, variable.m_LocalDimensions,
                          variable.m_GlobalDimensions, variable.m_Offsets, 16,
                          addLength);
    ++characteristicsCounter;

    // VALUE for SCALAR or STAT min, max for ARRAY
    WriteBoundsRecord(variable.m_IsScalar, stats, buffer,
                      characteristicsCounter, addLength);
    // TIME INDEX
    WriteCharacteristicRecord(characteristic_time_index, stats.TimeIndex,
                              buffer, characteristicsCounter, addLength);

    if (addLength == false) // only in metadata offset and payload offset
    {
        WriteCharacteristicRecord(characteristic_offset, stats.Offset, buffer,
                                  characteristicsCounter);
        WriteCharacteristicRecord(characteristic_payload_offset,
                                  stats.PayloadOffset, buffer,
                                  characteristicsCounter);
    }
    // END OF CHARACTERISTICS

    // Back to characteristics count and length
    CopyToBufferPosition(buffer, characteristicsCountPosition,
                         &characteristicsCounter); // count (1)
    const std::uint32_t characteristicsLength =
        buffer.size() - characteristicsCountPosition - 4 -
        1; // remove its own length (4) + characteristic counter (1)

    CopyToBufferPosition(buffer, characteristicsCountPosition + 1,
                         &characteristicsLength); // length
}

} // end namespace format
} // end namespace adios

#endif // BP1WRITER_TCC_
