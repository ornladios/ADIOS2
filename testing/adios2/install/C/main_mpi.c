/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#include <adios2_c.h>

#include <mpi.h>

#include <stdio.h>

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    adios2_adios *adios = adios2_init(MPI_COMM_WORLD, adios2_debug_mode_on);
    if (!adios)
    {
        fprintf(stderr, "adios2_init() failed\n");
        return 1;
    }
    adios2_finalize(adios);

    MPI_Finalize();

    return 0;
}
