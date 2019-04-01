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

#include <cstdint>
#include <cstdio>

namespace adios2
{
namespace helper
{
namespace mpi
{

using MPI_Comm = int;
using MPI_Status = std::uint64_t;
using MPI_Request = std::uint64_t;
using MPI_File = std::FILE *;
using MPI_Info = int;
using MPI_Datatype = int;
using MPI_Offset = long int;
using MPI_Fint = int;
using MPI_Op = int;

#define MPI_SUCCESS 0
#define MPI_ERR_BUFFER 1  /* Invalid buffer pointer */
#define MPI_ERR_COUNT 2   /* Invalid count argument */
#define MPI_ERR_TYPE 3    /* Invalid datatype argument */
#define MPI_ERR_TAG 4     /* Invalid tag argument */
#define MPI_ERR_COMM 5    /* Invalid communicator */
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

#define MPI_COMM_NULL 0
#define MPI_COMM_WORLD 1
#define MPI_COMM_SELF 2

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

#define MPI_ANY_SOURCE 0
#define MPI_ANY_TAG 0

#define MPI_SUM 0
#define MPI_MAX 1

#define MPI_MAX_PROCESSOR_NAME 32

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
MPI_Comm MPI_Comm_f2c(MPI_Fint comm);

int MPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
               void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
               MPI_Comm comm);
int MPI_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, const int *recvcounts, const int *displs,
                MPI_Datatype recvtype, int root, MPI_Comm comm);
int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  MPI_Comm comm);

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

int MPI_File_open(MPI_Comm comm, const char *filename, int amode, MPI_Info info,
                  MPI_File *fh);
int MPI_File_close(MPI_File *fh);
int MPI_File_get_size(MPI_File fh, MPI_Offset *size);
int MPI_File_read(MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                  MPI_Status *status);
int MPI_File_seek(MPI_File fh, MPI_Offset offset, int whence);

int MPI_Get_count(const MPI_Status *status, MPI_Datatype datatype, int *count);
int MPI_Error_string(int errorcode, char *string, int *resultlen);
int MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *comm_out);

int MPI_Get_processor_name(char *name, int *resultlen);

double MPI_Wtime();

int MPI_Reduce(const void *sendbuf, void *recvbuf, int count,
               MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);

int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count,
                  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

} // end namespace mpi
} // end namespace helper
} // end namespace adios

#endif /* ADIOS2_MPIDUMMY_H_ */
