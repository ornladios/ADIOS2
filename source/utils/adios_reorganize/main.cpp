/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include <stdexcept>

#include "utils/adios_reorganize/Reorganize.h"

#if ADIOS2_USE_MPI
#include <mpi.h>
#endif

int main(int argc, char *argv[])
{
#if ADIOS2_USE_MPI
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
#endif

    try
    {
        adios2::utils::Reorganize reorg(argc, argv);
        reorg.Run();
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << "\n";
    }

#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif
}
