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

#include "ADIOS_CPP.h"

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int rank, nproc;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    const bool adiosDebug = true;
    adios::ADIOS adios(MPI_COMM_WORLD, adios::Verbose::INFO, adiosDebug);

    // Application variable
    float frank = (float)rank;
    std::vector<double> myDoubles = {
        frank,        frank + 0.1f, frank + 0.2f, frank + 0.3f, frank + 0.4f,
        frank + 0.5f, frank + 0.6f, frank + 0.7f, frank + 0.8f, frank + 0.9f};
    const std::size_t Nx = myDoubles.size();

    const std::size_t rows = 3;
    const std::size_t columns = 3;

    std::vector<float> myMatrix;
    myMatrix.reserve(rows * columns);
    myMatrix.push_back(frank + 0.0);
    myMatrix.push_back(frank + 0.1), myMatrix.push_back(frank + 0.2);
    myMatrix.push_back(frank + 0.3);
    myMatrix.push_back(frank + 0.4), myMatrix.push_back(frank + 0.5);
    myMatrix.push_back(frank + 0.6);
    myMatrix.push_back(frank + 0.7), myMatrix.push_back(frank + 0.8);

    frank = -(float)rank;
    std::vector<float> myMatrix2 = {frank - 0.1f, frank - 0.2f, frank - 0.3f,
                                    frank - 0.4f, frank - 0.5f, frank - 0.6f,
                                    frank - 0.7f, frank - 0.8f, frank - 0.9f};

    try
    {
        // Define variable and local size
        adios::Variable<double> &ioMyDoubles = adios.DefineVariable<double>(
            "myDoubles", {1, Nx}, {nproc, Nx}, {rank, 0});
        adios::Variable<float> &ioMyMatrix = adios.DefineVariable<float>(
            "myMatrix", {rows, columns}, {nproc * rows, columns},
            {rank * rows, 0});
        adios::Variable<float> &ioMyMatrix2 = adios.DefineVariable<float>(
            "myMatrix2", {rows, columns}, {rows, nproc * columns},
            {0, rank * columns});

        // Define method for engine creation, it is basically straight-forward
        // parameters
        adios::Method &bpWriterSettings = adios.DeclareMethod("hello");
        bpWriterSettings.SetEngine("ADIOS1Writer");
        bpWriterSettings.SetParameters("profile_units=mus");
        bpWriterSettings.SetIOMode(adios::IOMode::COLLECTIVE);
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

        bpWriter->Write<float>(ioMyMatrix, myMatrix.data());
        bpWriter->Write<float>(ioMyMatrix2, myMatrix2.data());

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
