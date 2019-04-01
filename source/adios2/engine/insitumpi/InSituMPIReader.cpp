/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * InSituMPIReader.cpp
 *
 *  Created on: Dec 18, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#include "InSituMPIReader.h"
#include "InSituMPIFunctions.h"
#include "InSituMPISchedules.h"

#include "InSituMPIReader.tcc"

#include "adios2/helper/adiosFunctions.h" // CSVToVector

#include <chrono>
#include <iostream>
#include <thread> // sleep_for

namespace adios2
{
namespace core
{
namespace engine
{

InSituMPIReader::InSituMPIReader(IO &io, const std::string &name,
                                 const Mode mode, MPI_Comm mpiComm)
: Engine("InSituMPIReader", io, name, mode, mpiComm),
  m_BP3Deserializer(mpiComm, m_DebugMode)
{
    m_EndMessage = " in call to IO Open InSituMPIReader " + m_Name + "\n";
    Init();

    m_RankAllPeers = insitumpi::FindPeers(mpiComm, m_Name, false, m_CommWorld);
    MPI_Comm_rank(m_CommWorld, &m_GlobalRank);
    MPI_Comm_size(m_CommWorld, &m_GlobalNproc);
    MPI_Comm_rank(mpiComm, &m_ReaderRank);
    MPI_Comm_size(mpiComm, &m_ReaderNproc);
    m_RankDirectPeers =
        insitumpi::AssignPeers(m_ReaderRank, m_ReaderNproc, m_RankAllPeers);
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Reader " << m_ReaderRank << " Open(" << m_Name
                  << "). Fixed Read schedule = "
                  << (m_IO.m_DefinitionsLocked ? "yes" : "no")
                  << ". #readers=" << m_ReaderNproc
                  << " #writers=" << m_RankAllPeers.size()
                  << " #appsize=" << m_GlobalNproc
                  << " #direct_peers=" << m_RankDirectPeers.size() << std::endl;
    }

    m_WriteRootGlobalRank = insitumpi::ConnectDirectPeers(
        m_CommWorld, false, false, m_GlobalRank, m_RankDirectPeers);
    if (m_WriteRootGlobalRank > -1)
    {
        m_ReaderRootRank = m_ReaderRank;
        if (m_Verbosity == 5)
        {
            std::cout << "InSituMPI Reader " << m_ReaderRank
                      << " is connected to writer root, World rank = "
                      << m_WriteRootGlobalRank << std::endl;
        }
    }
    else
    {
        m_ReaderRootRank = -1;
    }

    ClearMetadataBuffer();

    // figure out who is the Reader Root
    std::vector<int> v(m_ReaderNproc);
    MPI_Allgather(&m_ReaderRootRank, 1, MPI_INT, v.data(), 1, MPI_INT,
                  m_MPIComm);
    for (int i = 0; i < m_ReaderNproc; i++)
    {
        if (v[i] != -1)
        {
            m_ReaderRootRank = i;
            break;
        }
    }

    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Reader " << m_ReaderRank
                  << "  figured that the Reader root is Reader "
                  << m_ReaderRootRank << std::endl;
    }
}

InSituMPIReader::~InSituMPIReader()
{
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Reader " << m_ReaderRank << " Deconstructor on "
                  << m_Name << "\n";
    }
}

void InSituMPIReader::ClearMetadataBuffer()
{
    // m_BP3Deserializer.m_Metadata
}

StepStatus InSituMPIReader::BeginStep(const StepMode mode,
                                      const float timeoutSeconds)
{
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Reader " << m_ReaderRank << " BeginStep()\n";
    }

    if (m_RankDirectPeers.size() == 0)
    {
        return StepStatus::EndOfStream;
    }

    // Wait for the Step message from primary Writer
    MPI_Status status;
    if (timeoutSeconds < 0.0)
    {
        /* No timeout: do independent, blocking wait for step message from
         * primary writer */
        int step;
        MPI_Recv(&step, 1, MPI_INT, m_RankDirectPeers[0],
                 insitumpi::MpiTags::Step, m_CommWorld, &status);

        if (m_Verbosity == 5)
        {
            std::cout << "InSituMPI Reader " << m_ReaderRank << " new step "
                      << step << " arrived for " << m_Name << std::endl;
        }
        m_CurrentStep = step;
        // FIXME: missing test whether all writers sent the same step
    }
    else
    {
        /* Have timeout: do a collective wait for a step within timeout.
           Make sure every writer comes to the same conclusion */
        int haveStepMsg = 0;
        uint64_t nanoTO = timeoutSeconds * 1000000000.0;
        if (nanoTO < 1)
        {
            nanoTO = 1; // avoid 0
        }
        uint64_t pollTime = nanoTO / 1000; // TO/100 seconds polling time
        if (pollTime < 1)
        {
            pollTime = 1; // min 1 nanosecond polling time
        }
        if (pollTime > 1000000000)
        {
            pollTime = 1000000000; // max 1 seconds polling time
        }
        if (m_Verbosity == 5 && !m_ReaderRank)
        {
            std::cout << "InSituMPI Reader Polling for " << nanoTO
                      << " nanosec with sleep time of " << pollTime
                      << " nanosec" << std::endl;
        }
        /* Poll */
        double waited = 0.0;
        double startTime, endTime;
        while (waited < timeoutSeconds)
        {
            startTime = MPI_Wtime();
            MPI_Iprobe(m_RankDirectPeers[0], insitumpi::MpiTags::Step,
                       m_CommWorld, &haveStepMsg, &status);
            if (haveStepMsg)
                break;
            std::this_thread::sleep_for(std::chrono::nanoseconds(pollTime));
            endTime = MPI_Wtime();
            waited += endTime - startTime;
        }
        /* Get step msg if available */
        const int NOT_A_STEP = -2; // must be less than any valid step
        int step = NOT_A_STEP;
        if (haveStepMsg)
        {

            MPI_Recv(&step, 1, MPI_INT, m_RankDirectPeers[0],
                     insitumpi::MpiTags::Step, m_CommWorld, &status);
        }
        /* Exchange steps */
        int maxstep;
        MPI_Allreduce(&step, &maxstep, 1, MPI_INT, MPI_MAX, m_MPIComm);

        if (m_Verbosity == 5 && !m_ReaderRank)
        {
            std::cout << "InSituMPI Reader Polling result is " << maxstep
                      << std::endl;
        }

        /* Mutually agreed result */
        if (maxstep != NOT_A_STEP)
        {
            /* Receive my msg now if there was a message on other process */
            if (step == NOT_A_STEP)
            {
                MPI_Recv(&step, 1, MPI_INT, m_RankDirectPeers[0],
                         insitumpi::MpiTags::Step, m_CommWorld, &status);
            }
            m_CurrentStep = step;
        }
        else
        {
            return StepStatus::NotReady;
        }
    }

    if (m_CurrentStep == -1)
    {
        return StepStatus::EndOfStream;
    }

    m_NCallsPerformGets = 0;

    // Sync. recv. the global metadata
    if (!m_RemoteDefinitionsLocked)
    {
        unsigned long mdLen = 0;

        if (m_ReaderRootRank == m_ReaderRank)
        {
            MPI_Recv(&mdLen, 1, MPI_UNSIGNED_LONG, m_WriteRootGlobalRank,
                     insitumpi::MpiTags::MetadataLength, m_CommWorld, &status);
            if (m_Verbosity == 5)
            {
                std::cout << "InSituMPI Reader " << m_ReaderRank
                          << " receiving metadata size = " << mdLen
                          << " from writer world rank " << m_WriteRootGlobalRank
                          << std::endl;
            }
            m_BP3Deserializer.m_Metadata.m_Buffer.resize(mdLen);
            MPI_Recv(m_BP3Deserializer.m_Metadata.m_Buffer.data(), mdLen,
                     MPI_CHAR, m_WriteRootGlobalRank,
                     insitumpi::MpiTags::Metadata, m_CommWorld, &status);
        }

        // broadcast metadata to every reader
        MPI_Bcast(&mdLen, 1, MPI_UNSIGNED_LONG, m_ReaderRootRank, m_MPIComm);
        m_BP3Deserializer.m_Metadata.m_Buffer.resize(mdLen);
        MPI_Bcast(m_BP3Deserializer.m_Metadata.m_Buffer.data(), mdLen, MPI_CHAR,
                  m_ReaderRootRank, m_MPIComm);

        // Parse metadata into Variables and Attributes maps
        m_IO.RemoveAllVariables();
        m_BP3Deserializer.ParseMetadata(m_BP3Deserializer.m_Metadata, *this);

        if (m_Verbosity == 5)
        {
            std::cout << "InSituMPI Reader " << m_ReaderRank << " found "
                      << m_IO.GetVariablesDataMap().size() << " variables and "
                      << m_IO.GetAttributesDataMap().size()
                      << " attributes in metadata. Is source row major = "
                      << m_BP3Deserializer.m_IsRowMajor << std::endl;
        }
    }

    // Recv the flag about fixed schedule on the sender side
    if (m_CurrentStep == 0)
    {
        int fixed = (m_RemoteDefinitionsLocked ? 1 : 0);
        if (m_ReaderRootRank == m_ReaderRank)
        {
            MPI_Status status;
            MPI_Recv(&fixed, 1, MPI_INT, m_WriteRootGlobalRank,
                     insitumpi::MpiTags::FixedRemoteSchedule, m_CommWorld,
                     &status);
        }

        // broadcast fixed schedule flag to every reader
        MPI_Bcast(&fixed, 1, MPI_INT, m_ReaderRootRank, m_MPIComm);
        m_RemoteDefinitionsLocked = (fixed ? true : false);
        if (m_ReaderRootRank == m_ReaderRank)
        {
            if (m_Verbosity == 5)
            {
                std::cout << "InSituMPI Reader " << m_ReaderRank
                          << " fixed Writer schedule = "
                          << m_RemoteDefinitionsLocked
                          << " fixed Reader schedule = "
                          << m_IO.m_DefinitionsLocked << std::endl;
            }
        }
    }

    // Force PerformGets() even if there will be nothing to actually read
    m_BP3Deserializer.m_PerformedGets = false;
    return StepStatus::OK;
}

void InSituMPIReader::PerformGets()
{
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Reader " << m_ReaderRank << " PerformGets()\n";
    }
    if (m_NCallsPerformGets > 0)
    {
        throw std::runtime_error("ERROR: InSituMPI engine only allows for 1 "
                                 "PerformGets() per step.");
    }
    m_NCallsPerformGets++;

    // send flag about this receiver's fixed schedule
    if (m_CurrentStep == 0)
    {
        if (m_ReaderRootRank == m_ReaderRank)
        {
            int fixed = (int)m_IO.m_DefinitionsLocked;
            MPI_Send(&fixed, 1, MPI_INT, m_WriteRootGlobalRank,
                     insitumpi::MpiTags::FixedRemoteSchedule, m_CommWorld);
        }
    }

    // Create read schedule per writer
    // const std::map<std::string, SubFileInfoMap> variablesSubFileInfo =
    if (m_CurrentStep == 0 || !m_IO.m_DefinitionsLocked)
    {
        m_ReadScheduleMap.clear();
        m_ReadScheduleMap =
            m_BP3Deserializer.PerformGetsVariablesSubFileInfo(m_IO);
    }
    // bool reader_IsRowMajor = IsRowMajor(m_IO.m_HostLanguage);
    // bool writer_IsRowMajor = m_BP3Deserializer.m_IsRowMajor;
    // recalculate seek offsets to payload offset 0 (beginning of blocks)
    int nRequests = insitumpi::FixSeeksToZeroOffset(
        m_ReadScheduleMap, helper::IsRowMajor(m_IO.m_HostLanguage));

    if (m_CurrentStep == 0 || !m_IO.m_DefinitionsLocked)
    {
        // Send schedule to writers
        SendReadSchedule(m_ReadScheduleMap);
    }

    if (m_CurrentStep == 0 || !m_IO.m_DefinitionsLocked ||
        !m_RemoteDefinitionsLocked)
    {
        // Allocate the MPI_Request and OngoingReceives vectors
        m_MPIRequests.reserve(nRequests);
        m_OngoingReceives.reserve(nRequests);

        // Make the receive requests for each variable
        AsyncRecvAllVariables();
    }

    ProcessReceives();

    m_BP3Deserializer.m_PerformedGets = true;
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Reader " << m_ReaderRank
                  << " completed PerformGets()\n";
    }
}

size_t InSituMPIReader::CurrentStep() const { return m_CurrentStep; }

int InSituMPIReader::Statistics(uint64_t bytesInPlace, uint64_t bytesCopied)
{
    if (bytesInPlace == 0)
        return 0;
    return ((bytesInPlace + bytesCopied) * 100) / bytesInPlace;
}

void InSituMPIReader::EndStep()
{
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Reader " << m_ReaderRank
                  << " EndStep(): received "
                  << Statistics(m_BytesReceivedInPlace,
                                m_BytesReceivedInTemporary)
                  << "% of data in place (zero-copy)" << std::endl;
    }
    if (!m_BP3Deserializer.m_PerformedGets)
    {
        PerformGets();
    }
    ClearMetadataBuffer();

    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Reader " << m_ReaderRank
                  << " completed EndStep()\n";
    }
}

// PRIVATE

void InSituMPIReader::SendReadSchedule(
    const std::map<std::string, helper::SubFileInfoMap> &variablesSubFileInfo)
{
    const bool profile = m_BP3Deserializer.m_Profiler.IsActive;

    // Serialized schedules, one per-writer
    std::map<int, std::vector<char>> serializedSchedules =
        insitumpi::SerializeLocalReadSchedule(m_RankAllPeers.size(),
                                              variablesSubFileInfo);

    // Writer ID -> number of peer readers
    std::vector<int> nReaderPerWriter(m_RankAllPeers.size());

    // Fill nReaderPerWriter for this reader
    for (const auto &schedulePair : serializedSchedules)
    {
        const auto peerID = schedulePair.first;
        nReaderPerWriter[peerID] = 1;
    }

    // Accumulate nReaderPerWriter for all readers
    void *sendBuf = nReaderPerWriter.data();
    if (m_ReaderRootRank == m_ReaderRank)
    {
        sendBuf = MPI_IN_PLACE;
    }
    MPI_Reduce(sendBuf, nReaderPerWriter.data(), nReaderPerWriter.size(),
               MPI_INT, MPI_SUM, m_ReaderRootRank, m_MPIComm);

    // Reader root sends nReaderPerWriter to writer root
    if (m_ReaderRootRank == m_ReaderRank)
    {
        MPI_Send(nReaderPerWriter.data(), nReaderPerWriter.size(), MPI_INT,
                 m_WriteRootGlobalRank, insitumpi::MpiTags::NumReaderPerWriter,
                 m_CommWorld);
    }

    // *2 because we need two requests per writer (one for sending the length
    // of the read schedule and another for the actual read schedule)
    std::vector<MPI_Request> request(serializedSchedules.size() * 2);
    std::vector<int> rsLengths(serializedSchedules.size());
    auto i = 0;

    for (const auto &schedulePair : serializedSchedules)
    {
        const auto peerID = schedulePair.first;
        const auto &schedule = schedulePair.second;
        rsLengths[i] = schedule.size();

        if (m_Verbosity == 5)
        {
            std::cout << "InSituMPI Reader " << m_ReaderRank
                      << " Send Read Schedule len = " << rsLengths[i]
                      << " to Writer " << peerID << " global rank "
                      << m_RankAllPeers[peerID] << std::endl;
        }
        MPI_Isend(&rsLengths[i], 1, MPI_INT, m_RankAllPeers[peerID],
                  insitumpi::MpiTags::ReadScheduleLength, m_CommWorld,
                  &request[i * 2]);
        MPI_Isend(schedule.data(), rsLengths[i], MPI_CHAR,
                  m_RankAllPeers[peerID], insitumpi::MpiTags::ReadSchedule,
                  m_CommWorld, &request[i * 2 + 1]);

        i++;
    }
    insitumpi::CompleteRequests(request, false, m_ReaderRank);
}

void InSituMPIReader::AsyncRecvAllVariables()
{
    // <variable, <writer, <steps, <SubFileInfo>>>>
    for (const auto &variablePair : m_ReadScheduleMap)
    {
        // AsyncRecvVariable(variablePair.first, variablePair.second);
        const std::string type(m_IO.InquireVariableType(variablePair.first));

        if (type == "compound")
        {
            // not supported
        }
#define declare_template_instantiation(T)                                      \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        core::Variable<T> *variable =                                          \
            m_IO.InquireVariable<T>(variablePair.first);                       \
        if (m_DebugMode && variable == nullptr)                                \
        {                                                                      \
            throw std::invalid_argument(                                       \
                "ERROR: variable " + variablePair.first +                      \
                " not found, in call to AsyncSendVariable\n");                 \
        }                                                                      \
        AsyncRecvVariable<T>(*variable, variablePair.second);                  \
    }

        ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
    }
}

void InSituMPIReader::ProcessReceives()
{
    const int nRequests = m_OngoingReceives.size();

    insitumpi::CompleteRequests(m_MPIRequests, false, m_ReaderRank);

    // Send final acknowledgment to the Writer
    int dummy = 1;
    MPI_Bcast(&dummy, 1, MPI_INT, m_ReaderRootRank, m_MPIComm);
    if (m_ReaderRootRank == m_ReaderRank)
    {
        MPI_Send(&dummy, 1, MPI_INT, m_WriteRootGlobalRank,
                 insitumpi::MpiTags::ReadCompleted, m_CommWorld);
    }

    // Deserialize received messages
    for (int i = 0; i < nRequests; i++)
    {
        if (m_OngoingReceives[i].inPlaceDataArray == nullptr)
        {
            const std::vector<char> &rawData =
                m_OngoingReceives[i].temporaryDataArray;
            const helper::SubFileInfo &sfi = m_OngoingReceives[i].sfi;
            const std::string *name = m_OngoingReceives[i].varNamePointer;
            m_BP3Deserializer.ClipMemory(*name, m_IO, rawData, sfi.BlockBox,
                                         sfi.IntersectionBox);
        }
    }

    m_OngoingReceives.clear();
    m_MPIRequests.clear();
}

#define declare_type(T)                                                        \
    void InSituMPIReader::DoGetSync(Variable<T> &variable, T *data)            \
    {                                                                          \
        GetSyncCommon(variable, data);                                         \
    }                                                                          \
    void InSituMPIReader::DoGetDeferred(Variable<T> &variable, T *data)        \
    {                                                                          \
        GetDeferredCommon(variable, data);                                     \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void InSituMPIReader::Init()
{
    InitParameters();
    InitTransports();
}

void InSituMPIReader::InitParameters()
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

void InSituMPIReader::InitTransports()
{
    // Nothing to process from m_IO.m_TransportsParameters
}

void InSituMPIReader::DoClose(const int transportIndex)
{
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Reader " << m_ReaderRank << " Close(" << m_Name
                  << ")\n";
    }
    if (m_Verbosity > 2)
    {
        uint64_t inPlaceBytes, inTempBytes;
        MPI_Reduce(&m_BytesReceivedInPlace, &inPlaceBytes, 1, MPI_LONG_LONG_INT,
                   MPI_SUM, 0, m_MPIComm);
        MPI_Reduce(&m_BytesReceivedInTemporary, &inTempBytes, 1,
                   MPI_LONG_LONG_INT, MPI_SUM, 0, m_MPIComm);
        if (m_ReaderRank == 0)
        {
            std::cout << "ADIOS InSituMPI Reader for " << m_Name << " received "
                      << Statistics(inPlaceBytes, inTempBytes)
                      << "% of data in place (zero-copy)" << std::endl;
        }
    }
}

#define declare_type(T)                                                        \
    std::map<size_t, std::vector<typename Variable<T>::Info>>                  \
    InSituMPIReader::DoAllStepsBlocksInfo(const Variable<T> &variable) const   \
    {                                                                          \
        return m_BP3Deserializer.AllStepsBlocksInfo(variable);                 \
    }                                                                          \
                                                                               \
    std::vector<typename Variable<T>::Info> InSituMPIReader::DoBlocksInfo(     \
        const Variable<T> &variable, const size_t step) const                  \
    {                                                                          \
        return m_BP3Deserializer.BlocksInfo(variable, step);                   \
    }

ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

} // end namespace engine
} // end namespace core
} // end namespace adios2
