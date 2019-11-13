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

#include "adios2/helper/adiosCommMPI.h"
#include "adios2/helper/adiosMath.h"
#include "adios2/toolkit/profiling/taustubs/tautimer.hpp"

#include "InSituMPIFunctions.h"
#include "InSituMPISchedules.h"
#include "InSituMPIWriter.h"
#include "InSituMPIWriter.tcc"

#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

InSituMPIWriter::InSituMPIWriter(IO &io, const std::string &name,
                                 const Mode mode, helper::Comm comm)
: Engine("InSituMPIWriter", io, name, mode, std::move(comm)),
  m_BP3Serializer(m_Comm, m_DebugMode)
{
    TAU_SCOPED_TIMER("InSituMPIWriter::Open");
    m_EndMessage = " in call to InSituMPIWriter " + m_Name + " Open\n";
    Init();
    m_BP3Serializer.Init(m_IO.m_Parameters, "in call to InSituMPI::Open write");

    m_RankAllPeers =
        insitumpi::FindPeers(CommAsMPI(m_Comm), m_Name, true, m_CommWorld);
    for (int i = 0; i < m_RankAllPeers.size(); i++)
    {
        m_RankToPeerID[m_RankAllPeers[i]] = i;
    }
    MPI_Comm_rank(m_CommWorld, &m_GlobalRank);
    MPI_Comm_size(m_CommWorld, &m_GlobalNproc);
    m_WriterRank = m_Comm.Rank();
    m_WriterNproc = m_Comm.Size();
    m_RankDirectPeers =
        insitumpi::AssignPeers(m_WriterRank, m_WriterNproc, m_RankAllPeers);
    int primaryContact = insitumpi::ConnectDirectPeers(
        m_CommWorld, true, (m_BP3Serializer.m_RankMPI == 0), m_GlobalRank,
        m_RankDirectPeers);
    m_AmIPrimaryContact = static_cast<bool>(primaryContact);
    if (m_RankAllPeers.empty())
    {
        throw(std::runtime_error(
            "No writers are found. Make sure that the writer and reader "
            "applications are launched as one application in MPMD mode."));
    }
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Writer " << m_WriterRank << " Open(" << m_Name
                  << "). #readers=" << m_RankAllPeers.size()
                  << " #writers=" << m_WriterNproc
                  << " #appsize=" << m_GlobalNproc
                  << " #direct_peers=" << m_RankDirectPeers.size()
                  << " primary_contact=" << (m_AmIPrimaryContact ? "yes" : "no")
                  << std::endl;
    }
}

InSituMPIWriter::~InSituMPIWriter() {}

StepStatus InSituMPIWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    TAU_SCOPED_TIMER("InSituMPIWriter::BeginStep");
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
    if (m_AmIPrimaryContact)
    {
        if (m_Verbosity == 5)
        {
            std::cout << "InSituMPI Writer " << m_WriterRank << " new step "
                      << m_CurrentStep << " for " << m_Name
                      << ". Notify peers..." << std::endl;
        }
        // Send the step to all reader peers, asynchronously
        // We need to call Wait on all Isend/Irecv calls at some point otherwise
        // MPI_Comm_free() will never release the communicator
        for (auto peerRank : m_RankDirectPeers)
        {
            m_MPIRequests.emplace_back();
            MPI_Isend(&m_CurrentStep, 1, MPI_INT, peerRank,
                      insitumpi::MpiTags::Step, m_CommWorld,
                      &m_MPIRequests.back());
        }
    }
    else
    {
        if (m_Verbosity == 5)
        {
            std::cout << "InSituMPI Writer " << m_WriterRank << " new step "
                      << m_CurrentStep << " for " << m_Name
                      << ". Notify nobody." << std::endl;
        }
    }

    m_NCallsPerformPuts = 0;
    m_BP3Serializer.m_DeferredVariables.clear();
    m_BP3Serializer.m_DeferredVariablesDataSize = 0;

    // start a fresh buffer with a new Process Group
    m_BP3Serializer.ResetBuffer(m_BP3Serializer.m_Data, true);
    m_BP3Serializer.ResetBuffer(m_BP3Serializer.m_Metadata, true);
    m_BP3Serializer.ResetIndices();
    if (!m_BP3Serializer.m_MetadataSet.DataPGIsOpen)
    {
        std::vector<std::string> empty;
        m_BP3Serializer.PutProcessGroupIndex(m_IO.m_Name, m_IO.m_HostLanguage,
                                             empty);
    }

    return StepStatus::OK;
}

void InSituMPIWriter::PerformPuts()
{
    TAU_SCOPED_TIMER("InSituMPIWriter::PerformPuts");
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

    if (m_RankDirectPeers.size() > 0)
    {

        if (m_CurrentStep == 0 || !m_WriterDefinitionsLocked)
        {
            // Create local metadata and send to reader peers
            // std::vector<char> mdVar = m_BP3Serializer.SerializeIndices(
            //    m_BP3Serializer.m_MetadataSet.VarsIndices);
            // Create Global metadata and send to readers
            m_BP3Serializer.SerializeData(m_IO, true); // advance timestep
            m_BP3Serializer.SerializeMetadataInData();
            m_BP3Serializer.AggregateCollectiveMetadata(
                m_Comm, m_BP3Serializer.m_Metadata, true);

            // store length long enough to survive Isend() completion
            // so don't move this into the next if branch
            unsigned long mdLen = m_BP3Serializer.m_Metadata.m_Position;

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
                              << m_BP3Serializer.m_Metadata.m_Position
                              << std::endl;
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
                int peerRank = m_RankDirectPeers[0];
                // send fix schedule info, then length of metadata array,
                // then metadata array

                MPI_Isend(&mdLen, 1, MPI_UNSIGNED_LONG, peerRank,
                          insitumpi::MpiTags::MetadataLength, m_CommWorld,
                          &request);
                MPI_Isend(m_BP3Serializer.m_Metadata.m_Buffer.data(), mdLen,
                          MPI_CHAR, peerRank, insitumpi::MpiTags::Metadata,
                          m_CommWorld, &request);
            }
        }

        // exchange flags about fixed schedule
        if (m_CurrentStep == 0)
        {
            int peerRank = m_RankDirectPeers[0];
            int fixed;

            if (m_BP3Serializer.m_RankMPI == 0)
            {
                // send flag about this sender's fixed schedule
                fixed = (int)m_WriterDefinitionsLocked;
                MPI_Send(&fixed, 1, MPI_INT, peerRank,
                         insitumpi::MpiTags::FixedRemoteSchedule, m_CommWorld);

                // recv flag about the receiver's fixed schedule
                MPI_Status status;
                MPI_Recv(&fixed, 1, MPI_INT, peerRank,
                         insitumpi::MpiTags::FixedRemoteSchedule, m_CommWorld,
                         &status);
            }
            // broadcast fixed schedule flag to every reader
            m_Comm.Bcast(&fixed, 1, 0);
            m_RemoteDefinitionsLocked = (fixed ? true : false);
            if (m_BP3Serializer.m_RankMPI == 0)
            {
                if (m_Verbosity == 5)
                {
                    std::cout << "InSituMPI Writer " << m_WriterRank
                              << " fixed Writer schedule = "
                              << m_WriterDefinitionsLocked
                              << " fixed Reader schedule = "
                              << m_RemoteDefinitionsLocked << std::endl;
                }
            }
        }

        // Receive read schedule from writers
        if (m_CurrentStep == 0 || !m_RemoteDefinitionsLocked)
        {
            ReceiveReadSchedule(m_WriteScheduleMap);

            if (m_Verbosity == 5)
            {
                std::cout << "InSituMPI Writer " << m_WriterRank
                          << " schedule:  ";
                insitumpi::PrintReadScheduleMap(m_WriteScheduleMap);
                std::cout << std::endl;
            }

            const int nRequests =
                insitumpi::GetNumberOfRequestsInWriteScheduleMap(
                    m_WriteScheduleMap);
            m_MPIRequests.reserve(m_MPIRequests.size() + nRequests);
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
    if (!m_RemoteDefinitionsLocked)
    {
        m_BP3Serializer.ResetBuffer(m_BP3Serializer.m_Data, true);
        m_BP3Serializer.ResetBuffer(m_BP3Serializer.m_Metadata, true);
        // FIXME: Somehow m_MetadataSet should be clean up too
    }
}

void InSituMPIWriter::AsyncSendVariable(std::string variableName)
{
    TAU_SCOPED_TIMER("InSituMPIWriter::AsyncSendVariable");
    const std::string type(m_IO.InquireVariableType(variableName));

    if (type == "compound")
    {
        // not supported
    }
#define declare_template_instantiation(T)                                      \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        Variable<T> *variable = m_IO.InquireVariable<T>(variableName);         \
        if (m_DebugMode && variable == nullptr)                                \
        {                                                                      \
            throw std::invalid_argument(                                       \
                "ERROR: variable " + variableName +                            \
                " not found, in call to AsyncSendVariable\n");                 \
        }                                                                      \
                                                                               \
        for (const auto &blockInfo : variable->m_BlocksInfo)                   \
        {                                                                      \
            AsyncSendVariable<T>(*variable, blockInfo);                        \
        }                                                                      \
        variable->m_BlocksInfo.clear();                                        \
    }

    ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
}

void InSituMPIWriter::EndStep()
{
    TAU_SCOPED_TIMER("InSituMPIWriter::EndStep");
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Writer " << m_WriterRank << " EndStep()\n";
    }
    if (m_BP3Serializer.m_DeferredVariables.size() > 0)
    {
        PerformPuts();
    }

    TAU_START("InSituMPIWriter::CompleteRequests");
    insitumpi::CompleteRequests(m_MPIRequests, true, m_WriterRank);
    m_MPIRequests.clear();
    TAU_STOP("InSituMPIWriter::CompleteRequests");

    TAU_START("WaitForReaderAck");
    // Wait for final acknowledgment from the readers
    int dummy = 0;
    if (m_BP3Serializer.m_RankMPI == 0 && m_RankDirectPeers.size() > 0)
    {
        MPI_Status status;
        MPI_Recv(&dummy, 1, MPI_INT, m_RankDirectPeers[0],
                 insitumpi::MpiTags::ReadCompleted, m_CommWorld, &status);
    }
    m_Comm.Bcast(&dummy, 1, 0);
    TAU_STOP("WaitForReaderAck");

    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Writer " << m_WriterRank
                  << " completed EndStep()\n";
    }
}

// PRIVATE

#define declare_type(T)                                                        \
    void InSituMPIWriter::DoPutSync(Variable<T> &variable, const T *values)    \
    {                                                                          \
        TAU_SCOPED_TIMER("InSituMPIWriter::Put");                              \
        PutSyncCommon(variable, variable.SetBlockInfo(values, m_CurrentStep)); \
        variable.m_BlocksInfo.clear();                                         \
    }                                                                          \
    void InSituMPIWriter::DoPutDeferred(Variable<T> &variable,                 \
                                        const T *values)                       \
    {                                                                          \
        TAU_SCOPED_TIMER("InSituMPIWriter::Put");                              \
        PutDeferredCommon(variable, values);                                   \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void InSituMPIWriter::Init()
{
    InitParameters();
    InitTransports();
}

void InSituMPIWriter::InitParameters()
{
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

void InSituMPIWriter::DoClose(const int transportIndex)
{
    TAU_SCOPED_TIMER("InSituMPIWriter::Close");
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Writer " << m_WriterRank << " Close(" << m_Name
                  << ")\n";
    }
    if (m_AmIPrimaryContact)
    {
        m_CurrentStep = -1; // -1 will indicate end of stream
        // Send -1 to all reader peers, asynchronously
        // We need to call Wait on all Isend/Irecv calls at some point otherwise
        // MPI_Comm_free() will never release the communicator
        for (auto peerRank : m_RankDirectPeers)
        {
            m_MPIRequests.emplace_back();
            MPI_Isend(&m_CurrentStep, 1, MPI_INT, peerRank,
                      insitumpi::MpiTags::Step, m_CommWorld,
                      &m_MPIRequests.back());
        }
        // The completion of the -1 step message is waited upon here
        // because the destructor may be called after MPI_Finalize()
        // so this is the last safe point to do this.
        if (m_Verbosity == 5)
        {
            std::cout << "InSituMPI Writer " << m_WriterRank
                      << " needs to wait on " << m_MPIRequests.size()
                      << " outstanding MPI async message request..."
                      << std::endl;
        }
        insitumpi::CompleteRequests(m_MPIRequests, true, m_WriterRank);
        m_MPIRequests.clear();
    }
}

void InSituMPIWriter::ReceiveReadSchedule(
    insitumpi::WriteScheduleMap &writeScheduleMap)
{
    TAU_SCOPED_TIMER("InSituMPIWriter::ReceiveReadSchedule");
    // Reader ID -> length of serialized read schedule
    std::map<int, int> rsLengths;
    // Reader ID -> serialized read schedule
    std::map<int, std::vector<char>> serializedSchedules;
    // Writer ID -> number of peer readers
    std::vector<int> nReaderPerWriter(m_WriterNproc);
    // Number of peer readers for this writer
    int nPeerReaders;

    // Writer root receives nReaderPerWriter from reader root
    if (m_WriterRank == 0)
    {
        MPI_Recv(nReaderPerWriter.data(), nReaderPerWriter.size(), MPI_INT,
                 m_RankDirectPeers[0], insitumpi::MpiTags::NumReaderPerWriter,
                 m_CommWorld, MPI_STATUS_IGNORE);
    }

    // Each writer receives the number of its peer readers
    m_Comm.Scatter(nReaderPerWriter.data(), 1, &nPeerReaders, 1, 0);

    std::vector<MPI_Request> requests(nPeerReaders);
    // Reader global rank -> length of serialized read schedule
    std::vector<int> rsLengthsTmp(nPeerReaders);

    // Receive the size of each read schedule from readers
    // We use MPI_ANY_SOURCE here because at this point
    // we don't know who our peer readers are
    for (auto i = 0; i < nPeerReaders; i++)
    {
        MPI_Irecv(&rsLengthsTmp[i], 1, MPI_INT, MPI_ANY_SOURCE,
                  insitumpi::MpiTags::ReadScheduleLength, m_CommWorld,
                  &requests[i]);
    }
    std::vector<MPI_Status> statuses =
        insitumpi::CompleteRequests(requests, true, m_WriterRank);

    // Now we can figure out who our peer readers are
    for (auto i = 0; i < nPeerReaders; i++)
    {
        const auto peerID = m_RankToPeerID[statuses[i].MPI_SOURCE];
        rsLengths[peerID] = rsLengthsTmp[i];
    }

    // Fianlly, we receive the read schedules from our peer readers
    auto i = 0;
    for (const auto &rsLenPair : rsLengths)
    {
        const auto peerID = rsLenPair.first;
        const auto rsLen = rsLenPair.second;

        serializedSchedules[peerID].resize(rsLen);
        MPI_Irecv(serializedSchedules[peerID].data(), rsLen, MPI_CHAR,
                  m_RankAllPeers[peerID], insitumpi::MpiTags::ReadSchedule,
                  m_CommWorld, &requests[i]);

        if (m_Verbosity == 5)
        {
            std::cout << "InSituMPI Writer " << m_WriterRank
                      << " receiving read schedule from Reader  " << peerID
                      << " global rank " << m_RankAllPeers[peerID]
                      << " length = " << rsLengths[peerID] << std::endl;
        }

        i++;
    }
    insitumpi::CompleteRequests(requests, true, m_WriterRank);

    // Build (and remember for fixed schedule) the read request table
    writeScheduleMap.clear();
    writeScheduleMap = insitumpi::DeserializeReadSchedule(serializedSchedules);
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
