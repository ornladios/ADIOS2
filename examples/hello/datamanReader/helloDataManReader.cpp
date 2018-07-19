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

std::string ip = "127.0.0.1";
std::string port = "12306";

adios2::Dims start({2, 3});
adios2::Dims count({2, 3});
size_t steps = 10;
int timeout = 20;

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

    // initialize ADIOS 2

    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
    adios2::IO dataManIO = adios.DeclareIO("WAN");

    dataManIO.SetEngine("DataMan");
    dataManIO.SetParameters({{"WorkflowMode", "subscribe"}});

    dataManIO.AddTransport(
        "WAN", {{"Library", "ZMQ"}, {"IPAddress", ip}, {"Port", port}});

    adios2::Engine dataManReader = dataManIO.Open("stream", adios2::Mode::Read);

    adios2::Variable<float> bpFloats;

    // read data
    size_t i = 0;
    auto start_time = std::chrono::system_clock::now();
    while (i < steps)
    {
        auto now_time = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
                now_time - start_time);
        if (duration.count() > timeout)
        {
            std::cout << "Timeout" << std::endl;
            return -1;
        }
        adios2::StepStatus status = dataManReader.BeginStep();
        if (status == adios2::StepStatus::OK)
        {
            bpFloats = dataManIO.InquireVariable<float>("bpFloats");
            bpFloats.SetSelection({start, count});
            dataManReader.Get<float>(bpFloats, myFloats.data(),
                                     adios2::Mode::Sync);
            Dump(myFloats, dataManReader.CurrentStep());
            i = dataManReader.CurrentStep();
            dataManReader.EndStep();
        }
        else if (status == adios2::StepStatus::NotReady)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        else if (status == adios2::StepStatus::EndOfStream)
        {
            break;
        }
    }

    // finalize
    dataManReader.Close();
    MPI_Finalize();

    return 0;
}
