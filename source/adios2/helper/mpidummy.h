/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * A dummy MPI 'implementation' for the BP READ API, to have an MPI-free version
 * of the API
 *
 */

#ifndef ADIOS2_HELPER_MPIDUMMY_H_
#define ADIOS2_HELPER_MPIDUMMY_H_

#include "stdint.h"
#include "stdio.h"

#include "adios2/common/ADIOSConfig.h"

// If MPI is available, use the types and constants from that implementation,
// rather than our fake ones
#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#else

typedef int MPI_Comm;
typedef uint64_t MPI_Status;
typedef uint64_t MPI_Request;
typedef int MPI_Info;
typedef int MPI_Datatype;
typedef int MPI_Fint;
typedef int MPI_Op;

#define MPI_SUCCESS 0
#define MPI_ERR_BUFFER 1  /* Invalid buffer pointer */
#define MPI_ERR_COUNT 2   /* Invalid count argument */
#define MPI_ERR_TYPE 3    /* Invalid datatype argument */
#define MPI_ERR_TAG 4     /* Invalid tag argument */
#define MPI_ERR_COMM 5    /* Invalid communicator */
#define MPI_ERR_ROOT 6    /* Invalid root process */
#define MPI_ERR_INTERN 17 /* Invalid memory */
#define MPI_MAX_ERROR_STRING 512
#define MPI_MODE_RDONLY 1
#define MPI_MODE_WRONLY 2
#define MPI_MODE_RDWR (MPI_MODE_RDONLY | MPI_MODE_RDONLY)
#define MPI_MODE_CREATE MPI_MODE_WRONLY
#define MPI_MODE_EXCL 0
#define MPI_MODE_DELETE_ON_CLOSE 0
#define MPI_MODE_UNIQUE_OPEN 0
#define MPI_MODE_SEQUENTIAL 0
#define MPI_MODE_APPEND 4
#define MPI_SEEK_SET SEEK_SET
#define MPI_SEEK_CUR SEEK_CUR
#define MPI_SEEK_END SEEK_END
#define MPI_BYTE 1 /* I need the size of the type here */
#define MPI_INFO_NULL 0

#define MPI_IN_PLACE ((void *)1)

#define MPI_COMM_NULL 0
#define MPI_COMM_WORLD 1
#define MPI_COMM_SELF 2

#define MPI_DATATYPE_NULL 0
#define MPI_INT 1
#define MPI_CHAR 2
#define MPI_DOUBLE 3
#define MPI_UNSIGNED 4
#define MPI_UNSIGNED_LONG 5
#define MPI_UNSIGNED_LONG_LONG 6
#define MPI_SHORT 7
#define MPI_LONG 8
#define MPI_UNSIGNED_CHAR 9
#define MPI_2INT 10
#define MPI_UNSIGNED_SHORT 11
#define MPI_LONG_LONG_INT 12
#define MPI_LONG_DOUBLE 13
#define MPI_FLOAT_INT 14
#define MPI_DOUBLE_INT 15
#define MPI_LONG_DOUBLE_INT 16
#define MPI_SHORT_INT 17
#define MPI_SIGNED_CHAR 18

#define MPI_ANY_SOURCE 0
#define MPI_ANY_TAG 0

#define MPI_OP_NULL 0
#define MPI_MAX 1
#define MPI_MIN 2
#define MPI_SUM 3
#define MPI_PROD 4
#define MPI_LAND 5
#define MPI_BAND 6
#define MPI_LOR 7
#define MPI_BOR 8
#define MPI_LXOR 9
#define MPI_BXOR 10
#define MPI_MAXLOC 11
#define MPI_MINLOC 12
#define MPI_REPLACE 13
#define MPI_NO_OP 14

#define MPI_MAX_PROCESSOR_NAME 32

#endif

// Use some preprocessor hackery to achieve two objectives:
// - If we don't build with a real MPI, these functions below will go in the
//   global namespace (as extern "C").
// - If we do have a real MPI, we put these functions into the helper::mpidummy
//   namespace so they can be used from the SMPI_* wrappers

#ifdef ADIOS2_HAVE_MPI
namespace adios2
{
namespace helper
{
namespace mpidummy
{
#else
#ifdef __cplusplus
extern "C" {
#endif
#endif

int MPI_Init(int *argc, char ***argv);
int MPI_Finalize();
int MPI_Initialized(int *flag);
int MPI_Finalized(int *flag);

int MPI_Barrier(MPI_Comm comm);
int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root,
              MPI_Comm comm);

int MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm);
int MPI_Comm_rank(MPI_Comm comm, int *rank);
int MPI_Comm_size(MPI_Comm comm, int *size);
int MPI_Comm_free(MPI_Comm *comm);
#ifndef ADIOS2_HAVE_MPI
MPI_Comm MPI_Comm_f2c(MPI_Fint comm);
#endif

int MPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
               void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
               MPI_Comm comm);
int MPI_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, const int *recvcounts, const int *displs,
                MPI_Datatype recvtype, int root, MPI_Comm comm);
int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  MPI_Comm comm);
int MPI_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int *recvcounts, int *displs,
                   MPI_Datatype recvtype, MPI_Comm comm);

int MPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                MPI_Comm comm);
int MPI_Scatterv(const void *sendbuf, const int *sendcounts, const int *displs,
                 MPI_Datatype sendtype, void *recvbuf, int recvcount,
                 MPI_Datatype recvtype, int root, MPI_Comm comm);

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
             MPI_Comm comm, MPI_Status *status);
int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
              MPI_Comm comm, MPI_Request *request);
int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest,
             int tag, MPI_Comm comm);
int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest,
              int tag, MPI_Comm comm, MPI_Request *request);

int MPI_Wait(MPI_Request *request, MPI_Status *status);

int MPI_Type_size(MPI_Datatype datatype, int *size);

int MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *comm_out);

int MPI_Get_processor_name(char *name, int *resultlen);

double MPI_Wtime();

int MPI_Reduce(const void *sendbuf, void *recvbuf, int count,
               MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);

int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count,
                  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

#ifdef ADIOS2_HAVE_MPI
} // end namespace mpi
} // end namespace helper
} // end namespace adios
#else
#ifdef __cplusplus
} // end extern "C"
#endif
#endif

#endif /* ADIOS2_MPIDUMMY_H_ */
