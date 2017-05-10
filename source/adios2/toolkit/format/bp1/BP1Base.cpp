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
    bool useDefaultBufferSize = true;
    bool useDefaultProfileUnits = true;

    for (const auto &pair : parameters)
    {
        const std::string key(pair.first);
        const std::string value(pair.second);

        if (key == "profile")
        {
            InitParameterProfile(value);
        }
        else if (key == "profile_units")
        {
            InitParameterProfileUnits(value);
            useDefaultProfileUnits = false;
        }
        else if (key == "buffer_growth")
        {
            InitParameterBufferGrowth(value);
        }
        else if (key == "init_buffer_size")
        {
            InitParameterInitBufferSize(value);
            useDefaultBufferSize = false;
        }
        else if (key == "max_buffer_size")
        {
            InitParameterMaxBufferSize(value);
        }
        else if (key == "verbose")
        {
            InitParameterVerbose(value);
        }
    }

    // default timer for buffering
    if (m_Profiler.IsActive == true && useDefaultProfileUnits == true)
    {
        m_Profiler.Timers.emplace(
            "buffering",
            profiling::Timer("buffering", DefaultTimeUnitEnum, m_DebugMode));
    }

    if (useDefaultBufferSize == true)
    {
        m_HeapBuffer.ResizeData(DefaultBufferSize);
    }
}

std::vector<std::string>
BP1Base::GetBPBaseNames(const std::vector<std::string> &names) const noexcept
{
    std::vector<std::string> bpBaseNames;
    bpBaseNames.reserve(names.size());

    for (const auto &name : names)
    {
        bpBaseNames.push_back(GetBPBaseName(name));
    }
    return bpBaseNames;
}

std::string BP1Base::GetBPBaseName(const std::string &name) const noexcept
{
    return AddExtension(name, ".bp");
}

std::vector<std::string>
BP1Base::GetBPNames(const std::vector<std::string> &names) const noexcept
{
    std::vector<std::string> bpNames;
    bpNames.reserve(names.size());

    for (const auto &name : names)
    {
        bpNames.push_back(GetBPName(name));
    }
    return bpNames;
}

std::string BP1Base::GetBPName(const std::string &name) const noexcept
{
    const std::string baseName = AddExtension(name, ".bp");
    // opens a file transport under name.bp/name.bp.rank
    const std::string bpName(baseName + "/" + baseName + "." +
                             std::to_string(m_BP1Aggregator.m_RankMPI));
    return bpName;
}

// PROTECTED
void BP1Base::InitParameterProfile(const std::string value)
{
    if (value == "off")
    {
        m_Profiler.IsActive = false;
    }
    else if (value == "on")
    {
        m_Profiler.IsActive = true; // default
    }
    else
    {
        if (m_DebugMode == true)
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
    if (m_Profiler.IsActive == false)
    {
        m_Profiler.Timers.clear(); // remove default
        return;
    }

    TimeUnit timeUnit = StringToTimeUnit(value, m_DebugMode);

    if (m_Profiler.Timers.count("buffering") == 1)
    {
        m_Profiler.Timers.erase("buffering");
    }

    m_Profiler.Timers.emplace(
        "buffering", profiling::Timer("buffering", timeUnit, m_DebugMode));
}

void BP1Base::InitParameterBufferGrowth(const std::string value)
{
    if (m_DebugMode == true)
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

        if (success == false || m_GrowthFactor <= 1.f)
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
        "SetParameter, valid syntax: init_buffer_size=10Gb, "
        "init_buffer_size=1000Mb, init_buffer_size=16Kb (minimum default), "
        " in call to Open");

    if (m_DebugMode == true)
    {
        if (value.size() < 2)
        {
            throw std::invalid_argument(errorMessage);
        }
    }

    const std::string number(value.substr(0, value.size() - 2));
    const std::string units(value.substr(value.size() - 2));
    const size_t factor = BytesFactor(units, m_DebugMode);
    size_t bufferSize = DefaultBufferSize; // from ADIOSTypes.h

    if (m_DebugMode == true)
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

        if (success == false || bufferSize < 16 * 1024) // 16384b
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
        "SetParameter, valid syntax: max_buffer_size=10Gb, "
        "max_buffer_size=1000Mb, max_buffer_size=16Kb (minimum default), "
        " in call to Open");

    if (m_DebugMode == true)
    {
        if (value.size() < 2)
        {
            throw std::invalid_argument(errorMessage);
        }
    }

    const std::string number(value.substr(0, value.size() - 2));
    const std::string units(value.substr(value.size() - 2));
    const size_t factor = BytesFactor(units, m_DebugMode);

    if (m_DebugMode == true)
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

        if (success == false || m_MaxBufferSize < 16 * 1024) // 16384b
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
    if (m_DebugMode == true)
    {
        bool success = true;
        try
        {
            m_Verbosity = static_cast<unsigned int>(std::stoi(value));
        }
        catch (std::exception &e)
        {
            success = false;
        }

        if (success == false || m_Verbosity < 0 || m_Verbosity > 5)
        {
            throw std::invalid_argument(
                "ERROR: value in verbose=value in IO SetParameters must be "
                "an integer in the range [0,5], in call to Open\n");
        }
    }
    else
    {
        m_Verbosity = static_cast<unsigned int>(std::stoi(value));
    }
}

std::vector<uint8_t>
BP1Base::GetTransportIDs(const std::vector<std::string> &transportsTypes) const
    noexcept
{
    auto lf_GetTransportID = [](const std::string method) -> uint8_t {

        int id = METHOD_UNKNOWN;
        if (method == "NULL")
        {
            id = METHOD_NULL;
        }
        else if (method == "FileDescriptor")
        {
            id = METHOD_POSIX;
        }
        else if (method == "FileStream")
        {
            id = METHOD_FSTREAM;
        }
        else if (method == "FilePointer")
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

} // end namespace format
} // end namespace adios
