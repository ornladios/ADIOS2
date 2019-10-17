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
#include <cstdlib>
#include <cstring>
#include <numeric>

#include <chrono>
#include <iostream>
#include <string>

// remove warnings on Windows
#ifdef _WIN32
#pragma warning(disable : 4996) // fopen
#pragma warning(disable : 4477) // strcpy, sprintf
#endif

// Use some preprocessor hackery to achieve two objectives:
// - If we don't build with a real MPI, these functions below will go in the
//   global namespace (they're extern "C" in this case, anyway).
// - If we do have a real MPI, we put these functions into the helper::mpidummy
//   namespace so they can be used from the SMPI_* wrappers

#ifdef ADIOS2_HAVE_MPI
namespace adios2
{
namespace helper
{
namespace mpidummy
{
#define MPIDUMMY mpidummy

#else

#define MPIDUMMY

#endif

static inline int CheckReturn(int ier)
{
    if (ier != MPI_SUCCESS)
    {
        std::cerr << "mpidummy: MPI function returned error code " << ier
                  << ". Aborting!" << std::endl;
        std::abort();
    }
    return ier;
}

#define RETURN_CHECK(ier) return CheckReturn(ier)

int MPI_Init(int * /*argc*/, char *** /*argv*/) { RETURN_CHECK(MPI_SUCCESS); }

int MPI_Finalize() { RETURN_CHECK(MPI_SUCCESS); }

int MPI_Initialized(int *flag)
{
    *flag = 1;
    RETURN_CHECK(MPI_SUCCESS);
}

int MPI_Finalized(int *flag)
{
    *flag = 0;
    RETURN_CHECK(MPI_SUCCESS);
}

int MPI_Comm_split(MPI_Comm /*comm*/, int /*color*/, int /*key*/,
                   MPI_Comm * /*comm_out*/)
{
    RETURN_CHECK(MPI_SUCCESS);
}

int MPI_Barrier(MPI_Comm /*comm*/) { RETURN_CHECK(MPI_SUCCESS); }

int MPI_Bcast(void * /*buffer*/, int /*count*/, MPI_Datatype /*datatype*/,
              int /*root*/, MPI_Comm /*comm*/)
{
    RETURN_CHECK(MPI_SUCCESS);
}

int MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm)
{
    *newcomm = comm;
    RETURN_CHECK(MPI_SUCCESS);
}

int MPI_Comm_rank(MPI_Comm /*comm*/, int *rank)
{
    *rank = 0;
    RETURN_CHECK(MPI_SUCCESS);
}

int MPI_Comm_size(MPI_Comm /*comm*/, int *size)
{
    *size = 1;
    RETURN_CHECK(MPI_SUCCESS);
}

int MPI_Comm_free(MPI_Comm *comm)
{
    *comm = 0;
    RETURN_CHECK(MPI_SUCCESS);
}

#ifndef ADIOS2_HAVE_MPI
MPI_Comm MPI_Comm_f2c(MPI_Fint comm) { return comm; }
#endif

int MPI_Gather(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
               void *recvbuf, int recvcnt, MPI_Datatype recvtype, int root,
               MPI_Comm comm)
{
    int ier = MPI_SUCCESS;
    int n;
    size_t nsent = 0, nrecv = 0;
    if (sendcnt > 0 && !sendbuf)
    {
        RETURN_CHECK(MPI_ERR_BUFFER);
    }
    if (recvcnt > 0 && !recvbuf)
    {
        RETURN_CHECK(MPI_ERR_BUFFER);
    }
    if (root != 0)
    {
        RETURN_CHECK(MPI_ERR_ROOT);
    }

    ier = MPIDUMMY::MPI_Type_size(sendtype, &n);
    if (ier != MPI_SUCCESS)
    {
        RETURN_CHECK(ier);
    }
    nsent = n * sendcnt;

    ier = MPIDUMMY::MPI_Type_size(recvtype, &n);
    if (ier != MPI_SUCCESS)
    {
        RETURN_CHECK(ier);
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

    RETURN_CHECK(ier);
}

int MPI_Gatherv(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                void *recvbuf, const int *recvcnts, const int * /*displs */,
                MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int ier = MPI_SUCCESS;
    if (*recvcnts != sendcnt)
    {
        ier = MPI_ERR_COUNT;
        RETURN_CHECK(ier);
    }

    ier = MPIDUMMY::MPI_Gather(sendbuf, sendcnt, sendtype, recvbuf, *recvcnts,
                               recvtype, root, comm);
    RETURN_CHECK(ier);
}

int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  MPI_Comm comm)
{
    return MPIDUMMY::MPI_Gather(sendbuf, sendcount, sendtype, recvbuf,
                                recvcount, recvtype, 0, comm);
}

int MPI_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int *recvcounts, int *displs,
                   MPI_Datatype recvtype, MPI_Comm comm)
{
    return MPIDUMMY::MPI_Gatherv(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcounts, displs, recvtype, 0, comm);
}

int MPI_Scatter(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                void *recvbuf, int recvcnt, MPI_Datatype recvtype, int root,
                MPI_Comm comm)
{
    int ier = MPI_SUCCESS;
    int n;
    size_t nsent = 0, nrecv = 0;
    if (sendcnt > 0 && !sendbuf)
    {
        RETURN_CHECK(MPI_ERR_BUFFER);
    }
    if (recvcnt > 0 && !recvbuf)
    {
        RETURN_CHECK(MPI_ERR_BUFFER);
    }
    if (root != 0)
    {
        RETURN_CHECK(MPI_ERR_ROOT);
    }

    ier = MPIDUMMY::MPI_Type_size(sendtype, &n);
    if (ier != MPI_SUCCESS)
    {
        RETURN_CHECK(ier);
    }
    nsent = n * sendcnt;

    ier = MPIDUMMY::MPI_Type_size(recvtype, &n);
    if (ier != MPI_SUCCESS)
    {
        RETURN_CHECK(ier);
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

    RETURN_CHECK(ier);
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
        ier = MPIDUMMY::MPI_Scatter(sendbuf, *sendcnts, sendtype, recvbuf,
                                    recvcnt, recvtype, root, comm);
    }

    RETURN_CHECK(ier);
}

int MPI_Recv(void * /*recvbuffer*/, int /*count*/, MPI_Datatype /*type*/,
             int /*source*/, int /*tag*/, MPI_Comm /*comm*/,
             MPI_Status * /*status*/)
{
    RETURN_CHECK(MPI_SUCCESS);
}

int MPI_Irecv(void * /*recvbuffer*/, int /*count*/, MPI_Datatype /*type*/,
              int /*source*/, int /*tag*/, MPI_Comm /*comm*/,
              MPI_Request * /*request*/)

{
    RETURN_CHECK(MPI_SUCCESS);
}

int MPI_Send(const void * /*sendbuffer*/, int /*count*/, MPI_Datatype /*type*/,
             int /*destination*/, int /*tag*/, MPI_Comm /*comm*/)
{
    RETURN_CHECK(MPI_SUCCESS);
}

int MPI_Isend(const void * /*recvbuffer*/, int /*count*/, MPI_Datatype /*type*/,
              int /*source*/, int /*tag*/, MPI_Comm /*comm*/,
              MPI_Request * /*request*/)
{
    RETURN_CHECK(MPI_SUCCESS);
}

int MPI_Wait(MPI_Request * /*request*/, MPI_Status * /*status*/)
{
    RETURN_CHECK(MPI_SUCCESS);
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
    RETURN_CHECK(MPI_SUCCESS);
}

int MPI_Reduce(const void *sendbuf, void *recvbuf, int count,
               MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
    if (sendbuf == MPI_IN_PLACE)
    {
        RETURN_CHECK(MPI_SUCCESS);
    }
    int ier, size_of_type;
    ier = MPIDUMMY::MPI_Type_size(datatype, &size_of_type);
    if (ier != MPI_SUCCESS)
    {
        RETURN_CHECK(ier);
    }

    std::memcpy(recvbuf, sendbuf, count * static_cast<size_t>(size_of_type));
    RETURN_CHECK(MPI_SUCCESS);
}

int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count,
                  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    return MPIDUMMY::MPI_Reduce(sendbuf, recvbuf, count, datatype, op, 0, comm);
}

int MPI_Type_size(MPI_Datatype datatype, int *size)
{
    if (datatype == MPI_CHAR)
    {
        *size = sizeof(char);
    }
    else if (datatype == MPI_INT)
    {
        *size = sizeof(int);
    }
    else if (datatype == MPI_LONG)
    {
        *size = sizeof(long);
    }
    else if (datatype == MPI_UNSIGNED)
    {
        *size = sizeof(unsigned int);
    }
    else if (datatype == MPI_UNSIGNED_LONG)
    {
        *size = sizeof(unsigned long);
    }
    else if (datatype == MPI_UNSIGNED_LONG_LONG)
    {
        *size = sizeof(unsigned long long);
    }
    else
    {
        RETURN_CHECK(MPI_ERR_TYPE);
    }
    RETURN_CHECK(MPI_SUCCESS);
}

#ifdef ADIOS2_HAVE_MPI
} // end namespace mpidummy
} // end namespace helper
} // end namespace adios2
#endif
