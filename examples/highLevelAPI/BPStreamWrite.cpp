/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloBPWriteStep.cpp  example for writing a variable using the WriteStep
 * function for time aggregation. Time step is saved as an additional (global)
 * single value variable, just for tracking purposes.
 *
 *  Created on: Oct 27, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include <algorithm> //std::for_each
#include <ios>       //std::ios_base::failure
#include <iostream>  //std::cout
#include <mpi.h>
#include <stdexcept> //std::invalid_argument std::exception
#include <vector>

#include <fstream> //ofstream for comparison with adios2 API

#include <adios2.h>

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Application variable
    std::vector<float> myFloats = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::vector<int> myInts = {0, -1, -2, -3, -4, -5, -6, -7, -8, -9};
    const std::size_t Nx = myFloats.size();

    try
    {
        adios2::fstream adios2File("Simple.bp", adios2::fstream::out,
                                   MPI_COMM_WORLD);

        std::fstream binFile("Simple.bin." + std::to_string(rank),
                             std::fstream::out | std::fstream::binary);

        for (unsigned int timeStep = 0; timeStep < 10; ++timeStep)
        {
            myFloats[0] = static_cast<float>(timeStep);
            myInts[0] = static_cast<int>(timeStep);

            // adios2
            {
                adios2File.write("myFloats", myFloats.data(), {size * Nx},
                                 {rank * Nx}, {Nx});
                adios2File.write("myInts", myInts.data(), {size * Nx},
                                 {rank * Nx}, {Nx}, adios2::endl);
            }

            // fstream
            {
                binFile.write("myFloats", 8);
                binFile.write(reinterpret_cast<char *>(myFloats.data()),
                              myFloats.size() * sizeof(float));
                binFile.write(reinterpret_cast<char *>(
                                  std::vector<std::size_t>({size * Nx}).data()),
                              1 * sizeof(size_t));
                binFile.write(reinterpret_cast<char *>(
                                  std::vector<std::size_t>({rank * Nx}).data()),
                              1 * sizeof(size_t));
                binFile.write(reinterpret_cast<char *>(
                                  std::vector<std::size_t>({Nx}).data()),
                              1 * sizeof(size_t));

                binFile.write("myInts", 8);
                binFile.write(reinterpret_cast<char *>(myFloats.data()),
                              myFloats.size() * sizeof(float));
                binFile.write(reinterpret_cast<char *>(
                                  std::vector<std::size_t>({size * Nx}).data()),
                              1 * sizeof(size_t));
                binFile.write(reinterpret_cast<char *>(
                                  std::vector<std::size_t>({rank * Nx}).data()),
                              1 * sizeof(size_t));
                binFile.write(reinterpret_cast<char *>(
                                  std::vector<std::size_t>({Nx}).data()),
                              1 * sizeof(size_t));
                binFile.write("\n", 2);
            }
        }

        binFile.close();
        adios2File.close();
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
