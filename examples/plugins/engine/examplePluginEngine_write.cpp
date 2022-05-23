/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * examplePluginEngine_write.cpp example showing how to use ExampleWritePlugin
 * engine
 *
 *  Created on: July 5, 2021
 *      Author: Caitlin Ross <caitlin.ross@kitware.com>
 */

#include <ios>       //std::ios_base::failure
#include <iostream>  //std::cout
#include <stdexcept> //std::invalid_argument std::exception
#include <vector>

#include "adios2.h"

void testStreaming(adios2::Engine &writer, std::vector<float> &myFloats,
                   adios2::Variable<float> &var)
{
    for (int i = 0; i < 2; i++)
    {
        if (i == 1)
        {
            for (auto &num : myFloats)
            {
                num *= 2;
            }
        }
        writer.BeginStep();
        writer.Put<float>(var, myFloats.data());
        writer.EndStep();
    }
}

int main(int argc, char *argv[])
{
    std::string config;
    if (argc > 1)
    {
        config = std::string(argv[1]);
    }

    bool streaming = false;
    if (argc > 2)
    {
        streaming = std::atoi(argv[2]) == 1;
    }

    /** Application variable */
    std::vector<float> myFloats = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    const std::size_t Nx = myFloats.size();

    try
    {
        /** ADIOS class factory of IO class objects */
        adios2::ADIOS adios(config);

        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines */
        adios2::IO io = adios.DeclareIO("writer");

        /** global array: name, { shape (total dimensions) }, { start (local) },
         * { count (local) }, all are constant dimensions */
        adios2::Variable<float> var = io.DefineVariable<float>(
            "data", {}, {}, {Nx}, adios2::ConstantDims);

        if (config.empty())
        {
            io.SetEngine("Plugin");
            adios2::Params params;
            params["PluginName"] = "WritePlugin";
            params["PluginLibrary"] = "PluginEngineWrite";
            io.SetParameters(params);
        }
        adios2::Engine writer = io.Open("TestPlugin", adios2::Mode::Write);

        if (streaming)
        {
            testStreaming(writer, myFloats, var);
        }
        else
        {
            writer.Put<float>(var, myFloats.data());
        }

        /** Engine becomes unreachable after this*/
        writer.Close();
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

    return 0;
}
