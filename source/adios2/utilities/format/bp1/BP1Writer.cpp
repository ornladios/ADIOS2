/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP1Writer.cpp
 *
 *  Created on: Feb 1, 2017
 *      Author: wfg
 */

#include "BP1Writer.h"
#include "BP1Writer.tcc"

#include <string>
#include <vector>

namespace adios
{
namespace format
{

std::size_t BP1Writer::GetProcessGroupIndexSize(
    const std::string name, const std::string timeStepName,
    const std::size_t numberOfTransports) const noexcept
{
    // pgIndex + list of methods (transports)
    return (name.length() + timeStepName.length() + 23) +
           (3 +
            numberOfTransports); // should be sufficient for data and metadata
                                 // pgindices
}

void BP1Writer::WriteProcessGroupIndex(
    const bool isFortran, const std::string name, const std::uint32_t processID,
    const std::vector<std::shared_ptr<Transport>> &transports,
    capsule::STLVector &heap, BP1MetadataSet &metadataSet) const noexcept
{
    std::vector<char> &metadataBuffer = metadataSet.PGIndex.Buffer;
    std::vector<char> &dataBuffer = heap.m_Data;

    metadataSet.DataPGLengthPosition = dataBuffer.size();
    dataBuffer.insert(dataBuffer.end(), 8, 0); // skip pg length (8)

    const std::size_t metadataPGLengthPosition = metadataBuffer.size();
    metadataBuffer.insert(metadataBuffer.end(), 2, 0); // skip pg length (2)

    // write name to metadata
    WriteNameRecord(name, metadataBuffer);
    // write if host language Fortran in metadata and data
    const char hostFortran =
        (isFortran) ? 'y' : 'n'; // if host language is fortran
    InsertToBuffer(metadataBuffer, &hostFortran);
    InsertToBuffer(dataBuffer, &hostFortran);
    // write name in data
    WriteNameRecord(name, dataBuffer);

    // processID in metadata,
    InsertToBuffer(metadataBuffer, &processID);
    // skip coordination var in data ....what is coordination var?
    dataBuffer.insert(dataBuffer.end(), 4, 0);

    // time step name to metadata and data
    const std::string timeStepName(std::to_string(metadataSet.TimeStep));
    WriteNameRecord(timeStepName, metadataBuffer);
    WriteNameRecord(timeStepName, dataBuffer);

    // time step to metadata and data
    InsertToBuffer(metadataBuffer, &metadataSet.TimeStep);
    InsertToBuffer(dataBuffer, &metadataSet.TimeStep);

    // offset to pg in data in metadata which is the current absolute position
    InsertToBuffer(metadataBuffer, reinterpret_cast<std::uint64_t *>(
                                       &heap.m_DataAbsolutePosition));

    // Back to writing metadata pg index length (length of group)
    const std::uint16_t metadataPGIndexLength =
        metadataBuffer.size() - metadataPGLengthPosition -
        2; // without length of group record
    CopyToBufferPosition(metadataBuffer, metadataPGLengthPosition,
                         &metadataPGIndexLength);
    // DONE With metadataBuffer

    // here write method in data
    const std::vector<std::uint8_t> methodIDs = GetMethodIDs(transports);
    const std::uint8_t methodsCount = methodIDs.size();
    InsertToBuffer(dataBuffer, &methodsCount); // count
    const std::uint16_t methodsLength =
        methodIDs.size() *
        3; // methodID (1) + method params length(2), no parameters for now
    InsertToBuffer(dataBuffer, &methodsLength); // length

    for (const auto methodID : methodIDs)
    {
        InsertToBuffer(dataBuffer, &methodID); // method ID,
        dataBuffer.insert(dataBuffer.end(), 2,
                          0); // skip method params length = 0 (2 bytes) for now
    }

    // update absolute position
    heap.m_DataAbsolutePosition +=
        dataBuffer.size() - metadataSet.DataPGLengthPosition;
    // pg vars count and position
    metadataSet.DataPGVarsCount = 0;
    metadataSet.DataPGVarsCountPosition = dataBuffer.size();
    // add vars count and length
    dataBuffer.insert(dataBuffer.end(), 12, 0);
    heap.m_DataAbsolutePosition += 12; // add vars count and length

    ++metadataSet.DataPGCount;
    metadataSet.DataPGIsOpen = true;
}

void BP1Writer::Advance(BP1MetadataSet &metadataSet, capsule::STLVector &buffer)
{
    FlattenData(metadataSet, buffer);
}

void BP1Writer::Close(BP1MetadataSet &metadataSet, capsule::STLVector &heap,
                      Transport &transport, bool &isFirstClose,
                      const bool doAggregation) const noexcept
{
    if (metadataSet.Log.IsActive == true)
        metadataSet.Log.Timers[0].SetInitialTime();

    if (isFirstClose == true)
    {
        if (metadataSet.DataPGIsOpen == true)
            FlattenData(metadataSet, heap);

        FlattenMetadata(metadataSet, heap);

        if (metadataSet.Log.IsActive == true)
            metadataSet.Log.Timers[0].SetInitialTime();

        if (doAggregation ==
            true) // N-to-M  where 1 <= M <= N-1, might need a new
                  // Log metadataSet.Log.m_Timers just for
                  // aggregation
        {
            // here call aggregator
        }
        isFirstClose = false;
    }

    if (doAggregation == true) // N-to-M  where 1 <= M <= N-1
    {
        // here call aggregator to select transports for Write and Close
    }
    else // N-to-N
    {
        transport.Write(heap.m_Data.data(), heap.m_Data.size()); // single write
        transport.Close();
    }
}

std::string BP1Writer::GetRankProfilingLog(
    const int rank, const BP1MetadataSet &metadataSet,
    const std::vector<std::shared_ptr<Transport>> &transports) const noexcept
{
    auto lf_WriterTimer = [](std::string &rankLog,
                             const profiling::Timer &timer) {
        rankLog += "'" + timer.m_Process + "_" + timer.GetUnits() + "': " +
                   std::to_string(timer.m_ProcessTime);
    };

    // prepare string dictionary per rank
    std::string rankLog("'rank_" + std::to_string(rank) + "': { ");

    auto &profiler = metadataSet.Log;
    rankLog += "'bytes': " + std::to_string(profiler.TotalBytes[0]) + ", ";
    lf_WriterTimer(rankLog, profiler.Timers[0]);
    rankLog += ", ";

    for (unsigned int t = 0; t < transports.size(); ++t)
    {
        rankLog += "'transport_" + std::to_string(t) + "': { ";
        rankLog += "'lib': '" + transports[t]->m_Type + "', ";

        for (unsigned int i = 0; i < 3; ++i)
        {
            lf_WriterTimer(rankLog, transports[t]->m_Profiler.Timers[i]);
            if (i < 2)
            {
                rankLog += ", ";
            }
            else
            {
                rankLog += " ";
            }
        }

        if (t == transports.size() - 1) // last element
        {
            rankLog += "}";
        }
        else
        {
            rankLog += "},";
        }
    }
    rankLog += " },";

    return rankLog;
}

// PRIVATE FUNCTIONS
void BP1Writer::WriteDimensionsRecord(
    std::vector<char> &buffer, const std::vector<std::size_t> &localDimensions,
    const std::vector<std::size_t> &globalDimensions,
    const std::vector<std::size_t> &globalOffsets, const unsigned int skip,
    const bool addType) const noexcept
{
    auto lf_WriteFlaggedDim = [](std::vector<char> &buffer, const char no,
                                 const std::size_t dimension) {
        InsertToBuffer(buffer, &no);
        InsertToBuffer(buffer,
                       reinterpret_cast<const std::uint64_t *>(&dimension));
    };

    // BODY Starts here
    if (globalDimensions.empty())
    {
        if (addType == true)
        {
            constexpr char no =
                'n'; // dimension format unsigned int value (not using
                     // memberID for now)
            for (const auto &localDimension : localDimensions)
            {
                lf_WriteFlaggedDim(buffer, no, localDimension);
                buffer.insert(buffer.end(), skip, 0);
            }
        }
        else
        {
            for (const auto &localDimension : localDimensions)
            {
                InsertToBuffer(buffer, reinterpret_cast<const std::uint64_t *>(
                                           &localDimension));
                buffer.insert(buffer.end(), skip, 0);
            }
        }
    }
    else
    {
        if (addType == true)
        {
            constexpr char no = 'n';
            for (unsigned int d = 0; d < localDimensions.size(); ++d)
            {
                lf_WriteFlaggedDim(buffer, no, localDimensions[d]);
                lf_WriteFlaggedDim(buffer, no, globalDimensions[d]);
                lf_WriteFlaggedDim(buffer, no, globalOffsets[d]);
            }
        }
        else
        {
            for (unsigned int d = 0; d < localDimensions.size(); ++d)
            {
                InsertToBuffer(buffer, reinterpret_cast<const std::uint64_t *>(
                                           &localDimensions[d]));
                InsertToBuffer(buffer, reinterpret_cast<const std::uint64_t *>(
                                           &globalDimensions[d]));
                InsertToBuffer(buffer, reinterpret_cast<const std::uint64_t *>(
                                           &globalOffsets[d]));
            }
        }
    }
}

void BP1Writer::WriteNameRecord(const std::string name,
                                std::vector<char> &buffer) const noexcept
{
    const std::uint16_t length = name.length();
    InsertToBuffer(buffer, &length);
    InsertToBuffer(buffer, name.c_str(), length);
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

void BP1Writer::FlattenData(BP1MetadataSet &metadataSet,
                            capsule::STLVector &heap) const noexcept
{
    auto &buffer = heap.m_Data;
    // vars count and Length (only for PG)
    CopyToBufferPosition(buffer, metadataSet.DataPGVarsCountPosition,
                         &metadataSet.DataPGVarsCount);
    const std::uint64_t varsLength = buffer.size() -
                                     metadataSet.DataPGVarsCountPosition - 8 -
                                     4; // without record itself and vars count
    CopyToBufferPosition(buffer, metadataSet.DataPGVarsCountPosition + 4,
                         &varsLength);

    // attributes (empty for now) count (4) and length (8) are zero by moving
    // positions in time step zero
    buffer.insert(buffer.end(), 12, 0);
    heap.m_DataAbsolutePosition += 12;

    // Finish writing pg group length
    const std::uint64_t dataPGLength =
        buffer.size() - metadataSet.DataPGLengthPosition -
        8; // without record itself, 12 due to empty attributes
    CopyToBufferPosition(buffer, metadataSet.DataPGLengthPosition,
                         &dataPGLength);

    ++metadataSet.TimeStep;
    metadataSet.DataPGIsOpen = false;
}

void BP1Writer::FlattenMetadata(BP1MetadataSet &metadataSet,
                                capsule::STLVector &heap) const noexcept
{
    auto lf_IndexCountLength =
        [](std::unordered_map<std::string, BP1Index> &indices,
           std::uint32_t &count, std::uint64_t &length) {

            count = indices.size();
            length = 0;
            for (auto &indexPair : indices) // set each index length
            {
                auto &indexBuffer = indexPair.second.Buffer;
                const std::uint32_t indexLength = indexBuffer.size() - 4;
                CopyToBufferPosition(indexBuffer, 0, &indexLength);

                length += indexBuffer.size(); // overall length
            }
        };

    auto lf_FlattenIndices =
        [](const std::uint32_t count, const std::uint64_t length,
           const std::unordered_map<std::string, BP1Index> &indices,
           std::vector<char> &buffer) {
            InsertToBuffer(buffer, &count);
            InsertToBuffer(buffer, &length);

            for (const auto &indexPair : indices) // set each index length
            {
                const auto &indexBuffer = indexPair.second.Buffer;
                InsertToBuffer(buffer, indexBuffer.data(), indexBuffer.size());
            }
        };

    // Finish writing metadata counts and lengths
    // PG Index
    const std::uint64_t pgCount = metadataSet.DataPGCount;
    const std::uint64_t pgLength = metadataSet.PGIndex.Buffer.size();

    // var index count and length (total), and each index length
    std::uint32_t varsCount;
    std::uint64_t varsLength;
    lf_IndexCountLength(metadataSet.VarsIndices, varsCount, varsLength);
    // attribute index count and length, and each index length
    std::uint32_t attributesCount;
    std::uint64_t attributesLength;
    lf_IndexCountLength(metadataSet.AttributesIndices, attributesCount,
                        attributesLength);

    const std::size_t footerSize = (pgLength + 16) + (varsLength + 12) +
                                   (attributesLength + 12) +
                                   metadataSet.MiniFooterSize;
    auto &buffer = heap.m_Data;
    buffer.reserve(buffer.size() + footerSize); // reserve data to fit metadata,
    // must replace with growth buffer
    // strategy

    // write pg index
    InsertToBuffer(buffer, &pgCount);
    InsertToBuffer(buffer, &pgLength);
    InsertToBuffer(buffer, metadataSet.PGIndex.Buffer.data(), pgLength);
    // Vars indices
    lf_FlattenIndices(varsCount, varsLength, metadataSet.VarsIndices, buffer);
    // Attribute indices
    lf_FlattenIndices(attributesCount, attributesLength,
                      metadataSet.AttributesIndices, buffer);

    // getting absolute offsets, minifooter is 28 bytes for now
    const std::uint64_t offsetPGIndex = heap.m_DataAbsolutePosition;
    const std::uint64_t offsetVarsIndex = offsetPGIndex + (pgLength + 16);
    const std::uint64_t offsetAttributeIndex =
        offsetVarsIndex + (varsLength + 12);

    InsertToBuffer(buffer, &offsetPGIndex);
    InsertToBuffer(buffer, &offsetVarsIndex);
    InsertToBuffer(buffer, &offsetAttributeIndex);

    // version
    if (IsLittleEndian())
    {
        const std::uint8_t endian = 0;
        InsertToBuffer(buffer, &endian);
        buffer.insert(buffer.end(), 2, 0);
        InsertToBuffer(buffer, &m_Version);
    }
    else
    {
    }

    heap.m_DataAbsolutePosition += footerSize;

    if (metadataSet.Log.IsActive == true)
        metadataSet.Log.TotalBytes.push_back(heap.m_DataAbsolutePosition);
}

//------------------------------------------------------------------------------
// Explicit instantiaiton of public tempaltes

#define declare_template_instantiation(T)                                      \
    template void BP1Writer::WriteVariablePayload(                             \
        const Variable<T> &variable, capsule::STLVector &heap,                 \
        const unsigned int nthreads) const noexcept;                           \
                                                                               \
    template void BP1Writer::WriteVariableMetadata(                            \
        const Variable<T> &variable, capsule::STLVector &heap,                 \
        BP1MetadataSet &metadataSet) const noexcept;

ADIOS_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

//------------------------------------------------------------------------------

} // end namespace format
} // end namespace adios
