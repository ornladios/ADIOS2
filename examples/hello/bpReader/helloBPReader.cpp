/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloBPReader.cpp: Simple self-descriptive example of how to read a variable
 * to a BP File.
 *
 * Try running like this from the build directory:
 *   mpirun -np 3 ./bin/hello_bpReader
 *
 *  Created on: Feb 16, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */
#include <ios>      //std::ios_base::failure
#include <iostream> //std::cout
#include <mpi.h>
#include <stdexcept> //std::invalid_argument std::exception
#include <vector>

#include <adios2.h>

#include "scr.h"

int main(int argc, char *argv[])
{
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    std::string filename = "myVector_cpp.bp";

    // Since ADIOS2 engine introspects the directory structure to determine the file format,
    // flush any cached dataset to restart from the parallel file system.
    // Collective over MPI_COMM_WORLD.
    SCR_Config("SCR_GLOBAL_RESTART=1");

    // SCR attempts to load an application's most recent checkpoint by default.
    // The user can specify a particular checkpoint by name by setting SCR_CURRENT.
    // Collective over MPI_COMM_WORLD.
    SCR_Configf("SCR_CURRENT=%s", filename.c_str());

    SCR_Init();

    // Query whether SCR successfully loaded a restart, and if so, its name.
    // Collective over MPI_COMM_WORLD.
    // Both parameters are output.
    // The first parameter is set to 1 if SCR loaded a checkpoint, 0 otherwise.
    // The second parameter will hold the name of the checkpoint if one was loaded.
    // The name returned is the same string provided in SCR_Start_output when the checkpoint was created.
    int have_restart;
    char scr_dset[SCR_MAX_FILENAME];
    SCR_Have_restart(&have_restart, scr_dset);

    // Start a restart phase.
    // Collective over MPI_COMM_WORLD.
    // Should only be called if SCR_Have_restart indicates that SCR loaded a checkpoint.
    // For convenience, SCR_Start_restart returns the name of the checkpoint again.
    // This will match the name returned by SCR_Have_restart.
    //SCR_Start_restart(scr_dset);

    // Each process should track whether it reads its data successfully.
    // Set to 0 if calling process fails to read its data.
    int scr_valid = 1;

    try
    {
        /** ADIOS class factory of IO class objects */
        adios2::ADIOS adios(MPI_COMM_WORLD);

        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines */
        adios2::IO bpIO = adios.DeclareIO("ReadBP");

        /** Engine derived class, spawned to start IO operations */
        adios2::Engine bpReader = bpIO.Open(filename, adios2::Mode::Read);

        const std::map<std::string, adios2::Params> variables =
            bpIO.AvailableVariables();

        for (const auto &variablePair : variables)
        {
            std::cout << "Name: " << variablePair.first;

            for (const auto &parameter : variablePair.second)
            {
                std::cout << "\t" << parameter.first << ": " << parameter.second
                          << "\n";
            }
        }

        /** Write variable for buffering */
        adios2::Variable<float> bpFloats =
            bpIO.InquireVariable<float>("bpFloats");
        adios2::Variable<int> bpInts = bpIO.InquireVariable<int>("bpInts");

        const std::size_t Nx = 10;
        if (bpFloats) // means found
        {
            std::vector<float> myFloats;

            // read only the chunk corresponding to our rank
            bpFloats.SetSelection({{Nx * rank}, {Nx}});
            // myFloats.data is pre-allocated
            bpReader.Get<float>(bpFloats, myFloats, adios2::Mode::Sync);

            if (rank == 0)
            {
                std::cout << "MyFloats: \n";
                for (const auto number : myFloats)
                {
                    std::cout << number << " ";
                }
                std::cout << "\n";
            }
        }

        if (bpInts) // means not found
        {
            std::vector<int> myInts;
            // read only the chunk corresponding to our rank
            bpInts.SetSelection({{Nx * rank}, {Nx}});

            bpReader.Get<int>(bpInts, myInts, adios2::Mode::Sync);

            if (rank == 0)
            {
                std::cout << "myInts: \n";
                for (const auto number : myInts)
                {
                    std::cout << number << " ";
                }
                std::cout << "\n";
            }
        }

        /** Close bp file, engine becomes unreachable after this*/
        bpReader.Close();
    }
    catch (std::invalid_argument &e)
    {
        if (rank == 0)
        {
            std::cerr
                << "Invalid argument exception, STOPPING PROGRAM from rank "
                << rank << "\n";
            std::cerr << e.what() << "\n";
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    catch (std::ios_base::failure &e)
    {
        if (rank == 0)
        {
            std::cerr << "IO System base failure exception, STOPPING PROGRAM "
                         "from rank "
                      << rank << "\n";
            std::cerr << e.what() << "\n";
            std::cerr << "The file " << filename << " does not exist."
                      << " Presumably this is because hello_bpWriter has not "
                         "been run."
                      << " Run ./hello_bpWriter before running this program.\n";
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    catch (std::exception &e)
    {
        if (rank == 0)
        {
            std::cerr << "Exception, STOPPING PROGRAM from rank " << rank
                      << "\n";
            std::cerr << e.what() << "\n";
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Complete restart phase.
    // Collective over MPI_COMM_WORLD.
    // Each process should indicate whether it successfully read its data.
    // An allreduce determines whether all ranks succeeded.
    // If any failed, SCR will attempt to load up the next most recent checkpoint, if any.
    //SCR_Complete_restart(scr_valid);

    SCR_Finalize();
    MPI_Finalize();

    return 0;
}
