/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <adios2_c.h>

#if !ADIOS2_USE_MPI
#error "ADIOS2_USE_MPI is not true for source using ADIOS2 MPI bindings"
#endif

#include <mpi.h>

#include <stdio.h>

int main(int argc, char **argv)
{
    int provided;

    // MPI_THREAD_MULTIPLE is only required if you enable the SST MPI_DP
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    adios2_adios *adios = adios2_init_mpi(MPI_COMM_WORLD);
    if (!adios)
    {
        fprintf(stderr, "adios2_init() failed\n");
        return 1;
    }
    adios2_finalize(adios);

    MPI_Finalize();

    return 0;
}
