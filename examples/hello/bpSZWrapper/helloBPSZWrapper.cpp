/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloBPSZWrapper.cpp: Simple self-descriptive example of how to write a
 * variable to a BP File that lives in several MPI processes and is compressed
 * with SZ http://www.bzip.org/
 *
 *  Created on: Jul 26, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include <ios>       //std::ios_base::failure
#include <iostream>  //std::cout
#include <numeric>   //std::iota
#include <stdexcept> //std::invalid_argument std::exception
#include <vector>

#include <adios2.h>

#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#endif

int main(int argc, char *argv[])
{

    int rank = 0, size = 1;

#ifdef ADIOS2_HAVE_MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
#endif

    /** Application variable uints from 0 to 1000 */
    std::vector<double> myvars(1000);
    std::iota(myvars.begin(), myvars.end(), 0.f);
    const std::size_t Nx = myvars.size();
    const std::size_t inputBytes = Nx * sizeof(double);

    try
    {
#ifdef ADIOS2_HAVE_MPI
        /** ADIOS class factory of IO class objects, DebugON is recommended */
        adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
        adios2::ADIOS adios(true);
#endif

        // Get a Transform of type SZ
        adios2::Operator &adiosSZ =
            adios.DefineOperator("SZVariableCompressor", "SZ");

        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines */
        adios2::IO &bpIO = adios.DeclareIO("BPFile_N2N_SZ");

        /** global array : name, { shape (total) }, { start (local) }, { count
         * (local) }, all are constant dimensions */
        adios2::Variable<double> &var = bpIO.DefineVariable<double>(
            "var", {size * Nx}, {rank * Nx}, {Nx}, adios2::ConstantDims);

        // 1st way: adding transform metadata to variable to Engine can decide:
        // &adiosSZ gets mapped to bpUInts.TransformInfo[SZID].Operator
        const unsigned int SZID = var.AddTransform(adiosSZ, {{"foo", "0.01"}});

        // 2nd way: treat Transforms as wrappers to underlying library.
        // you can redefine parameters
        const std::size_t estimatedSize =
            adiosSZ.BufferMaxSize(Nx * var.m_ElementSize);
        std::vector<char> compressedBuffer(estimatedSize);
        size_t compressedSize = adiosSZ.Compress(
            myvars.data(), var.m_Count, var.m_ElementSize, var.m_Type,
            compressedBuffer.data(), {{"accuracy", "0.01"}});

        compressedBuffer.resize(compressedSize);

        std::cout << "Rank " << rank << "\n";
        std::cout << "Compression summary:\n";
        std::cout << "Input data size: " << inputBytes << " bytes\n";
        std::cout << "SZ estimated output size: " << estimatedSize
                  << " bytes\n";
        std::cout << "SZ final output size: " << compressedSize << " bytes\n\n";

        // Allocate original data size
        std::vector<double> decompressedBuffer(Nx);
        size_t decompressedSize = adiosSZ.Decompress(
            compressedBuffer.data(), compressedSize, decompressedBuffer.data(),
            var.m_Count, var.m_Type, var.m_OperatorsInfo[SZID].Parameters);

        std::cout << "Decompression summary:\n";
        std::cout << "Decompressed size: " << decompressedSize << " bytes\n";
        std::cout << "Data:\n";

        for (int i = 0; i < decompressedBuffer.size(); i++)
        {
            if (i % 25 == 0)
            {
                std::cout << "\n";
            }
            std::cout << decompressedBuffer[i] << " ";
        }
        std::cout << "\n";
    }
    catch (std::invalid_argument &e)
    {
        std::cout << "Invalid argument exception, STOPPING PROGRAM from rank "
                  << rank << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::ios_base::failure &e)
    {
        std::cout
            << "IO System base failure exception, STOPPING PROGRAM from rank "
            << rank << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::exception &e)
    {
        std::cout << "Exception, STOPPING PROGRAM from rank " << rank << "\n";
        std::cout << e.what() << "\n";
    }

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return 0;
}
