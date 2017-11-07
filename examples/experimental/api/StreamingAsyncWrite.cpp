/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * StreamingAsyncWrite.cpp
 *
 *  Created on: Sep 22, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include <future>
#include <ios>      //std::ios_base::failure
#include <iostream> //std::cout
#include <mpi.h>
#include <stdexcept> //std::invalid_argument std::exception
#include <vector>

#include <adios2.h>

void Acquire(adios2::VariableBase &variable, std::vector<float> &data)
{
    // can be anything to set time and data
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /** Application variable */
    std::vector<float> myTime = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::vector<float> mySound = {0, -1, -2, -3, -4, -5, -6, -7, -8, -9};
    const std::size_t Nx = myTime.size();

    try
    {
        /** ADIOS class factory of IO class objects, DebugON is recommended
         */
        adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);

        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines */
        adios2::IO &streamingIO = adios.DeclareIO("StreamingWrite");
        streamingIO.SetEngine("DataManWriter");

        auto wanStream = streamingIO.AddTransport(
            "WAN", {{"IPAddress", "127.0.0.1"}, {"Port", "22"}});

        /// Setting flush policy
        // the user can set the memory explicitly for ADIOS2
        streamingIO.SetSingleParameter("MaxBufferSize", "100Mb");
        // or by number of steps (naively trying to allocate "N=3" steps
        // before
        // flushing
        // bpIO.SetSingleParameter("StepsToBuffer", "3");

        /** global array : name, { shape (total) }, { start (local) },
         *  { count (local) }, all are constant dimensions */
        adios2::Variable<float> &bpSound = streamingIO.DefineVariable<float>(
            "bpSound", {size * Nx}, {rank * Nx}, {Nx}, adios2::ConstantDims);

        adios2::Variable<float> &bpTime = streamingIO.DefineVariable<int>(
            "bpTime", {size * Nx}, {rank * Nx}, {Nx}, adios2::ConstantDims);

        /** Engine derived class, spawned to start IO operations */
        /**  Buffering is done at every Write */
        // we can choose between deferred and synch (default)
        auto bpStreamWriter =
            streamingIO.Open("myVector.bp", adios2::Mode::Write);

        /** Write variable for buffering */
        bool acquiringData = true;
        bool launchedAsync = false;
        std::future<void> advanceFuture;

        // infinite loop, unless error is detected
        while (acquiringData)
        {
            try
            {
                // can be anything depending on the data coming in
                Acquire(bpSound, mySound);
                Acquire(bpTime, myTime);

                bpStreamWriter.Write<float>(bpTime, myTime.data());
                bpStreamWriter.Write<float>(bpSound, mySound.data());

                if (launchedAsync)
                {
                    advanceFuture.get();
                }

                // if buffer is full it will flush, otherwise it only updates
                // step "batch"...bpTime and bpSound might have different "time
                // steps"
                advanceFuture = bpStreamWriter.AdvanceAsync();
                launchedAsync = true;
            }
            catch (std::exception &e)
            {
                if (rank == 0)
                {
                    std::cout << "Detected runtime error, stopping streaming\n";
                }
                acquiringData = false;
            }
        }

        // wait for last Advance if still buffering
        if (advanceFuture.valid())
        {
            advanceFuture.wait();
        }

        /** Engine becomes unreachable after Close */
        bpStreamWriter.Close();
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
