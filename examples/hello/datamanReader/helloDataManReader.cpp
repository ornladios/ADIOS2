/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloDataManReader_nompi.cpp
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

adios2::Dims start({2, 3});
adios2::Dims count({2, 3});

int rank, size;

template <class T>
void PrintData(std::vector<T> &data, size_t step)
{
    std::cout << "Rank: " << rank << " Step: " << step << " [";
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
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // initialize data
    size_t datasize = std::accumulate(count.begin(), count.end(), 1,
                                      std::multiplies<size_t>());
    std::vector<float> myFloats(datasize);

    // initialize adios2
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
    adios2::IO dataManIO = adios.DeclareIO("WAN");
    dataManIO.SetEngine("DataMan");
    dataManIO.SetParameters({{{"Library", "ZMQ"},
                              {"IPAddress", "127.0.0.1"},
                              {"Port", "12306"},
                              {"Timeout", "5"}}});

    // open stream
    adios2::Engine dataManReader = dataManIO.Open("stream", adios2::Mode::Read);

    // define variable
    adios2::Variable<float> bpFloats;

    // read data
    while (true)
    {
        auto status = dataManReader.BeginStep();
        if (status == adios2::StepStatus::OK)
        {
            bpFloats = dataManIO.InquireVariable<float>("bpFloats");
            bpFloats.SetSelection({start, count});
            dataManReader.Get<float>(bpFloats, myFloats.data(),
                                     adios2::Mode::Sync);
            PrintData(myFloats, dataManReader.CurrentStep());
            dataManReader.EndStep();
        }
        else if (status == adios2::StepStatus::EndOfStream)
        {
            std::cout << "End of stream" << std::endl;
            break;
        }
    }

    dataManReader.Close();
    MPI_Finalize();

    return 0;
}
