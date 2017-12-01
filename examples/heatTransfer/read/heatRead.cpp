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

#define str_helper(X) #X
#define str(X) str_helper(X)
#ifndef DEFAULT_CONFIG
#define DEFAULT_CONFIG "../heat.xml"
#endif
#define DEFAULT_CONFIG_STR str(DEFAULT_CONFIG)

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    if (argc < 2)
    {
        std::cout << "Not enough arguments: need an input file\n";
        return 1;
    }
    const char *inputfile = argv[1];

    /* World comm spans all applications started with the same aprun command
     on a Cray XK6. So we have to split and create the local
     'world' communicator for the reader only.
     In normal start-up, the communicator will just equal the MPI_COMM_WORLD.
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

    adios2::ADIOS ad(std::string(DEFAULT_CONFIG_STR), mpiReaderComm,
                     adios2::DebugON);

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

    adios2::Engine &bpReader =
        bpReaderIO.Open(inputfile, adios2::Mode::Read, mpiReaderComm);

    unsigned int gndx;
    unsigned int gndy;
    double *T;
    adios2::Dims readsize;
    adios2::Dims offset;
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
            adios2::Variable<unsigned int> *vgndx =
                bpReaderIO.InquireVariable<unsigned int>("gndx");
            gndx = vgndx->m_Value;
            // bpReader.GetSync<unsigned int>("gndx", gndx);

            adios2::Variable<unsigned int> *vgndy =
                bpReaderIO.InquireVariable<unsigned int>("gndy");
            gndy = vgndy->m_Value;
            // bpReader.GetSync<unsigned int>("gndy", gndy);

            if (rank == 0)
            {
                std::cout << "gndx       = " << gndx << std::endl;
                std::cout << "gndy       = " << gndy << std::endl;
            }

            // 1D decomposition of the columns, which is inefficient for
            // reading!
            readsize.push_back(gndx);
            readsize.push_back(gndy / nproc);
            offset.push_back(0LL);
            offset.push_back(rank * readsize[1]);
            if (rank == nproc - 1)
            {
                // last process should read all the rest of columns
                readsize[1] = gndy - readsize[1] * (nproc - 1);
            }

            std::cout << "rank " << rank << " reads " << readsize[1]
                      << " columns from offset " << offset[1] << std::endl;

            vT = bpReaderIO.InquireVariable<double>("T");
            T = new double[readsize[0] * readsize[1]];

            // Create a 2D selection for the subset
            vT->SetSelection(adios2::Box<adios2::Dims>(offset, readsize));
            firstStep = false;
        }

        if (!rank)
        {
            std::cout << "Processing step " << step << std::endl;
        }
        // Arrays are read by scheduling one or more of them
        // and performing the reads at once
        bpReader.GetDeferred<double>(*vT, T);
        bpReader.PerformGets();

        printDataStep(T, readsize.data(), offset.data(), rank, step);
        bpReader.EndStep();
        step++;
    }
    bpReader.Close();
    delete[] T;
    MPI_Finalize();
    return 0;
}
