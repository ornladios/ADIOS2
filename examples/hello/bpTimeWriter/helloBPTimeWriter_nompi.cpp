/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloBPTimeWriter_nompi.cpp  no mpi version of helloBPTimeWriter.cpp
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
    // Application variable
    std::vector<float> myFloats = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    const std::size_t Nx = myFloats.size();

    try
    {
        /** ADIOS class factory of IO class objects */
        adios2::ADIOS adios;

        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines */
        adios2::IO bpIO = adios.DeclareIO("BPFile_N2N");

        /** name, { shape (total dimensions) }, { start (local) }, { count
         * {local} } */
        adios2::Variable<float> bpFloats = bpIO.DefineVariable<float>(
            "bpFloats", {}, {}, {Nx}, adios2::ConstantDims);

        adios2::Variable<unsigned int> bpTimeStep =
            bpIO.DefineVariable<unsigned int>("timeStep");

        /** Engine derived class, spawned to start IO operations */
        adios2::Engine bpWriter = bpIO.Open("myVector.bp", adios2::Mode::Write);

        for (unsigned int timeStep = 0; timeStep < 10; ++timeStep)
        {
            bpWriter.BeginStep();

            // template type is optional but recommended
            bpWriter.Put<unsigned int>(bpTimeStep, timeStep);

            // modifying data
            myFloats[0] = static_cast<float>(timeStep);
            bpWriter.Put<float>(bpFloats, myFloats.data(), adios2::Mode::Sync);

            bpWriter.EndStep();
        }

        bpWriter.Close();
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
