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

#include <utility>

namespace adios
{

Transport::Transport(const std::string type, MPI_Comm mpiComm,
                     const bool debugMode)
: m_Type(type), m_MPIComm(mpiComm), m_DebugMode(debugMode)
{
    MPI_Comm_rank(m_MPIComm, &m_RankMPI);
    MPI_Comm_size(m_MPIComm, &m_SizeMPI);
}

void Transport::SetBuffer(char * /*buffer*/, size_t /*size*/) {}

void Transport::Flush() {}

void Transport::Close() {}

void Transport::InitProfiler(const std::string accessMode,
                             const Support::Resolutions resolution)
{
    m_Profiler.Timers.emplace_back("open", Support::Resolutions::mus);

    if (accessMode == "w" || accessMode == "write")
        m_Profiler.Timers.emplace_back("write", resolution);

    else if (accessMode == "a" || accessMode == "append")
        m_Profiler.Timers.emplace_back("append", resolution);

    else if (accessMode == "r" || accessMode == "read")
        m_Profiler.Timers.emplace_back("read", resolution);

    m_Profiler.Timers.emplace_back("close", Support::Resolutions::mus);

    m_Profiler.TotalBytes.push_back(0);
    m_Profiler.IsActive = true;
}

} // end namespace adios
