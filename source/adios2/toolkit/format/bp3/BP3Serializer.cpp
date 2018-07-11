/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP3Serializer.cpp
 *
 *  Created on: Feb 1, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "BP3Serializer.h"
#include "BP3Serializer.tcc"

#include <chrono>
#include <future>
#include <string>
#include <vector>

#include "adios2/helper/adiosFunctions.h" //helper::GetType<T>, helper::ReadValue<T>,
                                          // ReduceValue<T>

#ifdef _WIN32
#pragma warning(disable : 4503) // Windows complains about SubFileInfoMap levels
#endif

namespace adios2
{
namespace format
{

std::mutex BP3Serializer::m_Mutex;

BP3Serializer::BP3Serializer(MPI_Comm mpiComm, const bool debugMode)
: BP3Base(mpiComm, debugMode)
{
}

void BP3Serializer::PutProcessGroupIndex(
    const std::string &ioName, const std::string hostLanguage,
    const std::vector<std::string> &transportsTypes) noexcept
{
    ProfilerStart("buffering");
    std::vector<char> &metadataBuffer = m_MetadataSet.PGIndex.Buffer;

    std::vector<char> &dataBuffer = m_Data.m_Buffer;
    size_t &dataPosition = m_Data.m_Position;

    m_MetadataSet.DataPGLengthPosition = dataPosition;
    dataPosition += 8; // skip pg length (8)

    const std::size_t metadataPGLengthPosition = metadataBuffer.size();
    metadataBuffer.insert(metadataBuffer.end(), 2, '\0'); // skip pg length (2)

    // write name to metadata
    PutNameRecord(ioName, metadataBuffer);

    // write if data is column major in metadata and data
    const char columnMajor =
        (helper::IsRowMajor(hostLanguage) == false) ? 'y' : 'n';
    helper::InsertToBuffer(metadataBuffer, &columnMajor);
    helper::CopyToBuffer(dataBuffer, dataPosition, &columnMajor);

    // write name in data
    PutNameRecord(ioName, dataBuffer, dataPosition);

    // processID in metadata,
    const uint32_t processID = static_cast<uint32_t>(m_RankMPI);
    helper::InsertToBuffer(metadataBuffer, &processID);
    // skip coordination var in data ....what is coordination var?
    dataPosition += 4;

    // time step name to metadata and data
    const std::string timeStepName(std::to_string(m_MetadataSet.TimeStep));
    PutNameRecord(timeStepName, metadataBuffer);
    PutNameRecord(timeStepName, dataBuffer, dataPosition);

    // time step to metadata and data
    helper::InsertToBuffer(metadataBuffer, &m_MetadataSet.TimeStep);
    helper::CopyToBuffer(dataBuffer, dataPosition, &m_MetadataSet.TimeStep);

    // offset to pg in data in metadata which is the current absolute position
    helper::InsertU64(metadataBuffer, m_Data.m_AbsolutePosition);

    // Back to writing metadata pg index length (length of group)
    const uint16_t metadataPGIndexLength = static_cast<uint16_t>(
        metadataBuffer.size() - metadataPGLengthPosition - 2);

    size_t backPosition = metadataPGLengthPosition;
    helper::CopyToBuffer(metadataBuffer, backPosition, &metadataPGIndexLength);
    // DONE With metadataBuffer

    // here write method in data
    const std::vector<uint8_t> methodIDs = GetTransportIDs(transportsTypes);
    const uint8_t methodsCount = static_cast<uint8_t>(methodIDs.size());
    helper::CopyToBuffer(dataBuffer, dataPosition, &methodsCount); // count
    // methodID (1) + method params length(2), no parameters for now
    const uint16_t methodsLength = static_cast<uint16_t>(methodsCount * 3);

    helper::CopyToBuffer(dataBuffer, dataPosition, &methodsLength); // length

    for (const auto methodID : methodIDs)
    {
        helper::CopyToBuffer(dataBuffer, dataPosition, &methodID); // method ID,
        dataPosition += 2; // skip method params length = 0 (2 bytes) for now
    }

    // update absolute position
    m_Data.m_AbsolutePosition +=
        dataPosition - m_MetadataSet.DataPGLengthPosition;
    // pg vars count and position
    m_MetadataSet.DataPGVarsCount = 0;
    m_MetadataSet.DataPGVarsCountPosition = dataPosition;
    // add vars count and length
    dataPosition += 12;
    m_Data.m_AbsolutePosition += 12; // add vars count and length

    ++m_MetadataSet.DataPGCount;
    m_MetadataSet.DataPGIsOpen = true;

    ProfilerStop("buffering");
}

void BP3Serializer::SerializeData(core::IO &io, const bool advanceStep)
{
    ProfilerStart("buffering");
    SerializeDataBuffer(io);
    if (advanceStep)
    {
        ++m_MetadataSet.TimeStep;
        ++m_MetadataSet.CurrentStep;
    }
    ProfilerStop("buffering");
}

void BP3Serializer::CloseData(core::IO &io)
{
    ProfilerStart("buffering");

    if (!m_IsClosed)
    {
        if (m_MetadataSet.DataPGIsOpen)
        {
            SerializeDataBuffer(io);
        }

        SerializeMetadataInData();

        if (m_Profiler.IsActive)
        {
            m_Profiler.Bytes.at("buffering") = m_Data.m_AbsolutePosition;
        }

        m_IsClosed = true;
    }

    ProfilerStop("buffering");
}

void BP3Serializer::CloseStream(core::IO &io, const bool addMetadata)
{
    ProfilerStart("buffering");
    if (m_MetadataSet.DataPGIsOpen)
    {
        SerializeDataBuffer(io);
    }

    SerializeMetadataInData(false, addMetadata);

    if (m_Profiler.IsActive)
    {
        m_Profiler.Bytes.at("buffering") += m_Data.m_Position;
    }
    ProfilerStop("buffering");
}

void BP3Serializer::CloseStream(core::IO &io, size_t &metadataStart,
                                size_t &metadataCount, const bool addMetadata)
{

    ProfilerStart("buffering");
    if (m_MetadataSet.DataPGIsOpen)
    {
        SerializeDataBuffer(io);
    }

    metadataStart = m_Data.m_Position;

    SerializeMetadataInData(false, addMetadata);

    metadataCount = m_Data.m_Position - metadataStart;

    if (m_Profiler.IsActive)
    {
        m_Profiler.Bytes.at("buffering") += m_Data.m_Position;
    }
    ProfilerStop("buffering");
}

void BP3Serializer::ResetIndices()
{
    m_MetadataSet.AttributesIndices.clear();
    m_MetadataSet.VarsIndices.clear();
}

std::string BP3Serializer::GetRankProfilingJSON(
    const std::vector<std::string> &transportsTypes,
    const std::vector<profiling::IOChrono *> &transportsProfilers) noexcept
{
    auto lf_WriterTimer = [](std::string &rankLog,
                             const profiling::Timer &timer) {
        rankLog += "\"" + timer.m_Process + "_" + timer.GetShortUnits() +
                   "\": " + std::to_string(timer.m_ProcessTime) + ", ";
    };

    // prepare string dictionary per rank
    std::string rankLog("{ \"rank\": " + std::to_string(m_RankMPI) + ", ");

    auto &profiler = m_Profiler;

    std::string timeDate(profiler.Timers.at("buffering").m_LocalTimeDate);
    timeDate.pop_back();
    // avoid whitespace
    std::replace(timeDate.begin(), timeDate.end(), ' ', '_');

    rankLog += "\"start\": \"" + timeDate + "\", ";
    rankLog += "\"threads\": " + std::to_string(m_Threads) + ", ";
    rankLog +=
        "\"bytes\": " + std::to_string(profiler.Bytes.at("buffering")) + ", ";
    lf_WriterTimer(rankLog, profiler.Timers.at("buffering"));
    lf_WriterTimer(rankLog, profiler.Timers.at("memcpy"));
    lf_WriterTimer(rankLog, profiler.Timers.at("minmax"));
    lf_WriterTimer(rankLog, profiler.Timers.at("meta_sort_merge"));

    const size_t transportsSize = transportsTypes.size();

    for (unsigned int t = 0; t < transportsSize; ++t)
    {
        rankLog += "\"transport_" + std::to_string(t) + "\": { ";
        rankLog += "\"type\": \"" + transportsTypes[t] + "\", ";

        for (const auto &transportTimerPair : transportsProfilers[t]->Timers)
        {
            lf_WriterTimer(rankLog, transportTimerPair.second);
        }
        // replace last comma with space
        rankLog.pop_back();
        rankLog.pop_back();
        rankLog += " ";

        if (t == transportsSize - 1) // last element
        {
            rankLog += "}";
        }
        else
        {
            rankLog += "},";
        }
    }
    rankLog += " }"; // end rank entry

    return rankLog;
}

std::vector<char>
BP3Serializer::AggregateProfilingJSON(const std::string &rankProfilingLog)
{
    return SetCollectiveProfilingJSON(rankProfilingLog);
}

void BP3Serializer::AggregateCollectiveMetadata(MPI_Comm comm,
                                                BufferSTL &bufferSTL,
                                                const bool inMetadataBuffer)
{
    ProfilerStart("buffering");
    ProfilerStart("meta_sort_merge");

    auto &position = bufferSTL.m_Position;

    const uint64_t pgIndexStart =
        inMetadataBuffer ? position : position + bufferSTL.m_AbsolutePosition;
    AggregateIndex(m_MetadataSet.PGIndex, m_MetadataSet.DataPGCount, comm,
                   bufferSTL);

    const uint64_t variablesIndexStart =
        inMetadataBuffer ? position : position + bufferSTL.m_AbsolutePosition;
    AggregateMergeIndex(m_MetadataSet.VarsIndices, comm, bufferSTL);

    const uint64_t attributesIndexStart =
        inMetadataBuffer ? position : position + bufferSTL.m_AbsolutePosition;
    AggregateMergeIndex(m_MetadataSet.AttributesIndices, comm, bufferSTL);

    int rank;
    MPI_Comm_rank(comm, &rank);
    if (rank == 0)
    {
        PutMinifooter(pgIndexStart, variablesIndexStart, attributesIndexStart,
                      bufferSTL.m_Buffer, bufferSTL.m_Position,
                      inMetadataBuffer);

        if (inMetadataBuffer)
        {
            bufferSTL.m_AbsolutePosition = bufferSTL.m_Position;
        }
        else
        {
            bufferSTL.m_AbsolutePosition += bufferSTL.m_Position;
        }
    }

    ProfilerStop("meta_sort_merge");
    ProfilerStop("buffering");
}

void BP3Serializer::UpdateOffsetsInMetadata()
{
    auto lf_UpdatePGIndexOffsets = [&]() {

        auto &buffer = m_MetadataSet.PGIndex.Buffer;
        size_t &currentPosition = m_MetadataSet.PGIndex.LastUpdatedPosition;

        while (currentPosition < buffer.size())
        {
            ProcessGroupIndex pgIndex =
                ReadProcessGroupIndexHeader(buffer, currentPosition);

            const uint64_t updatedOffset =
                pgIndex.Offset +
                static_cast<uint64_t>(m_Data.m_AbsolutePosition);
            currentPosition -= sizeof(uint64_t);
            helper::CopyToBuffer(buffer, currentPosition, &updatedOffset);
        }
    };

    auto lf_UpdateIndexOffsets = [&](SerialElementIndex &index) {

        auto &buffer = index.Buffer;

        // First get the type:
        size_t headerPosition = 0;
        ElementIndexHeader header =
            ReadElementIndexHeader(buffer, headerPosition);
        const DataTypes dataTypeEnum = static_cast<DataTypes>(header.DataType);

        size_t &currentPosition = index.LastUpdatedPosition;

        while (currentPosition < buffer.size())
        {
            switch (dataTypeEnum)
            {

            case (type_string):
            {
                // do nothing, strings are obtained from metadata
                currentPosition = buffer.size();
                break;
            }

            case (type_byte):
            {
                UpdateIndexOffsetsCharacteristics<char>(currentPosition,
                                                        type_byte, buffer);
                break;
            }

            case (type_short):
            {
                UpdateIndexOffsetsCharacteristics<short>(currentPosition,
                                                         type_short, buffer);
                break;
            }

            case (type_integer):
            {
                UpdateIndexOffsetsCharacteristics<int>(currentPosition,
                                                       type_integer, buffer);
                break;
            }

            case (type_long):
            {
                UpdateIndexOffsetsCharacteristics<int64_t>(currentPosition,
                                                           type_long, buffer);

                break;
            }

            case (type_unsigned_byte):
            {
                UpdateIndexOffsetsCharacteristics<unsigned char>(
                    currentPosition, type_unsigned_byte, buffer);

                break;
            }

            case (type_unsigned_short):
            {
                UpdateIndexOffsetsCharacteristics<unsigned short>(
                    currentPosition, type_unsigned_short, buffer);

                break;
            }

            case (type_unsigned_integer):
            {
                UpdateIndexOffsetsCharacteristics<unsigned int>(
                    currentPosition, type_unsigned_integer, buffer);

                break;
            }

            case (type_unsigned_long):
            {
                UpdateIndexOffsetsCharacteristics<uint64_t>(
                    currentPosition, type_unsigned_long, buffer);

                break;
            }

            case (type_real):
            {
                UpdateIndexOffsetsCharacteristics<float>(currentPosition,
                                                         type_real, buffer);
                break;
            }

            case (type_double):
            {
                UpdateIndexOffsetsCharacteristics<double>(currentPosition,
                                                          type_double, buffer);

                break;
            }

            default:
                // TODO: complex, long double
                throw std::invalid_argument(
                    "ERROR: type " + std::to_string(header.DataType) +
                    " not supported in updating aggregated offsets\n");

            } // end switch
        }
    };

    // BODY OF FUNCTION STARTS HERE
    if (m_Aggregator.m_IsConsumer)
    {
        return;
    }

    // PG Indices
    lf_UpdatePGIndexOffsets();

    // Variable Indices
    for (auto &varIndexPair : m_MetadataSet.VarsIndices)
    {
        SerialElementIndex &index = varIndexPair.second;
        lf_UpdateIndexOffsets(index);
    }
}

// PRIVATE FUNCTIONS
void BP3Serializer::PutAttributes(core::IO &io)
{
    const auto attributesDataMap = io.GetAttributesDataMap();

    auto &buffer = m_Data.m_Buffer;
    auto &position = m_Data.m_Position;
    auto &absolutePosition = m_Data.m_AbsolutePosition;

    const size_t attributesCountPosition = position;

    // count is known ahead of time
    const uint32_t attributesCount =
        static_cast<uint32_t>(attributesDataMap.size());
    helper::CopyToBuffer(buffer, position, &attributesCount);

    // will go back
    const size_t attributesLengthPosition = position;
    position += 8; // skip attributes length

    absolutePosition += position - attributesCountPosition;

    uint32_t memberID = 0;

    for (const auto &attributePair : attributesDataMap)
    {
        const std::string name(attributePair.first);
        const std::string type(attributePair.second.first);

        if (type == "unknown")
        {
        }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        Stats<T> stats;                                                        \
        stats.Offset = absolutePosition;                                       \
        stats.MemberID = memberID;                                             \
        stats.Step = m_MetadataSet.TimeStep;                                   \
        stats.FileIndex = GetFileIndex();                                      \
        core::Attribute<T> &attribute = *io.InquireAttribute<T>(name);         \
        PutAttributeInData(attribute, stats);                                  \
        PutAttributeInIndex(attribute, stats);                                 \
    }
        ADIOS2_FOREACH_ATTRIBUTE_TYPE_1ARG(declare_type)
#undef declare_type

        ++memberID;
    }

    // complete attributes length
    const uint64_t attributesLength =
        static_cast<uint64_t>(position - attributesLengthPosition);

    size_t backPosition = attributesLengthPosition;
    helper::CopyToBuffer(buffer, backPosition, &attributesLength);
}

void BP3Serializer::PutDimensionsRecord(const Dims &localDimensions,
                                        const Dims &globalDimensions,
                                        const Dims &offsets,
                                        std::vector<char> &buffer) noexcept
{
    if (offsets.empty())
    {
        for (const auto localDimension : localDimensions)
        {
            helper::InsertU64(buffer, localDimension);
            buffer.insert(buffer.end(), 2 * sizeof(uint64_t), '\0');
        }
    }
    else
    {
        for (unsigned int d = 0; d < localDimensions.size(); ++d)
        {
            helper::InsertU64(buffer, localDimensions[d]);
            helper::InsertU64(buffer, globalDimensions[d]);
            helper::InsertU64(buffer, offsets[d]);
        }
    }
}

void BP3Serializer::PutDimensionsRecord(const Dims &localDimensions,
                                        const Dims &globalDimensions,
                                        const Dims &offsets,
                                        std::vector<char> &buffer,
                                        size_t &position,
                                        const bool isCharacteristic) noexcept
{
    auto lf_CopyDimension = [](std::vector<char> &buffer, size_t &position,
                               const size_t dimension,
                               const bool isCharacteristic) {
        if (!isCharacteristic)
        {
            constexpr char no = 'n';
            helper::CopyToBuffer(buffer, position, &no);
        }

        const uint64_t dimension64 = static_cast<uint64_t>(dimension);

        helper::CopyToBuffer(buffer, position, &dimension64);
    };

    // BODY Starts here
    if (offsets.empty())
    {
        unsigned int globalBoundsSkip = 18;
        if (isCharacteristic)
        {
            globalBoundsSkip = 16;
        }

        for (const auto &localDimension : localDimensions)
        {
            lf_CopyDimension(buffer, position, localDimension,
                             isCharacteristic);
            position += globalBoundsSkip;
        }
    }
    else
    {
        for (unsigned int d = 0; d < localDimensions.size(); ++d)
        {
            lf_CopyDimension(buffer, position, localDimensions[d],
                             isCharacteristic);
            lf_CopyDimension(buffer, position, globalDimensions[d],
                             isCharacteristic);
            lf_CopyDimension(buffer, position, offsets[d], isCharacteristic);
        }
    }
}

void BP3Serializer::PutNameRecord(const std::string name,
                                  std::vector<char> &buffer) noexcept
{
    const uint16_t length = static_cast<uint16_t>(name.length());
    helper::InsertToBuffer(buffer, &length);
    helper::InsertToBuffer(buffer, name.c_str(), length);
}

void BP3Serializer::PutNameRecord(const std::string name,
                                  std::vector<char> &buffer,
                                  size_t &position) noexcept
{
    const uint16_t length = static_cast<uint16_t>(name.length());
    helper::CopyToBuffer(buffer, position, &length);
    helper::CopyToBuffer(buffer, position, name.c_str(), length);
}

BP3Serializer::SerialElementIndex &BP3Serializer::GetSerialElementIndex(
    const std::string &name,
    std::unordered_map<std::string, SerialElementIndex> &indices,
    bool &isNew) const noexcept
{
    auto itName = indices.find(name);
    if (itName == indices.end())
    {
        indices.emplace(
            name, SerialElementIndex(static_cast<uint32_t>(indices.size())));
        isNew = true;
        return indices.at(name);
    }

    isNew = false;
    return itName->second;
}

void BP3Serializer::SerializeDataBuffer(core::IO &io) noexcept
{
    auto &buffer = m_Data.m_Buffer;
    auto &position = m_Data.m_Position;
    auto &absolutePosition = m_Data.m_AbsolutePosition;

    // vars count and Length (only for PG)
    helper::CopyToBuffer(buffer, m_MetadataSet.DataPGVarsCountPosition,
                         &m_MetadataSet.DataPGVarsCount);
    // without record itself and vars count
    const uint64_t varsLength =
        position - m_MetadataSet.DataPGVarsCountPosition - 8 - 4;
    helper::CopyToBuffer(buffer, m_MetadataSet.DataPGVarsCountPosition,
                         &varsLength);

    // attributes are only written once

    if (!m_MetadataSet.AreAttributesWritten)
    {
        const size_t attributesSizeInData = GetAttributesSizeInData(io);
        m_Data.Resize(position + attributesSizeInData,
                      "when writing Attributes in rank=0\n");

        PutAttributes(io);
        m_MetadataSet.AreAttributesWritten = true;
    }
    else
    {
        m_Data.Resize(position + 12, "for empty Attributes\n");
        position += 12;
        absolutePosition += 12;
    }

    // Finish writing pg group length without record itself
    const uint64_t dataPGLength =
        position - m_MetadataSet.DataPGLengthPosition - 8;
    helper::CopyToBuffer(buffer, m_MetadataSet.DataPGLengthPosition,
                         &dataPGLength);

    m_MetadataSet.DataPGIsOpen = false;
}

void BP3Serializer::SerializeMetadataInData(const bool updateAbsolutePosition,
                                            const bool inData)
{
    auto lf_SetIndexCountLength =
        [](std::unordered_map<std::string, SerialElementIndex> &indices,
           uint32_t &count, uint64_t &length) {

            count = static_cast<uint32_t>(indices.size());
            length = 0;
            for (auto &indexPair : indices) // set each index length
            {
                auto &indexBuffer = indexPair.second.Buffer;
                const uint32_t indexLength =
                    static_cast<uint32_t>(indexBuffer.size() - 4);
                size_t indexLengthPosition = 0;
                helper::CopyToBuffer(indexBuffer, indexLengthPosition,
                                     &indexLength);

                length += indexBuffer.size(); // overall length
            }
        };

    auto lf_FlattenIndices =
        [](const uint32_t count, const uint64_t length,
           const std::unordered_map<std::string, SerialElementIndex> &indices,
           std::vector<char> &buffer, size_t &position) {

            helper::CopyToBuffer(buffer, position, &count);
            helper::CopyToBuffer(buffer, position, &length);

            for (const auto &indexPair : indices) // set each index length
            {
                const auto &indexBuffer = indexPair.second.Buffer;
                helper::CopyToBuffer(buffer, position, indexBuffer.data(),
                                     indexBuffer.size());
            }
        };

    // Finish writing metadata counts and lengths
    // PG Index
    const uint64_t pgCount = m_MetadataSet.DataPGCount;
    const uint64_t pgLength = m_MetadataSet.PGIndex.Buffer.size();

    // var index count and length (total), and each index length
    uint32_t varsCount = 0;
    uint64_t varsLength = 0;
    lf_SetIndexCountLength(m_MetadataSet.VarsIndices, varsCount, varsLength);

    // attribute index count and length, and each index length
    uint32_t attributesCount = 0;
    uint64_t attributesLength = 0;
    lf_SetIndexCountLength(m_MetadataSet.AttributesIndices, attributesCount,
                           attributesLength);

    if (!inData)
    {
        return;
    }

    const size_t footerSize = static_cast<size_t>(
        (pgLength + 16) + (varsLength + 12) + (attributesLength + 12) +
        m_MetadataSet.MiniFooterSize);

    auto &buffer = m_Data.m_Buffer;
    auto &position = m_Data.m_Position;
    auto &absolutePosition = m_Data.m_AbsolutePosition;

    // reserve data to fit metadata,
    // must replace with growth buffer strategy?
    m_Data.Resize(position + footerSize,
                  " when writing metadata in bp data buffer");

    // write pg index
    helper::CopyToBuffer(buffer, position, &pgCount);
    helper::CopyToBuffer(buffer, position, &pgLength);
    helper::CopyToBuffer(buffer, position, m_MetadataSet.PGIndex.Buffer.data(),
                         static_cast<size_t>(pgLength));

    // Vars indices
    lf_FlattenIndices(varsCount, varsLength, m_MetadataSet.VarsIndices, buffer,
                      position);
    // Attribute indices
    lf_FlattenIndices(attributesCount, attributesLength,
                      m_MetadataSet.AttributesIndices, buffer, position);

    // getting absolute offset start, minifooter is 28 bytes for now
    const uint64_t pgIndexStart = static_cast<uint64_t>(absolutePosition);
    const uint64_t variablesIndexStart =
        static_cast<uint64_t>(pgIndexStart + (pgLength + 16));
    const uint64_t attributesIndexStart =
        static_cast<uint64_t>(variablesIndexStart + (varsLength + 12));

    PutMinifooter(pgIndexStart, variablesIndexStart, attributesIndexStart,
                  buffer, position);

    if (updateAbsolutePosition)
    {
        absolutePosition += footerSize;
    }

    if (m_Profiler.IsActive)
    {
        m_Profiler.Bytes.emplace("buffering", absolutePosition);
    }
}

void BP3Serializer::PutMinifooter(const uint64_t pgIndexStart,
                                  const uint64_t variablesIndexStart,
                                  const uint64_t attributesIndexStart,
                                  std::vector<char> &buffer, size_t &position,
                                  const bool addSubfiles)
{
    auto lf_CopyVersionChar = [](const std::string version,
                                 std::vector<char> &buffer, size_t &position) {
        helper::CopyToBuffer(buffer, position, version.c_str());
    };

    const std::string majorVersion(std::to_string(ADIOS2_VERSION_MAJOR));
    const std::string minorVersion(std::to_string(ADIOS2_VERSION_MINOR));
    const std::string patchVersion(std::to_string(ADIOS2_VERSION_PATCH));

    const std::string versionLongTag("ADIOS-BP v" + majorVersion + "." +
                                     minorVersion + "." + patchVersion);
    const size_t versionLongTagSize = versionLongTag.size();
    if (versionLongTagSize < 24)
    {
        helper::CopyToBuffer(buffer, position, versionLongTag.c_str(),
                             versionLongTagSize);
        position += 24 - versionLongTagSize;
    }
    else
    {
        helper::CopyToBuffer(buffer, position, versionLongTag.c_str(), 24);
    }

    lf_CopyVersionChar(majorVersion, buffer, position);
    lf_CopyVersionChar(minorVersion, buffer, position);
    lf_CopyVersionChar(patchVersion, buffer, position);
    ++position;

    helper::CopyToBuffer(buffer, position, &pgIndexStart);
    helper::CopyToBuffer(buffer, position, &variablesIndexStart);
    helper::CopyToBuffer(buffer, position, &attributesIndexStart);

    const uint8_t endianness = helper::IsLittleEndian() ? 0 : 1;
    helper::CopyToBuffer(buffer, position, &endianness);

    if (addSubfiles)
    {
        position += 1;
        helper::CopyToBuffer(buffer, position, &m_Version);
    }
    else
    {
        position += 2;
    }
    helper::CopyToBuffer(buffer, position, &m_Version);
}

void BP3Serializer::AggregateIndex(const SerialElementIndex &index,
                                   const size_t count, MPI_Comm comm,
                                   BufferSTL &bufferSTL)
{
    auto &buffer = bufferSTL.m_Buffer;
    auto &position = bufferSTL.m_Position;
    int rank;
    MPI_Comm_rank(comm, &rank);

    size_t countPosition = position;
    const size_t totalCount = helper::ReduceValues<size_t>(count, comm);

    if (rank == 0)
    {
        // Write count
        position += 16;
        bufferSTL.Resize(position, " in call to AggregateIndex BP3 metadata");
        const uint64_t totalCountU64 = static_cast<uint64_t>(totalCount);
        helper::CopyToBuffer(buffer, countPosition, &totalCountU64);
    }

    // write contents
    helper::GathervVectors(index.Buffer, buffer, position, comm);

    // get total length and write it after count and before index
    if (rank == 0)
    {
        const uint64_t totalLengthU64 =
            static_cast<uint64_t>(position - countPosition - 8);
        helper::CopyToBuffer(buffer, countPosition, &totalLengthU64);
    }
}

void BP3Serializer::AggregateMergeIndex(
    const std::unordered_map<std::string, SerialElementIndex> &indices,
    MPI_Comm comm, BufferSTL &bufferSTL)
{
    // first serialize index
    std::vector<char> serializedIndices = SerializeIndices(indices, comm);
    // gather in rank 0
    std::vector<char> gatheredSerialIndices;
    size_t gatheredSerialIndicesPosition = 0;

    helper::GathervVectors(serializedIndices, gatheredSerialIndices,
                           gatheredSerialIndicesPosition, comm);

    // deallocate local serialized Indices
    std::vector<char>().swap(serializedIndices);

    // deserialize in [name][rank] order
    const std::unordered_map<std::string, std::vector<SerialElementIndex>>
        nameRankIndices =
            DeserializeIndicesPerRankThreads(gatheredSerialIndices, comm);

    // deallocate gathered serial indices (full in rank 0 only)
    std::vector<char>().swap(gatheredSerialIndices);

    int rank;
    MPI_Comm_rank(comm, &rank);

    if (rank == 0)
    {
        // to write count and length
        auto &buffer = bufferSTL.m_Buffer;
        auto &position = bufferSTL.m_Position;
        size_t countPosition = position;

        // Write count
        position += 12;
        bufferSTL.Resize(position + gatheredSerialIndicesPosition +
                             m_MetadataSet.MiniFooterSize,
                         ", in call to AggregateMergeIndex BP3 metadata");
        const uint32_t totalCountU32 =
            static_cast<uint32_t>(nameRankIndices.size());
        helper::CopyToBuffer(buffer, countPosition, &totalCountU32);

        MergeSerializeIndices(nameRankIndices, comm, bufferSTL);

        // Write length
        const uint64_t totalLengthU64 =
            static_cast<uint64_t>(position - countPosition - 8);
        helper::CopyToBuffer(buffer, countPosition, &totalLengthU64);
    }
}

std::vector<char> BP3Serializer::SerializeIndices(
    const std::unordered_map<std::string, SerialElementIndex> &indices,
    MPI_Comm comm) const noexcept
{
    // pre-allocate
    size_t serializedIndicesSize = 0;
    for (const auto &indexPair : indices)
    {
        const SerialElementIndex &index = indexPair.second;
        serializedIndicesSize += 4 + index.Buffer.size();
    }

    std::vector<char> serializedIndices;
    serializedIndices.reserve(serializedIndicesSize);

    int rank;
    MPI_Comm_rank(comm, &rank);

    for (const auto &indexPair : indices)
    {
        const SerialElementIndex &index = indexPair.second;

        // add rank at the beginning
        const uint32_t rankSource = static_cast<uint32_t>(rank);
        helper::InsertToBuffer(serializedIndices, &rankSource);

        // insert buffer
        helper::InsertToBuffer(serializedIndices, index.Buffer.data(),
                               index.Buffer.size());
    }

    return serializedIndices;
}

std::unordered_map<std::string, std::vector<BP3Base::SerialElementIndex>>
BP3Serializer::DeserializeIndicesPerRankThreads(
    const std::vector<char> &serialized, MPI_Comm comm) const noexcept
{
    std::unordered_map<std::string, std::vector<SerialElementIndex>>
        deserialized;

    auto lf_Deserialize = [&](const int rankSource,
                              const size_t serializedPosition) {

        size_t localPosition = serializedPosition;
        ElementIndexHeader header =
            ReadElementIndexHeader(serialized, localPosition);

        std::vector<BP3Base::SerialElementIndex> *deserializedIndexes;
        // mutex portion
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            deserializedIndexes =
                &(deserialized
                      .emplace(std::piecewise_construct,
                               std::forward_as_tuple(header.Name),
                               std::forward_as_tuple(
                                   m_SizeMPI,
                                   SerialElementIndex(header.MemberID, 0)))
                      .first->second);
        }

        const size_t bufferSize = static_cast<size_t>(header.Length) + 4;
        SerialElementIndex &index = deserializedIndexes->at(rankSource);
        helper::InsertToBuffer(index.Buffer, &serialized[serializedPosition],
                               bufferSize);
    };

    // BODY OF FUNCTION starts here
    const size_t serializedSize = serialized.size();
    int rank;
    MPI_Comm_rank(comm, &rank);

    if (rank != 0 || serializedSize < 8)
    {
        return deserialized;
    }

    size_t serializedPosition = 0;

    if (m_Threads == 1)
    {
        while (serializedPosition < serializedSize)
        {
            const int rankSource = static_cast<int>(
                helper::ReadValue<uint32_t>(serialized, serializedPosition));

            if (serializedPosition <= serializedSize)
            {
                lf_Deserialize(rankSource, serializedPosition);
            }

            const size_t bufferSize = static_cast<size_t>(
                helper::ReadValue<uint32_t>(serialized, serializedPosition));
            serializedPosition += bufferSize;
        }

        return deserialized;
    }

    std::vector<std::future<void>> asyncs(m_Threads);
    std::vector<size_t> asyncPositions(m_Threads);
    std::vector<int> asyncRankSources(m_Threads);

    bool launched = false;

    while (serializedPosition < serializedSize)
    {
        // extract rank and index buffer size
        for (unsigned int t = 0; t < m_Threads; ++t)
        {
            if (serializedPosition >= serializedSize)
            {
                break;
            }

            const int rankSource = static_cast<int>(
                helper::ReadValue<uint32_t>(serialized, serializedPosition));
            asyncRankSources[t] = rankSource;
            asyncPositions[t] = serializedPosition;

            const size_t bufferSize = static_cast<size_t>(
                helper::ReadValue<uint32_t>(serialized, serializedPosition));
            serializedPosition += bufferSize;

            if (launched)
            {
                asyncs[t].get();
            }

            if (serializedPosition <= serializedSize)
            {
                asyncs[t] = std::async(std::launch::async, lf_Deserialize,
                                       asyncRankSources[t], asyncPositions[t]);
            }
        }

        launched = true;
    }

    for (auto &async : asyncs)
    {
        if (async.valid())
        {
            async.wait();
        }
    }

    return deserialized;
}

void BP3Serializer::MergeSerializeIndices(
    const std::unordered_map<std::string, std::vector<SerialElementIndex>>
        &nameRankIndices,
    MPI_Comm comm, BufferSTL &bufferSTL)
{
    auto lf_GetCharacteristics = [&](const std::vector<char> &buffer,
                                     size_t &position, const uint8_t dataType,
                                     uint8_t &count, uint32_t &length,
                                     uint32_t &timeStep)

    {
        const DataTypes dataTypeEnum = static_cast<DataTypes>(dataType);

        switch (dataTypeEnum)
        {

        case (type_string):
        {
            const auto characteristics =
                ReadElementIndexCharacteristics<std::string>(buffer, position,
                                                             type_string, true);
            count = characteristics.EntryCount;
            length = characteristics.EntryLength;
            timeStep = characteristics.Statistics.Step;
            break;
        }

        case (type_string_array):
        {
            const auto characteristics =
                ReadElementIndexCharacteristics<std::string>(
                    buffer, position, type_string_array, true);
            count = characteristics.EntryCount;
            length = characteristics.EntryLength;
            timeStep = characteristics.Statistics.Step;
            break;
        }

        case (type_byte):
        {
            const auto characteristics = ReadElementIndexCharacteristics<char>(
                buffer, position, type_byte, true);
            count = characteristics.EntryCount;
            length = characteristics.EntryLength;
            timeStep = characteristics.Statistics.Step;
            break;
        }

        case (type_short):
        {
            const auto characteristics = ReadElementIndexCharacteristics<short>(
                buffer, position, type_short, true);
            count = characteristics.EntryCount;
            length = characteristics.EntryLength;
            timeStep = characteristics.Statistics.Step;
            break;
        }

        case (type_integer):
        {
            const auto characteristics = ReadElementIndexCharacteristics<int>(
                buffer, position, type_integer, true);
            count = characteristics.EntryCount;
            length = characteristics.EntryLength;
            timeStep = characteristics.Statistics.Step;
            break;
        }

        case (type_long):
        {
            const auto characteristics =
                ReadElementIndexCharacteristics<int64_t>(buffer, position,
                                                         type_long, true);
            count = characteristics.EntryCount;
            length = characteristics.EntryLength;
            timeStep = characteristics.Statistics.Step;
            break;
        }

        case (type_unsigned_byte):
        {
            const auto characteristics =
                ReadElementIndexCharacteristics<unsigned char>(
                    buffer, position, type_unsigned_byte, true);
            count = characteristics.EntryCount;
            length = characteristics.EntryLength;
            timeStep = characteristics.Statistics.Step;
            break;
        }

        case (type_unsigned_short):
        {
            const auto characteristics =
                ReadElementIndexCharacteristics<unsigned short>(
                    buffer, position, type_unsigned_short, true);
            count = characteristics.EntryCount;
            length = characteristics.EntryLength;
            timeStep = characteristics.Statistics.Step;
            break;
        }

        case (type_unsigned_integer):
        {
            const auto characteristics =
                ReadElementIndexCharacteristics<unsigned int>(
                    buffer, position, type_unsigned_integer, true);
            count = characteristics.EntryCount;
            length = characteristics.EntryLength;
            timeStep = characteristics.Statistics.Step;
            break;
        }

        case (type_unsigned_long):
        {
            auto characteristics = ReadElementIndexCharacteristics<uint64_t>(
                buffer, position, type_unsigned_long, true);
            count = characteristics.EntryCount;
            length = characteristics.EntryLength;
            timeStep = characteristics.Statistics.Step;
            break;
        }

        case (type_real):
        {
            auto characteristics = ReadElementIndexCharacteristics<float>(
                buffer, position, type_real, true);
            count = characteristics.EntryCount;
            length = characteristics.EntryLength;
            timeStep = characteristics.Statistics.Step;
            break;
        }

        case (type_double):
        {
            auto characteristics = ReadElementIndexCharacteristics<double>(
                buffer, position, type_double, true);
            count = characteristics.EntryCount;
            length = characteristics.EntryLength;
            timeStep = characteristics.Statistics.Step;
            break;
        }

        default:
            // TODO: complex, long double
            throw std::invalid_argument(
                "ERROR: type " + std::to_string(dataType) +
                " not supported in BP3 Metadata Merge\n");

        } // end switch

    };

    auto lf_MergeRankSerial = [&](
        const std::vector<SerialElementIndex> &indices, BufferSTL &bufferSTL) {

        auto &bufferOut = bufferSTL.m_Buffer;
        auto &positionOut = bufferSTL.m_Position;

        // extract header
        ElementIndexHeader header;
        // index non-empty buffer
        size_t firstRank = 0;
        // index positions per rank
        std::vector<size_t> positions(indices.size(), 0);
        // merge index length
        size_t headerSize = 0;

        for (size_t r = 0; r < indices.size(); ++r)
        {
            const auto &buffer = indices[r].Buffer;
            if (buffer.empty())
            {
                continue;
            }
            size_t &position = positions[r];

            header = ReadElementIndexHeader(buffer, position);
            firstRank = r;

            headerSize = position;
            break;
        }

        if (m_DebugMode)
        {
            if (header.DataType == std::numeric_limits<uint8_t>::max() - 1)
            {
                throw std::runtime_error(
                    "ERROR: invalid data type for variable " + header.Name +
                    "when writing metadata index\n");
            }
        }

        // move all positions to headerSize
        for (size_t r = 0; r < indices.size(); ++r)
        {
            const auto &buffer = indices[r].Buffer;
            if (buffer.empty())
            {
                continue;
            }
            positions[r] = headerSize;
        }

        uint64_t setsCount = 0;
        unsigned int currentTimeStep = 1;
        bool marching = true;

        const size_t entryLengthPosition = positionOut;
        positionOut += headerSize;

        while (marching)
        {
            marching = false;

            for (size_t r = firstRank; r < indices.size(); ++r)
            {
                const auto &buffer = indices[r].Buffer;
                if (buffer.empty())
                {
                    continue;
                }

                auto &position = positions[r];
                if (position < buffer.size())
                {
                    marching = true;
                }
                else
                {
                    continue;
                }

                uint8_t count = 0;
                uint32_t length = 0;
                uint32_t timeStep = static_cast<uint32_t>(currentTimeStep);

                while (timeStep == currentTimeStep)
                {
                    size_t localPosition = position;
                    lf_GetCharacteristics(buffer, localPosition,
                                          header.DataType, count, length,
                                          timeStep);

                    if (timeStep != currentTimeStep)
                    {
                        break;
                    }

                    ++setsCount;

                    helper::CopyToBuffer(bufferOut, positionOut,
                                         &buffer[position], length + 5);

                    position += length + 5;

                    if (position >= buffer.size())
                    {
                        break;
                    }
                }
            }
            ++currentTimeStep;
        }

        const uint32_t entryLength =
            static_cast<uint32_t>(positionOut - entryLengthPosition - 4);

        size_t backPosition = entryLengthPosition;
        helper::CopyToBuffer(bufferOut, backPosition, &entryLength);
        helper::CopyToBuffer(bufferOut, backPosition,
                             &indices[firstRank].Buffer[4], headerSize - 8 - 4);
        helper::CopyToBuffer(bufferOut, backPosition, &setsCount);

    };

    auto lf_MergeRank = [&](const std::vector<SerialElementIndex> &indices,
                            BufferSTL &bufferSTL) {

        ElementIndexHeader header;
        size_t firstRank = 0;
        // index positions per rank
        std::vector<size_t> positions(indices.size(), 0);
        // merge index length
        size_t headerSize = 0;

        for (size_t r = 0; r < indices.size(); ++r)
        {
            const auto &buffer = indices[r].Buffer;
            if (buffer.empty())
            {
                continue;
            }
            size_t &position = positions[r];

            header = ReadElementIndexHeader(buffer, position);
            firstRank = r;

            headerSize = position;
            break;
        }

        if (m_DebugMode)
        {
            if (header.DataType == std::numeric_limits<uint8_t>::max() - 1)
            {
                throw std::runtime_error(
                    "ERROR: invalid data type for variable " + header.Name +
                    "when writing collective metadata\n");
            }
        }

        // move all positions to headerSize
        for (size_t r = 0; r < indices.size(); ++r)
        {
            const auto &buffer = indices[r].Buffer;
            if (buffer.empty())
            {
                continue;
            }
            positions[r] = headerSize;
        }

        uint64_t setsCount = 0;
        unsigned int currentTimeStep = 1;
        bool marching = true;
        std::vector<char> sorted;

        while (marching)
        {
            marching = false;

            for (size_t r = firstRank; r < indices.size(); ++r)
            {
                const auto &buffer = indices[r].Buffer;
                if (buffer.empty())
                {
                    continue;
                }

                auto &position = positions[r];
                if (position < buffer.size())
                {
                    marching = true;
                }
                else
                {
                    continue;
                }

                uint8_t count = 0;
                uint32_t length = 0;
                uint32_t timeStep = static_cast<uint32_t>(currentTimeStep);

                while (timeStep == currentTimeStep)
                {
                    size_t localPosition = position;
                    lf_GetCharacteristics(buffer, localPosition,
                                          header.DataType, count, length,
                                          timeStep);

                    if (timeStep != currentTimeStep)
                    {
                        break;
                    }

                    ++setsCount;

                    // here copy to sorted buffer
                    helper::InsertToBuffer(sorted, &buffer[position],
                                           length + 5);

                    position += length + 5;

                    if (position >= buffer.size())
                    {
                        break;
                    }
                }
            }
            ++currentTimeStep;
        }

        const uint32_t entryLength =
            static_cast<uint32_t>(headerSize + sorted.size() - 4);
        // Copy header to metadata buffer, need mutex here
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            auto &buffer = bufferSTL.m_Buffer;
            auto &position = bufferSTL.m_Position;

            helper::CopyToBuffer(buffer, position, &entryLength);
            helper::CopyToBuffer(buffer, position,
                                 &indices[firstRank].Buffer[4],
                                 headerSize - 8 - 4);
            helper::CopyToBuffer(buffer, position, &setsCount);
            helper::CopyToBuffer(buffer, position, sorted.data(),
                                 sorted.size());
        }
    };

    auto lf_MergeRankRange = [&](
        const std::unordered_map<std::string, std::vector<SerialElementIndex>>
            &nameRankIndices,
        const std::vector<std::string> &names, const size_t start,
        const size_t end, BufferSTL &bufferSTL)

    {
        for (auto i = start; i < end; ++i)
        {
            auto itIndex = nameRankIndices.find(names[i]);
            lf_MergeRank(itIndex->second, bufferSTL);
        }
    };

    // BODY OF FUNCTION STARTS HERE
    if (m_Threads == 1) // enforcing serial version for now
    {
        for (const auto &rankIndices : nameRankIndices)
        {
            lf_MergeRankSerial(rankIndices.second, bufferSTL);
        }
        return;
    }

    // TODO need to debug this part, if threaded per variable
    const size_t elements = nameRankIndices.size();
    const size_t stride = elements / m_Threads;        // elements per thread
    const size_t last = stride + elements % m_Threads; // remainder to last

    std::vector<std::thread> threads;
    threads.reserve(m_Threads);

    // copy names in order to use threads
    std::vector<std::string> names;
    names.reserve(nameRankIndices.size());

    for (const auto &nameRankIndexPair : nameRankIndices)
    {
        names.push_back(nameRankIndexPair.first);
    }

    for (unsigned int t = 0; t < m_Threads; ++t)
    {
        const size_t start = stride * t;
        size_t end = start + stride;

        if (t == m_Threads - 1)
        {
            end = start + last;
        }

        threads.push_back(
            std::thread(lf_MergeRankRange, std::ref(nameRankIndices),
                        std::ref(names), start, end, std::ref(bufferSTL)));
    }

    for (auto &thread : threads)
    {
        thread.join();
    }
}

std::vector<char>
BP3Serializer::SetCollectiveProfilingJSON(const std::string &rankLog) const
{
    // Gather sizes
    const size_t rankLogSize = rankLog.size();
    std::vector<size_t> rankLogsSizes =
        helper::GatherValues(rankLogSize, m_MPIComm);

    // Gatherv JSON per rank
    std::vector<char> profilingJSON(3);
    const std::string header("[\n");
    const std::string footer("\n]\n");
    size_t gatheredSize = 0;
    size_t position = 0;

    if (m_RankMPI == 0) // pre-allocate in destination
    {
        gatheredSize = std::accumulate(rankLogsSizes.begin(),
                                       rankLogsSizes.end(), size_t(0));

        profilingJSON.resize(gatheredSize + header.size() + footer.size() - 2);
        helper::CopyToBuffer(profilingJSON, position, header.c_str(),
                             header.size());
    }

    helper::GathervArrays(rankLog.c_str(), rankLog.size(), rankLogsSizes.data(),
                          rankLogsSizes.size(), &profilingJSON[position],
                          m_MPIComm);

    if (m_RankMPI == 0) // add footer to close JSON
    {
        position += gatheredSize - 2;
        helper::CopyToBuffer(profilingJSON, position, footer.c_str(),
                             footer.size());
    }

    return profilingJSON;
}

uint32_t BP3Serializer::GetFileIndex() const noexcept
{
    if (m_Aggregator.m_IsActive)
    {
        return static_cast<uint32_t>(m_Aggregator.m_SubStreamIndex);
    }

    return static_cast<uint32_t>(m_RankMPI);
}

size_t BP3Serializer::GetAttributesSizeInData(core::IO &io) const noexcept
{
    size_t attributesSizeInData = 12; // count + length

    auto &attributes = io.GetAttributesDataMap();

    for (const auto &attribute : attributes)
    {
        const std::string type = attribute.second.first;

        if (type == "compound")
        {
        }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        const std::string name = attribute.first;                              \
        const core::Attribute<T> &attribute = *io.InquireAttribute<T>(name);   \
        attributesSizeInData += GetAttributeSizeInData<T>(attribute);          \
    }
        ADIOS2_FOREACH_ATTRIBUTE_TYPE_1ARG(declare_type)
#undef declare_type
    }

    return attributesSizeInData;
}

//------------------------------------------------------------------------------
// Explicit instantiation of only public templates

#define declare_template_instantiation(T)                                      \
    template void BP3Serializer::PutVariablePayload(                           \
        const core::Variable<T> &,                                             \
        const typename core::Variable<T>::Info &) noexcept;                    \
                                                                               \
    template void BP3Serializer::PutVariableMetadata(                          \
        const core::Variable<T> &,                                             \
        const typename core::Variable<T>::Info &) noexcept;

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

//------------------------------------------------------------------------------

} // end namespace format
} // end namespace adios2
