/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileDeferredWrite.cpp
 *
 *  Created on: Sep 22, 2017
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
    std::vector<float> myFloats = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::vector<int> myInts = {0, -1, -2, -3, -4, -5, -6, -7, -8, -9};
    const std::size_t Nx = myFloats.size();

    try
    {
        /** ADIOS class factory of IO class objects, DebugON is recommended */
        adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);

        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines */
        adios2::IO &bpIO = adios.DeclareIO("BPFile_N2N");

        /// Setting flush policy
        // the user can set the memory explicitly for ADIOS (more advance users)
        bpIO.SetSingleParameter("MaxBufferSize", "100Mb");
        // or by number of steps (naively trying to allocate "N=3" steps before
        // flushing
        // bpIO.SetSingleParameter("StepsToBuffer", "3");

        /** global array : name, { shape (total) }, { start (local) }, {
         * count
         * (local) }, all are constant dimensions */
        adios2::Variable<float> &bpFloats = bpIO.DefineVariable<float>(
            "bpFloats", {size * Nx}, {rank * Nx}, {Nx}, adios2::ConstantDims);

        adios2::Variable<int> &bpInts = bpIO.DefineVariable<int>(
            "bpInts", {size * Nx}, {rank * Nx}, {Nx}, adios2::ConstantDims);

        /** Engine derived class, spawned to start IO operations */
        /** Deferred Mode: Write buffering will be done at Put(), the user makes
         *  the commitment that pointers passed at Write won't be reused */
        auto bpWriter = bpIO.Open("myVector.bp",
                                  adios2::Mode::Write | adios2::Mode::Deferred);

        // default = 0, in bp1 format this is translated to 1, but user should
        // be able to start at any step
        bpWriter.SetStep(1);

        /** Write variable for buffering */
        for (unsigned int t = 0; t < 100; ++t)
        {
            bpWriter.Write<float>(bpFloats, myFloats.data());
            bpWriter.Write<int>(bpInts, myInts.data());
            bpWriter.Advance(); // advances step

            if (t % 10 == 0) // checkpoint/restart every 10 steps, force flush
            {
                bpWriter.Flush();
            }
        }

        /** Engine becomes unreachable after Close */
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
