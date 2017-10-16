/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * examplePythonPlugin.cpp Example of using an Engine defined in python
 *
 *  Created on: Sept 21, 2017
 *      Author: Scott Wittenburg <scott.wittenburg@kitware.com
 */

#include <ios>       //std::ios_base::failure
#include <iostream>  //std::cout
#include <stdexcept> //std::invalid_argument std::exception
#include <vector>

#include <adios2.h>

int main(int argc, char *argv[])
{
    /** Application variable */
    std::vector<float> myFloats = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    const std::size_t Nx = myFloats.size();

    try
    {
        /** ADIOS class factory of IO class objects, DebugON is recommended */
        adios2::ADIOS adios(adios2::DebugON);

        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines */
        adios2::IO &io = adios.DeclareIO("PythonPluginIO");

        /** global array: name, { shape (total dimensions) }, { start (local) },
         * { count (local) }, all are constant dimensions */
        adios2::Variable<float> &var = io.DefineVariable<float>(
            "data", {}, {}, {Nx}, adios2::ConstantDims);

        /** Engine derived class, spawned to start IO operations */
        io.SetEngine("PythonEngine");
        io.SetParameters({{"PluginName", "FirstPythonPlugin"},
                          {"PluginModule", "TestPythonEngine"},
                          {"PluginClass", "TestPythonEngine"}});
        auto writer = io.Open("TestPythonPlugin", adios2::OpenMode::Write);

        if (!writer)
        {
            throw std::ios_base::failure("ERROR: writer not created at Open\n");
        }

        /** Write variable for buffering */
        writer->Write<float>(var, myFloats.data());

        /** Create bp file, engine becomes unreachable after this*/
        writer->Close();
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