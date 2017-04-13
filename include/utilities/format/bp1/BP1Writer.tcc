/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP1Writer.tcc
 *
 *  Created on: Apr 11, 2017
 *      Author: wfg
 */

#include "BP1Writer.h"

namespace adios
{
namespace format
{

// PRIVATE
template <class T, class U>
void BP1Writer::WriteVariableMetadataCommon(const Variable<T> &variable,
                                            Stats<U> &stats,
                                            capsule::STLVector &heap,
                                            BP1MetadataSet &metadataSet) const
    noexcept
{
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

template <class T, class U>
void BP1Writer::WriteVariableMetadataInData(const Variable<T> &variable,
                                            const Stats<U> &stats,
                                            capsule::STLVector &heap) const
    noexcept
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

template <class T, class U>
void BP1Writer::WriteVariableMetadataInIndex(const Variable<T> &variable,
                                             const Stats<U> &stats,
                                             const bool isNew,
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

template <class T, class U>
void BP1Writer::WriteVariableCharacteristics(const Variable<T> &variable,
                                             const Stats<U> &stats,
                                             std::vector<char> &buffer,
                                             const bool addLength) const
    noexcept
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
} // end namespace
