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

#include "adios2/helper/adiosMath.h"

#include "InSituMPIFunctions.h"
#include "InSituMPISchedules.h"
#include "InSituMPIWriter.h"
#include "InSituMPIWriter.tcc"

#include <iostream>

namespace adios2
{

InSituMPIWriter::InSituMPIWriter(IO &io, const std::string &name,
                                 const Mode mode, MPI_Comm mpiComm)
: Engine("InSituMPIWriter", io, name, mode, mpiComm),
  m_BP3Serializer(mpiComm, m_DebugMode)
{
    m_EndMessage = " in call to InSituMPIWriter " + m_Name + " Open\n";
    MPI_Comm_dup(MPI_COMM_WORLD, &m_CommWorld);
    Init();
    m_BP3Serializer.InitParameters(m_IO.m_Parameters);

    m_RankAllPeers = insitumpi::FindPeers(mpiComm, m_Name, true, m_CommWorld);
    MPI_Comm_rank(m_CommWorld, &m_GlobalRank);
    MPI_Comm_size(m_CommWorld, &m_GlobalNproc);
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
    insitumpi::ConnectDirectPeers(m_CommWorld, true,
                                  (m_BP3Serializer.m_RankMPI == 0),
                                  m_GlobalRank, m_RankDirectPeers);
}

InSituMPIWriter::~InSituMPIWriter() { MPI_Comm_free(&m_CommWorld); }

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
    // we don't care about keeping these requests
    MPI_Request request;
    for (auto peerRank : m_RankDirectPeers)
    {
        MPI_Isend(&m_CurrentStep, 1, MPI_INT, peerRank,
                  insitumpi::MpiTags::Step, m_CommWorld, &request);
    }

    m_NCallsPerformPuts = 0;
    m_BP3Serializer.m_DeferredVariables.clear();
    m_BP3Serializer.m_DeferredVariablesDataSize = 0;

    // start a fresh buffer with a new Process Group
    m_BP3Serializer.ResetBuffer(m_BP3Serializer.m_Data);
    if (!m_BP3Serializer.m_MetadataSet.DataPGIsOpen)
    {
        std::vector<std::string> empty;
        m_BP3Serializer.PutProcessGroupIndex(m_IO.m_HostLanguage, empty);
    }

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

    if (m_CurrentStep == 0 || !m_FixedSchedule)
    {
        // Create local metadata and send to reader peers
        // std::vector<char> mdVar = m_BP3Serializer.SerializeIndices(
        //    m_BP3Serializer.m_MetadataSet.VarsIndices);
        // Create Global metadata and send to readers
        m_BP3Serializer.SerializeData(m_IO, true); // advance timestep
        m_BP3Serializer.SerializeMetadataInData();
        m_BP3Serializer.AggregateCollectiveMetadata();

        // Send the metadata to all reader peers, asynchronously
        // we don't care about keeping these requests because
        // we will wait next for response from all readers
        if (m_BP3Serializer.m_RankMPI == 0)
        {
            if (m_Verbosity == 5)
            {
                std::cout << "InSituMPI Writer " << m_WriterRank
                          << " Metadata has = "
                          << m_BP3Serializer.m_MetadataSet.DataPGVarsCount
                          << " variables. size = "
                          << m_BP3Serializer.m_Metadata.m_Position << std::endl;
            }

            // FIXME: Which reader is actually listening for this request?
            if (m_Verbosity == 5)
            {
                std::cout << "InSituMPI Writer " << m_WriterRank
                          << " World rank = " << m_GlobalRank
                          << " sends metadata to Reader World rank = "
                          << m_RankDirectPeers[0] << std::endl;
            }
            MPI_Request request;
            // for (auto peerRank : m_RankDirectPeers)
            auto peerRank = m_RankDirectPeers[0];
            unsigned long mdLen = m_BP3Serializer.m_Metadata.m_Position;
            MPI_Isend(&mdLen, 1, MPI_UNSIGNED_LONG, peerRank,
                      insitumpi::MpiTags::MetadataLength, m_CommWorld,
                      &request);
            MPI_Isend(m_BP3Serializer.m_Metadata.m_Buffer.data(), mdLen,
                      MPI_CHAR, peerRank, insitumpi::MpiTags::Metadata,
                      m_CommWorld, &request);
        }

        // Collect the read requests from ALL readers
        // FIXME: How do we make this Irecv from all readers
        // std::vector<MPI_Request> requests(m_RankAllPeers.size());
        std::vector<MPI_Status> statuses(m_RankAllPeers.size());
        std::vector<std::vector<char>> serializedSchedules(
            m_RankAllPeers.size());
        for (int peerID = 0; peerID < m_RankAllPeers.size(); peerID++)
        {
            int mdLen;
            MPI_Recv(&mdLen, 1, MPI_INT, m_RankAllPeers[peerID],
                     insitumpi::MpiTags::ReadScheduleLength, m_CommWorld,
                     &statuses[peerID]);
            serializedSchedules[peerID].resize(mdLen);
            MPI_Recv(serializedSchedules[peerID].data(), mdLen, MPI_CHAR,
                     m_RankAllPeers[peerID], insitumpi::MpiTags::ReadSchedule,
                     m_CommWorld, &statuses[peerID]);
            if (m_Verbosity == 5)
            {
                std::cout << "InSituMPI Writer " << m_WriterRank
                          << " received read schedule from Reader  " << peerID
                          << " global rank " << m_RankAllPeers[peerID]
                          << " length = " << mdLen << std::endl;
            }
        }
        // MPI_Waitall(m_RankAllPeers.size(), requests.data(), statuses.data());

        // build (and remember for fixed schedule) the read request table
        // std::map<std::string, std::map<size_t, std::vector<SubFileInfo>>> map
        m_WriteScheduleMap.clear();
        m_WriteScheduleMap =
            insitumpi::DeserializeReadSchedule(serializedSchedules);
        if (m_Verbosity == 5)
        {
            std::cout << "InSituMPI Writer " << m_WriterRank << " schedule:  ";
            insitumpi::PrintReadScheduleMap(m_WriteScheduleMap);
            std::cout << std::endl;
        }

        // Make the send requests for each variable for each matching peer
        // request
        for (const auto &variableName : m_BP3Serializer.m_DeferredVariables)
        {
            // Create the async send for the variable
            AsyncSendVariable(variableName);
        }
    }

    m_BP3Serializer.m_DeferredVariables.clear();
    if (m_CurrentStep == 0 || !m_FixedSchedule)
    {
        m_BP3Serializer.ResetBuffer(m_BP3Serializer.m_Data, true);
        m_BP3Serializer.ResetBuffer(m_BP3Serializer.m_Metadata, true);
        // FIXME: Somehow m_MetadataSet should be clean up too
    }
}

void InSituMPIWriter::AsyncSendVariable(std::string variableName)
{
    const std::string type(m_IO.InquireVariableType(variableName));

    if (type == "compound")
    {
        // not supported
    }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::GetType<T>())                                     \
    {                                                                          \
        Variable<T> *variable = m_IO.InquireVariable<T>(variableName);         \
        if (m_DebugMode && variable == nullptr)                                \
        {                                                                      \
            throw std::invalid_argument(                                       \
                "ERROR: variable " + variableName +                            \
                " not found, in call to AsyncSendVariable\n");                 \
        }                                                                      \
        AsyncSendVariable<T>(*variable);                                       \
    }

    ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
}

void InSituMPIWriter::EndStep()
{
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Writer " << m_WriterRank << " EndStep()\n";
    }
    if (m_BP3Serializer.m_DeferredVariables.size() > 0)
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
                  insitumpi::MpiTags::Step, m_CommWorld, &request);
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
