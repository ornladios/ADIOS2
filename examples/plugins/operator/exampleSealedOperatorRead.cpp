/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ios>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <vector>

#include "adios2.h"

int main(int argc, char *argv[])
{
    std::string config;
    if (argc > 1)
    {
        config = std::string(argv[1]);
    }

    std::vector<double> myDoubles{100};
    std::iota(myDoubles.begin(), myDoubles.end(), 0);

    bool success = false;
    try
    {
        adios2::ADIOS adios(config);
        adios2::IO io = adios.DeclareIO("reader");
        adios2::Engine reader = io.Open("testSealedOperator.bp", adios2::Mode::Read);

        reader.BeginStep();
        auto var = io.InquireVariable<double>("data");
        if (!var)
        {
            std::cout << "variable does not exist" << std::endl;
        }

        if (config.empty())
        {
            io.SetEngine("BPFile");
            adios2::Params params;
            params["PluginName"] = "Encryption";
            params["PluginLibrary"] = "EncryptionOperator";
            params["PublicKeyFile"] = "test-public.key";
            params["SecretKeyFile"] = "test-secret.key";
            var.AddOperation("plugin", params);
        }

        std::vector<double> readDoubles;
        reader.Get<double>(var, readDoubles);
        reader.PerformGets();

        if (readDoubles == myDoubles)
        {
            std::cout << "data was read correctly!" << std::endl;
            success = true;
        }
        else
        {
            std::cout << "data was not read correctly!" << std::endl;
        }
        reader.EndStep();
        reader.Close();
    }
    catch (std::exception &e)
    {
        std::cout << "Exception, STOPPING PROGRAM\n";
        std::cout << e.what() << "\n";
    }

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
