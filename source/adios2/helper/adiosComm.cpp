/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Comm.cpp
 */

#include "adiosComm.h"
#include "adiosComm.tcc"

#include <ios> //std::ios_base::failure
#include <utility>

#include "adios2/common/ADIOSMPI.h"
#include "adios2/helper/adiosString.h"

#include "adiosMPIFunctions.h"

namespace adios2
{
namespace helper
{

Comm::Comm() = default;

Comm::Comm(MPI_Comm mpiComm) : m_MPIComm(mpiComm) {}

Comm::~Comm()
{
    // Handle the case where MPI is finalized before the ADIOS destructor is
    // called, which happens, e.g., with global / static ADIOS objects
    int flag;
    MPI_Finalized(&flag);
    if (!flag)
    {
        if (m_MPIComm != MPI_COMM_NULL && m_MPIComm != MPI_COMM_WORLD &&
            m_MPIComm != MPI_COMM_SELF)
        {
            SMPI_Comm_free(&m_MPIComm);
        }
    }
}

Comm::Comm(Comm &&comm) : m_MPIComm(comm.m_MPIComm)
{
    comm.m_MPIComm = MPI_COMM_NULL;
}

Comm &Comm::operator=(Comm &&comm)
{
    Comm(std::move(comm)).swap(*this);
    return *this;
}

void Comm::swap(Comm &comm) { std::swap(this->m_MPIComm, comm.m_MPIComm); }

Comm Comm::Duplicate(MPI_Comm mpiComm)
{
    MPI_Comm newComm;
    SMPI_Comm_dup(mpiComm, &newComm);
    return Comm(newComm);
}

void Comm::Free(const std::string &hint)
{
    if (m_MPIComm != MPI_COMM_NULL && m_MPIComm != MPI_COMM_WORLD &&
        m_MPIComm != MPI_COMM_SELF)
    {
        CheckMPIReturn(SMPI_Comm_free(&m_MPIComm), hint);
    }
}

Comm Comm::Split(int color, int key, const std::string &hint) const
{
    MPI_Comm newComm;
    CheckMPIReturn(MPI_Comm_split(m_MPIComm, color, key, &newComm), hint);
    return Comm(newComm);
}

std::string Comm::BroadcastFile(const std::string &fileName,
                                const std::string hint,
                                const int rankSource) const
{
    int rank;
    MPI_Comm_rank(m_MPIComm, &rank);
    std::string fileContents;

    // Read the file on rank 0 and broadcast it to everybody else
    if (rank == rankSource)
    {
        // load file contents
        fileContents = FileToString(fileName, hint);
    }
    fileContents = this->BroadcastValue(fileContents, rankSource);

    return fileContents;
}

Comm::Req Comm::IsendImpl(const void *buffer, size_t count,
                          MPI_Datatype datatype, int dest, int tag,
                          const std::string &hint) const
{
    Comm::Req req(datatype);

    if (count > DefaultMaxFileBatchSize)
    {
        const size_t batches = count / DefaultMaxFileBatchSize;

        size_t position = 0;
        for (size_t b = 0; b < batches; ++b)
        {
            int batchSize = static_cast<int>(DefaultMaxFileBatchSize);
            MPI_Request mpiReq;
            CheckMPIReturn(
                MPI_Isend(static_cast<char *>(const_cast<void *>(buffer)) +
                              position,
                          batchSize, datatype, dest, tag, m_MPIComm, &mpiReq),
                "in call to Isend batch " + std::to_string(b) + " " + hint +
                    "\n");
            req.m_MPIReqs.emplace_back(mpiReq);

            position += DefaultMaxFileBatchSize;
        }
        const size_t remainder = count % DefaultMaxFileBatchSize;
        if (remainder > 0)
        {
            int batchSize = static_cast<int>(remainder);
            MPI_Request mpiReq;
            CheckMPIReturn(
                MPI_Isend(static_cast<char *>(const_cast<void *>(buffer)) +
                              position,
                          batchSize, datatype, dest, tag, m_MPIComm, &mpiReq),
                "in call to Isend remainder batch " + hint + "\n");
            req.m_MPIReqs.emplace_back(mpiReq);
        }
    }
    else
    {
        int batchSize = static_cast<int>(count);
        MPI_Request mpiReq;
        CheckMPIReturn(
            MPI_Isend(static_cast<char *>(const_cast<void *>(buffer)),
                      batchSize, datatype, dest, tag, m_MPIComm, &mpiReq),
            " in call to Isend with single batch " + hint + "\n");
        req.m_MPIReqs.emplace_back(mpiReq);
    }
    return req;
}

Comm::Req Comm::IrecvImpl(void *buffer, size_t count, MPI_Datatype datatype,
                          int source, int tag, const std::string &hint) const
{
    Comm::Req req(datatype);

    if (count > DefaultMaxFileBatchSize)
    {
        const size_t batches = count / DefaultMaxFileBatchSize;
        size_t position = 0;
        for (size_t b = 0; b < batches; ++b)
        {
            int batchSize = static_cast<int>(DefaultMaxFileBatchSize);
            MPI_Request mpiReq;
            CheckMPIReturn(MPI_Irecv(static_cast<char *>(buffer) + position,
                                     batchSize, datatype, source, tag,
                                     m_MPIComm, &mpiReq),
                           "in call to Irecv batch " + std::to_string(b) + " " +
                               hint + "\n");
            req.m_MPIReqs.emplace_back(mpiReq);

            position += DefaultMaxFileBatchSize;
        }

        const size_t remainder = count % DefaultMaxFileBatchSize;
        if (remainder > 0)
        {
            int batchSize = static_cast<int>(remainder);
            MPI_Request mpiReq;
            CheckMPIReturn(MPI_Irecv(static_cast<char *>(buffer) + position,
                                     batchSize, datatype, source, tag,
                                     m_MPIComm, &mpiReq),
                           "in call to Irecv remainder batch " + hint + "\n");
            req.m_MPIReqs.emplace_back(mpiReq);
        }
    }
    else
    {
        int batchSize = static_cast<int>(count);
        MPI_Request mpiReq;
        CheckMPIReturn(MPI_Irecv(buffer, batchSize, datatype, source, tag,
                                 m_MPIComm, &mpiReq),
                       " in call to Isend with single batch " + hint + "\n");
        req.m_MPIReqs.emplace_back(mpiReq);
    }

    return req;
}

Comm::Req::Req() = default;

Comm::Req::Req(MPI_Datatype datatype) : m_MPIDatatype(datatype) {}

Comm::Req::~Req() {}

Comm::Req::Req(Req &&req)
: m_MPIDatatype(req.m_MPIDatatype), m_MPIReqs(std::move(req.m_MPIReqs))
{
}

Comm::Req &Comm::Req::operator=(Req &&req)
{
    Req(std::move(req)).swap(*this);
    return *this;
}

void Comm::Req::swap(Req &req)
{
    std::swap(this->m_MPIDatatype, req.m_MPIDatatype);
    std::swap(this->m_MPIReqs, req.m_MPIReqs);
}

Comm::Status Comm::Req::Wait(const std::string &hint)
{
    Comm::Status status;
    if (m_MPIReqs.empty())
    {
        return status;
    }

#ifdef ADIOS2_HAVE_MPI
    std::vector<MPI_Request> mpiRequests = std::move(m_MPIReqs);
    std::vector<MPI_Status> mpiStatuses(mpiRequests.size());

    if (mpiRequests.size() > 1)
    {
        int mpiReturn = MPI_Waitall(static_cast<int>(mpiRequests.size()),
                                    mpiRequests.data(), mpiStatuses.data());
        if (mpiReturn == MPI_ERR_IN_STATUS)
        {
            for (auto &mpiStatus : mpiStatuses)
            {
                if (mpiStatus.MPI_ERROR != MPI_SUCCESS)
                {
                    mpiReturn = mpiStatus.MPI_ERROR;
                    break;
                }
            }
        }
        CheckMPIReturn(mpiReturn, hint);
    }
    else
    {
        CheckMPIReturn(MPI_Wait(mpiRequests.data(), mpiStatuses.data()), hint);
    }

    // Our batched operation should be from only one source and have one tag.
    status.Source = mpiStatuses.front().MPI_SOURCE;
    status.Tag = mpiStatuses.front().MPI_TAG;

    // Accumulate the total count of our batched operation.
    for (auto &mpiStatus : mpiStatuses)
    {
        int mpiCount = 0;
        CheckMPIReturn(MPI_Get_count(&mpiStatus, m_MPIDatatype, &mpiCount),
                       hint);
        status.Count += mpiCount;
    }

    // Our batched operation was cancelled if any member was cancelled.
    for (auto &mpiStatus : mpiStatuses)
    {
        int mpiCancelled = 0;
        MPI_Test_cancelled(&mpiStatus, &mpiCancelled);
        if (mpiCancelled)
        {
            status.Cancelled = true;
            break;
        }
    }
#endif

    return status;
}

} // end namespace helper
} // end namespace adios2
