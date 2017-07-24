/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloBPZfpWrapper.cpp: Simple self-descriptive example of how to write a
 * variable to a BP File that lives in several MPI processes and is compressed
 * with Zfp https://computation.llnl.gov/projects/floating-point-compression
 *
 *  Created on: Jul 26, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */
#include <mpi.h>

#include <cstdint>   //std::int32_t
#include <ios>       //std::ios_base::failure
#include <iostream>  //std::cout
#include <numeric>   //std::iota
#include <stdexcept> //std::invalid_argument std::exception
#include <vector>

#include <adios2.h>

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /** Application variable uints from 0 to 100 */
    std::vector<float> myFloats(100);
    std::iota(myFloats.begin(), myFloats.end(), 0.f);
    const std::size_t Nx = myFloats.size();
    const std::size_t inputBytes = Nx * sizeof(float);

    try
    {
        /** ADIOS class factory of IO class objects, DebugON is recommended */
        adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);

        // Get a Transform of type BZip2
        adios2::Transform &adiosZfp = adios.GetTransform("zfp");

        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines */
        adios2::IO &bpIO = adios.DeclareIO("BPFile_N2N_Zfp");

        /** global array : name, { shape (total) }, { start (local) }, { count
         * (local) }, all are constant dimensions */
        adios2::Variable<float> &bpFloats = bpIO.DefineVariable<float>(
            "bpFloats", {size * Nx}, {rank * Nx}, {Nx}, adios2::ConstantDims);

        // 1st way: adding transform metadata to variable to Engine can decide:
        const unsigned int zfpID =
            bpFloats.AddTransform(adiosZfp, {{"Rate", "8"}});

        // 2nd way: treat Transforms as wrappers to underlying library:
        const std::size_t estimatedSize =
            adiosZfp.BufferMaxSize(myFloats.data(), bpFloats.m_Count,
                                   bpFloats.m_TransformsInfo[zfpID].Parameters);

        // Compress
        std::vector<char> compressedBuffer(estimatedSize);

        size_t compressedSize = adiosZfp.Compress(
            myFloats.data(), bpFloats.m_Count, bpFloats.m_ElementSize,
            bpFloats.m_Type, compressedBuffer.data(),
            bpFloats.m_TransformsInfo[zfpID].Parameters);

        compressedBuffer.resize(compressedSize);

        std::cout << "Compression summary:\n";
        std::cout << "Input data size: " << inputBytes << " bytes\n";
        std::cout << "Zfp estimated output size: " << estimatedSize
                  << " bytes\n";
        std::cout << "Zfp final output size: " << compressedSize
                  << " bytes\n\n";

        // Allocate original data size
        std::vector<float> decompressedBuffer(Nx);
        size_t decompressedSize = adiosZfp.Decompress(
            compressedBuffer.data(), compressedBuffer.size(),
            decompressedBuffer.data(), bpFloats.m_Count, bpFloats.m_Type,
            bpFloats.m_TransformsInfo[zfpID].Parameters);

        std::cout << "Decompression summary:\n";
        std::cout << "Decompressed size: " << decompressedSize << " bytes\n ";
        std::cout << "Data:\n";

        for (const auto number : decompressedBuffer)
        {
            if (static_cast<int>(number) % 25 == 0 && number != 0)
            {
                std::cout << "\n";
            }
            std::cout << number << " ";
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

    MPI_Finalize();

    return 0;
}
