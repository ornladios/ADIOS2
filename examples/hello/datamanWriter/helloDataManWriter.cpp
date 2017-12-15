/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloDataManWriter.cpp
 *
 *  Created on: Feb 16, 2017
 *      Author: Jason Wang
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

    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
    adios2::IO &dataManIO = adios.DeclareIO("WANIO");
    dataManIO.SetEngine("DataMan");
    dataManIO.SetParameters({
        {"Compression", "no"}, {"Format", "bp"},
    });
    dataManIO.AddTransport("WAN",
                           {
                               {"Library", "ZMQ"}, {"IPAddress", "127.0.0.1"},
                           });

    // Define variable and local size
    auto bpFloats = dataManIO.DefineVariable<float>("bpFloats", {}, {}, {Nx});

    // Create engine smart pointer to DataMan Engine due to polymorphism,
    // Open returns a smart pointer to Engine containing the Derived class
    adios2::Engine &dataManWriter =
        dataManIO.Open("myFloats.bp", adios2::Mode::Write);

    dataManWriter.PutSync<float>(bpFloats, myFloats.data());
    dataManWriter.Close();
    try
    {
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
