/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPSerializer.cpp
 *
 *  Created on: Sep 16, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "BPSerializer.h"
#include "BPSerializer.tcc"

namespace adios2
{
namespace format
{

// PUBLIC
BPSerializer::BPSerializer(const helper::Comm &comm, const bool debugMode,
                           const uint8_t version)
: BPBase(comm, debugMode), m_Version(version)
{
}

void BPSerializer::SerializeData(core::IO &io, const bool advanceStep)
{
    m_Profiler.Start("buffering");
    SerializeDataBuffer(io);
    if (advanceStep)
    {
        ++m_MetadataSet.TimeStep;
        ++m_MetadataSet.CurrentStep;
    }
    m_Profiler.Stop("buffering");
}

std::string BPSerializer::GetRankProfilingJSON(
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

    std::string timeDate(profiler.m_Timers.at("buffering").m_LocalTimeDate);
    timeDate.pop_back();
    // avoid whitespace
    std::replace(timeDate.begin(), timeDate.end(), ' ', '_');

    rankLog += "\"start\": \"" + timeDate + "\", ";
    rankLog += "\"threads\": " + std::to_string(m_Parameters.Threads) + ", ";
    rankLog +=
        "\"bytes\": " + std::to_string(profiler.m_Bytes.at("buffering")) + ", ";

    for (const auto &timerPair : profiler.m_Timers)
    {
        const profiling::Timer &timer = timerPair.second;
        rankLog += "\"" + timer.m_Process + "_" + timer.GetShortUnits() +
                   "\": " + std::to_string(timer.m_ProcessTime) + ", ";
    }

    const size_t transportsSize = transportsTypes.size();

    for (unsigned int t = 0; t < transportsSize; ++t)
    {
        rankLog += "\"transport_" + std::to_string(t) + "\": { ";
        rankLog += "\"type\": \"" + transportsTypes[t] + "\", ";

        for (const auto &transportTimerPair : transportsProfilers[t]->m_Timers)
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
BPSerializer::AggregateProfilingJSON(const std::string &rankLog) const
{
    // Gather sizes
    const size_t rankLogSize = rankLog.size();
    std::vector<size_t> rankLogsSizes = m_Comm.GatherValues(rankLogSize);

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

    m_Comm.GathervArrays(rankLog.c_str(), rankLog.size(), rankLogsSizes.data(),
                         rankLogsSizes.size(), &profilingJSON[position]);

    if (m_RankMPI == 0) // add footer to close JSON
    {
        position += gatheredSize - 2;
        helper::CopyToBuffer(profilingJSON, position, footer.c_str(),
                             footer.size());
    }

    return profilingJSON;
}

void BPSerializer::PutNameRecord(const std::string name,
                                 std::vector<char> &buffer) noexcept
{
    const uint16_t length = static_cast<uint16_t>(name.size());
    helper::InsertToBuffer(buffer, &length);
    helper::InsertToBuffer(buffer, name.c_str(), name.size());
}

void BPSerializer::PutNameRecord(const std::string name,
                                 std::vector<char> &buffer,
                                 size_t &position) noexcept
{
    const uint16_t length = static_cast<uint16_t>(name.length());
    helper::CopyToBuffer(buffer, position, &length);
    helper::CopyToBuffer(buffer, position, name.c_str(), length);
}

void BPSerializer::PutDimensionsRecord(const Dims &localDimensions,
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

void BPSerializer::PutDimensionsRecord(const Dims &localDimensions,
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

void BPSerializer::PutMinifooter(const uint64_t pgIndexStart,
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

void BPSerializer::UpdateOffsetsInMetadata()
{
    auto lf_UpdatePGIndexOffsets = [&]() {
        auto &buffer = m_MetadataSet.PGIndex.Buffer;
        size_t &currentPosition = m_MetadataSet.PGIndex.LastUpdatedPosition;
        const bool isLittleEndian = helper::IsLittleEndian();

        while (currentPosition < buffer.size())
        {
            ProcessGroupIndex pgIndex = ReadProcessGroupIndexHeader(
                buffer, currentPosition, isLittleEndian);

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
        ElementIndexHeader header = ReadElementIndexHeader(
            buffer, headerPosition, helper::IsLittleEndian());
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

#define declare_template_instantiation(T)                                      \
    template void BPSerializer::PutAttributeCharacteristicValueInIndex(        \
        uint8_t &, const core::Attribute<T> &, std::vector<char> &) noexcept;

ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

#define declare_template_instantiation(T)                                      \
                                                                               \
    template void BPSerializer::PutCharacteristicRecord(                       \
        const uint8_t, uint8_t &, const T &, std::vector<char> &) noexcept;    \
                                                                               \
    template void BPSerializer::PutCharacteristicRecord(                       \
        const uint8_t, uint8_t &, const T &, std::vector<char> &,              \
        size_t &) noexcept;                                                    \
                                                                               \
    template void BPSerializer::PutPayloadInBuffer(                            \
        const core::Variable<T> &, const typename core::Variable<T>::Info &,   \
        const bool) noexcept;

ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace format
} // end namespace adios2
