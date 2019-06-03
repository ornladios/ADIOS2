
/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 *
 */

#ifndef ADIOS2_HELPER_MPIWRAP_H_
#define ADIOS2_HELPER_MPIWRAP_H_

// Get either the real MPI or mpidummy into the global namespace
#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#else
#include "mpidummy.h"
using namespace adios2::helper::mpidummy;
#endif

// SMPI_* work just as the original MPI_* functions, but will also
// work if MPI_Init has not been called, as long asssuming the communicator is a
// single-proc communicator, by falling back to mpidummy in that case

extern "C" {
int SMPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm);
int SMPI_Comm_free(MPI_Comm *newcomm);
int SMPI_Comm_rank(MPI_Comm comm, int *rank);
int SMPI_Comm_size(MPI_Comm comm, int *size);
int SMPI_Barrier(MPI_Comm comm);
int SMPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root,
               MPI_Comm comm);
int SMPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                MPI_Comm comm);
int SMPI_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, const int *recvcounts, const int *displs,
                 MPI_Datatype recvtype, int root, MPI_Comm comm);
int SMPI_Reduce(const void *sendbuf, void *recvbuf, int count,
                MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
}

#endif
