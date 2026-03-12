/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <algorithm> //std::for_each
#include <array>
#include <chrono>
#include <ios>       //std::ios_base::failure
#include <iostream>  //std::cout
#include <stdexcept> //std::invalid_argument std::exception
#include <string>
#include <thread>
#include <vector>

#include <adios2.h>

std::string DimsToString(const adios2::Dims &dims)
{
    std::string s = "\"";
    for (size_t i = 0; i < dims.size(); i++)
    {
        if (i > 0)
        {
            s += ", ";
        }
        s += std::to_string(dims[i]);
    }
    s += "\"";
    return s;
}

void ReadVariable(const std::string &name, adios2::IO &io, adios2::Engine &reader, size_t step)
{
    adios2::Variable<double> variable = io.InquireVariable<double>(name);

    if (variable)
    {
        auto blocksInfo = reader.BlocksInfo(variable, step);

        std::cout << "    " << name << " has " << blocksInfo.size() << " blocks in step " << step
                  << std::endl;

        // create a data vector for each block
        std::vector<std::vector<double>> dataSet;
        dataSet.resize(blocksInfo.size());

        // schedule a read operation for each block separately
        int i = 0;
        for (auto &info : blocksInfo)
        {
            variable.SetBlockSelection(info.BlockID);
            reader.Get<double>(variable, dataSet[i], adios2::Mode::Deferred);
            ++i;
        }

        // Read in all blocks at once now
        reader.PerformGets();
        // data vectors now are filled with data

        i = 0;
        for (const auto &info : blocksInfo)
        {
            std::cout << "        block " << info.BlockID << " size = " << DimsToString(info.Count)
                      << " offset = " << DimsToString(info.Start) << " : ";

            for (const auto datum : dataSet[i])
            {
                std::cout << datum << " ";
            }
            std::cout << std::endl;
            ++i;
        }
    }
    else
    {
        std::cout << "    Variable " << name << " not found in step " << step << std::endl;
    }
}

int main(int argc, char *argv[])
{
    try
    {
        adios2::ADIOS adios;

        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines
         * Inline uses single IO for write/read */
        adios2::IO io = adios.DeclareIO("Input");

        io.SetParameters({{"verbose", "4"}});

        adios2::Engine reader = io.Open("localArray.bp", adios2::Mode::Read);

        while (true)
        {

            // Begin step
            adios2::StepStatus read_status = reader.BeginStep(adios2::StepMode::Read, 10.0f);
            if (read_status == adios2::StepStatus::NotReady)
            {
                // std::cout << "Stream not ready yet. Waiting...\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                continue;
            }
            else if (read_status != adios2::StepStatus::OK)
            {
                break;
            }

            size_t step = reader.CurrentStep();
            std::cout << "Process step " << step << ": " << std::endl;

            ReadVariable("v0", io, reader, step);
            ReadVariable("v1", io, reader, step);
            ReadVariable("v2", io, reader, step);
            ReadVariable("v3", io, reader, step);

            reader.EndStep();
        }

        reader.Close();
    }
    catch (std::invalid_argument &e)
    {
        std::cout << "Invalid argument exception, STOPPING PROGRAM\n";
        std::cout << e.what() << "\n";
    }
    catch (std::ios_base::failure &e)
    {
        std::cout << "IO System base failure exception, STOPPING PROGRAM\n";
        std::cout << e.what() << "\n";
    }
    catch (std::exception &e)
    {
        std::cout << "Exception, STOPPING PROGRAM from rank\n";
        std::cout << e.what() << "\n";
    }
}
