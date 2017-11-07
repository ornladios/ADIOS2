/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileTransientDeferredWrite.cpp : deferred write in a transient (time
 * aggregation) loop. Buffering is done at Flush
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

        /** global array : name, { shape (total) }, { start (local) }, {
         * count
         * (local) }, all are constant dimensions */
        adios2::Variable<float> &bpFloats = bpIO.DefineVariable<float>(
            "bpFloats", {size * Nx}, {rank * Nx}, {Nx}, adios2::ConstantDims);

        adios2::Variable<int> &bpInts = bpIO.DefineVariable<int>(
            "bpInts", {size * Nx}, {rank * Nx}, {Nx}, adios2::ConstantDims);

        /** Engine derived class, spawned to start IO operations */
        /** Deferred Mode: Write buffering will be done at Flush().
         *  The user must be the one enforcing the commitment that pointers
         * passed at Write won't be reused between Write and Flush */
        auto bpWriter = bpIO.Open("myVector.bp",
                                  adios2::Mode::Write | adios2::Mode::Deferred);

        for (unsigned int t = 0; t < 100; ++t)
        {

            /** Write variable for buffering */
            bpWriter.Write<float>(bpFloats, myFloats.data());
            bpWriter.Write<int>(bpInts, myInts.data());
            bpWriter.Flush();
        }

        /** Create bp file, engine becomes unreachable after this*/
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
