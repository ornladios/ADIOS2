/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * exampleSealedOperatorWrite.cpp example showing how to use
 * EncryptionOperator plugin in asymmetric mode (write with public key only)
 *
 *  Created on: Mar 5, 2026
 *      Author: Vicente Bolea
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
    const std::size_t Nx = myDoubles.size();

    bool success = false;
    try
    {
        adios2::ADIOS adios(config);
        adios2::IO io = adios.DeclareIO("writer");
        adios2::Variable<double> var =
            io.DefineVariable<double>("data", {}, {}, {Nx}, adios2::ConstantDims);

        if (config.empty())
        {
            io.SetEngine("BPFile");
            adios2::Params params;
            params["PluginName"] = "Encryption";
            params["PluginLibrary"] = "EncryptionOperator";
            params["PublicKeyFile"] = "test-public.key";
            var.AddOperation("plugin", params);
        }

        adios2::Engine writer = io.Open("testSealedOperator.bp", adios2::Mode::Write);

        writer.BeginStep();
        writer.Put<double>(var, myDoubles.data());
        writer.EndStep();

        writer.Close();
        success = true;
    }
    catch (std::exception &e)
    {
        std::cout << "Exception, STOPPING PROGRAM\n";
        std::cout << e.what() << "\n";
    }

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
