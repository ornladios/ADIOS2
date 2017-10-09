/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloBPTimeWriter.cpp  example for writing a variable using the Advance
 * function for time aggregation. Time step is saved as an additional (global)
 * single value variable, just for tracking purposes.
 *
 *  Created on: Feb 16, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include <algorithm> //std::for_each
#include <ios>       //std::ios_base::failure
#include <iostream>  //std::cout
#include <mpi.h>
#include <stdexcept> //std::invalid_argument std::exception
#include <vector>

#include <adios2.h>

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Application variable
    std::vector<float> myFloats = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    const std::size_t Nx = myFloats.size();

    try
    {
        /** ADIOS class factory of IO class objects, DebugON is recommended */
        adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);

        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines */
        adios2::IO &bpIO = adios.DeclareIO("BPFile_N2N");
        bpIO.SetParameters({{"Threads", "2"}});

        /** global array: name, { shape (total dimensions) }, { start
         * (local) },
         * { count (local) }, all are constant dimensions */
        adios2::Variable<float> &bpFloats = bpIO.DefineVariable<float>(
            "bpFloats", {size * Nx}, {rank * Nx}, {Nx}, adios2::ConstantDims);

        /** global single value variable: name */
        adios2::Variable<unsigned int> &bpTimeStep =
            bpIO.DefineVariable<unsigned int>("timeStep");

        /** Engine derived class, spawned to start IO operations */
        adios2::Engine &bpWriter =
            bpIO.Open("myVector.bp", adios2::Mode::Write);

        for (unsigned int timeStep = 0; timeStep < 10; ++timeStep)
        {
            bpWriter.BeginStep();
            if (rank == 0) // global single value, only saved by rank 0
            {
                bpWriter.PutSync<unsigned int>(bpTimeStep, timeStep);
            }

            myFloats[0] = timeStep;

            // template type is optional, but recommended
            bpWriter.PutSync<float>(bpFloats, myFloats.data());
            bpWriter.EndStep();
        }

        bpWriter.Close();
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
