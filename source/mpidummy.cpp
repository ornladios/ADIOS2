/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS is freely available under the terms of the BSD license described
 * in the COPYING file in the top level directory of this source distribution.
 *
 * Copyright (c) 2008 - 2009.  UT-BATTELLE, LLC. All rights reserved.
 */

/*
   A dummy MPI implementation for the BP READ API, to have an MPI-free version
   of the API
*/
/// \cond EXCLUDE_FROM_DOXYGEN
#define __STDC_FORMAT_MACROS
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstring>

//#define _LARGEFILE64_SOURCE
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
/// \endcond

#include "mpidummy.h"

#if defined(__APPLE__) || defined(__WIN32__) || defined(__CYGWIN__)
#define lseek64 lseek
#define open64 open
#endif

namespace adios
{

static char mpierrmsg[MPI_MAX_ERROR_STRING];

int MPI_Init(int * /*argc*/, char *** /*argv*/)
{
  mpierrmsg[0] = '\0';
  return MPI_SUCCESS;
}

int MPI_Finalize()
{
  mpierrmsg[0] = '\0';
  return MPI_SUCCESS;
}

int MPI_Initialized(int *flag)
{
  *flag = 1;
  return MPI_SUCCESS;
}

int MPI_Comm_split(MPI_Comm /*comm*/, int /*color*/, int /*key*/,
                   MPI_Comm * /*comm_out*/)
{
  return MPI_SUCCESS;
}

int MPI_Barrier(MPI_Comm /*comm*/) { return MPI_SUCCESS; }

int MPI_Bcast(void * /*buffer*/, int /*count*/, MPI_Datatype /*datatype*/,
              int /*root*/, MPI_Comm /*comm*/)
{
  return MPI_SUCCESS;
}

int MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm)
{
  *newcomm = comm;
  return MPI_SUCCESS;
}

int MPI_Comm_rank(MPI_Comm /*comm*/, int *rank)
{
  *rank = 0;
  return MPI_SUCCESS;
}

int MPI_Comm_size(MPI_Comm /*comm*/, int *size)
{
  *size = 1;
  return MPI_SUCCESS;
}

int MPI_Comm_free(MPI_Comm *comm)
{
  *comm = 0;
  return MPI_SUCCESS;
}

MPI_Comm MPI_Comm_f2c(MPI_Fint comm) { return comm; }

int MPI_Gather(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
               void *recvbuf, int recvcnt, MPI_Datatype recvtype, int root,
               MPI_Comm comm)
{
  int ier = MPI_SUCCESS;
  size_t n = 0, nsent = 0, nrecv = 0;
  if (!sendbuf || !recvbuf)
  {
    ier = MPI_ERR_BUFFER;
  }
  if (comm == MPI_COMM_NULL || root)
  {
    ier = MPI_ERR_COMM;
  }

  switch (sendtype)
  {
    case MPI_INT:
      n = sizeof(int);
      break;
    default:
      return MPI_ERR_TYPE;
  }
  nsent = n * sendcnt;

  switch (recvtype)
  {
    case MPI_INT:
      nrecv = sizeof(int);
      break;
    default:
      return MPI_ERR_TYPE;
  }
  nrecv = n * recvcnt;

  if (nrecv != nsent)
  {
    ier = MPI_ERR_COUNT;
  }

  if (ier == MPI_SUCCESS)
  {
    memcpy(recvbuf, sendbuf, nsent);
  }
  else
  {
    snprintf(mpierrmsg, ier, "could not gather data\n");
  }

  return ier;
}

int MPI_Gatherv(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                void *recvbuf, const int *recvcnts, const int *displs,
                MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int ier = MPI_SUCCESS;
  if (!recvcnts || !displs)
  {
    ier = MPI_ERR_BUFFER;
  }

  if (ier == MPI_SUCCESS)
  {
    ier = MPI_Gather(sendbuf, sendcnt, sendtype, recvbuf, *recvcnts, recvtype,
                     root, comm);
  }

  return ier;
}

int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  MPI_Comm comm)
{
  return MPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                    0, comm);
}

int MPI_Scatter(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                void *recvbuf, int recvcnt, MPI_Datatype recvtype, int root,
                MPI_Comm comm)
{
  int ier = MPI_SUCCESS;
  size_t n = 0, nsent = 0, nrecv = 0;
  if (!sendbuf || !recvbuf)
  {
    ier = MPI_ERR_BUFFER;
  }

  if (comm == MPI_COMM_NULL || root)
  {
    ier = MPI_ERR_COMM;
  }

  switch (sendtype)
  {
    case MPI_INT:
      n = sizeof(int);
      break;
    default:
      return MPI_ERR_TYPE;
  }
  nsent = n * sendcnt;

  switch (recvtype)
  {
    case MPI_INT:
      nrecv = sizeof(int);
      break;
    default:
      return MPI_ERR_TYPE;
  }
  nrecv = n * recvcnt;

  if (nrecv != nsent)
  {
    ier = MPI_ERR_COUNT;
  }

  if (ier == MPI_SUCCESS)
  {
    memcpy(recvbuf, sendbuf, nsent);
  }
  else
  {
    snprintf(mpierrmsg, ier, "could not scatter data\n");
  }

  return ier;
}

int MPI_Scatterv(const void *sendbuf, const int *sendcnts, const int *displs,
                 MPI_Datatype sendtype, void *recvbuf, int recvcnt,
                 MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int ier = MPI_SUCCESS;
  if (!sendcnts || !displs)
  {
    ier = MPI_ERR_BUFFER;
  }

  if (ier == MPI_SUCCESS)
  {
    ier = MPI_Scatter(sendbuf, *sendcnts, sendtype, recvbuf, recvcnt, recvtype,
                      root, comm);
  }

  return ier;
}

int MPI_Recv(void * /*recvbuffer*/, int /*count*/, MPI_Datatype /*type*/,
             int /*source*/, int /*tag*/, MPI_Comm /*comm*/,
             MPI_Status * /*status*/)
{
  return 0;
}

int MPI_Irecv(void * /*recvbuffer*/, int /*count*/, MPI_Datatype /*type*/,
              int /*source*/, int /*tag*/, MPI_Comm /*comm*/,
              MPI_Request * /*request*/)

{
  return 0;
}

int MPI_Send(const void * /*sendbuffer*/, int /*count*/, MPI_Datatype /*type*/,
             int /*destination*/, int /*tag*/, MPI_Comm /*comm*/)
{
  return 0;
}

int MPI_Isend(const void * /*recvbuffer*/, int /*count*/, MPI_Datatype /*type*/,
              int /*source*/, int /*tag*/, MPI_Comm /*comm*/,
              MPI_Request * /*request*/)
{
  return 0;
}

int MPI_Wait(MPI_Request * /*request*/, MPI_Status * /*status*/) { return 0; }

int MPI_File_open(MPI_Comm /*comm*/, const char *filename, int amode,
                  MPI_Info /*info*/, MPI_File *fh)
{
  *fh = open64(filename, amode);
  if (*fh == -1)
  {
    snprintf(mpierrmsg, MPI_MAX_ERROR_STRING, "File not found: %s", filename);
    return -1;
  }
  return MPI_SUCCESS;
}

int MPI_File_close(MPI_File *fh) { return close(*fh); }

int MPI_File_get_size(MPI_File fh, MPI_Offset *size)
{
  uint64_t curpos = lseek64(fh, 0, SEEK_CUR); // get the current seek pos
  uint64_t endpos =
      lseek64(fh, 0, SEEK_END);  // go to end, returned is the size in bytes
  lseek64(fh, curpos, SEEK_SET); // go back where we were
  *size = static_cast<MPI_Offset>(endpos);
  // printf("MPI_File_get_size: fh=%d, size=%lld\n", fh, *size);
  return MPI_SUCCESS;
}

int MPI_File_read(MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                  MPI_Status *status)
{
  // FIXME: int count can read only 2GB (*datatype size) array at max
  uint64_t bytes_to_read = static_cast<uint64_t>(count) * datatype;
  uint64_t bytes_read;
  bytes_read = read(fh, buf, bytes_to_read);
  if (bytes_read != bytes_to_read)
  {
    snprintf(mpierrmsg, MPI_MAX_ERROR_STRING,
             "could not read %" PRId64 " bytes. read only: %" PRId64 "\n",
             bytes_to_read, bytes_read);
    return -2;
  }
  *status = bytes_read;
  // printf("MPI_File_read: fh=%d, count=%d, typesize=%d, bytes read=%lld\n",
  // fh, count, datatype, *status);
  return MPI_SUCCESS;
}

int MPI_File_seek(MPI_File fh, MPI_Offset offset, int whence)
{
  lseek64(fh, offset, whence);
  // printf("MPI_File_seek: fh=%d, offset=%lld, whence=%d\n", fh, off, whence);
  return MPI_SUCCESS;
}

int MPI_Get_count(const MPI_Status *status, MPI_Datatype, int *count)
{
  *count = static_cast<int>(*status);
  return MPI_SUCCESS;
}

int MPI_Error_string(int /*errorcode*/, char *string, int *resultlen)
{
  // sprintf(string, "Dummy lib does not know error strings.
  // Code=%d\n",errorcode);
  strcpy(string, mpierrmsg);
  *resultlen = strlen(string);
  return MPI_SUCCESS;
}

double MPI_Wtime()
{
  // Implementation not tested
  timeval tv = {0, 0};
  gettimeofday(&tv, nullptr);
  return tv.tv_sec + tv.tv_usec * 1e-6;
}

int MPI_Get_processor_name(char *name, int *resultlen)
{
  sprintf(name, "0");
  *resultlen = 1;
  return 0;
}

} // end namespace adios
