/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloBPWriter_nompi.cpp sequential non-mpi version of helloBPWriter
 *
 *  Created on: Jan 9, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include <ios>       //std::ios_base::failure
#include <iostream>  //std::cout
#include <stdexcept> //std::invalid_argument std::exception
#include <vector>

#include <adios2.h>
#include <adios2/engine/plugin/PluginEngine.h>

#include "ExampleEnginePlugin.h"

int main(int argc, char *argv[])
{
    /** Application variable */
    std::vector<float> myFloats = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    const std::size_t Nx = myFloats.size();

    adios2::core::engine::PluginEngine::RegisterPlugin<
        adios2::core::engine::ExampleEnginePlugin>("MyPlugin");

    try
    {
        /** ADIOS class factory of IO class objects, DebugON is recommended */
        adios2::ADIOS adios(adios2::DebugON);

        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines */
        adios2::IO io = adios.DeclareIO("PluginIO");

        /** global array: name, { shape (total dimensions) }, { start (local) },
         * { count (local) }, all are constant dimensions */
        adios2::Variable<float> var = io.DefineVariable<float>(
            "data", {}, {}, {Nx}, adios2::ConstantDims);

        /** Engine derived class, spawned to start IO operations */
        io.SetEngine("PluginEngine");
        io.SetParameters({{"PluginName", "MyPlugin"}});
        adios2::Engine writer = io.Open("TestPlugin", adios2::Mode::Write);

        /** Write variable for buffering */
        writer.Put<float>(var, myFloats.data());

        /** Create bp file, engine becomes unreachable after this*/
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
