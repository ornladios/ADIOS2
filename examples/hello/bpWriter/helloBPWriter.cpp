/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloWriter.cpp
 *
 *  Created on: Feb 16, 2017
 *      Author: wfg
 */

#include <iostream>
#include <vector>

#include <mpi.h>

#include <adios2.h>

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const bool adiosDebug = true;
    adios::ADIOS adios(MPI_COMM_WORLD, adios::Verbose::INFO, adiosDebug);

    // Application variable
    std::vector<double> myDoubles = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    const std::size_t Nx = myDoubles.size();

    const std::size_t rows = 3;
    const std::size_t columns = 3;

    std::vector<float> myMatrix;
    if (rank % 2 == 0) // even rank
    {
        myMatrix.reserve(rows * columns);
        myMatrix.push_back(1);
        myMatrix.push_back(2), myMatrix.push_back(3);
        myMatrix.push_back(4);
        myMatrix.push_back(5), myMatrix.push_back(6);
        myMatrix.push_back(7);
        myMatrix.push_back(8), myMatrix.push_back(8);
    }

    std::vector<float> myMatrix2 = {-1, -2, -3, -4, -5, -6, -7, -8, -9};

    try
    {
        // Define variable and local size
        adios::Variable<double> &ioMyDoubles =
            adios.DefineLocalArray<double>("myDoubles", true, {Nx});

        adios::Variable<float> &ioMyMatrix =
            adios.DefineLocalArray<float>("myMatrix", true, {rows, columns});

        adios::Variable<float> &ioMyMatrix2 =
            adios.DefineLocalArray<float>("myMatrix2", false, {rows, columns});

        // Define method for engine creation, it is basically straight-forward
        // parameters
        adios::Method &bpWriterSettings = adios.DeclareMethod(
            "SingleFile"); // default method type is BPWriter
        bpWriterSettings.SetParameters("profile_units=mus");
        bpWriterSettings.AddTransport(
            "File", "profile_units=mus",
            "have_metadata_file=no"); // uses default POSIX library

        // Create engine smart pointer due to polymorphism,
        // Open returns a smart pointer to Engine containing the Derived class
        // Writer
        auto bpWriter = adios.Open("myDoubles.bp", "w", bpWriterSettings);

        if (bpWriter == nullptr)
            throw std::ios_base::failure(
                "ERROR: couldn't create bpWriter at Open\n");

        bpWriter->Write<double>(ioMyDoubles,
                                myDoubles.data()); // Base class Engine
                                                   // own the Write<T>
                                                   // that will call
                                                   // overloaded Write
                                                   // from Derived

        if (rank % 2 == 0) // even rank
        {
            bpWriter->Write<float>(ioMyMatrix, myMatrix.data());
            bpWriter->Write<float>(ioMyMatrix2, myMatrix2.data());
        }

        bpWriter->Close();
    }
    catch (std::invalid_argument &e)
    {
        if (rank == 0)
        {
            std::cout << "Invalid argument exception, STOPPING PROGRAM\n";
            std::cout << e.what() << "\n";
        }
    }
    catch (std::ios_base::failure &e)
    {
        if (rank == 0)
        {
            std::cout << "System exception, STOPPING PROGRAM\n";
            std::cout << e.what() << "\n";
        }
    }
    catch (std::exception &e)
    {
        if (rank == 0)
        {
            std::cout << "Exception, STOPPING PROGRAM\n";
            std::cout << e.what() << "\n";
        }
    }

    MPI_Finalize();

    return 0;
}
