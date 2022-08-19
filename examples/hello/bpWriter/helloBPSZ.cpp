/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloBPSZ.cpp : example passing runtime compression arguments
 *
 *  Created on: Aug 3, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include <ios>       //std::ios_base::failure
#include <iostream>  //std::cout
#include <numeric>   //std::iota
#include <stdexcept> //std::invalid_argument std::exception
#include <vector>

#include <adios2.h>
#if ADIOS2_USE_MPI
#include <mpi.h>
#endif

void Usage()
{
    std::cout << "\n";
    std::cout << "USAGE:\n";
    std::cout << "./helloBPSZ Nx sz_accuracy\n";
    std::cout << "\t Nx: size of float and double arrays to be compressed\n";
    std::cout << "\t sz_accuracy: absolute accuracy e.g. 0.1, 0.001, to skip "
                 "compression: -1\n\n";
}

int main(int argc, char *argv[])
{
    int rank, size;

#if ADIOS2_USE_MPI
    int provided;

    // MPI_THREAD_MULTIPLE is only required if you enable the SST MPI_DP
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
#else
    rank = 0;
    size = 1;
#endif

    try
    {
        if (argc != 3)
        {
            throw std::invalid_argument(
                "ERROR: need sz accuracy e.g. 0.01, 0.1 as "
                "2nd parameter in argv\n");
        }

        const std::size_t Nx = static_cast<std::size_t>(std::stoull(argv[1]));
        const double accuracy = std::stod(argv[2]);

        /** Application variable */
        std::vector<float> myFloats(Nx);
        std::vector<double> myDoubles(Nx);
        std::iota(myFloats.begin(), myFloats.end(), 0.);
        std::iota(myDoubles.begin(), myDoubles.end(), 0.);

        /** ADIOS class factory of IO class objects */
#if ADIOS2_USE_MPI
        adios2::ADIOS adios(MPI_COMM_WORLD);
#else
        adios2::ADIOS adios;
#endif

        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines */
        adios2::IO bpIO = adios.DeclareIO("BPFile_SZ");

        adios2::Variable<float> varFloats = bpIO.DefineVariable<float>(
            "bpFloats", {size * Nx}, {rank * Nx}, {Nx}, adios2::ConstantDims);

        adios2::Variable<double> varDoubles = bpIO.DefineVariable<double>(
            "bpDoubles", {size * Nx}, {rank * Nx}, {Nx}, adios2::ConstantDims);

        if (accuracy > 1E-16)
        {
            adios2::Operator op = adios.DefineOperator("SZCompressor", "sz");
            varFloats.AddOperation(op,
                                   {{"accuracy", std::to_string(accuracy)}});
            varDoubles.AddOperation(op,
                                    {{"accuracy", std::to_string(accuracy)}});
        }

        adios2::Attribute<double> attribute =
            bpIO.DefineAttribute<double>("SZ_accuracy", accuracy);

        // To avoid compiling warnings
        (void)attribute;

        /** Engine derived class, spawned to start IO operations */
        adios2::Engine bpFileWriter =
            bpIO.Open("SZexample.bp", adios2::Mode::Write);

        for (unsigned int t = 0; t < 3; ++t)
        {
            bpFileWriter.BeginStep();

            // here you can modify myFloats, myDoubles per step

            bpFileWriter.Put(varFloats, myFloats.data());
            bpFileWriter.Put(varDoubles, myDoubles.data());
            bpFileWriter.EndStep();
        }

        /** Create bp file, engine becomes unreachable after this*/
        bpFileWriter.Close();
    }
    catch (std::invalid_argument &e)
    {
        std::cerr << "Invalid argument exception: " << e.what() << "\n";
#if ADIOS2_USE_MPI
        std::cerr << "STOPPING PROGRAM from rank " << rank << "\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
#endif
    }
    catch (std::ios_base::failure &e)
    {
        std::cerr << "IO System base failure exception: " << e.what() << "\n";
#if ADIOS2_USE_MPI
        std::cerr << "STOPPING PROGRAM from rank " << rank << "\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
#endif
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
#if ADIOS2_USE_MPI
        std::cerr << "STOPPING PROGRAM from rank " << rank << "\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
#endif
    }

#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif

    return 0;
}
