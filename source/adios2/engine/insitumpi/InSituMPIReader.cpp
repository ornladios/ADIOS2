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
#include "InSituMPIReader.tcc"

#include <adios_error.h>

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
    MPI_Comm_dup(MPI_COMM_WORLD, &m_CommWorld);
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
                  << "). Fixed schedule = " << (m_FixedSchedule ? "yes" : "no")
                  << ". #readers=" << m_ReaderNproc
                  << " #writers=" << m_RankAllPeers.size()
                  << " #appsize=" << m_GlobalNproc
                  << " #direct peers=" << m_RankDirectPeers.size() << std::endl;
    }

    m_WriteRootGlobalRank = insitumpi::ConnectDirectPeers(
        m_CommWorld, false, false, m_GlobalRank, m_RankDirectPeers);
    if (m_WriteRootGlobalRank > -1)
    {
        m_ConnectedToWriteRoot = true;
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
                  << "  figured that the Reader root is rank = "
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
    MPI_Comm_free(&m_CommWorld);
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
    m_NDeferredGets = 0;

    // Sync. recv. the global metadata
    unsigned long mdLen;
    if (m_ConnectedToWriteRoot)
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
        MPI_Recv(m_BP3Deserializer.m_Metadata.m_Buffer.data(), mdLen, MPI_CHAR,
                 m_WriteRootGlobalRank, insitumpi::MpiTags::Metadata,
                 m_CommWorld, &status);
    }

    // broadcast metadata to every reader
    MPI_Bcast(&mdLen, 1, MPI_UNSIGNED_LONG, m_ReaderRootRank, m_MPIComm);
    m_BP3Deserializer.m_Metadata.m_Buffer.resize(mdLen);
    MPI_Bcast(m_BP3Deserializer.m_Metadata.m_Buffer.data(), mdLen, MPI_CHAR,
              m_ReaderRootRank, m_MPIComm);

    // Parse metadata into Variables and Attributes maps
    m_BP3Deserializer.ParseMetadata(m_BP3Deserializer.m_Metadata, m_IO);

    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Reader " << m_ReaderRank << " found "
                  << m_IO.GetVariablesDataMap().size() << " variables and "
                  << m_IO.GetAttributesDataMap().size()
                  << " attributes in metadata" << std::endl;
    }
    return StepStatus::EndOfStream; // FIXME: this should be OK
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

    // Collect local metadata from peers

    // Create global metadata on all readers

    m_NDeferredGets = 0;
}

void InSituMPIReader::EndStep()
{
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Reader " << m_ReaderRank << " EndStep()\n";
    }
    if (m_NDeferredGets == 0)
    {
        PerformGets();
    }
    ClearMetadataBuffer();
}

void InSituMPIReader::Close(const int transportIndex)
{
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Reader " << m_ReaderRank << " Close(" << m_Name
                  << ")\n";
    }
}

// PRIVATE

#define declare_type(T)                                                        \
    void InSituMPIReader::DoGetSync(Variable<T> &variable, T *data)            \
    {                                                                          \
        GetSyncCommon(variable, data);                                         \
    }                                                                          \
    void InSituMPIReader::DoGetDeferred(Variable<T> &variable, T *data)        \
    {                                                                          \
        GetDeferredCommon(variable, data);                                     \
    }                                                                          \
    void InSituMPIReader::DoGetDeferred(Variable<T> &variable, T &data)        \
    {                                                                          \
        GetDeferredCommon(variable, &data);                                    \
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

void InSituMPIReader::InitTransports()
{
    // Nothing to process from m_IO.m_TransportsParameters
}

} // end namespace adios2
