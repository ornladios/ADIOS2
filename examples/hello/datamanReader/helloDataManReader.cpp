/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloDataManReader_nompi.cpp
 *
 *  Created on: Jan 9, 2017
 *      Author: Jason Wang
 */

#include <chrono>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

#include <mpi.h>

#include <adios2.h>

void Dump(std::vector<float> &data)
{
    for (size_t i = 0; i < data.size(); ++i)
    {
        std::cout << data[i] << " ";
    }
    std::cout << std::endl;
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    try
    {
        adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);

        adios2::IO &dataManIO = adios.DeclareIO("WAN");
        dataManIO.SetEngine("DataMan");
        dataManIO.SetParameters({
            {"Blocking", "no"}, {"Format", "bp"},
        });
        dataManIO.AddTransport(
            "WAN", {
                       {"Library", "ZMQ"}, {"IPAddress", "127.0.0.1"},
                   });
        adios2::Engine &dataManReader =
            dataManIO.Open("stream", adios2::Mode::Read);

        adios2::Variable<float> *bpFloats;

        std::vector<float> myFloats(10);

        bpFloats = dataManIO.InquireVariable<float>("bpFloats");
        while (bpFloats == nullptr)
        {
            std::cout << "Variable bpFloats not read yet. Waiting...\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            bpFloats = dataManIO.InquireVariable<float>("bpFloats");
        }

        for (int i = 0; i < 100000; ++i)
        {
            adios2::StepStatus status = dataManReader.BeginStep();
            if (status == adios2::StepStatus::OK)
            {
                dataManReader.GetSync<float>(*bpFloats, myFloats.data());
                Dump(myFloats);
                dataManReader.EndStep();
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

    return 0;
}
