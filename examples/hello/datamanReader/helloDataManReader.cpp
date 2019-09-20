/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloDataManReader.cpp
 *
 *  Created on: Jan 9, 2017
 *      Author: Jason Wang
 */

#include <adios2.h>
#include <chrono>
#include <iostream>
#include <mpi.h>
#include <numeric>
#include <thread>
#include <vector>

int mpiRank, mpiSize;

template <class T>
void PrintData(std::vector<T> &data, size_t step)
{
    std::cout << "Rank: " << mpiRank << " Step: " << step << " [";
    for (size_t i = 0; i < data.size(); ++i)
    {
        std::cout << data[i] << " ";
    }
    std::cout << "]" << std::endl;
}

int main(int argc, char *argv[])
{
    // initialize MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);

    // initialize adios2
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
    adios2::IO dataManIO = adios.DeclareIO("whatever");
    dataManIO.SetEngine("DataMan");
    dataManIO.SetParameters(
        {{"IPAddress", "127.0.0.1"}, {"Port", "12306"}, {"Timeout", "5"}});

    // open stream
    adios2::Engine dataManReader =
        dataManIO.Open("HelloDataMan", adios2::Mode::Read);

    // define variable
    adios2::Variable<float> floatArrayVar;

    // read data
    std::vector<float> floatVector;
    while (true)
    {
        auto status = dataManReader.BeginStep();
        if (status == adios2::StepStatus::OK)
        {
            floatArrayVar = dataManIO.InquireVariable<float>("FloatArray");
            auto shape = floatArrayVar.Shape();
            size_t datasize = std::accumulate(shape.begin(), shape.end(), 1,
                                              std::multiplies<size_t>());
            floatVector.resize(datasize);
            dataManReader.Get<float>(floatArrayVar, floatVector.data(),
                                     adios2::Mode::Sync);
            dataManReader.EndStep();
            PrintData(floatVector, dataManReader.CurrentStep());
        }
        else if (status == adios2::StepStatus::EndOfStream)
        {
            std::cout << "End of stream" << std::endl;
            break;
        }
    }

    // clean up
    dataManReader.Close();
    MPI_Finalize();

    return 0;
}
