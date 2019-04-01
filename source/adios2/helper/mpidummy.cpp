/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS is freely available under the terms of the BSD license described
 * in the COPYING file in the top level directory of this source distribution.
 *
 * Copyright (c) 2008 - 2009.  UT-BATTELLE, LLC. All rights reserved.

   A dummy MPI implementation for the BP READ API, to have an MPI-free version
   of the API
*/

#include "mpidummy.h"

#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <numeric>

#include <chrono>
#include <string>

// remove warnings on Windows
#ifdef _WIN32
#pragma warning(disable : 4996) // fopen
#pragma warning(disable : 4477) // strcpy, sprintf
#endif

namespace adios2
{
namespace helper
{
namespace mpi
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

int MPI_Finalized(int *flag)
{
    *flag = 0;
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
    if (!sendbuf && !recvbuf)
    {
        return ier;
    }
    if (comm == MPI_COMM_NULL || root)
    {
        ier = MPI_ERR_COMM;
    }

    switch (sendtype)
    {
    case MPI_CHAR:
        n = sizeof(char);
        break;
    case MPI_INT:
        n = sizeof(int);
        break;
    case MPI_UNSIGNED:
        n = sizeof(unsigned int);
        break;
    case MPI_UNSIGNED_LONG:
        n = sizeof(unsigned long);
        break;
    case MPI_UNSIGNED_LONG_LONG:
        n = sizeof(unsigned long long);
        break;
    default:
        return MPI_ERR_TYPE;
    }
    nsent = n * sendcnt;

    switch (recvtype)
    {
    case MPI_CHAR:
        nrecv = sizeof(char);
        break;
    case MPI_INT:
        nrecv = sizeof(int);
        break;
    case MPI_UNSIGNED:
        nrecv = sizeof(unsigned int);
        break;
    case MPI_UNSIGNED_LONG:
        nrecv = sizeof(unsigned long);
        break;
    case MPI_UNSIGNED_LONG_LONG:
        nrecv = sizeof(unsigned long long);
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
        std::memcpy(recvbuf, sendbuf, nsent);
    }
    else
    {
        std::snprintf(mpierrmsg, ier, "could not gather data\n");
    }

    return ier;
}

int MPI_Gatherv(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                void *recvbuf, const int *recvcnts, const int * /*displs */,
                MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int ier = MPI_SUCCESS;
    if (*recvcnts != sendcnt)
    {
        ier = MPI_ERR_BUFFER;
        return ier;
    }

    ier = MPI_Gather(sendbuf, sendcnt, sendtype, recvbuf, *recvcnts, recvtype,
                     root, comm);
    return ier;
}

int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  MPI_Comm comm)
{
    return MPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                      recvtype, 0, comm);
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
        std::memcpy(recvbuf, sendbuf, nsent);
    }
    else
    {
        std::snprintf(mpierrmsg, ier, "could not scatter data\n");
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
        ier = MPI_Scatter(sendbuf, *sendcnts, sendtype, recvbuf, recvcnt,
                          recvtype, root, comm);
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
    std::string mode;
    if (amode | MPI_MODE_RDONLY)
    {
        mode += "r";
    }
    if (amode | MPI_MODE_WRONLY)
    {
        mode += "w";
    }
    if (amode | MPI_MODE_APPEND)
    {
        mode += "a";
    }
    mode += "b";

    *fh = std::fopen(filename, mode.c_str());
    if (!*fh)
    {
        std::snprintf(mpierrmsg, MPI_MAX_ERROR_STRING, "File not found: %s",
                      filename);
        return -1;
    }
    return MPI_SUCCESS;
}

int MPI_File_close(MPI_File *fh) { return fclose(*fh); }

int MPI_File_get_size(MPI_File fh, MPI_Offset *size)
{
    long curpos = std::ftell(fh);
    fseek(fh, 0, SEEK_END); // go to end, returned is the size in bytes
    long endpos = std::ftell(fh);
    std::fseek(fh, curpos, SEEK_SET); // go back where we were
    *size = static_cast<MPI_Offset>(endpos);
    // printf("MPI_File_get_size: fh=%d, size=%lld\n", fh, *size);
    return MPI_SUCCESS;
}

int MPI_File_read(MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                  MPI_Status *status)
{
    // FIXME: int count can read only 2GB (*datatype size) array at max
    size_t bytes_to_read = static_cast<size_t>(count) * datatype;
    size_t bytes_read;
    bytes_read = std::fread(buf, 1, bytes_to_read, fh);
    if (bytes_read != bytes_to_read)
    {
        std::snprintf(mpierrmsg, MPI_MAX_ERROR_STRING,
                      "could not read %llu bytes. read only: %llu"
                      "\n",
                      (unsigned long long)bytes_to_read,
                      (unsigned long long)bytes_read);
        return -2;
    }
    *status = bytes_read;
    // printf("MPI_File_read: fh=%d, count=%d, typesize=%d, bytes read=%lld\n",
    // fh, count, datatype, *status);
    return MPI_SUCCESS;
}

int MPI_File_seek(MPI_File fh, MPI_Offset offset, int whence)
{
    return std::fseek(fh, offset, whence) == MPI_SUCCESS;
}

int MPI_Get_count(const MPI_Status *status, MPI_Datatype, int *count)
{
    *count = static_cast<int>(*status);
    return MPI_SUCCESS;
}

int MPI_Error_string(int /*errorcode*/, char *string, int *resultlen)
{
    // std::sprintf(string, "Dummy lib does not know error strings.
    // Code=%d\n",errorcode);
    std::strcpy(string, mpierrmsg);
    *resultlen = static_cast<int>(std::strlen(string));
    return MPI_SUCCESS;
}

double MPI_Wtime()
{
    std::chrono::duration<double> now =
        std::chrono::high_resolution_clock::now().time_since_epoch();
    return now.count();
}

int MPI_Get_processor_name(char *name, int *resultlen)
{
    std::sprintf(name, "0");
    *resultlen = 1;
    return 0;
}

int MPI_Reduce(const void *sendbuf, void *recvbuf, int count,
               MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
    switch (datatype)
    {
    case MPI_CHAR:
        if (op == MPI_SUM)
        {
            char *recvBuffer = reinterpret_cast<char *>(recvbuf);
            const char *sendBuffer = reinterpret_cast<const char *>(sendbuf);
            *recvBuffer = std::accumulate(sendBuffer, sendBuffer + count, 0);
        }
        break;
    case MPI_INT:
        if (op == MPI_SUM)
        {
            int *recvBuffer = reinterpret_cast<int *>(recvbuf);
            const int *sendBuffer = reinterpret_cast<const int *>(sendbuf);
            *recvBuffer = std::accumulate(sendBuffer, sendBuffer + count, 0);
        }
        break;
    case MPI_UNSIGNED:
        if (op == MPI_SUM)
        {
            unsigned int *recvBuffer =
                reinterpret_cast<unsigned int *>(recvbuf);
            const unsigned int *sendBuffer =
                reinterpret_cast<const unsigned int *>(sendbuf);
            *recvBuffer = std::accumulate(sendBuffer, sendBuffer + count, 0);
        }
        break;
    case MPI_UNSIGNED_LONG:
        if (op == MPI_SUM)
        {
            unsigned long int *recvBuffer =
                reinterpret_cast<unsigned long int *>(recvbuf);
            const unsigned long int *sendBuffer =
                reinterpret_cast<const unsigned long int *>(sendbuf);
            *recvBuffer = std::accumulate(sendBuffer, sendBuffer + count, 0);
        }
        break;
    case MPI_UNSIGNED_LONG_LONG:
        if (op == MPI_SUM)
        {
            unsigned long long int *recvBuffer =
                reinterpret_cast<unsigned long long int *>(recvbuf);
            const unsigned long long int *sendBuffer =
                reinterpret_cast<const unsigned long long int *>(sendbuf);
            *recvBuffer =
                std::accumulate(sendBuffer, sendBuffer + count,
                                static_cast<unsigned long long int>(0));
        }
        break;
    default:
        return MPI_ERR_TYPE;
    }

    return 0;
}

int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count,
                  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    return MPI_Reduce(sendbuf, recvbuf, count, datatype, op, 0, comm);
}

} // end namespace mpi
} // end namespace helper
} // end namespace adios2
