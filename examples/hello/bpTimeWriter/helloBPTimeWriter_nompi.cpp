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
        /** ADIOS class factory of IO class objects, DebugON is recommended */
        adios::ADIOS adios(adios::DebugON);

        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines */
        adios::IO &bpIO = adios.DeclareIO("BPFile_N2N");

        /** name, { shape (total dimensions) }, { start (local) }, { count
         * {local} } */
        adios::Variable<float> &bpFloats = bpIO.DefineVariable<float>(
            "bpFloats", {}, {}, {Nx}, adios::ConstantDims);

        adios::Variable<unsigned int> &bpTimeStep =
            bpIO.DefineVariable<unsigned int>("timeStep");

        /** Engine derived class, spawned to start IO operations */
        auto bpWriter = bpIO.Open("myVector.bp", adios::OpenMode::Write);

        if (!bpWriter)
        {
            throw std::ios_base::failure(
                "ERROR: bpWriter not created at Open\n");
        }

        for (unsigned int timeStep = 0; timeStep < 10; ++timeStep)
        {
            // template type is optional but recommended
            bpWriter->Write<unsigned int>(bpTimeStep, timeStep);

            myFloats[0] = timeStep;
            bpWriter->Write<float>(bpFloats, myFloats.data());
            bpWriter->Advance();
        }

        bpWriter->Close();
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
