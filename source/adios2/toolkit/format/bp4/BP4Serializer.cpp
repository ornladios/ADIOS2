/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP4Serializer.cpp
 *
 *  Created on: Aug 1, 2018
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#include "BP4Serializer.h"
#include "BP4Serializer.tcc"

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

std::mutex BP4Serializer::m_Mutex;

BP4Serializer::BP4Serializer(MPI_Comm mpiComm, const bool debugMode)
: BP4Base(mpiComm, debugMode)
{
}

/*generate the header for the metadata index file*/
void BP4Serializer::MakeHeader(BufferSTL &b, const std::string fileType,
                               const bool isActive)
{
    auto lf_CopyVersionChar = [](const std::string version,
                                 std::vector<char> &buffer, size_t &position) {
        helper::CopyToBuffer(buffer, position, version.c_str());
    };

    auto &buffer = b.m_Buffer;
    auto &position = b.m_Position;
    auto &absolutePosition = b.m_AbsolutePosition;
    if (position > 0)
    {
        throw std::invalid_argument(
            "ERROR: BP4Serializer::MakeHeader can only be called for an empty "
            "buffer. This one for " +
            fileType + " already has content of " + std::to_string(position) +
            " bytes.");
    }

    if (b.GetAvailableSize() < 64)
    {
        b.Resize(position + 64, "BP4Serializer::MakeHeader " + fileType);
    }

    const std::string majorVersion(std::to_string(ADIOS2_VERSION_MAJOR));
    const std::string minorVersion(std::to_string(ADIOS2_VERSION_MINOR));
    const std::string patchVersion(std::to_string(ADIOS2_VERSION_PATCH));

    // byte 0-31: Readable tag
    if (position != m_VersionTagPosition)
    {
        throw std::runtime_error(
            "ADIOS Coding ERROR in BP4Serializer::MakeHeader. Version Tag "
            "position mismatch");
    }
    std::string versionLongTag("ADIOS-BP v" + majorVersion + "." +
                               minorVersion + "." + patchVersion + " ");
    size_t maxTypeLen = m_VersionTagLength - versionLongTag.size();
    const std::string fileTypeStr = fileType.substr(0, maxTypeLen);
    versionLongTag += fileTypeStr;
    const size_t versionLongTagSize = versionLongTag.size();
    if (versionLongTagSize < m_VersionTagLength)
    {
        helper::CopyToBuffer(buffer, position, versionLongTag.c_str(),
                             versionLongTagSize);
        position += m_VersionTagLength - versionLongTagSize;
    }
    else if (versionLongTagSize > m_VersionTagLength)
    {
        helper::CopyToBuffer(buffer, position, versionLongTag.c_str(),
                             m_VersionTagLength);
    }
    else
    {
        helper::CopyToBuffer(buffer, position, versionLongTag.c_str(),
                             m_VersionTagLength);
    }

    // byte 32-35: MAJOR MINOR PATCH Unused

    lf_CopyVersionChar(majorVersion, buffer, position);
    lf_CopyVersionChar(minorVersion, buffer, position);
    lf_CopyVersionChar(patchVersion, buffer, position);
    ++position;

    // Note: Reader does process and use bytes 36-38 in
    // BP4Deserialize.cpp::ParseMetadataIndex().
    // Order and position must match there.

    // byte 36: endianness
    if (position != m_EndianFlagPosition)
    {
        throw std::runtime_error(
            "ADIOS Coding ERROR in BP4Serializer::MakeHeader. Endian Flag "
            "position mismatch");
    }
    const uint8_t endianness = helper::IsLittleEndian() ? 0 : 1;
    helper::CopyToBuffer(buffer, position, &endianness);

    // byte 37: BP Version 4
    if (position != m_BPVersionPosition)
    {
        throw std::runtime_error(
            "ADIOS Coding ERROR in BP4Serializer::MakeHeader. Active Flag "
            "position mismatch");
    }
    const uint8_t version = 4;
    helper::CopyToBuffer(buffer, position, &version);

    // byte 38: Active flag (used in Index Table only)
    if (position != m_ActiveFlagPosition)
    {
        throw std::runtime_error(
            "ADIOS Coding ERROR in BP4Serializer::MakeHeader. Active Flag "
            "position mismatch");
    }
    const uint8_t activeFlag = (isActive ? 1 : 0);
    helper::CopyToBuffer(buffer, position, &activeFlag);

    // byte 39: unused
    position += 1;

    // byte 40-63: unused
    position += 24;
    absolutePosition = position;
}

void BP4Serializer::PutProcessGroupIndex(
    const std::string &ioName, const std::string hostLanguage,
    const std::vector<std::string> &transportsTypes) noexcept
{
    ProfilerStart("buffering");
    std::vector<char> &metadataBuffer = m_MetadataSet.PGIndex.Buffer;

    std::vector<char> &dataBuffer = m_Data.m_Buffer;
    size_t &dataPosition = m_Data.m_Position;
    const size_t pgBeginPosition = dataPosition;

    // write a block identifier [PGI
    const char pgi[] = "[PGI"; //  don't write \0!
    helper::CopyToBuffer(dataBuffer, dataPosition, pgi, sizeof(pgi) - 1);

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
    helper::InsertU64(metadataBuffer,
                      m_Data.m_AbsolutePosition + m_PreDataFileLength);

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
    m_Data.m_AbsolutePosition += dataPosition - pgBeginPosition;
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

void BP4Serializer::SerializeData(core::IO &io, const bool advanceStep)
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

size_t BP4Serializer::CloseData(core::IO &io)
{
    ProfilerStart("buffering");
    size_t dataEndsAt = m_Data.m_Position;
    if (!m_IsClosed)
    {
        if (m_MetadataSet.DataPGIsOpen)
        {
            SerializeDataBuffer(io);
        }
        dataEndsAt = m_Data.m_Position;

        SerializeMetadataInData(false, false);

        if (m_Profiler.IsActive)
        {
            m_Profiler.Bytes.at("buffering") = m_Data.m_AbsolutePosition;
        }

        m_Aggregator.Close();
        m_IsClosed = true;
    }

    ProfilerStop("buffering");
    return dataEndsAt;
}

size_t BP4Serializer::CloseStream(core::IO &io, const bool addMetadata)
{
    ProfilerStart("buffering");
    if (m_MetadataSet.DataPGIsOpen)
    {
        SerializeDataBuffer(io);
    }
    size_t dataEndsAt = m_Data.m_Position;
    SerializeMetadataInData(false, addMetadata);

    if (m_Profiler.IsActive)
    {
        m_Profiler.Bytes.at("buffering") += m_Data.m_Position;
    }
    ProfilerStop("buffering");

    return dataEndsAt;
}

size_t BP4Serializer::CloseStream(core::IO &io, size_t &metadataStart,
                                  size_t &metadataCount, const bool addMetadata)
{

    ProfilerStart("buffering");
    if (m_MetadataSet.DataPGIsOpen)
    {
        SerializeDataBuffer(io);
    }
    size_t dataEndsAt = m_Data.m_Position;
    metadataStart = m_Data.m_Position;
    SerializeMetadataInData(false, addMetadata);

    metadataCount = m_Data.m_Position - metadataStart;

    if (m_Profiler.IsActive)
    {
        m_Profiler.Bytes.at("buffering") += m_Data.m_Position;
    }
    ProfilerStop("buffering");
    return dataEndsAt;
}

void BP4Serializer::ResetIndices()
{
    m_MetadataSet.AttributesIndices.clear();
    m_MetadataSet.VarsIndices.clear();
}

/* Reset the local metadata buffer at the end of each step */
void BP4Serializer::ResetIndicesBuffer()
{
    m_MetadataSet.PGIndex.Buffer.resize(0);
    m_MetadataSet.PGIndex.LastUpdatedPosition = 0;
    m_MetadataSet.DataPGCount = 0;
    m_MetadataSet.DataPGLengthPosition = 0;
    m_MetadataSet.DataPGVarsCount = 0;
    m_MetadataSet.DataPGVarsCountPosition = 0;

    for (auto &variableIndexPair : m_MetadataSet.VarsIndices)
    {
        const std::string &variableName = variableIndexPair.first;
        SerialElementIndex &index = variableIndexPair.second;
        const size_t headersize = 15 + 8 + variableName.size();
        index.Buffer.resize(headersize);
        index.Count = 0;
        index.LastUpdatedPosition = headersize;
        index.Valid = false; // reset the flag to indicate the variable is not
                             // valid after the "endstep" call
    }

    for (auto &attributesIndexPair : m_MetadataSet.AttributesIndices)
    {
        const std::string &attributesName = attributesIndexPair.first;
        SerialElementIndex &index = attributesIndexPair.second;
        const size_t headersize = 15 + 8 + attributesName.size();
        index.Buffer.resize(headersize);
        index.Count = 0;
        index.Valid = false; // reset the flag to indicate the variable is not
                             // valid after the "endstep" call
    }
}

std::string BP4Serializer::GetRankProfilingJSON(
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
    lf_WriterTimer(rankLog, profiler.Timers.at("mkdir"));

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
BP4Serializer::AggregateProfilingJSON(const std::string &rankProfilingLog)
{
    return SetCollectiveProfilingJSON(rankProfilingLog);
}

void BP4Serializer::AggregateCollectiveMetadata(MPI_Comm comm,
                                                BufferSTL &bufferSTL,
                                                const bool inMetadataBuffer)
{
    ProfilerStart("buffering");
    ProfilerStart("meta_sort_merge");

    auto &position = bufferSTL.m_Position;

    // const uint64_t pgIndexStart =
    //    inMetadataBuffer ? position : position + bufferSTL.m_AbsolutePosition;
    /* save the starting position of the pgindex in the metadata file*/
    m_MetadataSet.pgIndexStart =
        inMetadataBuffer ? position : position + bufferSTL.m_AbsolutePosition;
    AggregateIndex(m_MetadataSet.PGIndex, m_MetadataSet.DataPGCount, comm,
                   bufferSTL);

    // const uint64_t variablesIndexStart =
    //    inMetadataBuffer ? position : position + bufferSTL.m_AbsolutePosition;
    /* save the starting position of the varindex in the metadata file*/
    m_MetadataSet.varIndexStart =
        inMetadataBuffer ? position : position + bufferSTL.m_AbsolutePosition;
    AggregateMergeIndex(m_MetadataSet.VarsIndices, comm, bufferSTL);

    // const uint64_t attributesIndexStart =
    //    inMetadataBuffer ? position : position + bufferSTL.m_AbsolutePosition;
    /* save the starting position of the attrindex in the metadata file*/
    m_MetadataSet.attrIndexStart =
        inMetadataBuffer ? position : position + bufferSTL.m_AbsolutePosition;
    AggregateMergeIndex(m_MetadataSet.AttributesIndices, comm, bufferSTL, true);

    int rank;
    SMPI_Comm_rank(comm, &rank);
    if (rank == 0)
    {
        /* no more minifooter in the global metadata*/
        // PutMinifooter(pgIndexStart, variablesIndexStart,
        // attributesIndexStart,
        //              bufferSTL.m_Buffer, bufferSTL.m_Position,
        //              inMetadataBuffer);

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

void BP4Serializer::UpdateOffsetsInMetadata()
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

#define make_case(T)                                                           \
    case (TypeTraits<T>::type_enum):                                           \
    {                                                                          \
        UpdateIndexOffsetsCharacteristics<T>(                                  \
            currentPosition, TypeTraits<T>::type_enum, buffer);                \
        break;                                                                 \
    }
                ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(make_case)
#undef make_case

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
void BP4Serializer::PutAttributes(core::IO &io)
{
    const auto &attributesDataMap = io.GetAttributesDataMap();

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

        // each attribute is only written to output once
        // so filter out the ones already written
        auto it = m_SerializedAttributes.find(name);
        if (it != m_SerializedAttributes.end())
        {
            continue;
        }

        if (type == "unknown")
        {
        }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        Stats<T> stats;                                                        \
        stats.Offset = absolutePosition + m_PreDataFileLength;                 \
        stats.MemberID = memberID;                                             \
        stats.Step = m_MetadataSet.TimeStep;                                   \
        stats.FileIndex = GetFileIndex();                                      \
        core::Attribute<T> &attribute = *io.InquireAttribute<T>(name);         \
        PutAttributeInData(attribute, stats);                                  \
        PutAttributeInIndex(attribute, stats);                                 \
    }
        ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(declare_type)
#undef declare_type

        ++memberID;
    }

    // complete attributes length
    const uint64_t attributesLength =
        static_cast<uint64_t>(position - attributesLengthPosition);

    size_t backPosition = attributesLengthPosition;
    helper::CopyToBuffer(buffer, backPosition, &attributesLength);
}

void BP4Serializer::PutDimensionsRecord(const Dims &localDimensions,
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

void BP4Serializer::PutDimensionsRecord(const Dims &localDimensions,
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

void BP4Serializer::PutNameRecord(const std::string name,
                                  std::vector<char> &buffer) noexcept
{
    const uint16_t length = static_cast<uint16_t>(name.size());
    helper::InsertToBuffer(buffer, &length);
    helper::InsertToBuffer(buffer, name.c_str(), name.size());
}

void BP4Serializer::PutNameRecord(const std::string name,
                                  std::vector<char> &buffer,
                                  size_t &position) noexcept
{
    const uint16_t length = static_cast<uint16_t>(name.length());
    helper::CopyToBuffer(buffer, position, &length);
    helper::CopyToBuffer(buffer, position, name.c_str(), length);
}

BP4Serializer::SerialElementIndex &BP4Serializer::GetSerialElementIndex(
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

void BP4Serializer::SerializeDataBuffer(core::IO &io) noexcept
{
    auto &buffer = m_Data.m_Buffer;
    auto &position = m_Data.m_Position;
    auto &absolutePosition = m_Data.m_AbsolutePosition;

    // vars count and Length (only for PG)
    helper::CopyToBuffer(buffer, m_MetadataSet.DataPGVarsCountPosition,
                         &m_MetadataSet.DataPGVarsCount);
    // without record itself and vars count
    // Note: m_MetadataSet.DataPGVarsCount has been incremented by 4
    // in previous CopyToBuffer operation!
    const uint64_t varsLength =
        position - m_MetadataSet.DataPGVarsCountPosition - 8;
    helper::CopyToBuffer(buffer, m_MetadataSet.DataPGVarsCountPosition,
                         &varsLength);

    // each attribute is only written to output once
    size_t attributesSizeInData = GetAttributesSizeInData(io);
    if (attributesSizeInData)
    {
        attributesSizeInData += 12; // count + length + end ID
        ResizeBuffer(position + attributesSizeInData + 4,
                     "when writing Attributes in rank=0\n ");
        PutAttributes(io);
    }
    else
    {
        ResizeBuffer(position + 12 + 4, "for empty Attributes\n");
        // Attribute index header for zero attributes: 0, 0LL
        // Resize() already takes care of this
        position += 12;
        absolutePosition += 12;
    }

    // write a block identifier PGI]
    const char pgiend[] = "PGI]"; // no \0
    helper::CopyToBuffer(buffer, position, pgiend, sizeof(pgiend) - 1);
    absolutePosition += sizeof(pgiend) - 1;

    // Finish writing pg group length INCLUDING the record itself and
    // including the closing padding but NOT the opening [PGI
    const uint64_t dataPGLength = position - m_MetadataSet.DataPGLengthPosition;
    helper::CopyToBuffer(buffer, m_MetadataSet.DataPGLengthPosition,
                         &dataPGLength);

    m_MetadataSet.DataPGIsOpen = false;
}

void BP4Serializer::SerializeMetadataInData(const bool updateAbsolutePosition,
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

    size_t dataEndsAt = m_Data.m_Position;

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

    /*
    std::cout << " -- Write footer at position " << std::to_string(position)
              << " with footer length " << std::to_string(footerSize)
              << " in buffer  = "
              << std::to_string(reinterpret_cast<std::uintptr_t>(buffer.data()))
              << std::endl;
    */

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

void BP4Serializer::PutMinifooter(const uint64_t pgIndexStart,
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
        const uint8_t zeros1 = 0;
        helper::CopyToBuffer(buffer, position, &zeros1);
        helper::CopyToBuffer(buffer, position, &m_Version);
    }
    else
    {
        const uint16_t zeros2 = 0;
        helper::CopyToBuffer(buffer, position, &zeros2);
    }
    helper::CopyToBuffer(buffer, position, &m_Version);
}

void BP4Serializer::AggregateIndex(const SerialElementIndex &index,
                                   const size_t count, MPI_Comm comm,
                                   BufferSTL &bufferSTL)
{
    auto &buffer = bufferSTL.m_Buffer;
    auto &position = bufferSTL.m_Position;
    int rank;
    SMPI_Comm_rank(comm, &rank);

    size_t countPosition = position;
    const size_t totalCount = helper::ReduceValues<size_t>(count, comm);

    if (rank == 0)
    {
        // Write count
        position += 16;
        bufferSTL.Resize(position, " in call to AggregateIndex BP4 metadata");
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

void BP4Serializer::AggregateMergeIndex(
    const std::unordered_map<std::string, SerialElementIndex> &indices,
    MPI_Comm comm, BufferSTL &bufferSTL, const bool isRankConstant)
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
        nameRankIndices = DeserializeIndicesPerRankThreads(
            gatheredSerialIndices, comm, isRankConstant);

    // deallocate gathered serial indices (full in rank 0 only)
    std::vector<char>().swap(gatheredSerialIndices);

    int rank;
    SMPI_Comm_rank(comm, &rank);

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
                         ", in call to AggregateMergeIndex BP4 metadata");
        const uint32_t totalCountU32 =
            static_cast<uint32_t>(nameRankIndices.size());
        helper::CopyToBuffer(buffer, countPosition, &totalCountU32);

        // MergeSerializeIndices(nameRankIndices, comm, bufferSTL);
        /* merge and serialize all the indeices at each step */
        MergeSerializeIndicesPerStep(nameRankIndices, comm, bufferSTL);

        // Write length
        const uint64_t totalLengthU64 =
            static_cast<uint64_t>(position - countPosition - 8);
        helper::CopyToBuffer(buffer, countPosition, &totalLengthU64);
    }
}

std::vector<char> BP4Serializer::SerializeIndices(
    const std::unordered_map<std::string, SerialElementIndex> &indices,
    MPI_Comm comm) const noexcept
{
    // pre-allocate
    size_t serializedIndicesSize = 0;
    for (const auto &indexPair : indices)
    {
        const SerialElementIndex &index = indexPair.second;
        if (!index.Valid)
        {
            continue; // if the variable is not put (not valid) at current step,
                      // skip
        }
        serializedIndicesSize += 4 + index.Buffer.size();
    }

    std::vector<char> serializedIndices;
    serializedIndices.reserve(serializedIndicesSize);

    int rank;
    SMPI_Comm_rank(comm, &rank);

    for (const auto &indexPair : indices)
    {
        const SerialElementIndex &index = indexPair.second;

        if (!index.Valid)
        {
            continue; // if the variable is not put (not valid) at current step,
                      // skip
        }

        // add rank at the beginning
        const uint32_t rankSource = static_cast<uint32_t>(rank);
        helper::InsertToBuffer(serializedIndices, &rankSource);

        // insert buffer
        helper::InsertToBuffer(serializedIndices, index.Buffer.data(),
                               index.Buffer.size());
    }

    return serializedIndices;
}

std::unordered_map<std::string, std::vector<BP4Base::SerialElementIndex>>
BP4Serializer::DeserializeIndicesPerRankThreads(
    const std::vector<char> &serialized, MPI_Comm comm,
    const bool isRankConstant) const noexcept
{
    std::unordered_map<std::string, std::vector<SerialElementIndex>>
        deserialized;

    auto lf_Deserialize_no_mutex = [&](const int rankSource,
                                       const size_t serializedPosition,
                                       const bool isRankConstant) {
        size_t localPosition = serializedPosition;

        ElementIndexHeader header =
            ReadElementIndexHeader(serialized, localPosition);

        if (isRankConstant)
        {
            if (deserialized.count(header.Name) == 1)
            {
                return;
            }
        }

        std::vector<BP4Base::SerialElementIndex> *deserializedIndexes;

        // deserializedIndexes =
        // &(deserialized.emplace(std::piecewise_construct,
        //                        std::forward_as_tuple(header.Name),
        //                        std::forward_as_tuple(
        //                        m_SizeMPI, SerialElementIndex(header.MemberID,
        //                        0))).first->second);

        auto search = deserialized.find(header.Name);
        if (search == deserialized.end())
        {
            // variable does not exist, we need to add it
            deserializedIndexes =
                &(deserialized
                      .emplace(std::piecewise_construct,
                               std::forward_as_tuple(header.Name),
                               std::forward_as_tuple(
                                   m_SizeMPI,
                                   SerialElementIndex(header.MemberID, 0)))
                      .first->second);
            // std::cout << "rank " << rankSource << ": did not find " <<
            // header.Name << ", added it" << std::endl;
        }
        else
        {
            deserializedIndexes = &(search->second);
            // std::cout << "rank " << rankSource << ": found " << header.Name
            // << std::endl;
        }

        const size_t bufferSize = static_cast<size_t>(header.Length) + 4;
        SerialElementIndex &index = deserializedIndexes->at(rankSource);
        helper::InsertToBuffer(index.Buffer, &serialized[serializedPosition],
                               bufferSize);
    };

    auto lf_Deserialize = [&](const int rankSource,
                              const size_t serializedPosition,
                              const bool isRankConstant) {
        size_t localPosition = serializedPosition;
        ElementIndexHeader header =
            ReadElementIndexHeader(serialized, localPosition);

        if (isRankConstant)
        {
            if (deserialized.count(header.Name) == 1)
            {
                return;
            }
        }

        std::vector<BP4Base::SerialElementIndex> *deserializedIndexes;
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
    SMPI_Comm_rank(comm, &rank);

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
                lf_Deserialize_no_mutex(rankSource, serializedPosition,
                                        isRankConstant);
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
                                       asyncRankSources[t], asyncPositions[t],
                                       isRankConstant);
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

/* Merge and serialize all the indices at each step */
void BP4Serializer::MergeSerializeIndicesPerStep(
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

#define make_case(T)                                                           \
    case (TypeTraits<T>::type_enum):                                           \
    {                                                                          \
        const auto characteristics = ReadElementIndexCharacteristics<T>(       \
            buffer, position, TypeTraits<T>::type_enum, true);                 \
        count = characteristics.EntryCount;                                    \
        length = characteristics.EntryLength;                                  \
        timeStep = characteristics.Statistics.Step;                            \
        break;                                                                 \
    }
            ADIOS2_FOREACH_STDTYPE_1ARG(make_case)
#undef make_case

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

        default:
            // TODO: complex, long double
            throw std::invalid_argument(
                "ERROR: type " + std::to_string(dataType) +
                " not supported in BP4 Metadata Merge\n");

        } // end switch
    };

    auto lf_MergeRankSerial =
        [&](const std::vector<SerialElementIndex> &indices,
            BufferSTL &bufferSTL) {
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
            // unsigned int currentTimeStep = 1;

            const size_t entryLengthPosition = positionOut;
            positionOut += headerSize;

            for (size_t r = firstRank; r < indices.size(); ++r)
            {
                const auto &buffer = indices[r].Buffer;
                if (buffer.empty())
                {
                    continue;
                }
                auto &position = positions[r];
                // std::cout << "rank: " << r << ", positions[r]: " <<
                // positions[r] << ", position: " << position << ",
                // buffer.size(): " << buffer.size() << std::endl;
                if (position >= buffer.size())
                {
                    continue;
                }

                uint8_t count = 0;
                uint32_t length = 0;
                // uint32_t timeStep = static_cast<uint32_t>(currentTimeStep);
                uint32_t timeStep = 1;

                while (true)
                {
                    if (position >= buffer.size())
                    {
                        break;
                    }
                    size_t localPosition = position;
                    lf_GetCharacteristics(buffer, localPosition,
                                          header.DataType, count, length,
                                          timeStep);
                    // std::cout << "rank: " << r << ", timeStep: " <<
                    // timeStep<< std::endl;

                    ++setsCount;

                    // std::cout << "setsCount: " << setsCount << ",
                    // positionOut: " << positionOut << ", position: " <<
                    // position<< std::endl;
                    helper::CopyToBuffer(bufferOut, positionOut,
                                         &buffer[position], length + 5);

                    position += length + 5;
                    // std::cout << "length: " << length << ", position: " <<
                    // position << ", positions[r]: " << positions[r] <<
                    // std::endl;
                }
            }

            const uint32_t entryLength =
                static_cast<uint32_t>(positionOut - entryLengthPosition - 4);

            size_t backPosition = entryLengthPosition;
            helper::CopyToBuffer(bufferOut, backPosition, &entryLength);
            helper::CopyToBuffer(bufferOut, backPosition,
                                 &indices[firstRank].Buffer[4],
                                 headerSize - 8 - 4);
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
        std::vector<char> sorted;

        for (size_t r = firstRank; r < indices.size(); ++r)
        {
            const auto &buffer = indices[r].Buffer;
            if (buffer.empty())
            {
                continue;
            }

            auto &position = positions[r];
            if (position >= buffer.size())
            {
                continue;
            }

            uint8_t count = 0;
            uint32_t length = 0;
            uint32_t timeStep = static_cast<uint32_t>(currentTimeStep);

            size_t localPosition = position;
            lf_GetCharacteristics(buffer, localPosition, header.DataType, count,
                                  length, timeStep);
            ++setsCount;

            // here copy to sorted buffer
            helper::InsertToBuffer(sorted, &buffer[position], length + 5);
            position += length + 5;
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

    auto lf_MergeRankRange =
        [&](const std::unordered_map<
                std::string, std::vector<SerialElementIndex>> &nameRankIndices,
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

void BP4Serializer::MergeSerializeIndices(
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

#define make_case(T)                                                           \
    case (TypeTraits<T>::type_enum):                                           \
    {                                                                          \
        const auto characteristics = ReadElementIndexCharacteristics<T>(       \
            buffer, position, TypeTraits<T>::type_enum, true);                 \
        count = characteristics.EntryCount;                                    \
        length = characteristics.EntryLength;                                  \
        timeStep = characteristics.Statistics.Step;                            \
        break;                                                                 \
    }
            ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(make_case)
#undef make_case

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

        default:
            // TODO: complex, long double
            throw std::invalid_argument(
                "ERROR: type " + std::to_string(dataType) +
                " not supported in BP4 Metadata Merge\n");

        } // end switch
    };

    auto lf_MergeRankSerial =
        [&](const std::vector<SerialElementIndex> &indices,
            BufferSTL &bufferSTL) {
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
                                 &indices[firstRank].Buffer[4],
                                 headerSize - 8 - 4);
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

    auto lf_MergeRankRange =
        [&](const std::unordered_map<
                std::string, std::vector<SerialElementIndex>> &nameRankIndices,
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
BP4Serializer::SetCollectiveProfilingJSON(const std::string &rankLog) const
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

uint32_t BP4Serializer::GetFileIndex() const noexcept
{
    if (m_Aggregator.m_IsActive)
    {
        return static_cast<uint32_t>(m_Aggregator.m_SubStreamIndex);
    }

    return static_cast<uint32_t>(m_RankMPI);
}

size_t BP4Serializer::GetAttributesSizeInData(core::IO &io) const noexcept
{
    size_t attributesSizeInData = 0;

    auto &attributes = io.GetAttributesDataMap();

    for (const auto &attribute : attributes)
    {
        const std::string type = attribute.second.first;

        // each attribute is only written to output once
        // so filter out the ones already written
        auto it = m_SerializedAttributes.find(attribute.first);
        if (it != m_SerializedAttributes.end())
        {
            continue;
        }

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
        ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(declare_type)
#undef declare_type
    }

    return attributesSizeInData;
}

//------------------------------------------------------------------------------
// Explicit instantiation of only public templates

#define declare_template_instantiation(T)                                      \
    template void BP4Serializer::PutVariablePayload(                           \
        const core::Variable<T> &, const typename core::Variable<T>::Info &,   \
        const bool) noexcept;                                                  \
                                                                               \
    template void BP4Serializer::PutVariableMetadata(                          \
        const core::Variable<T> &, const typename core::Variable<T>::Info &,   \
        const bool) noexcept;

ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

//------------------------------------------------------------------------------

} // end namespace format
} // end namespace adios2
