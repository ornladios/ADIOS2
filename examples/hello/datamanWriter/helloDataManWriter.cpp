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

size_t steps = 100;
std::string ip = "127.0.0.1";
std::string port = "12306";

adios2::Dims shape({200, 640, 512});
adios2::Dims start({0, 0, 0});
adios2::Dims count({200, 60, 80});

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

    // initialize ADIOS 2

    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
    adios2::IO dataManIO = adios.DeclareIO("WAN");

    dataManIO.SetEngine("DataMan");
    dataManIO.SetParameters({{"WorkflowMode", "subscribe"}});
    dataManIO.AddTransport(
        "WAN", {{"Library", "ZMQ"}, {"IPAddress", ip}, {"Port", port}});

    auto bpFloats =
        dataManIO.DefineVariable<float>("bpFloats", shape, start, count);

    adios2::Engine dataManWriter =
        dataManIO.Open("myFloats.bp", adios2::Mode::Write);

    // write data

    for (int i = 0; i < steps; ++i)
    {
        dataManWriter.BeginStep();
        // start[0] = 0;
        // bpFloats.SetSelection({start, count});
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
