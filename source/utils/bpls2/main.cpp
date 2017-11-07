/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * bpls2Main.cpp
 *
 *  Created on: Oct 5, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include <iostream>
#include <stdexcept>

#include "adios2/ADIOSMPI.h"
#include "utils/bpls2/BPLS2.h"

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    try
    {
        adios2::utils::BPLS2 bpls2(argc, argv);
        bpls2.Run();
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << "\n";
    }

    MPI_Finalize();
}
