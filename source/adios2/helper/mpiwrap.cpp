
/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 */

#include "mpiwrap.h"

static inline bool SMPI_Available()
{
    int flag;
    ::MPI_Initialized(&flag);
    if (!flag)
    {
        return false;
    }
    ::MPI_Finalized(&flag);
    return !flag;
}

#ifdef ADIOS2_HAVE_MPI

// if MPI is available use it when possible, otherwise fall back to mpidummy
#define DISPATCH(FUNC, ...)                                                    \
    do                                                                         \
    {                                                                          \
        if (SMPI_Available())                                                  \
        {                                                                      \
            return ::FUNC(__VA_ARGS__);                                        \
        }                                                                      \
        return adios2::helper::mpidummy::FUNC(__VA_ARGS__);                    \
    } while (0)

#else

// if compiled without MPI, always dispatch to mpidummy
#define DISPATCH(FUNC, ...) return ::FUNC(__VA_ARGS__)

#endif

int SMPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm)
{
    DISPATCH(MPI_Comm_dup, comm, newcomm);
}

int SMPI_Comm_free(MPI_Comm *comm) { DISPATCH(MPI_Comm_free, comm); }

int SMPI_Comm_rank(MPI_Comm comm, int *rank)
{
    DISPATCH(MPI_Comm_rank, comm, rank);
}

int SMPI_Comm_size(MPI_Comm comm, int *size)
{
    DISPATCH(MPI_Comm_size, comm, size);
}

int SMPI_Barrier(MPI_Comm comm) { DISPATCH(MPI_Barrier, comm); }

int SMPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root,
               MPI_Comm comm)
{
    DISPATCH(MPI_Bcast, buffer, count, datatype, root, comm);
}

int SMPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                MPI_Comm comm)
{
    DISPATCH(MPI_Gather, sendbuf, sendcount, sendtype, recvbuf, recvcount,
             recvtype, root, comm);
}

int SMPI_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, const int *recvcounts, const int *displs,
                 MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    DISPATCH(MPI_Gatherv, sendbuf, sendcount, sendtype, recvbuf, recvcounts,
             displs, recvtype, root, comm);
}

int SMPI_Reduce(const void *sendbuf, void *recvbuf, int count,
                MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
    DISPATCH(MPI_Reduce, sendbuf, recvbuf, count, datatype, op, root, comm);
}
