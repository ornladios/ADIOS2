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

#include "adios2/ADIOSMPI.h"

namespace adios
{

Transport::Transport(const std::string type, const std::string library,
                     MPI_Comm mpiComm, const bool debugMode)
: m_Type(type), m_Library(library), m_MPIComm(mpiComm), m_DebugMode(debugMode)
{
    MPI_Comm_rank(m_MPIComm, &m_RankMPI);
    MPI_Comm_size(m_MPIComm, &m_SizeMPI);
}

void Transport::InitProfiler(const OpenMode openMode, const TimeUnit timeUnit)
{
    m_Profiler.Timers.emplace(std::make_pair(
        "open", profiling::Timer("open", TimeUnit::Microseconds, m_DebugMode)));

    if (openMode == OpenMode::Write)
    {
        m_Profiler.Timers.emplace(
            "write", profiling::Timer("write", timeUnit, m_DebugMode));

        m_Profiler.Bytes.emplace("write", 0);
    }
    else if (openMode == OpenMode::Append)
    {
        m_Profiler.Timers.emplace(
            "append", profiling::Timer("append", timeUnit, m_DebugMode));
        m_Profiler.Bytes.emplace("append", 0);
    }
    else if (openMode == OpenMode::Read)
    {
        m_Profiler.Timers.emplace(
            "read", profiling::Timer("read", timeUnit, m_DebugMode));
        m_Profiler.Bytes.emplace("read", 0);
    }

    m_Profiler.Timers.emplace(
        "close",
        profiling::Timer("close", TimeUnit::Microseconds, m_DebugMode));

    m_Profiler.IsActive = true;
}

void Transport::SetBuffer(char * /*buffer*/, size_t /*size*/)
{
    if (m_DebugMode)
    {
        std::invalid_argument("ERROR: " + m_Name + " transport type " + m_Type +
                              " using library " + m_Library +
                              " doesn't implement the SetBuffer function\n");
    }
}

} // end namespace adios
