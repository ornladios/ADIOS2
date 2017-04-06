/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * xmlParser.cpp
 *
 *  Created on: Nov 2, 2016
 *      Author: wfg
 */

#include <iostream>
#include <stdexcept>
#include <string>

#include "../../include/ADIOS.h"

#ifdef HAVE_MPI
#include <mpi.h>
#else
#include "../../include/mpidummy.h"
using namespace adios;
#endif

static void Usage();

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    try
    {
        if (argc != 2)
        {
            Usage();
        }
        else
        {
            const std::string xmlConfigFile(argv[1]);
            adios::ADIOS adios(xmlConfigFile, MPI_COMM_WORLD, true);
            adios.MonitorGroups(std::cout);
        }
    }
    catch (std::exception &e)
    {
        if (rank == 0)
        {
            std::cout << "ERROR: exception caught\n";
            std::cout << e.what() << "\n";
        }
    }

    MPI_Finalize();
    return 0;
}

static void Usage()
{
    std::cout << "Program to test XML Config file parsing\n";
    std::cout << "Usage: \n";
    std::cout << "\t MPI version: ./xmlParser_mpi xmlConfigFile\n";
    std::cout << "\t Non-MPI version: ./xmlParser_nompi xmlConfigFile\n";
}
