/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloBPWriteStep.cpp  example for writing a variable using the WriteStep
 * function for time aggregation. Time step is saved as an additional (global)
 * single value variable, just for tracking purposes. Non-mpi version.
 *
 *  Created on: Oct 27, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include <algorithm> //std::for_each
#include <ios>       //std::ios_base::failure
#include <iostream>  //std::cout
#include <stdexcept> //std::invalid_argument std::exception
#include <vector>

#include <adios2.h>

int main(int argc, char *argv[])
{
    // Application variables
    std::vector<float> myFloats = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::vector<int> myInts = {0, -1, -2, -3, -4, -5, -6, -7, -8, -9};
    const std::size_t Nx = myFloats.size();

    try
    {
        adios2::fstream adios2File("Simple_nompi.bp", adios2::fstream::out);

        for (unsigned int timeStep = 0; timeStep < 10; ++timeStep)
        {
            myFloats[0] = static_cast<float>(timeStep);
            myInts[0] = static_cast<int>(timeStep);

            adios2File.write("myFloats", myFloats.data(), {}, {}, {Nx});
            adios2File.write("myInts", myInts.data(), {}, {}, {Nx},
                             adios2::endl);
        }
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
        std::cout << "Exception, STOPPING PROGRAM\n";
        std::cout << e.what() << "\n";
    }

    return 0;
}
