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

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "HeatTransfer.h"
#include "IO.h"
#include "Settings.h"

void printUsage()
{
    std::cout << "Usage: heatTransfer  config   output  N  M   nx  ny   steps "
                 "iterations\n"
              << "  config: XML config file to use\n"
              << "  output: name of output data file/stream\n"
              << "  N:      number of processes in X dimension\n"
              << "  M:      number of processes in Y dimension\n"
              << "  nx:     local array size in X dimension per processor\n"
              << "  ny:     local array size in Y dimension per processor\n"
              << "  steps:  the total number of steps to output\n"
              << "  iterations: one step consist of this many iterations\n\n";
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    /* When writer and reader is launched together with a single mpirun command,
       the world comm spans all applications. We have to split and create the
       local 'world' communicator mpiHeatTransferComm for the writer only.
       When writer and reader is launched separately, the mpiHeatTransferComm
       communicator will just equal the MPI_COMM_WORLD.
     */

    int wrank, wnproc;
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wnproc);

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
	MPI_Barrier(mpiHeatTransferComm);
        double time_settings = MPI_Wtime();
        HeatTransfer ht(settings);
	MPI_Barrier(mpiHeatTransferComm);
        double time_htdeclare = MPI_Wtime();
        IO io(settings, mpiHeatTransferComm);
	MPI_Barrier(mpiHeatTransferComm);
	double time_io = MPI_Wtime();
        ht.init(false);
	MPI_Barrier(mpiHeatTransferComm);
	double time_init = MPI_Wtime();
        // ht.printT("Initialized T:", mpiHeatTransferComm);
        ht.heatEdges();
	MPI_Barrier(mpiHeatTransferComm);
        double time_heatEdges = MPI_Wtime();
        ht.exchange(mpiHeatTransferComm);
	MPI_Barrier(mpiHeatTransferComm);
        double time_exchange = MPI_Wtime();
        // ht.printT("Heated T:", mpiHeatTransferComm);

        io.write(0, ht, settings, mpiHeatTransferComm);
	MPI_Barrier(mpiHeatTransferComm);
        double time_iowrite = MPI_Wtime();

        double stepTime, totalTime = 0;

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

            stepTime = MPI_Wtime();
            io.write(t, ht, settings, mpiHeatTransferComm);
            stepTime = MPI_Wtime() - stepTime;
            if(!rank) {
                std::cout<<"Step " << t <<": write time = " << stepTime << std::endl;
            }

            totalTime += stepTime;
           
 
        }
        MPI_Barrier(mpiHeatTransferComm);

        double timeEnd = MPI_Wtime();
        if (rank == 0) {
            std::cout<<"Total write time = " << totalTime << std::endl;
            std::cout << "Total writer run time = " << timeEnd - timeStart << "s\n";
	    std::cout<<"Settings Declare time = " << time_settings - timeStart << std::endl;
	    std::cout<<"HT Declare time = " << time_htdeclare - time_settings<< std::endl;
            std::cout<<"IO Declare time = " << time_io - time_htdeclare << std::endl;
	    std::cout<<" ht init time = " << time_init - time_io<< std::endl;
	    std::cout<<"ht.heatEdges time = " << time_heatEdges- time_init << std::endl;
	    std::cout<<"ht.exchange time = " << time_exchange - time_heatEdges<< std::endl;
	    std::cout<<"io.write time = " << time_iowrite - time_exchange<< std::endl;
        }
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
