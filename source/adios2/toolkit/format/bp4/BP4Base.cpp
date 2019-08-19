/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP4Base.cpp
 *
 *  Created on: Aug 1, 2018
 *      Author: Lipeng Wan wanl@ornl.gov
 */
#include "BP4Base.h"
#include "BP4Base.tcc"

#include <algorithm> // std::transform
#include <iostream>  //std::cout Warnings

#include "adios2/common/ADIOSTypes.h"     //PathSeparator
#include "adios2/helper/adiosFunctions.h" //CreateDirectory, StringToTimeUnit,

#include "adios2/toolkit/format/bpOperation/compress/BPBZIP2.h"
#include "adios2/toolkit/format/bpOperation/compress/BPBlosc.h"
#include "adios2/toolkit/format/bpOperation/compress/BPMGARD.h"
#include "adios2/toolkit/format/bpOperation/compress/BPPNG.h"
#include "adios2/toolkit/format/bpOperation/compress/BPSZ.h"
#include "adios2/toolkit/format/bpOperation/compress/BPZFP.h"

namespace adios2
{
namespace format
{

const std::set<std::string> BP4Base::m_TransformTypes = {
    {"unknown", "none", "identity", "bzip2", "sz", "zfp", "mgard", "png",
     "blosc"}};

const std::map<int, std::string> BP4Base::m_TransformTypesToNames = {
    {transform_unknown, "unknown"},   {transform_none, "none"},
    {transform_identity, "identity"}, {transform_sz, "sz"},
    {transform_zfp, "zfp"},           {transform_mgard, "mgard"},
    {transform_png, "png"},           {transform_bzip2, "bzip2"},
    {transform_blosc, "blosc"}
    // {transform_zlib, "zlib"},
    //    {transform_szip, "szip"},
    //    {transform_isobar, "isobar"},
    //    {transform_aplod, "aplod"},
    //    {transform_alacrity, "alacrity"},

    //    {transform_sz, "sz"},
    //    {transform_lz4, "lz4"},
};

BP4Base::BP4Base(MPI_Comm mpiComm, const bool debugMode)
: m_MPIComm(mpiComm), m_DebugMode(debugMode)
{
    SMPI_Comm_rank(m_MPIComm, &m_RankMPI);
    SMPI_Comm_size(m_MPIComm, &m_SizeMPI);
    m_Profiler.IsActive = true; // default
}

BP4Base::~BP4Base() {}

void BP4Base::InitParameters(const Params &parameters)
{
    // flags for defaults that require constructors
    bool useDefaultInitialBufferSize = true;
    bool useDefaultProfileUnits = true;

    for (const auto &pair : parameters)
    {
        std::string key(pair.first);
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        std::string value(pair.second);
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);

        if (key == "profile")
        {
            InitParameterProfile(value);
        }
        else if (key == "opentimeoutsecs")
        {
            m_OpenTimeoutSecs = InitParameterFloat(value, "OpenTimeoutSecs");

            if (m_OpenTimeoutSecs < 0.0)
            {
                m_OpenTimeoutSecs = 2144448000.0f; // int-friendly 68 years
            }
        }
        else if (key == "beginsteppollingfrequencysecs")
        {
            m_BeginStepPollingFrequencySecs =
                InitParameterFloat(value, "BeginStepPollingFrequencySecs");
            if (m_BeginStepPollingFrequencySecs < 0.0)
            {
                m_BeginStepPollingFrequencySecs = 1.0; // a second
            }
            m_BeginStepPollingFrequencyIsSet = true;
        }
        else if (key == "profileunits")
        {
            InitParameterProfileUnits(value);
            useDefaultProfileUnits = false;
        }
        else if (key == "buffergrowthfactor")
        {
            InitParameterBufferGrowth(value);
        }
        else if (key == "initialbuffersize")
        {
            InitParameterInitBufferSize(value);
            useDefaultInitialBufferSize = false;
        }
        else if (key == "maxbuffersize")
        {
            InitParameterMaxBufferSize(value);
        }
        else if (key == "threads")
        {
            InitParameterThreads(value);
        }
        else if (key == "statslevel")
        {
            InitParameterStatsLevel(value);
        }
        else if (key == "statsblocksize")
        {
            m_StatsBlockSize = InitParameterSizeT(value, "StatsBlockSize");
        }
        else if (key == "collectivemetadata")
        {
            InitParameterCollectiveMetadata(value);
        }
        else if (key == "flushstepscount")
        {
            InitParameterFlushStepsCount(value);
        }
        else if (key == "substreams")
        {
            InitParameterSubStreams(value);
        }
        else if (key == "node-local")
        {
            InitParameterNodeLocal(value);
        }
    }

    // default timer for buffering
    if (m_Profiler.IsActive && useDefaultProfileUnits)
    {
        auto lf_EmplaceTimer = [&](const std::string process) {
            m_Profiler.Timers.emplace(
                process,
                profiling::Timer(process, DefaultTimeUnitEnum, m_DebugMode));
        };

        lf_EmplaceTimer("buffering");
        lf_EmplaceTimer("memcpy");
        lf_EmplaceTimer("minmax");
        lf_EmplaceTimer("meta_sort_merge");
        lf_EmplaceTimer("aggregation");
        lf_EmplaceTimer("mkdir");

        m_Profiler.Bytes.emplace("buffering", 0);
    }

    ProfilerStart("buffering");
    if (useDefaultInitialBufferSize)
    {
        m_Data.Resize(DefaultInitialBufferSize, "in call to Open");
    }
    ProfilerStop("buffering");
}

std::string BP4Base::RemoveTrailingSlash(const std::string &name) const noexcept
{
    size_t len = name.size();
    while (name[len - 1] == PathSeparator)
    {
        --len;
    }
    return name.substr(0, len);
}

std::vector<std::string>
BP4Base::GetBPBaseNames(const std::vector<std::string> &names) const noexcept
{
    std::vector<std::string> bpBaseNames;
    bpBaseNames.reserve(names.size());

    for (const auto &name : names)
    {
        bpBaseNames.push_back(RemoveTrailingSlash(name));
    }
    return bpBaseNames;
}

std::vector<std::string>
BP4Base::GetBPMetadataFileNames(const std::vector<std::string> &names) const
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

std::string BP4Base::GetBPMetadataFileName(const std::string &name) const
    noexcept
{
    const std::string bpName = RemoveTrailingSlash(name);
    const size_t index = 0; // global metadata file is generated by rank 0
    /* the name of the metadata file is "md.0" */
    const std::string bpMetaDataRankName(bpName + PathSeparator + "md." +
                                         std::to_string(index));
    return bpMetaDataRankName;
}

std::vector<std::string> BP4Base::GetBPMetadataIndexFileNames(
    const std::vector<std::string> &names) const noexcept
{
    std::vector<std::string> metadataIndexFileNames;
    metadataIndexFileNames.reserve(names.size());
    for (const auto &name : names)
    {
        metadataIndexFileNames.push_back(GetBPMetadataIndexFileName(name));
    }
    return metadataIndexFileNames;
}

std::string BP4Base::GetBPMetadataIndexFileName(const std::string &name) const
    noexcept
{
    const std::string bpName = RemoveTrailingSlash(name);
    /* the name of the metadata index file is "md.idx" */
    const std::string bpMetaDataIndexRankName(bpName + PathSeparator +
                                              "md.idx");
    return bpMetaDataIndexRankName;
}

std::vector<std::string>
BP4Base::GetBPSubStreamNames(const std::vector<std::string> &names) const
    noexcept
{
    std::vector<std::string> bpNames;
    bpNames.reserve(names.size());

    for (const auto &name : names)
    {
        bpNames.push_back(
            GetBPSubStreamName(name, static_cast<unsigned int>(m_RankMPI)));
    }
    return bpNames;
}

std::string BP4Base::GetBPSubFileName(const std::string &name,
                                      const size_t subFileIndex,
                                      const bool hasSubFiles) const noexcept
{
    return GetBPSubStreamName(name, subFileIndex, hasSubFiles);
}

size_t BP4Base::GetBPIndexSizeInData(const std::string &variableName,
                                     const Dims &count) const noexcept
{
    size_t indexSize = 23; // header
    indexSize += 4 + 32;   // "[VMD" and padded " *VMD]" up to 31 char length
    indexSize += variableName.size();

    // characteristics 3 and 4, check variable number of dimensions
    const size_t dimensions = count.size();
    indexSize += 28 * dimensions; // 28 bytes per dimension
    indexSize += 1;               // id

    // characteristics, offset + payload offset in data
    indexSize += 2 * (1 + 8);
    // characteristic 0, if scalar add value, for now only allowing string
    if (dimensions == 1)
    {
        indexSize += 2 * sizeof(uint64_t); // complex largest size
        indexSize += 1;                    // id
        indexSize += 1;                    // id
    }

    // characteristic statistics
    indexSize += 5;        // count + length
    if (m_StatsLevel == 1) // default, only min and max and dimensions
    {
        const size_t nElems = helper::GetTotalSize(count);
        const size_t nSubblocks = nElems / m_StatsBlockSize;
        indexSize += 2 * (nSubblocks + 1) * (2 * sizeof(uint64_t) + 1);
        indexSize += dimensions * 2;
        indexSize += 1 + 2; // id + # of subblocks field
    }
    // dimensions
    indexSize += 28 * dimensions + 1;

    // extra 12 bytes for attributes in case of last variable
    // extra 4 bytes for PGI] in case of last variable
    return indexSize + 12 + 4;
}

void BP4Base::ResetBuffer(BufferSTL &bufferSTL,
                          const bool resetAbsolutePosition,
                          const bool zeroInitialize)
{
    ProfilerStart("buffering");
    bufferSTL.m_Position = 0;
    if (resetAbsolutePosition)
    {
        bufferSTL.m_AbsolutePosition = 0;
    }
    if (zeroInitialize)
    {
        bufferSTL.m_Buffer.assign(bufferSTL.m_Buffer.size(), '\0');
    }
    ProfilerStop("buffering");
}

BP4Base::ResizeResult BP4Base::ResizeBuffer(const size_t dataIn,
                                            const std::string hint)
{
    ProfilerStart("buffering");
    const size_t currentSize = m_Data.m_Buffer.size();
    const size_t requiredSize = dataIn + m_Data.m_Position;

    ResizeResult result = ResizeResult::Unchanged;

    if (dataIn > m_MaxBufferSize)
    {
        throw std::runtime_error(
            "ERROR: data size: " +
            std::to_string(static_cast<float>(dataIn) / (1024. * 1024.)) +
            " Mb is too large for adios2 bp MaxBufferSize=" +
            std::to_string(static_cast<float>(m_MaxBufferSize) /
                           (1024. * 1024.)) +
            "Mb, try increasing MaxBufferSize in call to IO SetParameters " +
            hint + "\n");
    }

    if (requiredSize <= currentSize)
    {
        // do nothing, unchanged is default
    }
    else if (requiredSize > m_MaxBufferSize)
    {
        if (currentSize < m_MaxBufferSize)
        {
            m_Data.Resize(m_MaxBufferSize, " when resizing buffer to " +
                                               std::to_string(m_MaxBufferSize) +
                                               "bytes, " + hint + "\n");
        }
        result = ResizeResult::Flush;
    }
    else // buffer must grow
    {
        if (currentSize < m_MaxBufferSize)
        {
            const size_t nextSize =
                std::min(m_MaxBufferSize,
                         helper::NextExponentialSize(requiredSize, currentSize,
                                                     m_GrowthFactor));
            m_Data.Resize(nextSize, " when resizing buffer to " +
                                        std::to_string(nextSize) + "bytes, " +
                                        hint);
            result = ResizeResult::Success;
        }
    }

    ProfilerStop("buffering");
    return result;
}

// PROTECTED
void BP4Base::InitOnOffParameter(const std::string value, bool &parameter,
                                 const std::string hint)
{
    std::string valueLC(value);
    std::transform(valueLC.begin(), valueLC.end(), valueLC.begin(), ::tolower);

    if (valueLC == "off")
    {
        parameter = false;
    }
    else if (valueLC == "on")
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

void BP4Base::InitParameterProfile(const std::string value)
{
    InitOnOffParameter(value, m_Profiler.IsActive, "valid: Profile On or Off");
}

void BP4Base::InitParameterProfileUnits(const std::string value)
{
    auto lf_EmplaceTimer = [&](const std::string process,
                               const TimeUnit timeUnit) {
        if (m_Profiler.Timers.count(process) == 1)
        {
            m_Profiler.Timers.erase(process);
        }
        m_Profiler.Timers.emplace(
            process, profiling::Timer(process, timeUnit, m_DebugMode));
    };

    TimeUnit timeUnit = helper::StringToTimeUnit(value, m_DebugMode);

    lf_EmplaceTimer("buffering", timeUnit);
    lf_EmplaceTimer("memcpy", timeUnit);
    lf_EmplaceTimer("minmax", timeUnit);
    lf_EmplaceTimer("meta_sort_merge", timeUnit);
    lf_EmplaceTimer("aggregation", timeUnit);
    lf_EmplaceTimer("mkdir", timeUnit);

    m_Profiler.Bytes.emplace("buffering", 0);
}

void BP4Base::InitParameterBufferGrowth(const std::string value)
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

void BP4Base::InitParameterInitBufferSize(const std::string value)
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
    const size_t factor = helper::BytesFactor(units, m_DebugMode);
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

    m_Data.Resize(bufferSize, "bufferSize " + std::to_string(bufferSize) +
                                  ", in call to Open");
}

void BP4Base::InitParameterMaxBufferSize(const std::string value)
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
    const size_t factor = helper::BytesFactor(units, m_DebugMode);

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

void BP4Base::InitParameterThreads(const std::string value)
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

void BP4Base::InitParameterAsyncThreads(const std::string value)
{
    int asyncThreads = -1;

    if (m_DebugMode)
    {
        bool success = true;
        std::string description;

        try
        {
            asyncThreads = std::stoi(value);
        }
        catch (std::exception &e)
        {
            success = false;
            description = std::string(e.what());
        }

        if (!success || asyncThreads < 0)
        {
            throw std::invalid_argument(
                "ERROR: value in AsyncThreads=value in IO SetParameters must "
                "be "
                "an integer >= 0 (default = 1, no async = 0) \nadditional "
                "description: " +
                description + "\n, in call to Open\n");
        }
    }
    else
    {
        asyncThreads = std::stoi(value);
    }

    m_AsyncThreads = static_cast<unsigned int>(asyncThreads);
}

void BP4Base::InitParameterStatsLevel(const std::string value)
{
    int level = -1;

    if (m_DebugMode)
    {
        bool success = true;
        std::string description;

        try
        {
            level = std::stoi(value);
        }
        catch (std::exception &e)
        {
            success = false;
            description = std::string(e.what());
        }

        if (!success || level < 0 || level > 1)
        {
            throw std::invalid_argument(
                "ERROR: value in StatsLevel=value in IO SetParameters must be "
                "an integer 0 or 1, \nadditional "
                "description: " +
                description + "\n, in call to Open\n");
        }
    }
    else
    {
        level = std::stoi(value);
    }

    m_StatsLevel = static_cast<unsigned int>(level);
}

void BP4Base::InitParameterCollectiveMetadata(const std::string value)
{
    InitOnOffParameter(value, m_CollectiveMetadata,
                       "valid: CollectiveMetadata On or Off");
}

void BP4Base::InitParameterFlushStepsCount(const std::string value)
{
    long long int flushStepsCount = -1;

    if (m_DebugMode)
    {
        bool success = true;
        std::string description;

        try
        {
            flushStepsCount = std::stoll(value);
        }
        catch (std::exception &e)
        {
            success = false;
            description = std::string(e.what());
        }

        if (!success || flushStepsCount < 1)
        {
            throw std::invalid_argument(
                "ERROR: value in FlushStepscount=value in IO SetParameters "
                "must be an integer >= 1 (default) \nadditional "
                "description: " +
                description + "\n, in call to Open\n");
        }
    }
    else
    {
        flushStepsCount = std::stoll(value);
    }

    m_FlushStepsCount = static_cast<size_t>(flushStepsCount);
}

void BP4Base::InitParameterNodeLocal(const std::string value)
{
    InitOnOffParameter(value, m_NodeLocal, "valid: node-local On or Off");
}

std::vector<uint8_t>
BP4Base::GetTransportIDs(const std::vector<std::string> &transportsTypes) const
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
        else if (method == "WAN_zmq")
        {
            id = METHOD_ZMQ;
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

size_t BP4Base::GetProcessGroupIndexSize(const std::string name,
                                         const std::string timeStepName,
                                         const size_t transportsSize) const
    noexcept
{
    // pgIndex + list of methods (transports)
    size_t pgSize =
        (name.length() + timeStepName.length() + 23) + (3 + transportsSize);

    return pgSize;
}

BP4Base::ProcessGroupIndex
BP4Base::ReadProcessGroupIndexHeader(const std::vector<char> &buffer,
                                     size_t &position,
                                     const bool isLittleEndian) const noexcept
{
    ProcessGroupIndex index;
    index.Length =
        helper::ReadValue<uint16_t>(buffer, position, isLittleEndian);
    index.Name = ReadBP4String(buffer, position, isLittleEndian);
    index.IsColumnMajor =
        helper::ReadValue<char>(buffer, position, isLittleEndian);
    index.ProcessID =
        helper::ReadValue<int32_t>(buffer, position, isLittleEndian);
    index.StepName = ReadBP4String(buffer, position, isLittleEndian);
    index.Step = helper::ReadValue<uint32_t>(buffer, position, isLittleEndian);
    index.Offset =
        helper::ReadValue<uint64_t>(buffer, position, isLittleEndian);
    return index;
}

BP4Base::ElementIndexHeader
BP4Base::ReadElementIndexHeader(const std::vector<char> &buffer,
                                size_t &position,
                                const bool isLittleEndian) const noexcept
{
    ElementIndexHeader header;
    header.Length =
        helper::ReadValue<uint32_t>(buffer, position, isLittleEndian);
    header.MemberID =
        helper::ReadValue<uint32_t>(buffer, position, isLittleEndian);
    header.GroupName = ReadBP4String(buffer, position, isLittleEndian);
    header.Name = ReadBP4String(buffer, position, isLittleEndian);
    header.Path = ReadBP4String(buffer, position, isLittleEndian);
    header.DataType =
        helper::ReadValue<int8_t>(buffer, position, isLittleEndian);
    header.CharacteristicsSetsCount =
        helper::ReadValue<uint64_t>(buffer, position, isLittleEndian);

    return header;
}

std::string BP4Base::ReadBP4String(const std::vector<char> &buffer,
                                   size_t &position,
                                   const bool isLittleEndian) const noexcept
{
    const size_t size = static_cast<size_t>(
        helper::ReadValue<uint16_t>(buffer, position, isLittleEndian));

    if (size == 0)
    {
        return std::string();
    }

    const std::string values(&buffer[position], size);
    position += size;
    return values;
}

void BP4Base::ProfilerStart(const std::string process) noexcept
{
    if (m_Profiler.IsActive)
    {
        m_Profiler.Timers.at(process).Resume();
    }
}

void BP4Base::ProfilerStop(const std::string process) noexcept
{
    if (m_Profiler.IsActive)
    {
        m_Profiler.Timers.at(process).Pause();
    }
}

BP4Base::TransformTypes
BP4Base::TransformTypeEnum(const std::string transformType) const noexcept
{
    TransformTypes transformEnum = transform_unknown;

    for (const auto &pair : m_TransformTypesToNames)
    {
        if (pair.second == transformType)
        {
            transformEnum = static_cast<TransformTypes>(pair.first);
            break;
        }
    }

    return transformEnum;
}

void BP4Base::InitParameterSubStreams(const std::string value)
{
    int subStreams = -1;

    if (m_DebugMode)
    {
        bool success = true;
        std::string description;

        try
        {
            subStreams = std::stoi(value);
        }
        catch (std::exception &e)
        {
            success = false;
            description = std::string(e.what());
        }

        if (!success)
        {
            throw std::invalid_argument(
                "ERROR: can't convert parameter value in substreams=value "
                " to an integer number, in call to Open\n");
        }

        if (subStreams < 1)
        {
            if (m_RankMPI == 0)
            {
                std::cout << "Warning substreams is less than 1, using "
                             "substreams=1, in call to Open\n";
            }
            subStreams = 1;
        }
        else if (subStreams > m_SizeMPI)
        {
            if (m_RankMPI == 0)
            {
                std::cout << "WARNING: substreams larger than MPI_Size, using "
                             "substreams=MPI_Size, in call to Open\n";
            }
            subStreams = m_SizeMPI;
        }
    }
    else
    {
        subStreams = std::stoi(value);
    }

    if (subStreams < m_SizeMPI)
    {
        m_Aggregator.Init(subStreams, m_MPIComm);
    }
}

void BP4Base::InitParameterOpenTimeoutSecs(const std::string value)
{
    bool success = true;
    std::string description;

    try
    {
        m_OpenTimeoutSecs = std::stof(value);
    }
    catch (std::exception &e)
    {
        success = false;
        description = std::string(e.what());
    }

    if (!success)
    {
        throw std::invalid_argument("ERROR: m_TimeoutOpenSecs value: "
                                    "could not convert number: " +
                                    description + "\n, in call to Open\n");
    }

    if (m_OpenTimeoutSecs < 0.0)
    {
        m_OpenTimeoutSecs = std::numeric_limits<float>::max() / 10000;
    }
}

float BP4Base::InitParameterFloat(const std::string value,
                                  const std::string parameterName)
{
    bool success = true;
    float retval = 0.0f;
    std::string description;

    try
    {
        retval = std::stof(value);
    }
    catch (std::exception &e)
    {
        success = false;
        description = std::string(e.what());
    }

    if (!success)
    {
        throw std::invalid_argument(
            "ERROR: Parameter " + parameterName + " value (" + value +
            ") could not be converted to a float: " + description +
            "\n, in call to Open\n");
    }
    return retval;
}

size_t BP4Base::InitParameterSizeT(const std::string value,
                                   const std::string parameterName)
{
    bool success = true;
    size_t retval = 0;
    std::string description;

    try
    {
        retval = std::stoull(value);
    }
    catch (std::exception &e)
    {
        success = false;
        description = std::string(e.what());
    }

    if (!success)
    {
        throw std::invalid_argument(
            "ERROR: Parameter " + parameterName + " value (" + value +
            ") could not be converted to a unsigned long long: " + description +
            "\n, in call to Open\n");
    }
    return retval;
}

std::shared_ptr<BPOperation>
BP4Base::SetBPOperation(const std::string type) const noexcept
{
    std::shared_ptr<BPOperation> bpOp;
    if (type == "sz")
    {
        bpOp = std::make_shared<BPSZ>();
    }
    else if (type == "zfp")
    {
        bpOp = std::make_shared<BPZFP>();
    }
    else if (type == "mgard")
    {
        bpOp = std::make_shared<BPMGARD>();
    }
    else if (type == "bzip2")
    {
        bpOp = std::make_shared<BPBZIP2>();
    }
    else if (type == "png")
    {
        bpOp = std::make_shared<BPPNG>();
    }
    else if (type == "blosc")
    {
        bpOp = std::make_shared<BPBlosc>();
    }

    return bpOp;
}

// PRIVATE
std::string BP4Base::GetBPSubStreamName(const std::string &name,
                                        const size_t rank,
                                        const bool hasSubFiles) const noexcept
{
    if (!hasSubFiles)
    {
        return name;
    }

    const std::string bpName = RemoveTrailingSlash(name);

    const size_t index =
        m_Aggregator.m_IsActive ? m_Aggregator.m_SubStreamIndex : rank;

    /* the name of a data file starts with "data." */
    const std::string bpRankName(bpName + PathSeparator + "data." +
                                 std::to_string(index));
    return bpRankName;
}

#define declare_template_instantiation(T)                                      \
    template BP4Base::Characteristics<T>                                       \
    BP4Base::ReadElementIndexCharacteristics(                                  \
        const std::vector<char> &, size_t &, const BP4Base::DataTypes,         \
        const bool, const bool) const;                                         \
                                                                               \
    template std::map<size_t, std::shared_ptr<BPOperation>>                    \
    BP4Base::SetBPOperations<T>(                                               \
        const std::vector<core::VariableBase::Operation> &) const;

ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace format
} // end namespace adios2
