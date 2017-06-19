/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP1Base.cpp
 *
 *  Created on: Feb 7, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "BP1Base.h"
#include "BP1Base.tcc"

#include "adios2/helper/adiosFunctions.h" //CreateDirectory, StringToTimeUnit

namespace adios
{
namespace format
{

BP1Base::BP1Base(MPI_Comm mpiComm, const bool debugMode)
: m_HeapBuffer(debugMode), m_BP1Aggregator(mpiComm, debugMode),
  m_DebugMode(debugMode)
{
}

void BP1Base::InitParameters(const Params &parameters)
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
        else if (key == "Verbose")
        {
            InitParameterVerbose(value);
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
        m_HeapBuffer.ResizeData(DefaultInitialBufferSize);
    }
}

std::vector<std::string>
BP1Base::GetBPBaseNames(const std::vector<std::string> &names) const noexcept
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
BP1Base::GetBPNames(const std::vector<std::string> &baseNames) const noexcept
{
    auto lf_GetBPName = [](const std::string &baseName,
                           const int rank) -> std::string {

        const std::string bpBaseName = AddExtension(baseName, ".bp");
        // name.bp.dir/name.bp.rank
        const std::string bpName(bpBaseName + ".dir/" + bpBaseName + "." +
                                 std::to_string(rank));
        return bpName;
    };

    std::vector<std::string> bpNames;
    bpNames.reserve(baseNames.size());

    for (const auto &baseName : baseNames)
    {
        bpNames.push_back(lf_GetBPName(baseName, m_BP1Aggregator.m_RankMPI));
    }
    return bpNames;
}

// PROTECTED
void BP1Base::InitParameterProfile(const std::string value)
{
    if (value == "off" || value == "Off")
    {
        m_Profiler.IsActive = false;
    }
    else if (value == "on" || value == "On")
    {
        m_Profiler.IsActive = true; // default
    }
    else
    {
        if (m_DebugMode)
        {
            throw std::invalid_argument("ERROR: IO SetParameters profile "
                                        "invalid value, valid: "
                                        "profile=on or "
                                        "profile=off, in call to Open\n");
        }
    }
}

void BP1Base::InitParameterProfileUnits(const std::string value)
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

void BP1Base::InitParameterBufferGrowth(const std::string value)
{
    if (m_DebugMode)
    {
        bool success = true;
        try
        {
            m_GrowthFactor = std::stof(value);
        }
        catch (std::exception &e)
        {
            success = false;
        }

        if (!success || m_GrowthFactor <= 1.f)
        {
            throw std::invalid_argument(
                "ERROR: IO SetParameter buffer_growth value "
                "can't be less or equal than 1 (default = 1.5), or couldn't "
                "convert number, in call to Open\n");
        }
    }
    else
    {
        m_GrowthFactor = std::stof(value);
    }
}

void BP1Base::InitParameterInitBufferSize(const std::string value)
{
    const std::string errorMessage(
        "ERROR: couldn't convert value of init_buffer_size IO "
        "SetParameter, valid syntax: InitialBufferSize=10Gb, "
        "InitialBufferSize=1000Mb, InitialBufferSize=16Kb (minimum default), "
        " in call to Open");

    if (m_DebugMode)
    {
        if (value.size() < 2)
        {
            throw std::invalid_argument(errorMessage);
        }
    }

    const std::string number(value.substr(0, value.size() - 2));
    const std::string units(value.substr(value.size() - 2));
    const size_t factor = BytesFactor(units, m_DebugMode);
    size_t bufferSize = DefaultInitialBufferSize; // from ADIOSTypes.h

    if (m_DebugMode)
    {
        bool success = true;
        try
        {
            bufferSize = static_cast<size_t>(std::stoul(number) * factor);
        }
        catch (std::exception &e)
        {
            success = false;
        }

        if (!success || bufferSize < DefaultInitialBufferSize) // 16384b
        {
            throw std::invalid_argument(errorMessage);
        }
    }
    else
    {
        bufferSize = static_cast<size_t>(std::stoul(number) * factor);
    }

    m_HeapBuffer.ResizeData(bufferSize);
}

void BP1Base::InitParameterMaxBufferSize(const std::string value)
{
    const std::string errorMessage(
        "ERROR: couldn't convert value of max_buffer_size IO "
        "SetParameter, valid syntax: MaxBufferSize=10Gb, "
        "MaxBufferSize=1000Mb, MaxBufferSize=16Kb (minimum default), "
        " in call to Open");

    if (m_DebugMode)
    {
        if (value.size() < 2)
        {
            throw std::invalid_argument(errorMessage);
        }
    }

    const std::string number(value.substr(0, value.size() - 2));
    const std::string units(value.substr(value.size() - 2));
    const size_t factor = BytesFactor(units, m_DebugMode);

    if (m_DebugMode)
    {
        bool success = true;
        try
        {
            m_MaxBufferSize = static_cast<size_t>(std::stoul(number) * factor);
        }
        catch (std::exception &e)
        {
            success = false;
        }

        if (!success || m_MaxBufferSize < 16 * 1024) // 16384b
        {
            throw std::invalid_argument(errorMessage);
        }
    }
    else
    {
        m_MaxBufferSize = static_cast<size_t>(std::stoul(number) * factor);
    }
}

void BP1Base::InitParameterVerbose(const std::string value)
{
    int verbosity = -1;

    if (m_DebugMode)
    {
        bool success = true;

        try
        {
            verbosity = std::stoi(value);
        }
        catch (std::exception &e)
        {
            success = false;
        }

        if (!success || verbosity < 0 || verbosity > 5)
        {
            throw std::invalid_argument(
                "ERROR: value in Verbose=value in IO SetParameters must be "
                "an integer in the range [0,5], in call to Open\n");
        }
    }
    else
    {
        verbosity = std::stoi(value);
    }

    m_Verbosity = static_cast<unsigned int>(verbosity);
}

std::vector<uint8_t>
BP1Base::GetTransportIDs(const std::vector<std::string> &transportsTypes) const
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

size_t BP1Base::GetProcessGroupIndexSize(const std::string name,
                                         const std::string timeStepName,
                                         const size_t transportsSize) const
    noexcept
{
    // pgIndex + list of methods (transports)
    size_t pgSize =
        (name.length() + timeStepName.length() + 23) + (3 + transportsSize);

    return pgSize;
}

#define declare_template_instantiation(T)                                      \
    template BP1Base::ResizeResult BP1Base::ResizeBuffer(                      \
        const Variable<T> &variable);

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace format
} // end namespace adios
