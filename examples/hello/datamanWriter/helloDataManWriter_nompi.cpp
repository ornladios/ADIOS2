/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloDataManWriter.cpp
 *
 *  Created on: Feb 16, 2017
 *      Author: Jason Wang
 */

#include <iostream>
#include <vector>

#include <adios2.h>

int main(int argc, char *argv[])
{
    // Application variable
    std::vector<float> myFloats = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    const std::size_t Nx = myFloats.size();

    try
    {
        adios2::ADIOS adios(adios2::DebugON);
        adios2::IO &dataManIO = adios.DeclareIO("WANIO");
        dataManIO.SetEngine("DataManWriter");
        dataManIO.SetParameters(
            {{"compress", "no"}, {"method", "dump"}, {"type", "wan"}});

        // Define variable and local size
        auto bpFloats =
            dataManIO.DefineVariable<float>("bpFloats", {}, {}, {Nx});

        // Create engine smart pointer to DataMan Engine due to polymorphism,
        // Open returns a smart pointer to Engine containing the Derived class
        auto dataManWriter =
            dataManIO.Open("myFloats.bp", adios2::OpenMode::Write);

        if (!dataManWriter)
        {
            throw std::ios_base::failure(
                "ERROR: failed to create DataMan I/O engine at Open\n");
        }

        dataManWriter->Write<float>(bpFloats, myFloats.data());
        dataManWriter->Close();
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
