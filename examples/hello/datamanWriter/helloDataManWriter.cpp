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
#include <vector>

int rank, size;
size_t steps = 10000;
std::string ip = "127.0.0.1";
int port = 22306;

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

    std::vector<float> myFloats = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    const std::size_t Nx = myFloats.size();

    try
    {

        adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
        adios2::IO &dataManIO = adios.DeclareIO("WANIO");
        dataManIO.SetEngine("DataMan");
        dataManIO.SetParameters({{"Blocking", "no"}});

        dataManIO.AddTransport("WAN", {{"Library", "ZMQ"},
                                       {"IPAddress", ip},
                                       {"Port", std::to_string(port)}});

        auto bpFloats =
            dataManIO.DefineVariable<float>("bpFloats", {}, {}, {Nx});

        adios2::Engine &dataManWriter =
            dataManIO.Open("myFloats.bp", adios2::Mode::Write);

        for (int i = 0; i < steps; ++i)
        {
            std::vector<float> myFloats_rank = myFloats;
            for (auto &j : myFloats_rank)
            {
                j += rank * 10000;
            }
            dataManWriter.BeginStep();
            dataManWriter.PutSync<float>(bpFloats, myFloats_rank.data());
            Dump(myFloats_rank, dataManWriter.CurrentStep());
            dataManWriter.EndStep();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            for (auto &j : myFloats)
            {
                j += 1;
            }
        }

        dataManWriter.Close();
    }
    catch (std::invalid_argument &e)
    {
        std::cout << "Invalid argument exception, STOPPING PROGRAM from rank "
                  << rank << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::ios_base::failure &e)
    {
        std::cout
            << "IO System base failure exception, STOPPING PROGRAM from rank "
            << rank << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::exception &e)
    {
        std::cout << "Exception, STOPPING PROGRAM from rank " << rank << "\n";
        std::cout << e.what() << "\n";
    }

    MPI_Finalize();

    return 0;
}
