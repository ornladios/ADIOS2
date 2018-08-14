/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloDataManSubscribeWriter.cpp
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

// adios2 dataman configurations
std::string adiosEngine = "DataMan";
std::string workflowMode = "subscribe";
std::vector<adios2::Params> transportParams = {
    {{"Library", "ZMQ"}, {"IPAddress", "127.0.0.1"}, {"Port", "12306"}},
    {{"Library", "ZMQ"},
     {"IPAddress", "127.0.0.1"},
     {"Port", "12307"},
     {"CompressionMethod", "sz"},
     {"sz:accuracy", "10"}},
    {{"Library", "ZMQ"},
     {"IPAddress", "127.0.0.1"},
     {"Port", "12308"},
     {"CompressionMethod", "zfp"},
     {"zfp:rate", "4"}}};

// data properties
size_t steps = 1000;
adios2::Dims shape({10, 10});
adios2::Dims start({0, 0});
adios2::Dims count({6, 8});

int rank, size;

template <class T>
void Dump(std::vector<T> &data, size_t step)
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
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // initialize data
    size_t datasize = std::accumulate(count.begin(), count.end(), 1,
                                      std::multiplies<size_t>());
    std::vector<float> myFloats(datasize);
    for (size_t i = 0; i < datasize; ++i)
    {
        myFloats[i] = i + rank * 10000;
    }

    // initialize ADIOS2 with DataMan
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
    adios2::IO dataManIO = adios.DeclareIO("WAN");
    dataManIO.SetEngine(adiosEngine);
    dataManIO.SetParameters({{"WorkflowMode", workflowMode}});
    for (const auto &i : transportParams)
    {
        dataManIO.AddTransport("WAN", i);
    }

    // open stream
    adios2::Engine dataManWriter =
        dataManIO.Open("stream", adios2::Mode::Write);

    // define variable
    auto bpFloats =
        dataManIO.DefineVariable<float>("bpFloats", shape, start, count);

    // write data
    for (int i = 0; i < steps; ++i)
    {
        dataManWriter.BeginStep();
        dataManWriter.Put<float>(bpFloats, myFloats.data(), adios2::Mode::Sync);
        Dump(myFloats, dataManWriter.CurrentStep());
        dataManWriter.EndStep();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        for (auto &j : myFloats)
        {
            j += 1;
        }
    }

    // finalize
    dataManWriter.Close();
    MPI_Finalize();

    return 0;
}
