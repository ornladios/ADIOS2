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

#include <iostream>

namespace adios2
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
                  << (m_FixedLocalSchedule ? "yes" : "no")
                  << ". #readers=" << m_ReaderNproc
                  << " #writers=" << m_RankAllPeers.size()
                  << " #appsize=" << m_GlobalNproc
                  << " #direct peers=" << m_RankDirectPeers.size() << std::endl;
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
    // Wait for the Step message from all peers
    // with a timeout

    // FIXME: This is a blocking receive here, must make it async to handle
    // timeouts but then should not issue receives more than once per actual
    // step
    // FIXME: All processes should timeout or succeed together.
    // If some timeouts (and never checks back) and the others succeed,
    // the global metadata operation will hang
    std::vector<MPI_Request> requests(m_RankDirectPeers.size());
    std::vector<MPI_Status> statuses(m_RankDirectPeers.size());
    std::vector<int> steps(m_RankDirectPeers.size());
    for (int peerID = 0; peerID < m_RankDirectPeers.size(); peerID++)
    {
        MPI_Irecv(&steps[peerID], 1, MPI_INT, m_RankDirectPeers[peerID],
                  insitumpi::MpiTags::Step, m_CommWorld, &requests[peerID]);
    }
    MPI_Waitall(m_RankDirectPeers.size(), requests.data(), statuses.data());

    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Reader " << m_ReaderRank << " new step "
                  << steps[0] << " arrived for " << m_Name << std::endl;
    }
    m_CurrentStep = steps[0];
    // FIXME: missing test whether all writers sent the same step

    if (m_CurrentStep == -1)
    {
        return StepStatus::EndOfStream;
    }

    m_NCallsPerformGets = 0;

    // Sync. recv. the global metadata
    if (!m_FixedRemoteSchedule)
    {
        unsigned long mdLen = 0;

        if (m_ReaderRootRank == m_ReaderRank)
        {
            MPI_Status status;
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
        m_IO.RemoveAllAttributes();
        m_BP3Deserializer.ParseMetadata(m_BP3Deserializer.m_Metadata, m_IO);

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
        int fixed = (m_FixedRemoteSchedule ? 1 : 0);
        if (m_ReaderRootRank == m_ReaderRank)
        {
            MPI_Status status;
            MPI_Recv(&fixed, 1, MPI_INT, m_WriteRootGlobalRank,
                     insitumpi::MpiTags::FixedRemoteSchedule, m_CommWorld,
                     &status);
        }

        // broadcast fixed schedule flag to every reader
        MPI_Bcast(&fixed, 1, MPI_INT, m_ReaderRootRank, m_MPIComm);
        m_FixedRemoteSchedule = (fixed ? true : false);
        if (m_ReaderRootRank == m_ReaderRank)
        {
            if (m_Verbosity == 5)
            {
                std::cout << "InSituMPI Reader " << m_ReaderRank
                          << " fixed Writer schedule = "
                          << m_FixedRemoteSchedule
                          << " fixed Reader schedule = " << m_FixedLocalSchedule
                          << std::endl;
            }
        }
    }
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
            int fixed = (int)m_FixedLocalSchedule;
            MPI_Send(&fixed, 1, MPI_INT, m_WriteRootGlobalRank,
                     insitumpi::MpiTags::FixedRemoteSchedule, m_CommWorld);
        }
    }

    // Create read schedule per writer
    // const std::map<std::string, SubFileInfoMap> variablesSubFileInfo =
    m_ReadScheduleMap.clear();
    m_ReadScheduleMap = m_BP3Deserializer.PerformGetsVariablesSubFileInfo(m_IO);
    // bool reader_IsRowMajor = IsRowMajor(m_IO.m_HostLanguage);
    // bool writer_IsRowMajor = m_BP3Deserializer.m_IsRowMajor;
    // recalculate seek offsets to payload offset 0 (beginning of blocks)
    int nRequests = insitumpi::FixSeeksToZeroOffset(
        m_ReadScheduleMap, IsRowMajor(m_IO.m_HostLanguage));

    if (m_CurrentStep == 0 || !m_FixedLocalSchedule)
    {
        // Send schedule to writers
        SendReadSchedule(m_ReadScheduleMap);
    }

    if (m_CurrentStep == 0 || !m_FixedLocalSchedule || !m_FixedRemoteSchedule)
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

    // Send final acknowledgment to the Writer
    int dummy = 1;
    MPI_Bcast(&dummy, 1, MPI_INT, m_ReaderRootRank, m_MPIComm);
    if (m_ReaderRootRank == m_ReaderRank)
    {
        MPI_Send(&dummy, 1, MPI_INT, m_WriteRootGlobalRank,
                 insitumpi::MpiTags::ReadCompleted, m_CommWorld);
    }

    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Reader " << m_ReaderRank
                  << " completed EndStep()\n";
    }
}

// PRIVATE

void InSituMPIReader::SendReadSchedule(
    const std::map<std::string, SubFileInfoMap> &variablesSubFileInfo)
{
    const bool profile = m_BP3Deserializer.m_Profiler.IsActive;

    // serialized schedules, one per-writer
    std::vector<std::vector<char>> serializedSchedules =
        insitumpi::SerializeLocalReadSchedule(m_RankAllPeers.size(),
                                              variablesSubFileInfo);

    std::vector<MPI_Request> request(m_RankAllPeers.size());
    std::vector<int> mdLen(m_RankAllPeers.size());
    for (int i = 0; i < m_RankAllPeers.size(); i++)
    {
        mdLen[i] = serializedSchedules[i].size();
        if (m_Verbosity == 5)
        {
            std::cout << "InSituMPI Reader " << m_ReaderRank
                      << " Send Read Schedule len = " << mdLen[i]
                      << " to Writer " << i << " global rank "
                      << m_RankAllPeers[i] << std::endl;
        }
        MPI_Isend(&(mdLen[i]), 1, MPI_INT, m_RankAllPeers[i],
                  insitumpi::MpiTags::ReadScheduleLength, m_CommWorld,
                  &(request[i]));
        MPI_Isend(serializedSchedules[i].data(), mdLen[i], MPI_CHAR,
                  m_RankAllPeers[i], insitumpi::MpiTags::ReadSchedule,
                  m_CommWorld, &(request[i]));
    }
    std::vector<MPI_Status> status(m_RankAllPeers.size());
    MPI_Waitall(m_RankAllPeers.size(), request.data(), status.data());
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
    else if (type == adios2::GetType<T>())                                     \
    {                                                                          \
        Variable<T> *variable = m_IO.InquireVariable<T>(variablePair.first);   \
        if (m_DebugMode && variable == nullptr)                                \
        {                                                                      \
            throw std::invalid_argument(                                       \
                "ERROR: variable " + variablePair.first +                      \
                " not found, in call to AsyncSendVariable\n");                 \
        }                                                                      \
        AsyncRecvVariable<T>(*variable, variablePair.second);                  \
    }

        ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
    }
}

void InSituMPIReader::ProcessReceives()
{
    const int nRequests = m_OngoingReceives.size();
    int nCompletedRequests = 0;
    int index, ierr;
    MPI_Status status;
    while (nCompletedRequests != nRequests)
    {
        ierr = MPI_Waitany(nRequests, m_MPIRequests.data(), &index, &status);
        if (ierr == MPI_SUCCESS)
        {
            if (0 <= index && index < nRequests)
            {
                if (m_OngoingReceives[index].inPlaceDataArray == nullptr)
                {
                    const std::vector<char> &rawData =
                        m_OngoingReceives[index].temporaryDataArray;
                    const SubFileInfo &sfi = m_OngoingReceives[index].sfi;
                    const std::string *name =
                        m_OngoingReceives[index].varNamePointer;
                    m_BP3Deserializer.ClipContiguousMemory(*name, m_IO, rawData,
                                                           sfi.BlockBox,
                                                           sfi.IntersectionBox);
                }
                // MPI_Request_free(&m_MPIRequests[index]); // not required???
                // m_MPIRequests[index] = MPI_REQUEST_NULL; // not required???
                ++nCompletedRequests;
            }
            else
            {
                std::cerr
                    << "InSituMPI Reader " << m_ReaderRank
                    << " MPI Error when waiting for receives to complete. "
                       "Received index = "
                    << index << std::endl;
            }
        }
        else
        {
            std::cerr << "InSituMPI Reader " << m_ReaderRank
                      << " MPI Error when waiting for receives to complete. "
                         "Error code = "
                      << ierr << std::endl;
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
ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

void InSituMPIReader::Init()
{
    InitParameters();
    InitTransports();
}

void InSituMPIReader::InitParameters()
{
    auto itFixedSchedule = m_IO.m_Parameters.find("FixedSchedule");
    if (itFixedSchedule == m_IO.m_Parameters.end())
    {
        m_FixedLocalSchedule = false;
    }
    else if (itFixedSchedule->second == "true")
    {
        m_FixedLocalSchedule = true;
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

} // end namespace adios2
