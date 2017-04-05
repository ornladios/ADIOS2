/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloADIOSNoXML_OOP.cpp
 *
 *  Created on: Jan 9, 2017
 *      Author: wfg
 */

#include <iostream>
#include <vector>

#include "ADIOS_CPP.h"

int main(int argc, char *argv[])
{
    const bool adiosDebug = true;
    adios::ADIOS adios(adiosDebug);

    try
    {
        // Define method for engine creation, it is basically straight-forward
        // parameters
        adios::Method &bpReaderSettings = adios.DeclareMethod(
            "SingleFile"); // default method type is BPWriter/BPReader
        bpReaderSettings.AddTransport("File"); // uses default POSIX library

        // Create engine smart pointer due to polymorphism,
        // Open returns a smart pointer to Engine containing the Derived class
        // Writer
        auto bpReader = adios.Open("myDoubles.bp", "r", bpReaderSettings);

        if (bpReader == nullptr)
            throw std::ios_base::failure(
                "ERROR: couldn't create bpReader at Open\n");
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
