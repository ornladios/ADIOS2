/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * InSituMPIWriter.cpp
 * Class to exchange data using MPI between Writer and Reader
 *  partition of an application
 * It requires MPI
 *
 *  Created on: Dec 18, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#include "InSituMPIWriter.h"
#include "InSituMPIFunctions.h"
#include "InSituMPIWriter.tcc"

#include "adios2/helper/adiosFunctions.h"

#include <iostream>

namespace adios2
{

InSituMPIWriter::InSituMPIWriter(IO &io, const std::string &name,
                                 const Mode mode, MPI_Comm mpiComm)
: Engine("InSituMPIWriter", io, name, mode, mpiComm)
{
    m_EndMessage = " in call to InSituMPIWriter " + m_Name + " Open\n";
    Init();

    m_RankAllPeers = insitumpi::FindPeers(mpiComm, m_Name, true);
    MPI_Comm_rank(MPI_COMM_WORLD, &m_GlobalRank);
    MPI_Comm_size(MPI_COMM_WORLD, &m_GlobalNproc);
    MPI_Comm_rank(mpiComm, &m_WriterRank);
    MPI_Comm_size(mpiComm, &m_WriterNproc);
    m_RankDirectPeers =
        insitumpi::AssignPeers(m_WriterRank, m_WriterNproc, m_RankAllPeers);
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Writer " << m_WriterRank << " Open(" << m_Name
                  << "). Fixed schedule = " << (m_FixedSchedule ? "yes" : "no")
                  << ". #readers=" << m_RankAllPeers.size()
                  << " #writers=" << m_WriterNproc
                  << " #appsize=" << m_GlobalNproc
                  << " #direct peers=" << m_RankDirectPeers.size() << std::endl;
    }
    insitumpi::ConnectDirectPeers(true, m_GlobalRank, m_RankDirectPeers);
}

StepStatus InSituMPIWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Writer " << m_WriterRank << " BeginStep()\n";
    }
    if (mode != StepMode::Append)
    {
        throw std::invalid_argument(
            "ERROR: InSituMPI engine only supports appending steps "
            "(BeginStep(adios2::StepMode::Append)");
    }

    m_CurrentStep++; // 0 is the first step
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Writer " << m_WriterRank << " new step "
                  << m_CurrentStep << " for " << m_Name << ". Notify peers..."
                  << std::endl;
    }
    // Send the step to all reader peers, asynchronously
    MPI_Request request;
    for (auto peerRank : m_RankDirectPeers)
    {
        MPI_Isend(&m_CurrentStep, 1, MPI_INT, peerRank,
                  insitumpi::MpiTags::Step, MPI_COMM_WORLD, &request);
    }

    m_NCallsPerformPuts = 0;
    m_NDeferredPuts = 0;
    return StepStatus::OK;
}

void InSituMPIWriter::PerformPuts()
{
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Writer " << m_WriterRank << " PerformPuts()\n";
    }
    if (m_NCallsPerformPuts > 0)
    {
        throw std::runtime_error("ERROR: InSituMPI engine only allows for 1 "
                                 "PerformPuts() per step.");
    }
    m_NCallsPerformPuts++;
    m_NDeferredPuts = 0;
}

void InSituMPIWriter::EndStep()
{
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Writer " << m_WriterRank << " EndStep()\n";
    }
    if (m_NDeferredPuts == 0)
    {
        PerformPuts();
    }

    // TODO: Blocking wait for all data transfers to finish
}

void InSituMPIWriter::Close(const int transportIndex)
{
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Writer " << m_WriterRank << " Close(" << m_Name
                  << ")\n";
    }
    m_CurrentStep = -1; // -1 will indicate end of stream
    // Send -1 to all reader peers, asynchronously
    MPI_Request request;
    for (auto peerRank : m_RankDirectPeers)
    {
        MPI_Isend(&m_CurrentStep, 1, MPI_INT, peerRank,
                  insitumpi::MpiTags::Step, MPI_COMM_WORLD, &request);
    }
}

// PRIVATE

#define declare_type(T)                                                        \
    void InSituMPIWriter::DoPutSync(Variable<T> &variable, const T *values)    \
    {                                                                          \
        PutSyncCommon(variable, values);                                       \
    }                                                                          \
    void InSituMPIWriter::DoPutDeferred(Variable<T> &variable,                 \
                                        const T *values)                       \
    {                                                                          \
        PutDeferredCommon(variable, values);                                   \
    }                                                                          \
    void InSituMPIWriter::DoPutDeferred(Variable<T> &, const T &value) {}

ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

void InSituMPIWriter::Init()
{
    InitParameters();
    InitTransports();
}

void InSituMPIWriter::InitParameters()
{
    auto itFixedSchedule = m_IO.m_Parameters.find("FixedSchedule");
    if (itFixedSchedule == m_IO.m_Parameters.end())
    {
        m_FixedSchedule = false;
    }
    else if (itFixedSchedule->second == "true")
    {
        m_FixedSchedule = true;
    }

    auto itVerbosity = m_IO.m_Parameters.find("verbose");
    if (itVerbosity != m_IO.m_Parameters.end())
    {
        m_Verbosity = std::stoi(itVerbosity->second);
        if (m_DebugMode)
        {
            if (m_Verbosity < 0 || m_Verbosity > 5)
                throw std::invalid_argument(
                    "ERROR: Method verbose argument must be an "
                    "integer in the range [0,5], in call to "
                    "Open or Engine constructor\n");
        }
    }
}

void InSituMPIWriter::InitTransports()
{
    // Nothing to process from m_IO.m_TransportsParameters
}

} // end namespace adios2
