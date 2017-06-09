/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP1Writer.cpp
 *
 *  Created on: Feb 1, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "BP1Writer.h"
#include "BP1Writer.tcc"

#include <string>
#include <vector>

namespace adios
{
namespace format
{

BP1Writer::BP1Writer(MPI_Comm mpiComm, const bool debugMode)
: BP1Base(mpiComm, debugMode)
{
}

size_t BP1Writer::GetProcessGroupIndexSize(const std::string name,
                                           const std::string timeStepName,
                                           const size_t transportsSize) const
    noexcept
{
    // pgIndex + list of methods (transports)
    size_t pgSize =
        (name.length() + timeStepName.length() + 23) + (3 + transportsSize);

    return pgSize;
}

void BP1Writer::WriteProcessGroupIndex(
    const std::string hostLanguage,
    const std::vector<std::string> &transportsTypes) noexcept
{
    if (m_Profiler.IsActive)
    {
        m_Profiler.Timers.at("buffering").Resume();
    }

    std::vector<char> &metadataBuffer = m_MetadataSet.PGIndex.Buffer;

    std::vector<char> &dataBuffer = m_HeapBuffer.m_Data;
    size_t &dataPosition = m_HeapBuffer.m_DataPosition;

    m_MetadataSet.DataPGLengthPosition = dataPosition;
    dataPosition += 8; // skip pg length (8)

    const std::size_t metadataPGLengthPosition = metadataBuffer.size();
    metadataBuffer.insert(metadataBuffer.end(), 2, 0); // skip pg length (2)

    // write name to metadata
    const std::string name(std::to_string(m_BP1Aggregator.m_RankMPI));

    WriteNameRecord(name, metadataBuffer);
    // write if host language Fortran in metadata and data
    const char hostFortran = (hostLanguage == "Fortran") ? 'y' : 'n';
    InsertToBuffer(metadataBuffer, &hostFortran);
    CopyToBuffer(dataBuffer, dataPosition, &hostFortran);
    // write name in data
    WriteNameRecord(name, dataBuffer, dataPosition);

    // processID in metadata,
    const uint32_t processID = static_cast<uint32_t>(m_BP1Aggregator.m_RankMPI);
    InsertToBuffer(metadataBuffer, &processID);
    // skip coordination var in data ....what is coordination var?
    dataPosition += 4;

    // time step name to metadata and data
    const std::string timeStepName(std::to_string(m_MetadataSet.TimeStep));
    WriteNameRecord(timeStepName, metadataBuffer);
    WriteNameRecord(timeStepName, dataBuffer, dataPosition);

    // time step to metadata and data
    InsertToBuffer(metadataBuffer, &m_MetadataSet.TimeStep);
    CopyToBuffer(dataBuffer, dataPosition, &m_MetadataSet.TimeStep);

    // offset to pg in data in metadata which is the current absolute position
    InsertToBuffer(metadataBuffer, reinterpret_cast<uint64_t *>(
                                       &m_HeapBuffer.m_DataAbsolutePosition));

    // Back to writing metadata pg index length (length of group)
    const uint16_t metadataPGIndexLength =
        metadataBuffer.size() - metadataPGLengthPosition - 2;
    size_t backPosition = metadataPGLengthPosition;
    CopyToBuffer(metadataBuffer, backPosition, &metadataPGIndexLength);
    // DONE With metadataBuffer

    // here write method in data
    const std::vector<uint8_t> methodIDs = GetTransportIDs(transportsTypes);
    const uint8_t methodsCount = methodIDs.size();
    CopyToBuffer(dataBuffer, dataPosition, &methodsCount); // count
    // methodID (1) + method params length(2), no parameters for now
    const uint16_t methodsLength = methodIDs.size() * 3;

    CopyToBuffer(dataBuffer, dataPosition, &methodsLength); // length

    for (const auto methodID : methodIDs)
    {
        CopyToBuffer(dataBuffer, dataPosition, &methodID); // method ID,
        dataPosition += 2; // skip method params length = 0 (2 bytes) for now
    }

    // update absolute position
    m_HeapBuffer.m_DataAbsolutePosition +=
        dataPosition - m_MetadataSet.DataPGLengthPosition;
    // pg vars count and position
    m_MetadataSet.DataPGVarsCount = 0;
    m_MetadataSet.DataPGVarsCountPosition = dataPosition;
    // add vars count and length
    dataPosition += 12;
    m_HeapBuffer.m_DataAbsolutePosition += 12; // add vars count and length

    ++m_MetadataSet.DataPGCount;
    m_MetadataSet.DataPGIsOpen = true;

    if (m_Profiler.IsActive)
    {
        m_Profiler.Timers.at("buffering").Pause();
    }
}

void BP1Writer::Advance()
{
    if (m_Profiler.IsActive)
    {
        m_Profiler.Timers.at("buffering").Resume();
    }

    FlattenData();

    if (m_Profiler.IsActive)
    {
        m_Profiler.Timers.at("buffering").Pause();
    }
}

void BP1Writer::Close() noexcept
{
    if (m_Profiler.IsActive)
    {
        m_Profiler.Timers.at("buffering").Resume();
    }

    if (!m_IsClosed)
    {
        if (m_MetadataSet.DataPGIsOpen)
        {
            FlattenData();
        }

        FlattenMetadata();
        m_IsClosed = true;
    }

    if (m_Profiler.IsActive)
    {
        m_Profiler.Timers.at("buffering").Pause();
    }
}

std::string BP1Writer::GetRankProfilingLog(
    const std::vector<std::string> &transportsTypes,
    const std::vector<profiling::IOChrono *> &transportsProfilers) noexcept
{
    auto lf_WriterTimer = [](std::string &rankLog,
                             const profiling::Timer &timer) {
        rankLog += "'" + timer.m_Process + "_" + timer.GetShortUnits() + "': " +
                   std::to_string(timer.m_ProcessTime) + ", ";
    };

    // prepare string dictionary per rank
    std::string rankLog("'rank_" + std::to_string(m_BP1Aggregator.m_RankMPI) +
                        "': { ");

    auto &profiler = m_Profiler;
    rankLog +=
        "'bytes': " + std::to_string(profiler.Bytes.at("buffering")) + ", ";
    lf_WriterTimer(rankLog, profiler.Timers.at("buffering"));

    const size_t transportsSize = transportsTypes.size();

    for (unsigned int t = 0; t < transportsSize; ++t)
    {
        rankLog += "'transport_" + std::to_string(t) + "': { ";
        rankLog += "'lib': '" + transportsTypes[t] + "', ";

        for (const auto &transportTimerPair : transportsProfilers[t]->Timers)
        {
            lf_WriterTimer(rankLog, transportTimerPair.second);
            rankLog += ", ";
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
    rankLog += " }";

    return rankLog;
}

// PRIVATE FUNCTIONS
void BP1Writer::WriteDimensionsRecord(const Dims localDimensions,
                                      const Dims globalDimensions,
                                      const Dims offsets,
                                      std::vector<char> &buffer) noexcept
{
    if (offsets.empty())
    {
        for (const auto &localDimension : localDimensions)
        {
            InsertToBuffer(buffer,
                           reinterpret_cast<const uint64_t *>(&localDimension));
            buffer.insert(buffer.end(), 2 * sizeof(uint64_t), 0);
        }
    }
    else
    {
        for (unsigned int d = 0; d < localDimensions.size(); ++d)
        {
            InsertToBuffer(buffer, reinterpret_cast<const uint64_t *>(
                                       &localDimensions[d]));
            InsertToBuffer(buffer, reinterpret_cast<const uint64_t *>(
                                       &globalDimensions[d]));
            InsertToBuffer(buffer,
                           reinterpret_cast<const uint64_t *>(&offsets[d]));
        }
    }
}

void BP1Writer::WriteDimensionsRecord(const Dims localDimensions,
                                      const Dims globalDimensions,
                                      const Dims offsets,
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
            CopyToBuffer(buffer, position, &no);
        }

        CopyToBuffer(buffer, position,
                     reinterpret_cast<const uint64_t *>(&dimension));
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

void BP1Writer::WriteNameRecord(const std::string name,
                                std::vector<char> &buffer) noexcept
{
    const uint16_t length = name.length();
    InsertToBuffer(buffer, &length);
    InsertToBuffer(buffer, name.c_str(), length);
}

void BP1Writer::WriteNameRecord(const std::string name,
                                std::vector<char> &buffer,
                                size_t &position) noexcept
{
    const uint16_t length = name.length();
    CopyToBuffer(buffer, position, &length);
    CopyToBuffer(buffer, position, name.c_str(), length);
}

BP1Index &
BP1Writer::GetBP1Index(const std::string name,
                       std::unordered_map<std::string, BP1Index> &indices,
                       bool &isNew) const noexcept
{
    auto itName = indices.find(name);
    if (itName == indices.end())
    {
        indices.emplace(name, BP1Index(indices.size()));
        isNew = true;
        return indices.at(name);
    }

    isNew = false;
    return itName->second;
}

void BP1Writer::FlattenData() noexcept
{
    auto &buffer = m_HeapBuffer.m_Data;
    auto &position = m_HeapBuffer.m_DataPosition;

    // vars count and Length (only for PG)
    CopyToBuffer(buffer, m_MetadataSet.DataPGVarsCountPosition,
                 &m_MetadataSet.DataPGVarsCount);
    // without record itself and vars count
    const uint64_t varsLength =
        position - m_MetadataSet.DataPGVarsCountPosition - 8 - 4;
    CopyToBuffer(buffer, m_MetadataSet.DataPGVarsCountPosition, &varsLength);

    // attributes (empty for now) count (4) and length (8) are zero by moving
    // positions in time step zero
    position += 12;
    m_HeapBuffer.m_DataAbsolutePosition += 12;

    // Finish writing pg group length
    // without record itself, 12 due to empty attributes
    const uint64_t dataPGLength =
        position - m_MetadataSet.DataPGLengthPosition - 8;
    CopyToBuffer(buffer, m_MetadataSet.DataPGLengthPosition, &dataPGLength);

    ++m_MetadataSet.TimeStep;
    m_MetadataSet.DataPGIsOpen = false;
}

void BP1Writer::FlattenMetadata() noexcept
{
    auto lf_IndexCountLength =
        [](std::unordered_map<std::string, BP1Index> &indices, uint32_t &count,
           uint64_t &length) {

            count = indices.size();
            length = 0;
            for (auto &indexPair : indices) // set each index length
            {
                auto &indexBuffer = indexPair.second.Buffer;
                const uint32_t indexLength = indexBuffer.size() - 4;
                size_t indexLengthPosition = 0;
                CopyToBuffer(indexBuffer, indexLengthPosition, &indexLength);

                length += indexBuffer.size(); // overall length
            }
        };

    auto lf_FlattenIndices =
        [](const uint32_t count, const uint64_t length,
           const std::unordered_map<std::string, BP1Index> &indices,
           std::vector<char> &buffer, size_t &position) {

            CopyToBuffer(buffer, position, &count);
            CopyToBuffer(buffer, position, &length);

            for (const auto &indexPair : indices) // set each index length
            {
                const auto &indexBuffer = indexPair.second.Buffer;
                CopyToBuffer(buffer, position, indexBuffer.data(),
                             indexBuffer.size());
            }
        };

    // Finish writing metadata counts and lengths
    // PG Index
    const uint64_t pgCount = m_MetadataSet.DataPGCount;
    const uint64_t pgLength = m_MetadataSet.PGIndex.Buffer.size();

    // var index count and length (total), and each index length
    uint32_t varsCount;
    uint64_t varsLength;
    lf_IndexCountLength(m_MetadataSet.VarsIndices, varsCount, varsLength);
    // attribute index count and length, and each index length
    uint32_t attributesCount;
    uint64_t attributesLength;
    lf_IndexCountLength(m_MetadataSet.AttributesIndices, attributesCount,
                        attributesLength);

    const size_t footerSize = (pgLength + 16) + (varsLength + 12) +
                              (attributesLength + 12) +
                              m_MetadataSet.MiniFooterSize;

    auto &buffer = m_HeapBuffer.m_Data;
    auto &position = m_HeapBuffer.m_DataPosition;

    // reserve data to fit metadata,
    // must replace with growth buffer strategy
    m_HeapBuffer.ResizeData(position + footerSize);
    // buffer.resize(position + footerSize);

    // write pg index
    CopyToBuffer(buffer, position, &pgCount);
    CopyToBuffer(buffer, position, &pgLength);
    CopyToBuffer(buffer, position, m_MetadataSet.PGIndex.Buffer.data(),
                 pgLength);

    // Vars indices
    lf_FlattenIndices(varsCount, varsLength, m_MetadataSet.VarsIndices, buffer,
                      position);
    // Attribute indices
    lf_FlattenIndices(attributesCount, attributesLength,
                      m_MetadataSet.AttributesIndices, buffer, position);

    // getting absolute offsets, minifooter is 28 bytes for now
    const uint64_t offsetPGIndex = m_HeapBuffer.m_DataAbsolutePosition;
    const uint64_t offsetVarsIndex = offsetPGIndex + (pgLength + 16);
    const uint64_t offsetAttributeIndex = offsetVarsIndex + (varsLength + 12);

    CopyToBuffer(buffer, position, &offsetPGIndex);
    CopyToBuffer(buffer, position, &offsetVarsIndex);
    CopyToBuffer(buffer, position, &offsetAttributeIndex);

    // version
    if (IsLittleEndian())
    {
        const uint8_t endian = 0;
        CopyToBuffer(buffer, position, &endian);
        position += 2;
        CopyToBuffer(buffer, position, &m_Version);
    }
    else
    {
    }

    m_HeapBuffer.m_DataAbsolutePosition += footerSize;

    if (m_Profiler.IsActive)
    {
        m_Profiler.Bytes.emplace("buffering",
                                 m_HeapBuffer.m_DataAbsolutePosition);
    }
}

//------------------------------------------------------------------------------
// Explicit instantiation of only public templates

#define declare_template_instantiation(T)                                      \
    template BP1Writer::ResizeResult BP1Writer::ResizeBuffer(                  \
        const Variable<T> &variable);                                          \
                                                                               \
    template void BP1Writer::WriteVariableMetadata(                            \
        const Variable<T> &variable) noexcept;                                 \
                                                                               \
    template void BP1Writer::WriteVariablePayload(                             \
        const Variable<T> &variable) noexcept;

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

//------------------------------------------------------------------------------

} // end namespace format
} // end namespace adios
