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

#include "adios2/common/ADIOSMPI.h"
#include "utils/adios_reorganize/Reorganize.h"

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    try
    {
        adios2::utils::Reorganize reorg(argc, argv);
        reorg.Run();
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << "\n";
    }

    MPI_Finalize();
}
