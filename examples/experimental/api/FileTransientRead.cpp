/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileTransientRead.cpp
 *
 *  Created on: Sep 25, 2017
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

        /**
         * Engine derived class, spawned to start IO operations
         */
        adios2::Engine &bpReader = bpIO.Open("myVector.bp", adios2::Mode::Read);

        /** Find variables for reading */
        adios2::Variable<float> *bpFloats =
            bpReader.InquireVariable<float>("bpFloats");

        if (bpFloats == nullptr)
        {
            throw std::runtime_error("ERROR: variable " + bpFloats->m_Name +
                                     " not found\n");
        }

        adios2::Variable<int> *bpInts = bpReader.InquireVariable<int>("bpInts");

        if (bpInts == nullptr)
        {
            throw std::runtime_error("ERROR: variable " + bpInts->m_Name +
                                     " not found\n");
        }

        /** Dimensions Bounding Box {start}, {count} assume constant across time
         * iterations */
        adios2::Box<adios2::Dims> boxDims({0}, {5});
        bpFloats->SetSelection(boxDims);
        bpInts->SetSelection(boxDims);

        for (unsigned int t = 0; t < 100; ++t)
        {
            // Set time selection
            /** Steps Bounding Box {start}, {count} */
            adios2::Box<adios2::Steps> boxSteps({static_cast<std::size_t>(t)},
                                                {1});

            bpFloats->SetStepSelection(boxSteps);
            bpInts->SetStepSelection(boxSteps);

            /** execute Read instructions within step one by one, pointer data
             * is available after Read*/
            if (bpReader.GetAdvanceStatus() == adios2::AdvanceStatus::OK)
            {
                bpReader.Read<float>(*bpFloats, myFloats.data());
                bpReader.Read<int>(*bpInts, myInts.data());
            }

            // do tasks here with the variables
            // ...
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
