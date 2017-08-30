/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileReadAPI.cpp
 *
 *  Created on: Sep 22, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include <adios2.h>
#include <mpi.h>

#include <vector>

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

        /** Write variable for buffering */
        adios2::Variable<float> *bpFloats =
            bpReader.InquireVariable<float>("bpFloats");
        adios2::Variable<int> *bpInts = bpReader.InquireVariable<int>("bpInts");

        // Set selection
        /** Dimensions Bounding Box {start}, {count} */
        adios2::Box<adios2::Dims> boxDims({0}, {5});

        /** Steps Bounding Box {start}, {count} */
        adios2::Box<adios2::Steps> boxSteps({1}, {2});

        if (bpFloats != nullptr)
        {
            bpFloats->SetSelection(boxDims, boxSteps);
            bpReader.Read<float>(*bpFloats, myFloats.data());
            // myFloats.data() is available
        }

        if (bpInts != nullptr)
        {
            bpFloats->SetSelection(boxDims, boxSteps);
            bpReader.Read<int>(*bpInts, myInts.data());
            // myInts.data() is available
        }

        /** Close bp file, engine becomes unreachable after this */
        bpReader.Close();
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
