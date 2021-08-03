/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloBPReader.cpp: Simple self-descriptive example of how to read a variable
 * from a BP File.
 *
 *  Created on: Feb 16, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include <ios>       //std::ios_base::failure
#include <iostream>  //std::cout
#include <stdexcept> //std::invalid_argument std::exception
#include <vector>

#include <adios2.h>

int main(int argc, char *argv[])
{
    std::string filename = "myVector_cpp.bp";

    try
    {
        /** ADIOS class factory of IO class objects */
        adios2::ADIOS adios;

        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines */
        adios2::IO bpIO = adios.DeclareIO("ReadBP");

        /** Engine derived class, spawned to start IO operations */
        adios2::Engine bpReader = bpIO.Open(filename, adios2::Mode::Read);

        const std::map<std::string, adios2::Params> variables =
            bpIO.AvailableVariables(true);

        std::cout << "List of variables:";
        for (const auto &variablePair : variables)
        {
            std::cout << "  " << variablePair.first;
        }
        std::cout << std::endl;

        /** Write variable for buffering */
        adios2::Variable<float> bpFloats =
            bpIO.InquireVariable<float>("bpFloats");

        adios2::Variable<int> bpInts = bpIO.InquireVariable<int>("bpInts");

        if (bpFloats)
        {
            std::vector<float> myFloats;
            bpReader.Get<float>(bpFloats, myFloats, adios2::Mode::Sync);
            std::cout << "Float vector inside " << filename << ": {";
            for (auto &x : myFloats)
            {
                std::cout << x << ", ";
            }
            std::cout << "}\n";
        }

        if (bpInts)
        {
            std::vector<int> myInts;
            bpReader.Get<int>(bpInts, myInts, adios2::Mode::Sync);
        }
        else
        {
            std::cout << "There are no integer datasets in " << filename
                      << ".\n";
        }

        /** Close bp file, engine becomes unreachable after this*/
        bpReader.Close();
    }
    catch (std::invalid_argument &e)
    {
        std::cerr << "Invalid argument exception, STOPPING PROGRAM\n";
        std::cerr << e.what() << "\n";
    }
    catch (std::ios_base::failure &e)
    {
        std::cerr << "IO System base failure exception, STOPPING PROGRAM\n";
        std::cerr << e.what() << "\n";
        std::cerr << "The file " << filename << " does not exist."
                  << " Presumably this is because hello_bpWriter has not been "
                     "run. Run ./hello_bpWriter before running this program.\n";
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception, STOPPING PROGRAM\n";
        std::cerr << e.what() << "\n";
    }

    return 0;
}
