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

// adios2 dataman configurations
std::string adiosEngine = "DataMan";
std::string workflowMode = "p2p";
std::vector<adios2::Params> transportParams = {{{"Library", "ZMQ"},
                                                {"IPAddress", "127.0.0.1"},
                                                {"Port", "12306"},
                                                {"Timeout", "5"}}};

// data properties
size_t steps = 100;
adios2::Dims start({2, 3});
adios2::Dims count({2, 3});

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
    adios2::Engine dataManReader = dataManIO.Open("stream", adios2::Mode::Read);

    // read data
    size_t i = 0;
    adios2::Variable<float> bpFloats;
    auto start_time = std::chrono::system_clock::now();
    adios2::StepStatus status;
    for (int i = 0; i < steps; ++i)
    {
        status = dataManReader.BeginStep();
        if (status == adios2::StepStatus::OK)
        {
            bpFloats = dataManIO.InquireVariable<float>("bpFloats");
            bpFloats.SetSelection({start, count});
            dataManReader.Get<float>(bpFloats, myFloats.data(),
                                     adios2::Mode::Sync);
            Dump(myFloats, dataManReader.CurrentStep());
            dataManReader.EndStep();
        }
        else if (status == adios2::StepStatus::EndOfStream)
        {
            std::cout << "EndOfStream\n";
            break;
        }
    }

    // finalize
    dataManReader.Close();
    MPI_Finalize();

    return 0;
}
