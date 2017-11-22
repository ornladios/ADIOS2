/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloSstWriter.cpp
 *
 *  Created on: Aug 17, 2017
 *      Author: Greg Eisenhauer
 */

#include <iostream>
#include <vector>

#include <mpi.h>

#include <adios2.h>

int main(int argc, char *argv[])
{
    // Application variable
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    std::vector<float> myFloats = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    const std::size_t Nx = myFloats.size();

    try
    {
        adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
        adios2::IO &sstIO = adios.DeclareIO("WANIO");
        sstIO.SetEngine("SstWriter");
        sstIO.SetParameters({{"peer-to-peer", "yes"},
                             {"real_time", "yes"},
                             {"compress", "no"},
                             {"method", "dump"}});

        // Define variable and local size
        auto bpFloats = sstIO.DefineVariable<float>("bpFloats", {}, {}, {Nx});

        // Create engine smart pointer to Sst Engine due to polymorphism,
        // Open returns a smart pointer to Engine containing the Derived class
        adios2::Engine &sstWriter =
            sstIO.Open("helloSst.bp", adios2::Mode::Write);

        sstWriter.PutSync<float>(bpFloats, myFloats.data());
        sstWriter.Close();
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
