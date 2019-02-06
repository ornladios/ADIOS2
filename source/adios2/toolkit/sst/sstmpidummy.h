#ifndef SSTMPIDUMMY_H_
#define SSTMPIDUMMY_H_

#include <sys/time.h>

typedef int MPI_Comm;
typedef int MPI_Status;
typedef int MPI_request;
typedef int MPI_Datatype;
typedef int MPI_Op;

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
#define MPI_INFO_NULL 0

#define MPI_COMM_NULL 0
#define MPI_COMM_WORLD 1
#define MPI_COMM_SELF 2

#define MPI_INT 1
#define MPI_CHAR 2
#define MPI_BYTE 2
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
#define MPI_LONG_LONG 12
#define MPI_LONG_DOUBLE 13
#define MPI_FLOAT_INT 14
#define MPI_DOUBLE_INT 15
#define MPI_LONG_DOUBLE_INT 16
#define MPI_SHORT_INT 17

#define MPI_ANY_SOURCE 0
#define MPI_ANY_TAG 0

#define MPI_MAX 0
#define MPI_MIN 1
#define MPI_SUM 2
#define MPI_PROD 3
#define MPI_LAND 4
#define MPI_BAND 5
#define MPI_LOR 6
#define MPI_BOR 7
#define MPI_LXOR 8
#define MPI_BXOR 9
#define MPI_MAXLOC 10
#define MPI_MINLOC 11
#define MPI_REPLACE 12
#define MPI_NO_OP 13

static int MPI_Comm_rank(MPI_Comm comm, int *rank)
{
    *rank = 0;
    return MPI_SUCCESS;
}

static int MPI_Comm_size(MPI_Comm comm, int *size)
{
    *size = 1;
    return MPI_SUCCESS;
}

static int MPI_Gather(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                      void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                      int root, MPI_Comm comm)
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
    case MPI_LONG:
        n = sizeof(unsigned long);
        break;
    case MPI_UNSIGNED_LONG_LONG:
    case MPI_LONG_LONG:
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
    case MPI_LONG:
        nrecv = sizeof(unsigned long);
        break;
    case MPI_UNSIGNED_LONG_LONG:
    case MPI_LONG_LONG:
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
        memcpy(recvbuf, sendbuf, nsent);
    }
    else
    {
        fprintf(stderr, "could not gather data\n");
    }

    return ier;
}

static int MPI_Gatherv(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                       void *recvbuf, const int *recvcnts, const int *displs,
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

static int MPI_Allgather(const void *sendbuf, int sendcount,
                         MPI_Datatype sendtype, void *recvbuf, int recvcount,
                         MPI_Datatype recvtype, MPI_Comm comm)
{
    return MPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                      recvtype, 0, comm);
}

static int MPI_Allreduce(void *send_data, void *recv_data, int count,
                         MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    return MPI_Gather(send_data, count, datatype, recv_data, count, datatype, 0,
                      comm);
}

static int MPI_Reduce(void *send_data, void *recv_data, int count,
                      MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
    return MPI_Allreduce(send_data, recv_data, count, datatype, op, comm);
}

static int MPI_Allgatherv(const void *sendbuf, int sendcount,
                          MPI_Datatype sendtype, void *recvbuf, int *recvcounts,
                          int *displs, MPI_Datatype recvtype, MPI_Comm comm)
{
    *recvcounts = 1;
    *displs = 0;
    return MPI_Gather(sendbuf, sendcount, sendtype, recvbuf, sendcount,
                      recvtype, 0, comm);
}

static int MPI_Barrier(MPI_Comm comm) { return MPI_SUCCESS; }

static int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root,
                     MPI_Comm comm)
{
    return MPI_SUCCESS;
}

static double MPI_Wtime()
{
    struct timeval tv;

    if (gettimeofday(&tv, NULL) < 0)
    {
        perror("could not get time");
        return (0.);
    }

    return (tv.tv_sec + ((double)tv.tv_usec / 1000000.));
}

#endif
