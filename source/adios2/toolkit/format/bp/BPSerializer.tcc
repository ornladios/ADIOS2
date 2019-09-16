/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPSerializer.tcc
 *
 *  Created on: Sep 16, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP_BPSERIALIZER_TCC_
#define ADIOS2_TOOLKIT_FORMAT_BP_BPSERIALIZER_TCC_

#include "BPSerializer.h"

namespace adios2
{
namespace format
{

template <class T>
inline void BPSerializer::PutAttributeCharacteristicValueInIndex(
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
void BPSerializer::PutCharacteristicRecord(const uint8_t characteristicID,
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
void BPSerializer::PutCharacteristicRecord(const uint8_t characteristicID,
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

template <class T>
inline void BPSerializer::PutPayloadInBuffer(
    const core::Variable<T> &variable,
    const typename core::Variable<T>::Info &blockInfo,
    const bool sourceRowMajor) noexcept
{
    const size_t blockSize = helper::GetTotalSize(blockInfo.Count);
    m_Profiler.Start("memcpy");
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
                                    blockInfo.Data, blockSize,
                                    m_Parameters.Threads);
    }
    m_Profiler.Stop("memcpy");
    m_Data.m_AbsolutePosition += blockSize * sizeof(T); // payload size
}

// PRIVATE
template <class T>
void BPSerializer::UpdateIndexOffsetsCharacteristics(size_t &currentPosition,
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

    size_t dimensionsSize = 0; // get it from dimensions characteristics

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
        case (characteristic_minmax):
        {
            // first get the number of subblocks
            const uint16_t M =
                helper::ReadValue<uint16_t>(buffer, currentPosition);
            currentPosition += 2 * sizeof(T); // block min/max
            if (M > 1)
            {
                currentPosition += 1 + 4; // method, blockSize
                currentPosition +=
                    dimensionsSize * sizeof(uint16_t); // N-dim division
                currentPosition += 2 * M * sizeof(T);  // M * min/max
            }
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
            dimensionsSize = static_cast<size_t>(helper::ReadValue<uint8_t>(
                buffer, currentPosition, isLittleEndian));

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

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BP_BPSERIALIZER_TCC_ */
