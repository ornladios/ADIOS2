/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * examplePluginOperatorRead.cpp example showing how to use EncryptionOperator
 * plugin
 *
 *  Created on: July 5, 2021
 *      Author: Caitlin Ross <caitlin.ross@kitware.com>
 */

#include <ios>       //std::ios_base::failure
#include <iostream>  //std::cout
#include <stdexcept> //std::invalid_argument std::exception
#include <vector>

#include "adios2.h"

int main(int argc, char *argv[])
{
    std::string config;
    if (argc > 1)
    {
        config = std::string(argv[1]);
    }

    /** Application variable */
    std::vector<double> myDoubles = {
        0.0001, 1.0001, 2.0001, 3.0001, 4.0001, 5.0001, 6.0001, 7.0001, 8.0001, 9.0001,
        1.0001, 2.0001, 3.0001, 4.0001, 5.0001, 6.0001, 7.0001, 8.0001, 9.0001, 8.0001,
        2.0001, 3.0001, 4.0001, 5.0001, 6.0001, 7.0001, 8.0001, 9.0001, 8.0001, 7.0001,
        3.0001, 4.0001, 5.0001, 6.0001, 7.0001, 8.0001, 9.0001, 8.0001, 7.0001, 6.0001,
        4.0001, 5.0001, 6.0001, 7.0001, 8.0001, 9.0001, 8.0001, 7.0001, 6.0001, 5.0001,
        5.0001, 6.0001, 7.0001, 8.0001, 9.0001, 8.0001, 7.0001, 6.0001, 5.0001, 4.0001,
        6.0001, 7.0001, 8.0001, 9.0001, 8.0001, 7.0001, 6.0001, 5.0001, 4.0001, 3.0001,
        7.0001, 8.0001, 9.0001, 8.0001, 7.0001, 6.0001, 5.0001, 4.0001, 3.0001, 2.0001,
        8.0001, 9.0001, 8.0001, 7.0001, 6.0001, 5.0001, 4.0001, 3.0001, 2.0001, 1.0001,
        9.0001, 8.0001, 7.0001, 6.0001, 5.0001, 4.0001, 3.0001, 2.0001, 1.0001, 0.0001,
    };

    bool success = false;
    try
    {
        /** ADIOS class factory of IO class objects */
        adios2::ADIOS adios(config);

        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines */
        adios2::IO io = adios.DeclareIO("reader");

        /** global array: name, { shape (total dimensions) }, { start (local) },
         * { count (local) }, all are constant dimensions */
        adios2::Engine reader = io.Open("testOperator.bp", adios2::Mode::Read);
        reader.BeginStep();
        auto var = io.InquireVariable<double>("data");
        if (!var)
        {
            std::cout << "variable does not exist" << std::endl;
        }

        if (config.empty())
        {
            io.SetEngine("BPFile");
            /* PluginName -> <operator name> is required. If your operator needs
             * other parameters, they can be passed in here as well. */
            adios2::Params params;
            params["PluginName"] = "MyOperator";
            params["PluginLibrary"] = "adios2_encryption_operator";
            params["SecretKeyFile"] = "test-key";
            var.AddOperation("plugin", params);
        }

        std::vector<double> readDoubles;
        reader.Get<double>(var, readDoubles);
        reader.PerformGets();

        if (readDoubles == myDoubles)
        {
            std::cout << "data was read correctly!" << std::endl;
        }
        else
        {
            std::cout << "data was not read correctly!" << std::endl;
        }
        reader.EndStep();

        /** Engine becomes unreachable after this*/
        reader.Close();
        success = true;
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

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
