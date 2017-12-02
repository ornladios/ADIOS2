/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * IO_ADIOS2.cpp
 *
 *  Created on: Nov 2017
 *      Author: Norbert Podhorszki
 *
 */
#include <mpi.h>

#include "adios2.h"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "PrintDataStep.h"
#include "ReadSettings.h"

void printUsage()
{
    std::cout << "Usage: heatRead  config  input  N  M \n"
              << "  config: XML config file to use\n"
              << "  input:  name of input data file/stream\n"
              << "  N:      number of processes in X dimension\n"
              << "  M:      number of processes in Y dimension\n\n";
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    /* When writer and reader is launched together with a single mpirun command,
       the world comm spans all applications. We have to split and create the
       local 'world' communicator for the reader only.
       When writer and reader is launched separately, the mpiReaderComm
       communicator will just equal the MPI_COMM_WORLD.
     */

    int wrank, wnproc;
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wnproc);
    MPI_Barrier(MPI_COMM_WORLD);

    const unsigned int color = 2;
    MPI_Comm mpiReaderComm;
    MPI_Comm_split(MPI_COMM_WORLD, color, wrank, &mpiReaderComm);

    int rank, nproc;
    MPI_Comm_rank(mpiReaderComm, &rank);
    MPI_Comm_size(mpiReaderComm, &nproc);

    try
    {
        ReadSettings settings(argc, argv, rank, nproc);
        adios2::ADIOS ad(settings.configfile, mpiReaderComm, adios2::DebugON);

        // Define method for engine creation
        // 1. Get method def from config file or define new one

        adios2::IO &bpReaderIO = ad.DeclareIO("reader");
        if (!bpReaderIO.InConfigFile())
        {
            // if not defined by user, we can change the default settings
            // BPFileWriter is the default engine
            bpReaderIO.SetEngine("ADIOS1Reader");
            bpReaderIO.SetParameters({{"num_threads", "2"}});

            // ISO-POSIX file output is the default transport (called "File")
            // Passing parameters to the transport
            bpReaderIO.AddTransport("File", {{"verbose", "4"}});
        }

        adios2::Engine &bpReader = bpReaderIO.Open(
            settings.inputfile, adios2::Mode::Read, mpiReaderComm);

        double *T;
        adios2::Variable<double> *vT = nullptr;
        bool firstStep = true;
        int step = 0;

        while (true)
        {
            adios2::StepStatus status =
                bpReader.BeginStep(adios2::StepMode::NextAvailable);
            if (status != adios2::StepStatus::OK)
                break;

            if (firstStep)
            {
                vT = bpReaderIO.InquireVariable<double>("T");
                unsigned int gndx = vT->m_Shape[0];
                unsigned int gndy = vT->m_Shape[1];

                if (rank == 0)
                {
                    std::cout << "gndx       = " << gndx << std::endl;
                    std::cout << "gndy       = " << gndy << std::endl;
                }

                settings.DecomposeArray(gndx, gndy);
                T = new double[settings.readsize[0] * settings.readsize[1]];

                // Create a 2D selection for the subset
                vT->SetSelection(adios2::Box<adios2::Dims>(settings.offset,
                                                           settings.readsize));
                firstStep = false;
                MPI_Barrier(mpiReaderComm); // sync processes just for stdout
            }

            if (!rank)
            {
                std::cout << "Processing step " << step << std::endl;
            }
            // Arrays are read by scheduling one or more of them
            // and performing the reads at once
            bpReader.GetDeferred<double>(*vT, T);
            bpReader.PerformGets();

            printDataStep(T, settings.readsize.data(), settings.offset.data(),
                          rank, step);
            bpReader.EndStep();
            step++;
        }
        bpReader.Close();
        delete[] T;
    }
    catch (std::invalid_argument &e) // command-line argument errors
    {
        std::cout << e.what() << std::endl;
        printUsage();
    }

    MPI_Finalize();
    return 0;
}
