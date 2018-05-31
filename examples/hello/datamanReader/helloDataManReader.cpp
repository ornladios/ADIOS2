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

int rank, size;
std::string ip = "127.0.0.1";
int port = 12306;

void Dump(std::vector<float> &data, size_t step)
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

    int nThreads = 1;

    try
    {
        if (argc == 2)
        {
            nThreads = atoi(argv[1]);
        }

        adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);

        adios2::IO &dataManIO = adios.DeclareIO("WAN");
        dataManIO.SetEngine("DataMan");
        dataManIO.SetParameters({{"Blocking", "no"}});

        for (int i = 0; i < nThreads; ++i)
        {
            int port_thread = port + i;
            dataManIO.AddTransport("WAN",
                                   {{"Library", "ZMQ"},
                                    {"IPAddress", ip},
                                    {"Port", std::to_string(port_thread)}});
        }

        adios2::Engine &dataManReader =
            dataManIO.Open("stream", adios2::Mode::Read);

        adios2::Variable<float> *bpFloats;

        std::vector<float> myFloats(10);

        for (int i = 0; i < 1000; ++i)
        {
            adios2::StepStatus status = dataManReader.BeginStep();
            if (status == adios2::StepStatus::OK)
            {
                bpFloats = dataManIO.InquireVariable<float>("bpFloats");
                dataManReader.Get<float>(*bpFloats, myFloats.data(),
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

        dataManReader.Close();
    }
    catch (std::invalid_argument &e)
    {
        std::cout << "Invalid argument exception, STOPPING PROGRAM\n";
        std::cout << e.what() << "\n";
    }
    catch (std::ios_base::failure &e)
    {
        std::cout << "System exception, STOPPING PROGRAM\n";
        std::cout << e.what() << "\n";
    }
    catch (std::exception &e)
    {
        std::cout << "Exception, STOPPING PROGRAM\n";
        std::cout << e.what() << "\n";
    }

    MPI_Finalize();

    return 0;
}
