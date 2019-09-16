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
#include <tuple>
#include <vector>

#include "adios2/helper/adiosFunctions.h" //helper::GetType<T>, helper::ReadValue<T>

#ifdef _WIN32
#pragma warning(disable : 4503) // Windows complains about SubFileInfoMap levels
#endif

namespace adios2
{
namespace format
{

BP4Serializer::BP4Serializer(helper::Comm const &comm, const bool debugMode)
: BPSerializer(comm, debugMode, 4), BP4Base(comm, debugMode),
  BPBase(comm, debugMode)
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
    m_Profiler.Start("buffering");
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

    m_Profiler.Stop("buffering");
}

size_t BP4Serializer::CloseData(core::IO &io)
{
    m_Profiler.Start("buffering");
    size_t dataEndsAt = m_Data.m_Position;
    if (!m_IsClosed)
    {
        if (m_MetadataSet.DataPGIsOpen)
        {
            SerializeDataBuffer(io);
        }
        dataEndsAt = m_Data.m_Position;

        SerializeMetadataInData(false, false);

        if (m_Profiler.m_IsActive)
        {
            m_Profiler.m_Bytes.at("buffering") = m_Data.m_AbsolutePosition;
        }

        m_Aggregator.Close();
        m_IsClosed = true;
    }

    m_Profiler.Stop("buffering");
    return dataEndsAt;
}

size_t BP4Serializer::CloseStream(core::IO &io, const bool addMetadata)
{
    m_Profiler.Start("buffering");
    if (m_MetadataSet.DataPGIsOpen)
    {
        SerializeDataBuffer(io);
    }
    size_t dataEndsAt = m_Data.m_Position;
    SerializeMetadataInData(false, addMetadata);

    if (m_Profiler.m_IsActive)
    {
        m_Profiler.m_Bytes.at("buffering") += m_Data.m_Position;
    }
    m_Profiler.Stop("buffering");

    return dataEndsAt;
}

size_t BP4Serializer::CloseStream(core::IO &io, size_t &metadataStart,
                                  size_t &metadataCount, const bool addMetadata)
{

    m_Profiler.Start("buffering");
    if (m_MetadataSet.DataPGIsOpen)
    {
        SerializeDataBuffer(io);
    }
    size_t dataEndsAt = m_Data.m_Position;
    metadataStart = m_Data.m_Position;
    SerializeMetadataInData(false, addMetadata);

    metadataCount = m_Data.m_Position - metadataStart;

    if (m_Profiler.m_IsActive)
    {
        m_Profiler.m_Bytes.at("buffering") += m_Data.m_Position;
    }
    m_Profiler.Stop("buffering");
    return dataEndsAt;
}

void BP4Serializer::ResetIndices()
{
    m_MetadataSet.AttributesIndices.clear();
    m_MetadataSet.VarsIndices.clear();
}

/* Reset the local metadata indices */
void BP4Serializer::ResetAllIndices()
{
    m_MetadataSet.PGIndex.Buffer.resize(0);
    m_MetadataSet.PGIndex.LastUpdatedPosition = 0;
    m_MetadataSet.DataPGCount = 0;
    m_MetadataSet.DataPGLengthPosition = 0;
    m_MetadataSet.DataPGVarsCount = 0;
    m_MetadataSet.DataPGVarsCountPosition = 0;

    // for (auto &variableIndexPair : m_MetadataSet.VarsIndices)
    // {
    //     const std::string &variableName = variableIndexPair.first;
    //     SerialElementIndex &index = variableIndexPair.second;
    //     const size_t headersize = 15 + 8 + variableName.size();
    //     index.Buffer.resize(headersize);
    //     index.Count = 0;
    //     index.LastUpdatedPosition = headersize;
    //     index.Valid = false; // reset the flag to indicate the variable is
    //     not
    //                          // valid after the "endstep" call
    // }

    // for (auto &attributesIndexPair : m_MetadataSet.AttributesIndices)
    // {
    //     const std::string &attributesName = attributesIndexPair.first;
    //     SerialElementIndex &index = attributesIndexPair.second;
    //     const size_t headersize = 15 + 8 + attributesName.size();
    //     index.Buffer.resize(headersize);
    //     index.Count = 0;
    //     index.Valid = false; // reset the flag to indicate the variable is
    //     not
    //                          // valid after the "endstep" call
    // }
    m_MetadataSet.AttributesIndices.clear();
    m_MetadataSet.VarsIndices.clear();
}

/* Reset the metadata index table*/
void BP4Serializer::ResetMetadataIndexTable() { m_MetadataIndexTable.clear(); }

void BP4Serializer::AggregateCollectiveMetadata(helper::Comm const &comm,
                                                BufferSTL &bufferSTL,
                                                const bool inMetadataBuffer)
{
    m_Profiler.Start("buffering");
    m_Profiler.Start("meta_sort_merge");

    AggregateCollectiveMetadataIndices(comm, bufferSTL);

    // auto &position = bufferSTL.m_Position;

    // // const uint64_t pgIndexStart =
    // //    inMetadataBuffer ? position : position +
    // bufferSTL.m_AbsolutePosition;
    // /* save the starting position of the pgindex in the metadata file*/
    // m_MetadataSet.pgIndexStart =
    //     inMetadataBuffer ? position : position +
    //     bufferSTL.m_AbsolutePosition;
    // AggregateIndex(m_MetadataSet.PGIndex, m_MetadataSet.DataPGCount, comm,
    //                bufferSTL);

    // // const uint64_t variablesIndexStart =
    // //    inMetadataBuffer ? position : position +
    // bufferSTL.m_AbsolutePosition;
    // /* save the starting position of the varindex in the metadata file*/
    // m_MetadataSet.varIndexStart =
    //     inMetadataBuffer ? position : position +
    //     bufferSTL.m_AbsolutePosition;
    // AggregateMergeIndex(m_MetadataSet.VarsIndices, comm, bufferSTL);

    // // const uint64_t attributesIndexStart =
    // //    inMetadataBuffer ? position : position +
    // bufferSTL.m_AbsolutePosition;
    // /* save the starting position of the attrindex in the metadata file*/
    // m_MetadataSet.attrIndexStart =
    //     inMetadataBuffer ? position : position +
    //     bufferSTL.m_AbsolutePosition;
    // AggregateMergeIndex(m_MetadataSet.AttributesIndices, comm, bufferSTL,
    // true);

    int rank = comm.Rank();
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

    bufferSTL.Resize(bufferSTL.m_Position, "after collective metadata is done");

    m_Profiler.Stop("meta_sort_merge");
    m_Profiler.Stop("buffering");
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
                // const uint32_t indexLength =
                //     static_cast<uint32_t>(indexBuffer.size() - 4);
                // size_t indexLengthPosition = 0;
                // helper::CopyToBuffer(indexBuffer, indexLengthPosition,
                //                      &indexLength);

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

    if (m_Profiler.m_IsActive)
    {
        m_Profiler.m_Bytes.emplace("buffering", absolutePosition);
    }
}

void BP4Serializer::AggregateIndex(const SerialElementIndex &index,
                                   const size_t count, helper::Comm const &comm,
                                   BufferSTL &bufferSTL)
{
    auto &buffer = bufferSTL.m_Buffer;
    auto &position = bufferSTL.m_Position;
    int rank = comm.Rank();

    size_t countPosition = position;
    const size_t totalCount = comm.ReduceValues<size_t>(count);

    if (rank == 0)
    {
        // Write count
        position += 16;
        bufferSTL.Resize(position, " in call to AggregateIndex BP4 metadata");
        const uint64_t totalCountU64 = static_cast<uint64_t>(totalCount);
        helper::CopyToBuffer(buffer, countPosition, &totalCountU64);
    }

    // write contents
    comm.GathervVectors(index.Buffer, buffer, position);

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
    helper::Comm const &comm, BufferSTL &bufferSTL, const bool isRankConstant)
{
    // first serialize index
    std::vector<char> serializedIndices = SerializeIndices(indices, comm);
    // gather in rank 0
    std::vector<char> gatheredSerialIndices;
    size_t gatheredSerialIndicesPosition = 0;

    comm.GathervVectors(serializedIndices, gatheredSerialIndices,
                        gatheredSerialIndicesPosition);

    // deallocate local serialized Indices
    std::vector<char>().swap(serializedIndices);

    // deserialize in [name][rank] order
    const std::unordered_map<std::string, std::vector<SerialElementIndex>>
        nameRankIndices = DeserializeIndicesPerRankThreads(
            gatheredSerialIndices, comm, isRankConstant);

    // deallocate gathered serial indices (full in rank 0 only)
    std::vector<char>().swap(gatheredSerialIndices);

    int rank = comm.Rank();

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
    helper::Comm const &comm) const noexcept
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

    int rank = comm.Rank();

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
BP4Serializer::DeserializeIndicesPerRankSingleThread(
    const std::vector<char> &serialized, helper::Comm const &comm,
    const bool isRankConstant) const noexcept
{
    std::unordered_map<std::string, std::vector<SerialElementIndex>>
        deserialized;

    /*
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
    };*/

    // BODY OF FUNCTION starts here
    const size_t serializedSize = serialized.size();
    int rank = comm.Rank();

    if (rank != 0 || serializedSize < 8)
    {
        return deserialized;
    }

    size_t serializedPosition = 0;

    while (serializedPosition < serializedSize - 4)
    {
        const int rankSource = static_cast<int>(
            helper::ReadValue<uint32_t>(serialized, serializedPosition));

        if (serializedPosition <= serializedSize)
        {
            // lf_Deserialize_no_mutex(rankSource, serializedPosition,
            //                        isRankConstant);
            size_t localPosition = serializedPosition;

            ElementIndexHeader header =
                ReadElementIndexHeader(serialized, localPosition);

            if (!isRankConstant || deserialized.count(header.Name) != 1)
            {

                std::vector<BP4Base::SerialElementIndex> *deserializedIndexes;

                // deserializedIndexes =
                // &(deserialized.emplace(std::piecewise_construct,
                //                        std::forward_as_tuple(header.Name),
                //                        std::forward_as_tuple(
                //                        m_SizeMPI,
                //                        SerialElementIndex(header.MemberID,
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
                                           m_SizeMPI, SerialElementIndex(
                                                          header.MemberID, 0)))
                              .first->second);
                    // std::cout << "rank " << rankSource << ": did not find "
                    // << header.Name << ", added it" << std::endl;
                }
                else
                {
                    deserializedIndexes = &(search->second);
                    // std::cout << "rank " << rankSource << ": found " <<
                    // header.Name
                    // << std::endl;
                }

                const size_t bufferSize =
                    static_cast<size_t>(header.Length) + 4;
                SerialElementIndex &index = deserializedIndexes->at(rankSource);
                helper::InsertToBuffer(
                    index.Buffer, &serialized[serializedPosition], bufferSize);
            }
        }

        const size_t bufferSize = static_cast<size_t>(
            helper::ReadValue<uint32_t>(serialized, serializedPosition));
        serializedPosition += bufferSize;
    }

    return deserialized;
}

std::unordered_map<std::string, std::vector<BP4Base::SerialElementIndex>>
BP4Serializer::DeserializeIndicesPerRankThreads(
    const std::vector<char> &serialized, helper::Comm const &comm,
    const bool isRankConstant) const noexcept
{
    if (m_Parameters.Threads == 1)
    {
        return BP4Serializer::DeserializeIndicesPerRankSingleThread(
            serialized, comm, isRankConstant);
    }

    std::unordered_map<std::string, std::vector<SerialElementIndex>>
        deserialized;
    /*
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
    };*/

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
    int rank = comm.Rank();

    if (rank != 0 || serializedSize < 8)
    {
        return deserialized;
    }

    size_t serializedPosition = 0;

    std::vector<std::future<void>> asyncs(m_Parameters.Threads);
    std::vector<size_t> asyncPositions(m_Parameters.Threads);
    std::vector<int> asyncRankSources(m_Parameters.Threads);

    bool launched = false;

    while (serializedPosition < serializedSize)
    {
        // extract rank and index buffer size
        for (unsigned int t = 0; t < m_Parameters.Threads; ++t)
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

void BP4Serializer::AggregateCollectiveMetadataIndices(helper::Comm const &comm,
                                                       BufferSTL &outBufferSTL)
{
    int rank = comm.Rank();
    int size = comm.Size();

    BufferSTL inBufferSTL;

    // pre-allocate with rank 0 data
    // size_t pgCount = 0; //< tracks global PG count
    if (rank == 0)
    {
        // assumes that things are more or less balanced
        m_PGIndicesInfo.clear();
        // m_PGRankIndices.reserve(m_MetadataSet.PGIndex.Buffer.size());

        m_VariableIndicesInfo.clear();
        // m_VariableRankIndices.reserve(m_MetadataSet.VarsIndices.size());

        m_AttributesIndicesInfo.clear();
        // m_AttributesRankIndices.reserve(m_MetadataSet.AttributesIndices.size());
    }

    auto lf_IndicesSize =
        [&](const std::unordered_map<std::string, SerialElementIndex> &indices)
        -> size_t

    {
        size_t indicesSize = 0;
        for (const auto &indexPair : indices)
        {
            indicesSize += indexPair.second.Buffer.size();
        }
        return indicesSize;
    };

    auto lf_SerializeIndices =
        [&](const std::unordered_map<std::string, SerialElementIndex> &indices,
            size_t &position)

    {
        for (const auto &indexPair : indices)
        {
            const auto &buffer = indexPair.second.Buffer;
            helper::CopyToBuffer(m_SerializedIndices, position, buffer.data(),
                                 buffer.size());
        }
    };

    auto lf_SerializeAllIndices = [&](helper::Comm const &comm,
                                      const int rank) {
        const size_t pgIndicesSize = m_MetadataSet.PGIndex.Buffer.size();
        const size_t variablesIndicesSize =
            lf_IndicesSize(m_MetadataSet.VarsIndices);
        const size_t attributesIndicesSize =
            lf_IndicesSize(m_MetadataSet.AttributesIndices);

        // first pre-allocate
        const size_t serializedIndicesSize = 8 * 4 + pgIndicesSize +
                                             variablesIndicesSize +
                                             attributesIndicesSize;

        m_SerializedIndices.reserve(serializedIndicesSize + 4);
        m_SerializedIndices.resize(serializedIndicesSize + 4);

        const uint32_t rank32 = static_cast<uint32_t>(rank);
        const uint64_t size64 = static_cast<uint64_t>(serializedIndicesSize);
        const uint64_t variablesIndexOffset =
            static_cast<uint64_t>(pgIndicesSize + 36);
        const uint64_t attributesIndexOffset =
            static_cast<uint64_t>(pgIndicesSize + 36 + variablesIndicesSize);

        size_t position = 0;
        helper::CopyToBuffer(m_SerializedIndices, position, &rank32);
        helper::CopyToBuffer(m_SerializedIndices, position, &size64);
        helper::CopyToBuffer(m_SerializedIndices, position,
                             &variablesIndexOffset);
        helper::CopyToBuffer(m_SerializedIndices, position,
                             &attributesIndexOffset);
        helper::CopyToBuffer(m_SerializedIndices, position,
                             &m_MetadataSet.DataPGCount);

        helper::CopyToBuffer(m_SerializedIndices, position,
                             m_MetadataSet.PGIndex.Buffer.data(),
                             m_MetadataSet.PGIndex.Buffer.size());
        lf_SerializeIndices(m_MetadataSet.VarsIndices, position);
        lf_SerializeIndices(m_MetadataSet.AttributesIndices, position);
    };

    auto lf_LocatePGIndices =
        [&](std::unordered_map<size_t,
                               std::vector<std::tuple<size_t, size_t, size_t>>>
                &pgIndicesInfo,
            const int rankSource, const std::vector<char> &serialized,
            const size_t position, const size_t endPosition) {
            size_t stepStartPosition = position;
            size_t stepBuffersize = 0;
            size_t pgCountPerStep = 0;
            size_t localPosition = position;
            uint32_t currentStep = 0;

            while (localPosition < endPosition)
            {
                size_t indexPosition = localPosition;
                const ProcessGroupIndex header = ReadProcessGroupIndexHeader(
                    serialized, indexPosition, helper::IsLittleEndian());
                if (header.Step == currentStep)
                {
                    stepBuffersize += header.Length + 2;
                    pgCountPerStep++;
                }
                else
                {
                    // found a new step
                    if (currentStep == 0)
                    {
                        // start of going through a new pg buffer
                        stepStartPosition = localPosition;
                        stepBuffersize = header.Length + 2;
                        pgCountPerStep = 1;
                        currentStep = header.Step;
                    }
                    else
                    {
                        // record the pg info of previous step
                        std::tuple<size_t, size_t, size_t> stepPGIndexTuple =
                            std::make_tuple(pgCountPerStep, stepStartPosition,
                                            stepBuffersize);
                        auto search = pgIndicesInfo.find(currentStep);
                        if (search == pgIndicesInfo.end())
                        {
                            // the time step hasn't been added to the
                            // unordered_map, add it
                            pgIndicesInfo.emplace(
                                currentStep,
                                std::vector<
                                    std::tuple<size_t, size_t, size_t>>());
                            pgIndicesInfo[currentStep].push_back(
                                stepPGIndexTuple);
                        }
                        else
                        {
                            pgIndicesInfo[currentStep].push_back(
                                stepPGIndexTuple);
                        }
                        stepStartPosition = localPosition;
                        stepBuffersize = header.Length + 2;
                        pgCountPerStep = 1;
                        currentStep = header.Step;
                    }
                }
                localPosition += header.Length + 2;
                if (localPosition >= endPosition)
                {
                    // record the pg info of the last step
                    std::tuple<size_t, size_t, size_t> stepPGIndexTuple =
                        std::make_tuple(pgCountPerStep, stepStartPosition,
                                        stepBuffersize);
                    auto search = pgIndicesInfo.find(currentStep);
                    if (search == pgIndicesInfo.end())
                    {
                        // the time step hasn't been added to the unordered_map,
                        // add it
                        pgIndicesInfo.emplace(
                            currentStep,
                            std::vector<std::tuple<size_t, size_t, size_t>>());
                        pgIndicesInfo[currentStep].push_back(stepPGIndexTuple);
                    }
                    else
                    {
                        pgIndicesInfo[currentStep].push_back(stepPGIndexTuple);
                    }
                }
            }
        };

    auto lf_GetCharacteristics = [&](const std::vector<char> &buffer,
                                     size_t &position, const uint8_t dataType,
                                     uint8_t &count, uint32_t &length,
                                     uint32_t &timeStep)

    {
        const DataTypes dataTypeEnum = static_cast<DataTypes>(dataType);
        const bool isLittleEndian = helper::IsLittleEndian();

        switch (dataTypeEnum)
        {

#define make_case(T)                                                           \
    case (TypeTraits<T>::type_enum):                                           \
    {                                                                          \
        const auto characteristics = ReadElementIndexCharacteristics<T>(       \
            buffer, position, TypeTraits<T>::type_enum, true, isLittleEndian); \
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
                    buffer, position, type_string_array, true, isLittleEndian);
            count = characteristics.EntryCount;
            length = characteristics.EntryLength;
            timeStep = characteristics.Statistics.Step;
            break;
        }

        default:
            throw std::invalid_argument(
                "ERROR: type " + std::to_string(dataType) +
                " not supported in BP4 Metadata Merge\n");

        } // end switch
    };

    auto lf_LocateVarIndices =
        [&](std::unordered_map<
                size_t,
                std::unordered_map<std::string,
                                   std::vector<std::tuple<size_t, size_t>>>>
                &indicesInfo,
            const int rankSource, const std::vector<char> &serialized,
            const size_t position, const size_t endPosition)

    {
        // uint32_t currentStep = 0;

        size_t localPosition = position;
        while (localPosition < endPosition)
        {
            // std::cout << "var localPosition: " << localPosition << std::endl;
            size_t indexPosition = localPosition;
            const ElementIndexHeader header = ReadElementIndexHeader(
                serialized, indexPosition, helper::IsLittleEndian());

            // std::cout << "var indexPosition after ReadElementIndexHeader: "
            // << indexPosition << std::endl;

            uint8_t count = 0;
            uint32_t length = 0;
            uint32_t timeStep = 0;

            lf_GetCharacteristics(serialized, indexPosition, header.DataType,
                                  count, length, timeStep);

            // std::cout << "timeStep: " << timeStep << ", " << "header.Name: "
            // << header.Name << ", " << "header.Length: " << header.Length <<
            // ", " << "header.CharacteristicsSetsCount: " <<
            // header.CharacteristicsSetsCount << std::endl;
            size_t varIndexBufferSize = static_cast<size_t>(header.Length) + 4;
            // std::cout << "varIndexBufferSize: " << varIndexBufferSize <<
            // std::endl;

            auto stepSearch = indicesInfo.find(timeStep);
            if (stepSearch == indicesInfo.end())
            {
                // std::cout << "didn't find step " << timeStep << std::endl;
                // the time step hasn't been added to the unordered_map, add it
                std::unordered_map<std::string,
                                   std::vector<std::tuple<size_t, size_t>>>
                    varIndexInfo;
                varIndexInfo.emplace(header.Name,
                                     std::vector<std::tuple<size_t, size_t>>());
                std::tuple<size_t, size_t> varIndexTuple =
                    std::make_tuple(localPosition, varIndexBufferSize);
                varIndexInfo[header.Name].push_back(varIndexTuple);
                indicesInfo.emplace(timeStep, varIndexInfo);
                // std::cout <<
                // std::get<0>(indicesInfo[timeStep][header.Name][rankSource])
                // <<
                // ", " <<
                // std::get<1>(indicesInfo[timeStep][header.Name][rankSource])
                // << std::endl; currentStep = timeStep;
            }
            else
            {
                // std::cout << "found step " << timeStep << std::endl;
                // if (timeStep == currentStep)
                // {
                //     auto varSearch = indicesInfo[timeStep].find(header.Name);
                //     if (varSearch == indicesInfo[timeStep].end())
                //     {
                //         // found a new variable at this step
                //         indicesInfo[timeStep].emplace(header.Name,
                //         std::vector<std::tuple<size_t, size_t>>());
                //         std::tuple<size_t, size_t> varIndexTuple =
                //         std::make_tuple(localPosition, varIndexBufferSize);
                //         indicesInfo[timeStep][header.Name].push_back(varIndexTuple);

                //     }
                //     else
                //     {
                //         // found new block of the same variable at the same
                //         step
                //         std::get<1>(indicesInfo[timeStep][header.Name].back())
                //         += varIndexBufferSize;
                //     }
                // }
                // else
                // {
                // found a new step of a new rank
                auto varSearch = indicesInfo[timeStep].find(header.Name);
                if (varSearch == indicesInfo[timeStep].end())
                {
                    // std::cout << "didn't find var " << header.Name <<
                    // std::endl;
                    // found a new variable at this step
                    indicesInfo[timeStep].emplace(
                        header.Name, std::vector<std::tuple<size_t, size_t>>());
                    std::tuple<size_t, size_t> varIndexTuple =
                        std::make_tuple(localPosition, varIndexBufferSize);
                    indicesInfo[timeStep][header.Name].push_back(varIndexTuple);
                }
                else
                {
                    // std::cout << "found var " << header.Name << std::endl;
                    // variable already exists, insert the location info of this
                    // variable for this rank
                    std::tuple<size_t, size_t> varIndexTuple =
                        std::make_tuple(localPosition, varIndexBufferSize);
                    indicesInfo[timeStep][header.Name].push_back(varIndexTuple);
                    // currentStep = timeStep;
                }
            }

            localPosition += varIndexBufferSize;
        }
    };

    auto lf_LocateAttrIndices =
        [&](std::unordered_map<
                size_t,
                std::unordered_map<std::string,
                                   std::vector<std::tuple<size_t, size_t>>>>
                &indicesInfo,
            const int rankSource, const std::vector<char> &serialized,
            const size_t position, const size_t endPosition)

    {
        // uint32_t currentStep = 0;

        size_t localPosition = position;
        while (localPosition < endPosition)
        {
            // std::cout << "attr localPosition: " << localPosition <<
            // std::endl;
            size_t indexPosition = localPosition;
            const ElementIndexHeader header = ReadElementIndexHeader(
                serialized, indexPosition, helper::IsLittleEndian());

            // std::cout << "attr indexPosition after ReadElementIndexHeader: "
            // << indexPosition << std::endl;

            uint8_t count = 0;
            uint32_t length = 0;
            uint32_t timeStep = 0;

            lf_GetCharacteristics(serialized, indexPosition, header.DataType,
                                  count, length, timeStep);
            if (indicesInfo[timeStep][header.Name].size() == 1)
            {
                return;
            }

            // std::cout << "timeStep: " << timeStep << ", " << "header.Name: "
            // << header.Name << ", " << "header.Length: " << header.Length <<
            // std::endl;
            size_t attrIndexBufferSize = static_cast<size_t>(header.Length) + 4;
            // std::cout << "attrIndexBufferSize: " << attrIndexBufferSize <<
            // std::endl;

            auto stepSearch = indicesInfo.find(timeStep);
            if (stepSearch == indicesInfo.end())
            {
                // the time step hasn't been added to the unordered_map, add it
                std::unordered_map<std::string,
                                   std::vector<std::tuple<size_t, size_t>>>
                    attrIndexInfo;
                attrIndexInfo.emplace(
                    header.Name, std::vector<std::tuple<size_t, size_t>>());
                std::tuple<size_t, size_t> attrIndexTuple =
                    std::make_tuple(localPosition, attrIndexBufferSize);
                attrIndexInfo[header.Name].push_back(attrIndexTuple);
                indicesInfo.emplace(timeStep, attrIndexInfo);
                // std::cout <<
                // std::get<0>(indicesInfo[timeStep][header.Name][rankSource])
                // <<
                // ", " <<
                // std::get<1>(indicesInfo[timeStep][header.Name][rankSource])
                // << std::endl; currentStep = timeStep;
            }
            else
            {
                // if (timeStep == currentStep)
                // {
                //     auto attrSearch =
                //     indicesInfo[timeStep].find(header.Name); if (attrSearch
                //     == indicesInfo[timeStep].end())
                //     {
                //         // found a new attribute at this step
                //         indicesInfo[timeStep].emplace(header.Name,
                //         std::vector<std::tuple<size_t, size_t>>());
                //         std::tuple<size_t, size_t> attrIndexTuple =
                //         std::make_tuple(localPosition, attrIndexBufferSize);
                //         indicesInfo[timeStep][header.Name].push_back(attrIndexTuple);

                //     }
                //     else
                //     {
                //         // found new block of the same attribute at the same
                //         step
                //         std::get<1>(indicesInfo[timeStep][header.Name].back())
                //         += attrIndexBufferSize;
                //     }
                // }
                // else
                // {
                // found a new step of a new rank
                auto attrSearch = indicesInfo[timeStep].find(header.Name);
                if (attrSearch == indicesInfo[timeStep].end())
                {
                    // found a new attribute at this step
                    indicesInfo[timeStep].emplace(
                        header.Name, std::vector<std::tuple<size_t, size_t>>());
                    std::tuple<size_t, size_t> attrIndexTuple =
                        std::make_tuple(localPosition, attrIndexBufferSize);
                    indicesInfo[timeStep][header.Name].push_back(
                        attrIndexTuple);
                }
                else
                {
                    // attribute already exists, insert the location info of
                    // this attribute for this rank
                    std::tuple<size_t, size_t> attrIndexTuple =
                        std::make_tuple(localPosition, attrIndexBufferSize);
                    indicesInfo[timeStep][header.Name].push_back(
                        attrIndexTuple);
                }
                // currentStep = timeStep;
                //}
            }

            localPosition += attrIndexBufferSize;
        }
    };

    auto lf_LocateAllIndices =
        [&](const int rankSource, const std::vector<size_t> headerInfo,
            const std::vector<char> &serialized, const size_t position)

    {
        const size_t rankIndicesSize = headerInfo[0];
        const size_t variablesIndexOffset = headerInfo[1] + position;
        const size_t attributesIndexOffset = headerInfo[2] + position;
        // pgCount += headerInfo[3];
        // std::cout << "rankIndicesSize: " << rankIndicesSize << ", " <<
        // "variablesIndexOffset: " << variablesIndexOffset << ", " <<
        // "attributesIndexOffset: " << attributesIndexOffset << std::endl;
        size_t localPosition = position + 36;

        size_t endPosition = variablesIndexOffset;
        // first deserialize pg indices
        lf_LocatePGIndices(m_PGIndicesInfo, rankSource, serialized,
                           localPosition, endPosition);
        // for (auto const& pair: m_PGIndicesInfo)
        // {
        //     std::cout << "rank " << rankSource << ", step " << pair.first <<
        //     ": " << std::get<0>(pair.second[rankSource]) << ", " <<
        //     std::get<1>(pair.second[rankSource]) << ", " <<
        //     std::get<2>(pair.second[rankSource]) << std::endl;
        // }
        // {
        //     std::lock_guard<std::mutex> lock(m_Mutex);
        //     helper::InsertToBuffer(m_PGRankIndices,
        //     &serialized[localPosition],
        //                            pgIndexLength);
        // }

        // deserialize variable indices
        localPosition = variablesIndexOffset;
        endPosition = attributesIndexOffset;

        lf_LocateVarIndices(m_VariableIndicesInfo, rankSource, serialized,
                            localPosition, endPosition);

        // for (auto const& pair: m_VariableIndicesInfo)
        // {
        //     std::cout << "rank " << rankSource << ", step " << pair.first <<
        //     ": " << std::endl; for (auto const& subpair: pair.second)
        //     {
        //         std::cout << "    " << subpair.first << ": " <<
        //         std::get<0>(subpair.second[rankSource]) << ", " <<
        //         std::get<1>(subpair.second[rankSource]) << std::endl;
        //     }

        // }

        // deserialize attributes indices
        localPosition = attributesIndexOffset;
        endPosition = rankIndicesSize + 4 + position;
        // attributes are constant and unique across ranks
        lf_LocateAttrIndices(m_AttributesIndicesInfo, rankSource, serialized,
                             localPosition, endPosition);

        // for (auto const& pair: m_AttributesIndicesInfo)
        // {
        //     std::cout << "step " << pair.first << ": " << std::endl;
        //     for (auto const& subpair: pair.second)
        //     {
        //         std::cout << "    " << subpair.second.size() << std::endl;
        //         std::cout << "    " << subpair.first << ": " <<
        //         std::get<0>(subpair.second[0]) << ", " <<
        //         std::get<1>(subpair.second[0]) << std::endl;
        //     }
        // }
    };

    auto lf_SortMergeIndices =
        [&](const std::unordered_map<
                size_t, std::vector<std::tuple<size_t, size_t, size_t>>>
                &pgIndicesInfo,
            const std::unordered_map<
                size_t,
                std::unordered_map<std::string,
                                   std::vector<std::tuple<size_t, size_t>>>>
                &varIndicesInfo,
            const std::unordered_map<
                size_t,
                std::unordered_map<std::string,
                                   std::vector<std::tuple<size_t, size_t>>>>
                &attrIndicesInfo,
            const std::vector<char> &serialized) {
            auto &position = outBufferSTL.m_Position;
            auto &buffer = outBufferSTL.m_Buffer;

            size_t totalStep = pgIndicesInfo.size();
            std::vector<size_t> timeSteps;
            timeSteps.reserve(pgIndicesInfo.size());
            for (auto const &pair : pgIndicesInfo)
            {
                timeSteps.push_back(pair.first);
            }
            std::sort(timeSteps.begin(), timeSteps.end());
            m_MetadataIndexTable[rank] = {};
            for (auto t : timeSteps)
            {
                // std::cout << t << std::endl;
                // std::cout << "step " << t << ": " << std::endl;
                std::vector<uint64_t> ptrs;
                // std::cout << "position: " << position << std::endl;

                const uint64_t pgIndexStart = position;
                ptrs.push_back(pgIndexStart);
                std::vector<std::tuple<size_t, size_t, size_t>>
                    perStepPGIndicesInfo = pgIndicesInfo.at(t);
                size_t perStepPGCountPosition = position;
                position += 16; // skip the pgcount and pglength
                uint64_t perStepPGCountU64 = 0;
                // std::cout << "  pg index counts & locations: " << std::endl;
                for (auto &item : perStepPGIndicesInfo)
                {
                    // std::cout << "    " << std::get<0>(item) << ", " <<
                    // std::get<1>(item) << ", " << std::get<2>(item) <<
                    // std::endl;
                    size_t start = std::get<1>(item);
                    size_t length = std::get<2>(item);
                    std::copy(serialized.begin() + start,
                              serialized.begin() + start + length,
                              buffer.begin() + position);
                    position += length;
                    perStepPGCountU64 += std::get<0>(item);
                }
                uint64_t perStepPGLengthU64 =
                    position - perStepPGCountPosition - 16;
                helper::CopyToBuffer(buffer, perStepPGCountPosition,
                                     &perStepPGCountU64);
                helper::CopyToBuffer(buffer, perStepPGCountPosition,
                                     &perStepPGLengthU64);
                // std::cout << "  pg count at this step: " << perStepPGCountU64
                // << std::endl; std::cout << "position: " << position <<
                // std::endl;

                const uint64_t variablesIndexStart = position;
                ptrs.push_back(variablesIndexStart);
                if (!varIndicesInfo.empty())
                {

                    std::unordered_map<std::string,
                                       std::vector<std::tuple<size_t, size_t>>>
                        perStepVarIndicesInfo = varIndicesInfo.at(t);
                    size_t perStepVarCountPosition = position;
                    const uint32_t perStepVarCountU32 =
                        static_cast<uint32_t>(perStepVarIndicesInfo.size());
                    helper::CopyToBuffer(buffer, perStepVarCountPosition,
                                         &perStepVarCountU32);
                    position += 12; // skip for count and length
                    // std::cout << "  var index locations: " << std::endl;
                    for (auto const &pair : perStepVarIndicesInfo)
                    {
                        // std::cout << "    " << pair.first << ": " <<
                        // std::endl;
                        const size_t entryLengthPosition = position;
                        size_t headerStartPosition =
                            std::get<0>(pair.second[0]);
                        size_t localPosition = headerStartPosition;
                        // std::cout << "    " << entryLengthPosition << ", " <<
                        // headerStartPosition << std::endl;
                        ElementIndexHeader header =
                            ReadElementIndexHeader(serialized, localPosition);
                        size_t headerSize = localPosition - headerStartPosition;
                        // std::cout << "    " << headerSize << std::endl;
                        position += headerSize; // skip the header
                        uint64_t setsCount = 0;
                        for (auto &item : pair.second)
                        {
                            // std::cout << "      " << std::get<0>(item) << ",
                            // " << std::get<1>(item) << std::endl;
                            size_t start = std::get<0>(item);
                            size_t length = std::get<1>(item);
                            std::copy(serialized.begin() + start + headerSize,
                                      serialized.begin() + start + length,
                                      buffer.begin() + position);
                            position += length - headerSize;
                            setsCount++;
                        }
                        const uint32_t entryLength = static_cast<uint32_t>(
                            position - entryLengthPosition - 4);
                        size_t backPosition = entryLengthPosition;
                        helper::CopyToBuffer(buffer, backPosition,
                                             &entryLength);
                        helper::CopyToBuffer(
                            buffer, backPosition,
                            &serialized[headerStartPosition + 4],
                            headerSize - 8 - 4);
                        helper::CopyToBuffer(buffer, backPosition, &setsCount);
                    }
                    const uint64_t perStepVarLengthU64 = static_cast<uint64_t>(
                        position - perStepVarCountPosition - 8);
                    helper::CopyToBuffer(buffer, perStepVarCountPosition,
                                         &perStepVarLengthU64);
                }
                else
                {
                    size_t perStepVarCountPosition = position;
                    const uint32_t perStepVarCountU32 =
                        static_cast<uint32_t>(0);
                    helper::CopyToBuffer(buffer, perStepVarCountPosition,
                                         &perStepVarCountU32);
                    const uint64_t perStepVarLengthU64 =
                        static_cast<uint64_t>(0);
                    helper::CopyToBuffer(buffer, perStepVarCountPosition,
                                         &perStepVarLengthU64);
                    position += 12; // skip for count and length
                }

                // std::cout << "position: " << position << std::endl;
                const uint64_t attributesIndexStart = position;
                ptrs.push_back(attributesIndexStart);

                if (!attrIndicesInfo.empty())
                {
                    std::unordered_map<std::string,
                                       std::vector<std::tuple<size_t, size_t>>>
                        perStepAttrIndicesInfo = attrIndicesInfo.at(t);
                    size_t perStepAttrCountPosition = position;
                    const uint32_t perStepAttrCountU32 =
                        static_cast<uint32_t>(perStepAttrIndicesInfo.size());
                    helper::CopyToBuffer(buffer, perStepAttrCountPosition,
                                         &perStepAttrCountU32);
                    position += 12; // skip for length
                    // std::cout << "  attr index locations: " << std::endl;
                    for (auto const &pair : perStepAttrIndicesInfo)
                    {
                        // std::cout << "    " << pair.first << ": " <<
                        // std::endl;
                        for (auto &item : pair.second)
                        {
                            // std::cout << "      " << std::get<0>(item) << ",
                            // " << std::get<1>(item) << std::endl;
                            size_t start = std::get<0>(item);
                            size_t length = std::get<1>(item);
                            std::copy(serialized.begin() + start,
                                      serialized.begin() + start + length,
                                      buffer.begin() + position);
                            position += length;
                        }
                    }
                    const uint64_t perStepAttrLengthU64 = static_cast<uint64_t>(
                        position - perStepAttrCountPosition - 8);
                    helper::CopyToBuffer(buffer, perStepAttrCountPosition,
                                         &perStepAttrLengthU64);
                }
                else
                {
                    size_t perStepAttrCountPosition = position;
                    const uint32_t perStepAttrCountU32 =
                        static_cast<uint32_t>(0);
                    helper::CopyToBuffer(buffer, perStepAttrCountPosition,
                                         &perStepAttrCountU32);
                    const uint64_t perStepAttrLengthU64 =
                        static_cast<uint64_t>(0);
                    helper::CopyToBuffer(buffer, perStepAttrCountPosition,
                                         &perStepAttrLengthU64);
                    position += 12; // skip for count and length
                }

                // std::cout << "position: " << position << std::endl;
                const uint64_t currentStepEndPos = position;
                ptrs.push_back(currentStepEndPos);
                m_MetadataIndexTable[rank][t] = ptrs;
                // std::cout << "flag!" << std::endl;
            }

            // size_t countPosition = position;

            // const uint32_t totalCountU32 =
            //     static_cast<uint32_t>(deserializedIndices.size());
            // helper::CopyToBuffer(buffer, countPosition, &totalCountU32);
            // position += 12; // skip for length

            // MergeSerializeIndices(deserializedIndices, comm, bufferSTL);

            // // Write length
            // const uint64_t totalLengthU64 =
            //     static_cast<uint64_t>(position - countPosition - 8);
            // helper::CopyToBuffer(buffer, countPosition, &totalLengthU64);
        };

    // BODY of function starts here

    lf_SerializeAllIndices(comm, rank); // Set m_SerializedIndices

    // use bufferSTL (will resize) to GatherV
    // const size_t extraSize = 16 + 12 + 12 + m_MetadataSet.MiniFooterSize;

    comm.GathervVectors(m_SerializedIndices, inBufferSTL.m_Buffer,
                        inBufferSTL.m_Position, 0, 0);

    // deserialize, it's all local inside rank 0
    if (rank == 0)
    {
        const size_t serializedSize = inBufferSTL.m_Position;
        const std::vector<char> &serialized = inBufferSTL.m_Buffer;
        size_t serializedPosition = 0;
        std::vector<size_t> headerInfo(4);
        const bool isLittleEndian = helper::IsLittleEndian();

        // if (m_Parameters.Threads == 1)
        {
            while (serializedPosition < serializedSize)
            {
                size_t localPosition = serializedPosition;

                const int rankSource =
                    static_cast<int>(helper::ReadValue<uint32_t>(
                        serialized, localPosition, isLittleEndian));

                for (auto i = 0; i < 4; ++i)
                {
                    headerInfo[i] =
                        static_cast<size_t>(helper::ReadValue<uint64_t>(
                            serialized, localPosition, isLittleEndian));
                }

                lf_LocateAllIndices(rankSource, headerInfo, serialized,
                                    serializedPosition);
                serializedPosition += headerInfo[0] + 4;
            }
        }
        // TODO: threaded version
    }

    // now merge (and sort variables and attributes) indices
    if (rank == 0)
    {
        auto &buffer = outBufferSTL.m_Buffer;
        const std::vector<char> &serialized = inBufferSTL.m_Buffer;

        // std::cout << "serialized buffer size: " << serialized.size() <<
        // std::endl; std::cout << "position in serialized buffer: " <<
        // inBufferSTL.m_Position << std::endl;
        // buffer.reserve(serialized.size());
        // buffer.resize(serialized.size());

        size_t totalStep = m_PGIndicesInfo.size();
        size_t perStepExtraSize = 16 + 12 + 12;
        const size_t totalExtraSize = totalStep * perStepExtraSize;
        // std::cout <<
        // outBufferSTL.m_Position+inBufferSTL.m_Position+totalExtraSize <<
        // std::endl;
        buffer.reserve(outBufferSTL.m_Position + inBufferSTL.m_Position +
                       totalExtraSize);
        buffer.resize(outBufferSTL.m_Position + inBufferSTL.m_Position +
                      totalExtraSize);
        lf_SortMergeIndices(m_PGIndicesInfo, m_VariableIndicesInfo,
                            m_AttributesIndicesInfo, serialized);
    }
}

/* Merge and serialize all the indices at each step */
void BP4Serializer::MergeSerializeIndicesPerStep(
    const std::unordered_map<std::string, std::vector<SerialElementIndex>>
        &nameRankIndices,
    helper::Comm const &comm, BufferSTL &bufferSTL)
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
    if (m_Parameters.Threads == 1) // enforcing serial version for now
    {
        for (const auto &rankIndices : nameRankIndices)
        {
            lf_MergeRankSerial(rankIndices.second, bufferSTL);
        }
        return;
    }

    // TODO need to debug this part, if threaded per variable
    const size_t elements = nameRankIndices.size();
    const size_t stride =
        elements / m_Parameters.Threads; // elements per thread
    const size_t last =
        stride + elements % m_Parameters.Threads; // remainder to last

    std::vector<std::thread> threads;
    threads.reserve(m_Parameters.Threads);

    // copy names in order to use threads
    std::vector<std::string> names;
    names.reserve(nameRankIndices.size());

    for (const auto &nameRankIndexPair : nameRankIndices)
    {
        names.push_back(nameRankIndexPair.first);
    }

    for (unsigned int t = 0; t < m_Parameters.Threads; ++t)
    {
        const size_t start = stride * t;
        size_t end = start + stride;

        if (t == m_Parameters.Threads - 1)
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
