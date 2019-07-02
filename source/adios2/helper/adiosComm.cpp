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

} // end namespace helper
} // end namespace adios2
