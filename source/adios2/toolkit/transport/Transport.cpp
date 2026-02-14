/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Transport.cpp
 *
 *  Created on: Dec 5, 2016
 *      Author: wfg
 */

#include "Transport.h"
#include "adios2/core/CoreTypes.h"
#include <adios2sys/SystemTools.hxx>
#include <algorithm> // max

#include "adios2/helper/adiosFunctions.h" //CreateDirectory

namespace
{
#if ADIOS2_ENABLE_DELAYED_WRITE
// Turn this feature on at configure time with:
//
//     -DADIOS2_ENABLE_DELAYED_WRITE:BOOL=ON
//
double GetWriteDelay(const std::string &fileName)
{
    double delayFraction = 0.0;

    try
    {
        std::string envVarName("ADIOS2_WRITE_DELAY_");
        envVarName += fileName;
        const char *delayEnvVal = std::getenv(envVarName.c_str());
        if (delayEnvVal)
        {
            // try to parse the value of the env variable value as a number
            delayFraction = std::stod({delayEnvVal});
        }
    }
    catch (std::exception const &ex)
    {
        std::cout << "Error when looking for delay environment variable" << ex.what() << std::endl;
    }

    return delayFraction;
}
#endif
};

namespace adios2
{

Transport::Transport(const std::string type, const std::string library, helper::Comm const &comm)
: m_Type(type), m_Library(library), m_Comm(comm), m_BaseOffset(0), m_BaseSize(0)
{
}

void Transport::WriteV(const core::iovec *iov, const int iovcnt, size_t start)
{
    if (iovcnt > 0)
    {
        Write(static_cast<const char *>(iov[0].iov_base), iov[0].iov_len, start);
        for (int c = 1; c < iovcnt; ++c)
        {
            Write(static_cast<const char *>(iov[c].iov_base), iov[c].iov_len);
        }
    }
    else if (start != MaxSizeT)
    {
        Seek(start);
    }
}

void Transport::InitProfiler(const Mode openMode, const TimeUnit timeUnit)
{
    m_Profiler.m_IsActive = true;

    m_Profiler.m_Timers.emplace(
        std::make_pair("open", profiling::Timer("open", TimeUnit::Microseconds)));

    if (openMode == Mode::Write)
    {
        m_Profiler.m_Timers.emplace("write", profiling::Timer("write", timeUnit));

        m_Profiler.m_Bytes.emplace("write", 0);
    }
    else if (openMode == Mode::Append)
    {
        /*
        m_Profiler.Timers.emplace(
            "append", profiling::Timer("append", timeUnit));
        m_Profiler.Bytes.emplace("append", 0);
        */
        m_Profiler.m_Timers.emplace("write", profiling::Timer("write", timeUnit));

        m_Profiler.m_Bytes.emplace("write", 0);

        m_Profiler.m_Timers.emplace("read", profiling::Timer("read", timeUnit));

        m_Profiler.m_Bytes.emplace("read", 0);
    }
    else if (openMode == Mode::Read)
    {
        m_Profiler.m_Timers.emplace("read", profiling::Timer("read", timeUnit));
        m_Profiler.m_Bytes.emplace("read", 0);
    }

    m_Profiler.m_Timers.emplace("close", profiling::Timer("close", TimeUnit::Microseconds));
}

void Transport::OpenChain(const std::string &name, const Mode openMode,
                          const helper::Comm &chainComm, const bool async, const bool directio)
{
    helper::Throw<std::invalid_argument>("Toolkit", "transport::Transport", "NotImplemented",
                                         "ERROR: " + m_Name + " transport type " + m_Type +
                                             " using library " + m_Library +
                                             " doesn't implement the OpenChain function\n");
}

void Transport::SetParameters(const Params &parameters) {}

void Transport::SetBuffer(char * /*buffer*/, size_t /*size*/)
{
    helper::Throw<std::invalid_argument>("Toolkit", "transport::Transport", "NotImplemented",
                                         "ERROR: " + m_Name + " transport type " + m_Type +
                                             " using library " + m_Library +
                                             " doesn't implement the SetBuffer function\n");
}

void Transport::Flush()
{
    helper::Throw<std::invalid_argument>("Toolkit", "transport::Transport", "NotImplemented",
                                         "ERROR: " + m_Name + " transport type " + m_Type +
                                             " using library " + m_Library +
                                             " doesn't implement the Flush function\n");
}

size_t Transport::GetSize() { return 0; }

void Transport::ProfilerWriteBytes(size_t bytes) noexcept
{
    if (m_Profiler.m_IsActive)
    {
        m_Profiler.m_Bytes.at("write") += bytes;
    }
}

void Transport::ProfilerStart(const std::string process) noexcept
{
    if (m_Profiler.m_IsActive)
    {
        m_Profiler.m_Timers.at(process).Resume();
    }
}

void Transport::ProfilerStop(const std::string process) noexcept
{
    if (m_Profiler.m_IsActive)
    {
#if ADIOS2_ENABLE_DELAYED_WRITE
        if (process == "write")
        {
            std::string fname = adios2sys::SystemTools::GetFilenameName(m_Name);
            std::replace(fname.begin(), fname.end(), '.', '_');
            double writeDelayFraction = GetWriteDelay(fname);
            m_Profiler.m_Timers.at(process).Pause(writeDelayFraction);
            return;
        }
#endif
        m_Profiler.m_Timers.at(process).Pause();
    }
}

void Transport::CheckName() const
{
    if (m_Name.empty())
    {
        helper::Throw<std::invalid_argument>("Toolkit", "transport::Transport", "CheckName",
                                             "name can't be empty for " + m_Library +
                                                 " transport ");
    }
}

} // end namespace adios2
