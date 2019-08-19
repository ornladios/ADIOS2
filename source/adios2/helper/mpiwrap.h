
/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 *
 */

#ifndef ADIOS2_HELPER_MPIWRAP_H_
#define ADIOS2_HELPER_MPIWRAP_H_

// Overview of the MPI wrapping:
// =============================
//
// Including this header provides a unified way of dealing with the
// building against an actual MPI library (ADIOS2_HAVE_MPI) vs using
// a dummy implementation, and also deals with running serially
// when built against MPI, but not calling MPI_Init
//
// MPI types / constants
// ---------------------
//
// If ADIOS2_HAVE_MPI is defined, the actual MPI types (in particular MPI_Comm)
// from the real MPI are always used, even in the dummy implementations that are
// used when MPI hasn't been initialized.
// Otherwise, the definitions from mpidummy.h are used. These types are in the
// global namespace.
//
// MPI_* functions
// -------------
//
// The usual MPI functions are always defined in the global namespace (extern
// "C"). If ADIOS2_HAVE_MPI is defined, the declarations come from mpi.h.
// Otherwise, these functions are declared in mpidummy.h, also extern "C".
//
// If ADIOS2_HAVE_MPI is defined, the declarations in mpidummy.h are put into
// adios2::helper::mpidummy in order to not interfere with the extern "C" ones,
// but this way they are still available to use in the SMPI_* wrappers
//
// SMPI_* wrappers
// ---------------
// The SMPI wrappers provide a way to write code that works in all three of the
// following cases:
// - If ADIOS2_HAVE_MPI is undefined, these wrappers call the corresponding
//   extern "C" MPI functions, which in this case are the mpidummy
//   implementations.
// - If ADIOS2_HAVE_MPI is defined, and MPI has been initialized, these wrappers
//   call the corresponding extern "C" MPI functions, which in this case are the
//   real MPI implementations.
// - If ADIOS2_HAVE_MPI is defined, and MPI has not been initialized, these
//   wrappers call the corresponding adios2::helper::mpidummy MPI functions,
//   which are serial only but don't require the real MPI to be initialized.

#include "adios2/common/ADIOSConfig.h"

// Get either the real MPI or mpidummy into the global namespace
#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#else
#include "mpidummy.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

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
int SMPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype,
                   MPI_Comm comm);
int SMPI_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                    void *recvbuf, int *recvcounts, int *displs,
                    MPI_Datatype recvtype, MPI_Comm comm);
int SMPI_Reduce(const void *sendbuf, void *recvbuf, int count,
                MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int SMPI_Allreduce(const void *sendbuf, void *recvbuf, int count,
                   MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

#ifdef __cplusplus
}
#endif

#endif /* ADIOS2_HELPER_MPIWRAP_H_ */
