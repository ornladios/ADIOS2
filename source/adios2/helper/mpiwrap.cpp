
/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 */

#include "mpiwrap.h"

static inline bool SMPI_Available()
{
#ifdef ADIOS2_HAVE_MPI
    int flag;
    ::MPI_Initialized(&flag);
    if (!flag)
    {
        return false;
    }
    ::MPI_Finalized(&flag);
    return !flag;
#else
    // if compiled without MPI, always fall back to mpidummy
    return false;
#endif
}

#define MAKE_MPI_WRAPPER1(NAME, TARG1, ARG1)                                   \
    int S##NAME(TARG1 ARG1)                                                    \
    {                                                                          \
        if (SMPI_Available())                                                  \
        {                                                                      \
            return ::NAME(ARG1);                                               \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            return adios2::helper::mpidummy::NAME(ARG1);                       \
        }                                                                      \
    }

#define MAKE_MPI_WRAPPER2(NAME, TARG1, ARG1, TARG2, ARG2)                      \
    int S##NAME(TARG1 ARG1, TARG2 ARG2)                                        \
    {                                                                          \
        if (SMPI_Available())                                                  \
        {                                                                      \
            return ::NAME(ARG1, ARG2);                                         \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            return adios2::helper::mpidummy::NAME(ARG1, ARG2);                 \
        }                                                                      \
    }

#define MAKE_MPI_WRAPPER5(NAME, TARG1, ARG1, TARG2, ARG2, TARG3, ARG3, TARG4,  \
                          ARG4, TARG5, ARG5)                                   \
    int S##NAME(TARG1 ARG1, TARG2 ARG2, TARG3 ARG3, TARG4 ARG4, TARG5 ARG5)    \
    {                                                                          \
        if (SMPI_Available())                                                  \
        {                                                                      \
            return ::NAME(ARG1, ARG2, ARG3, ARG4, ARG5);                       \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            return adios2::helper::mpidummy::NAME(ARG1, ARG2, ARG3, ARG4,      \
                                                  ARG5);                       \
        }                                                                      \
    }

#define MAKE_MPI_WRAPPER7(NAME, TARG1, ARG1, TARG2, ARG2, TARG3, ARG3, TARG4,  \
                          ARG4, TARG5, ARG5, TARG6, ARG6, TARG7, ARG7)         \
    int S##NAME(TARG1 ARG1, TARG2 ARG2, TARG3 ARG3, TARG4 ARG4, TARG5 ARG5,    \
                TARG6 ARG6, TARG7 ARG7)                                        \
    {                                                                          \
        if (SMPI_Available())                                                  \
        {                                                                      \
            return ::NAME(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7);           \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            return adios2::helper::mpidummy::NAME(ARG1, ARG2, ARG3, ARG4,      \
                                                  ARG5, ARG6, ARG7);           \
        }                                                                      \
    }

#define MAKE_MPI_WRAPPER8(NAME, TARG1, ARG1, TARG2, ARG2, TARG3, ARG3, TARG4,  \
                          ARG4, TARG5, ARG5, TARG6, ARG6, TARG7, ARG7, TARG8,  \
                          ARG8)                                                \
    int S##NAME(TARG1 ARG1, TARG2 ARG2, TARG3 ARG3, TARG4 ARG4, TARG5 ARG5,    \
                TARG6 ARG6, TARG7 ARG7, TARG8 ARG8)                            \
    {                                                                          \
        if (SMPI_Available())                                                  \
        {                                                                      \
            return ::NAME(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8);     \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            return adios2::helper::mpidummy::NAME(ARG1, ARG2, ARG3, ARG4,      \
                                                  ARG5, ARG6, ARG7, ARG8);     \
        }                                                                      \
    }

#define MAKE_MPI_WRAPPER9(NAME, TARG1, ARG1, TARG2, ARG2, TARG3, ARG3, TARG4,  \
                          ARG4, TARG5, ARG5, TARG6, ARG6, TARG7, ARG7, TARG8,  \
                          ARG8, TARG9, ARG9)                                   \
    int S##NAME(TARG1 ARG1, TARG2 ARG2, TARG3 ARG3, TARG4 ARG4, TARG5 ARG5,    \
                TARG6 ARG6, TARG7 ARG7, TARG8 ARG8, TARG9 ARG9)                \
    {                                                                          \
        if (SMPI_Available())                                                  \
        {                                                                      \
            return ::NAME(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8,      \
                          ARG9);                                               \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            return adios2::helper::mpidummy::NAME(                             \
                ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9);         \
        }                                                                      \
    }

MAKE_MPI_WRAPPER2(MPI_Comm_dup, MPI_Comm, comm, MPI_Comm *, newcomm)
MAKE_MPI_WRAPPER1(MPI_Comm_free, MPI_Comm *, newcomm)
MAKE_MPI_WRAPPER2(MPI_Comm_rank, MPI_Comm, comm, int *, rank)
MAKE_MPI_WRAPPER2(MPI_Comm_size, MPI_Comm, comm, int *, size)
MAKE_MPI_WRAPPER1(MPI_Barrier, MPI_Comm, comm)
MAKE_MPI_WRAPPER5(MPI_Bcast, void *, buffer, int, count, MPI_Datatype, datatype,
                  int, root, MPI_Comm, comm)
MAKE_MPI_WRAPPER8(MPI_Gather, const void *, sendbuf, int, sendcount,
                  MPI_Datatype, sendtype, void *, recvbuf, int, recvcount,
                  MPI_Datatype, recvtype, int, root, MPI_Comm, comm)
MAKE_MPI_WRAPPER9(MPI_Gatherv, const void *, sendbuf, int, sendcount,
                  MPI_Datatype, sendtype, void *, recvbuf, const int *,
                  recvcounts, const int *, displs, MPI_Datatype, recvtype, int,
                  root, MPI_Comm, comm)
MAKE_MPI_WRAPPER7(MPI_Reduce, const void *, sendbuf, void *, recvbuf, int,
                  count, MPI_Datatype, datatype, MPI_Op, op, int, root,
                  MPI_Comm, comm)
