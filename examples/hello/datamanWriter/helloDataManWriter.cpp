/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloDataManWriter.cpp
 *
 *  Created on: Feb 16, 2017
 *      Author: Jason Wang
 */

#include <adios2.h>
#include <iostream>
#include <mpi.h>
#include <numeric>
#include <thread>
#include <vector>

size_t steps = 10000;
adios2::Dims shape({10, 10});
adios2::Dims start({0, 0});
adios2::Dims count({6, 8});

int rank, size;

template <class T>
void PrintData(std::vector<T> &data, const size_t step)
{
    std::cout << "Rank: " << rank << " Step: " << step << " [";
    for (const auto i : data)
    {
        std::cout << i << " ";
    }
    std::cout << "]" << std::endl;
}

template <class T>
std::vector<T> GenerateData(const size_t step)
{
    size_t datasize = std::accumulate(count.begin(), count.end(), 1,
                                      std::multiplies<size_t>());
    std::vector<T> myVec(datasize);
    for (size_t i = 0; i < datasize; ++i)
    {
        myVec[i] = i + rank * 10000 + step;
    }
    return myVec;
}

int main(int argc, char *argv[])
{
    // initialize MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // initialize adios2
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
    adios2::IO dataManIO = adios.DeclareIO("WAN");
    dataManIO.SetEngine("DataMan");
    dataManIO.SetParameters(
        {{"IPAddress", "127.0.0.1"}, {"Port", "12306"}, {"Timeout", "5"}});

    // open stream
    adios2::Engine dataManWriter =
        dataManIO.Open("myFloats.bp", adios2::Mode::Write);

    // define variable
    auto bpFloats =
        dataManIO.DefineVariable<float>("bpFloats", shape, start, count);

    // write data
    for (size_t i = 0; i < steps; ++i)
    {
        auto myFloats = GenerateData<float>(i);
        dataManWriter.BeginStep();
        dataManWriter.Put(bpFloats, myFloats.data(), adios2::Mode::Sync);
        PrintData(myFloats, dataManWriter.CurrentStep());
        dataManWriter.EndStep();
    }

    dataManWriter.Close();
    MPI_Finalize();

    return 0;
}
