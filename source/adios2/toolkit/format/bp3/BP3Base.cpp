/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP1Base.cpp
 *
 *  Created on: Feb 7, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adios2/ADIOSTypes.h"            //PathSeparator
#include "adios2/helper/adiosFunctions.h" //CreateDirectory, StringToTimeUnit,
#include <adios2/toolkit/format/bp3/BP3Base.h>
#include <adios2/toolkit/format/bp3/BP3Base.tcc>
// ReadValue

namespace adios2
{
namespace format
{

BP3Base::BP3Base(MPI_Comm mpiComm, const bool debugMode)
: m_MPIComm(mpiComm), m_DebugMode(debugMode)
{
    MPI_Comm_rank(m_MPIComm, &m_RankMPI);
    MPI_Comm_size(m_MPIComm, &m_SizeMPI);
    m_Profiler.IsActive = true; // default
}

void BP3Base::InitParameters(const Params &parameters)
{
    // flags for defaults that require constructors
    bool useDefaultInitialBufferSize = true;
    bool useDefaultProfileUnits = true;

    for (const auto &pair : parameters)
    {
        const std::string key(pair.first);
        const std::string value(pair.second);

        if (key == "Profile")
        {
            InitParameterProfile(value);
        }
        else if (key == "ProfileUnits")
        {
            InitParameterProfileUnits(value);
            useDefaultProfileUnits = false;
        }
        else if (key == "BufferGrowthFactor")
        {
            InitParameterBufferGrowth(value);
        }
        else if (key == "InitialBufferSize")
        {
            InitParameterInitBufferSize(value);
            useDefaultInitialBufferSize = false;
        }
        else if (key == "MaxBufferSize")
        {
            InitParameterMaxBufferSize(value);
        }
        else if (key == "Threads")
        {
            InitParameterThreads(value);
        }
        else if (key == "Verbose")
        {
            InitParameterVerbose(value);
        }
        else if (key == "CollectiveMetadata")
        {
            InitParameterCollectiveMetadata(value);
        }
    }

    // default timer for buffering
    if (m_Profiler.IsActive && useDefaultProfileUnits)
    {
        m_Profiler.Timers.emplace(
            "buffering",
            profiling::Timer("buffering", DefaultTimeUnitEnum, m_DebugMode));

        m_Profiler.Bytes.emplace("buffering", 0);
    }

    if (useDefaultInitialBufferSize)
    {
        m_Data.Resize(DefaultInitialBufferSize, "in call to Open");
    }
}

std::vector<std::string>
BP3Base::GetBPBaseNames(const std::vector<std::string> &names) const noexcept
{
    auto lf_GetBPBaseName = [](const std::string &name) -> std::string {

        const std::string bpBaseName(AddExtension(name, ".bp") + ".dir");
        return bpBaseName;
    };

    std::vector<std::string> bpBaseNames;
    bpBaseNames.reserve(names.size());

    for (const auto &name : names)
    {
        bpBaseNames.push_back(lf_GetBPBaseName(name));
    }
    return bpBaseNames;
}

std::vector<std::string>
BP3Base::GetBPMetadataFileNames(const std::vector<std::string> &names) const
    noexcept
{
    std::vector<std::string> metadataFileNames;
    metadataFileNames.reserve(names.size());
    for (const auto &name : names)
    {
        metadataFileNames.push_back(GetBPMetadataFileName(name));
    }
    return metadataFileNames;
}

std::string BP3Base::GetBPMetadataFileName(const std::string &name) const
    noexcept
{
    return AddExtension(name, ".bp");
}

std::vector<std::string>
BP3Base::GetBPRankNames(const std::vector<std::string> &names) const noexcept
{
    std::vector<std::string> bpNames;
    bpNames.reserve(names.size());

    for (const auto &name : names)
    {
        bpNames.push_back(
            GetBPRankName(name, static_cast<unsigned int>(m_RankMPI)));
    }
    return bpNames;
}

std::string BP3Base::GetBPSubFileName(const std::string &name,
                                      const unsigned int subFileIndex) const
    noexcept
{
    return GetBPRankName(name, subFileIndex);
}

// PROTECTED
void BP3Base::InitOnOffParameter(const std::string value, bool &parameter,
                                 const std::string hint)
{
    if (value == "off" || value == "Off")
    {
        parameter = false;
    }
    else if (value == "on" || value == "On")
    {
        parameter = true;
    }
    else
    {
        if (m_DebugMode)
        {
            throw std::invalid_argument("ERROR: IO SetParameters profile "
                                        "invalid value, " +
                                        hint + " in call to Open\n");
        }
    }
}

void BP3Base::InitParameterProfile(const std::string value)
{
    InitOnOffParameter(value, m_Profiler.IsActive, "valid: Profile On or Off");
}

void BP3Base::InitParameterProfileUnits(const std::string value)
{
    TimeUnit timeUnit = StringToTimeUnit(value, m_DebugMode);

    if (m_Profiler.Timers.count("buffering") == 1)
    {
        m_Profiler.Timers.erase("buffering");
    }

    m_Profiler.Timers.emplace(
        "buffering", profiling::Timer("buffering", timeUnit, m_DebugMode));

    m_Profiler.Bytes.emplace("buffering", 0);
}

void BP3Base::InitParameterBufferGrowth(const std::string value)
{
    if (m_DebugMode)
    {
        bool success = true;
        std::string description;

        try
        {
            m_GrowthFactor = std::stof(value);
        }
        catch (std::exception &e)
        {
            success = false;
            description = std::string(e.what());
        }

        if (!success || m_GrowthFactor <= 1.f)
        {
            throw std::invalid_argument(
                "ERROR: BufferGrowthFactor value "
                "can't be less or equal than 1 (default = 1.5), or "
                "couldn't "
                "convert number,\n additional description:" +
                description + "\n, in call to Open\n");
        }
    }
    else
    {
        m_GrowthFactor = std::stof(value);
    }
}

void BP3Base::InitParameterInitBufferSize(const std::string value)
{
    if (m_DebugMode)
    {
        if (value.size() < 2)
        {
            throw std::invalid_argument(
                "ERROR: wrong value for InitialBufferSize, it must be "
                "larger than 16Kb (minimum default), in call to Open\n");
        }
    }

    const std::string number(value.substr(0, value.size() - 2));
    const std::string units(value.substr(value.size() - 2));
    const size_t factor = BytesFactor(units, m_DebugMode);
    size_t bufferSize = DefaultInitialBufferSize; // from ADIOSTypes.h

    if (m_DebugMode)
    {
        bool success = true;
        std::string description;

        try
        {
            bufferSize = static_cast<size_t>(std::stoul(number) * factor);
        }
        catch (std::exception &e)
        {
            success = false;
            description = std::string(e.what());
        }

        if (!success || bufferSize < DefaultInitialBufferSize) // 16384b
        {
            throw std::invalid_argument(
                "ERROR: wrong value for InitialBufferSize, it must be "
                "larger "
                "than "
                "16Kb (minimum default), additional description: " +
                description + " in call to Open\n");
        }
    }
    else
    {
        bufferSize = static_cast<size_t>(std::stoul(number) * factor);
    }

    // m_HeapBuffer.ResizeData(bufferSize);
    m_Data.Resize(bufferSize, "bufferSize " + std::to_string(bufferSize) +
                                  ", in call to Open");
}

void BP3Base::InitParameterMaxBufferSize(const std::string value)
{
    if (m_DebugMode)
    {
        if (value.size() < 2)
        {
            throw std::invalid_argument(
                "ERROR: couldn't convert value of max_buffer_size IO "
                "SetParameter, valid syntax: MaxBufferSize=10Gb, "
                "MaxBufferSize=1000Mb, MaxBufferSize=16Kb (minimum "
                "default), "
                " in call to Open");
        }
    }

    const std::string number(value.substr(0, value.size() - 2));
    const std::string units(value.substr(value.size() - 2));
    const size_t factor = BytesFactor(units, m_DebugMode);

    if (m_DebugMode)
    {
        bool success = true;
        std::string description;

        try
        {
            m_MaxBufferSize = static_cast<size_t>(std::stoul(number) * factor);
        }
        catch (std::exception &e)
        {
            success = false;
            description = std::string(e.what());
        }

        if (!success || m_MaxBufferSize < 16 * 1024) // 16384b
        {
            throw std::invalid_argument(
                "ERROR: couldn't convert value of max_buffer_size IO "
                "SetParameter, valid syntax: MaxBufferSize=10Gb, "
                "MaxBufferSize=1000Mb, MaxBufferSize=16Kb (minimum "
                "default), "
                "\nadditional description: " +
                description + " in call to Open");
        }
    }
    else
    {
        m_MaxBufferSize = static_cast<size_t>(std::stoul(number) * factor);
    }
}

void BP3Base::InitParameterThreads(const std::string value)
{
    int threads = -1;

    if (m_DebugMode)
    {
        bool success = true;
        std::string description;

        try
        {
            threads = std::stoi(value);
        }
        catch (std::exception &e)
        {
            success = false;
            description = std::string(e.what());
        }

        if (!success || threads < 1)
        {
            throw std::invalid_argument(
                "ERROR: value in Threads=value in IO SetParameters must be "
                "an integer >= 1 (default) \nadditional description: " +
                description + "\n, in call to Open\n");
        }
    }
    else
    {
        threads = std::stoi(value);
    }

    m_Threads = static_cast<unsigned int>(threads);
}

void BP3Base::InitParameterVerbose(const std::string value)
{
    int verbosity = -1;

    if (m_DebugMode)
    {
        bool success = true;
        std::string description;

        try
        {
            verbosity = std::stoi(value);
        }
        catch (std::exception &e)
        {
            success = false;
            description = std::string(e.what());
        }

        if (!success || verbosity < 0 || verbosity > 5)
        {
            throw std::invalid_argument(
                "ERROR: value in Verbose=value in IO SetParameters must be "
                "an integer in the range [0,5], \nadditional "
                "description: " +
                description + "\n, in call to Open\n");
        }
    }
    else
    {
        verbosity = std::stoi(value);
    }

    m_Verbosity = static_cast<unsigned int>(verbosity);
}

void BP3Base::InitParameterCollectiveMetadata(const std::string value)
{
    InitOnOffParameter(value, m_CollectiveMetadata,
                       "valid: CollectiveMetadata On or Off");
}

std::vector<uint8_t>
BP3Base::GetTransportIDs(const std::vector<std::string> &transportsTypes) const
    noexcept
{
    auto lf_GetTransportID = [](const std::string method) -> uint8_t {

        int id = METHOD_UNKNOWN;
        if (method == "File_NULL")
        {
            id = METHOD_NULL;
        }
        else if (method == "File_POSIX")
        {
            id = METHOD_POSIX;
        }
        else if (method == "File_fstream")
        {
            id = METHOD_FSTREAM;
        }
        else if (method == "File_stdio")
        {
            id = METHOD_FILE;
        }

        return static_cast<uint8_t>(id);
    };

    std::vector<uint8_t> transportsIDs;
    transportsIDs.reserve(transportsTypes.size());

    for (const auto transportType : transportsTypes)
    {
        transportsIDs.push_back(lf_GetTransportID(transportType));
    }

    return transportsIDs;
}

size_t BP3Base::GetProcessGroupIndexSize(const std::string name,
                                         const std::string timeStepName,
                                         const size_t transportsSize) const
    noexcept
{
    // pgIndex + list of methods (transports)
    size_t pgSize =
        (name.length() + timeStepName.length() + 23) + (3 + transportsSize);

    return pgSize;
}

BP3Base::ElementIndexHeader
BP3Base::ReadElementIndexHeader(const std::vector<char> &buffer,
                                size_t &position) const noexcept
{
    ElementIndexHeader header;
    header.Length = ReadValue<uint32_t>(buffer, position);
    header.MemberID = ReadValue<uint32_t>(buffer, position);
    header.GroupName = ReadBP3String(buffer, position);
    header.Name = ReadBP3String(buffer, position);
    header.Path = ReadBP3String(buffer, position);
    header.DataType = ReadValue<int8_t>(buffer, position);
    header.CharacteristicsSetsCount = ReadValue<uint64_t>(buffer, position);

    return header;
}

std::string BP3Base::ReadBP3String(const std::vector<char> &buffer,
                                   size_t &position) const noexcept
{
    const size_t size =
        static_cast<size_t>(ReadValue<uint16_t>(buffer, position));

    if (size == 0)
    {
        return std::string();
    }

    const std::string values(&buffer[position], size);
    position += size;
    return values;
}

void BP3Base::ProfilerStart(const std::string process)
{
    if (m_Profiler.IsActive)
    {
        m_Profiler.Timers.at(process).Resume();
    }
}

void BP3Base::ProfilerStop(const std::string process)
{
    if (m_Profiler.IsActive)
    {
        m_Profiler.Timers.at(process).Pause();
    }
}

std::string BP3Base::GetBPRankName(const std::string &name,
                                   const unsigned int rank) const noexcept
{
    const std::string bpName = AddExtension(name, ".bp");

    // path/root.bp.dir/root.bp.rank
    std::string bpRoot = bpName;
    const auto lastPathSeparator(bpName.find_last_of(PathSeparator));

    if (lastPathSeparator != std::string::npos)
    {
        bpRoot = bpName.substr(lastPathSeparator);
    }
    const std::string bpRankName(bpName + ".dir/" + bpRoot + "." +
                                 std::to_string(rank));
    return bpRankName;
}

#define declare_template_instantiation(T)                                      \
    template BP3Base::ResizeResult BP3Base::ResizeBuffer(                      \
        const Variable<T> &variable);                                          \
                                                                               \
    template BP3Base::Characteristics<T>                                       \
    BP3Base::ReadElementIndexCharacteristics(const std::vector<char> &buffer,  \
                                             size_t &position,                 \
                                             const bool untilTimeStep) const;

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace format
} // end namespace adios2
