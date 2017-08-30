/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloBPReader.cpp: Simple self-descriptive example of how to read a variable
 * to a BP File.
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

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /** Application variable */
    const std::size_t Nx = 10;
    std::vector<float> myFloats(Nx);
    std::vector<int> myInts(Nx);

    try
    {
        /** ADIOS class factory of IO class objects, DebugON is recommended */
        adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);

        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines */
        adios2::IO &bpIO = adios.DeclareIO("ReadBP");

        /** Engine derived class, spawned to start IO operations */
        auto bpReader = bpIO.Open("myVector.bp", adios2::Mode::Read);

        if (!bpReader)
        {
            throw std::ios_base::failure(
                "ERROR: bpWriter not created at Open\n");
        }

        /** Write variable for buffering */
        adios2::Variable<float> *bpFloats =
            bpReader->InquireVariable<float>("bpFloats");
        adios2::Variable<int> *bpInts =
            bpReader->InquireVariable<int>("bpInts");

        /** Set selection */
        adios2::Box

            if (bpFloats != nullptr)
        {
            bpReader->Read<float>(*bpFloats, myFloats.data());
        }

        if (bpFloats != nullptr)
        {
            bpReader->Read<int>(*bpInts, myInts.data());
        }

        /** Create bp  file, engine becomes unreachable after this*/
        bpReader->Close();
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
