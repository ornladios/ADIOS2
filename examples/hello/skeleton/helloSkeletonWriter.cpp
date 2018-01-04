/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 *  Created on: Jan 2018
 *      Author: Norbert Podhorszki
 */

#include <chrono>
#include <ios>      //std::ios_base::failure
#include <iostream> //std::cout
#include <mpi.h>
#include <stdexcept> //std::invalid_argument std::exception
#include <thread>
#include <vector>

#include "HelloSkeletonArgs.h"

#include <adios2.h>

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int wrank, wnproc;
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wnproc);

    const unsigned int color = 1;
    MPI_Comm mpiWriterComm;
    MPI_Comm_split(MPI_COMM_WORLD, color, wrank, &mpiWriterComm);

    int rank, nproc;
    MPI_Comm_rank(mpiWriterComm, &rank);
    MPI_Comm_size(mpiWriterComm, &nproc);

    try
    {
        HelloSkeletonArgs settings(true, argc, argv, rank, nproc);

        std::vector<float> myArray(settings.ndx * settings.ndy);

        adios2::ADIOS adios(settings.configfile, mpiWriterComm);

        adios2::IO &io = adios.DeclareIO("writer");

        adios2::Variable<float> &varArray = io.DefineVariable<float>(
            "myArray", {settings.gndx, settings.gndy},
            {settings.offsx, settings.offsy}, {settings.ndx, settings.ndy},
            adios2::ConstantDims);

        adios2::Engine &writer =
            io.Open(settings.streamname, adios2::Mode::Write);

        for (int step = 0; step < settings.steps; ++step)
        {
            int idx = 0;
            for (int j = 0; j < settings.ndy; ++j)
            {
                for (int i = 0; i < settings.ndx; ++i)
                {
                    myArray[idx] = rank + (step / 100.0f);
                    ++idx;
                }
            }
            writer.BeginStep(adios2::StepMode::Append);
            writer.PutDeferred<float>(varArray, myArray.data());
            writer.EndStep();
            std::this_thread::sleep_for(
                std::chrono::milliseconds(settings.sleeptime));
        }

        writer.Close();
    }
    catch (std::invalid_argument &e)
    {
        std::cout << "Invalid argument exception, STOPPING PROGRAM from rank "
                  << rank << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::ios_base::failure &e)
    {
        std::cout << "IO System base failure exception, STOPPING PROGRAM "
                     "from rank "
                  << rank << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::exception &e)
    {
        std::cout << "Exception, STOPPING PROGRAM from rank " << rank << "\n";
        std::cout << e.what() << "\n";
    }

    MPI_Finalize();

    return 0;
}
