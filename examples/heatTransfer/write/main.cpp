/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * main.cpp
 *
 * Recreates heat_transfer.f90 (Fortran) ADIOS tutorial example in C++
 *
 * Created on: Feb 2017
 *     Author: Norbert Podhorszki
 *
 */
#include <mpi.h>

#include <future> //std::future, std::async
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "HeatTransfer.h"
#include "IO.h"
#include "Settings.h"

void printUsage()
{
    std::cout << "Usage: heatTransfer  output  N  M   nx  ny   steps "
                 "iterations async\n"
              << "  output: name of output file\n"
              << "  N:      number of processes in X dimension\n"
              << "  M:      number of processes in Y dimension\n"
              << "  nx:     local array size in X dimension per processor\n"
              << "  ny:     local array size in Y dimension per processor\n"
              << "  steps:  the total number of steps to output\n"
              << "  iterations: one step consist of this many iterations\n"
              << "  async: on or off (default) \n\n";
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    /* World comm spans all applications started with the same aprun command
       on a Cray XK6. So we have to split and create the local
       'world' communicator for heat_transfer only.
       In normal start-up, the communicator will just equal the MPI_COMM_WORLD.
    */

    int wrank, wnproc;
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wnproc);
    MPI_Barrier(MPI_COMM_WORLD);

    const unsigned int color = 1;
    MPI_Comm mpiHeatTransferComm;
    MPI_Comm_split(MPI_COMM_WORLD, color, wrank, &mpiHeatTransferComm);

    int rank, nproc;
    MPI_Comm_rank(mpiHeatTransferComm, &rank);
    MPI_Comm_size(mpiHeatTransferComm, &nproc);

    try
    {
        double timeStart = MPI_Wtime();
        Settings settings(argc, argv, rank, nproc);
        HeatTransfer ht(settings);
        IO io(settings, mpiHeatTransferComm);

        ht.init(false);
        // ht.printT("Initialized T:", mpiHeatTransferComm);
        ht.heatEdges();
        ht.exchange(mpiHeatTransferComm);
        // ht.printT("Heated T:", mpiHeatTransferComm);

        std::future<void> futureWrite;
        if (settings.async)
        {
            futureWrite =
                std::async(std::launch::async, &IO::write, &io, 0, std::ref(ht),
                           std::ref(settings), mpiHeatTransferComm);
        }
        else
        {
            io.write(0, ht, settings, mpiHeatTransferComm);
        }

        for (unsigned int t = 1; t <= settings.steps; ++t)
        {
            if (rank == 0)
                std::cout << "Step " << t << ":\n";
            for (unsigned int iter = 1; iter <= settings.iterations; ++iter)
            {
                ht.iterate();
                ht.exchange(mpiHeatTransferComm);
                ht.heatEdges();
            }

            if (settings.async)
            {
                futureWrite.get();
                futureWrite = std::async(std::launch::async, &IO::write, &io, t,
                                         std::ref(ht), std::ref(settings),
                                         mpiHeatTransferComm);
            }
            else
            {
                io.write(t, ht, settings, mpiHeatTransferComm);
            }
        }
        if (settings.async)
        {
            futureWrite.get();
        }
        MPI_Barrier(mpiHeatTransferComm);

        double timeEnd = MPI_Wtime();
        if (rank == 0)
            std::cout << "Total runtime = " << timeEnd - timeStart << "s\n";
    }
    catch (std::invalid_argument &e) // command-line argument errors
    {
        std::cout << e.what() << std::endl;
        printUsage();
    }
    catch (std::ios_base::failure &e) // I/O failure (e.g. file not found)
    {
        std::cout << "I/O base exception caught\n";
        std::cout << e.what() << std::endl;
    }
    catch (std::exception &e) // All other exceptions
    {
        std::cout << "Exception caught\n";
        std::cout << e.what() << std::endl;
    }

    MPI_Finalize();
    return 0;
}
