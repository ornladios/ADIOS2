/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Created by Dmitry Ganyushin ganyushindi@ornl.gov
 *
 * Helper functions for all examples
 */
#include "decomp.h"
#include "mpivars.h"
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* random integer from {minv, minv+1, ..., maxv}
 including minv and maxv */
long long int get_random(int minv, int maxv)
{
    long long int n;
    time_t t;
    /* Intializes random number generator */
    srand((unsigned)time(&t));
    n = (rand() % (maxv - minv + 1)) + minv;
    return n;
}
/* gather the local sizes of arrays and sum them up
 so that each process knows the global shape
 and its own offset in the global space */
void gather_decomp_1d(long long int *mysize, long long int *myshape, long long int *myoffset)
{
    long long int *sizes;
    int i;
    sizes = malloc(sizeof(long long int) * (size_t)nproc);
    MPI_Allgather(mysize, 1, MPI_LONG_LONG, sizes, 1, MPI_LONG_LONG, app_comm);

    *myshape = 0;
    for (i = 0; i < nproc; i++)
    {
        *myshape += sizes[i];
    }
    *myoffset = 0;
    for (i = 0; i < rank; i++)
    {
        *myoffset += sizes[i];
    }

    free(sizes);
    return;
}

void decomp_1d(long long int globalsize, long long int *myoffset, long long int *mysize)
{
    long long int rem;
    *mysize = globalsize / nproc;
    rem = globalsize - (nproc * *mysize);
    if (rank < rem)
    {
        mysize = mysize + 1;
        *myoffset = rank * *mysize;
    }
    else
    {
        *myoffset = rank * *mysize + rem;
    }
    return;
}
