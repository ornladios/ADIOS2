/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * main.cpp
 *
 *  Created on: Mar 7, 2018
 *      Author: Norbert Podhorszki, pnorbert@ornl.gov
 */

#include <iostream>
#include <stdexcept>

#include "utils/adios_reorganize/Reorganize.h"

#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#endif

int main(int argc, char *argv[])
{
#ifdef ADIOS2_HAVE_MPI
    MPI_Init(&argc, &argv);
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

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif
}
