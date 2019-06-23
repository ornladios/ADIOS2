/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloBPWriter_nompi.cpp sequential non-mpi version of helloBPWriter
 *
 *  Created on: Jan 9, 2017
 *      Author: William F Godoy godoywf@ornl.gov
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
        adios2::IO bpIO = adios.DeclareIO("BPFile_N2N");

        /** global array: name, { shape (total dimensions) }, { start (local) },
         * { count (local) }, all are constant dimensions */
        adios2::Variable<float> bpFloats = bpIO.DefineVariable<float>(
            "bpFloats", {}, {}, {Nx}, adios2::ConstantDims);

        std::string filename = "myVector_cpp.bp";
        /** Engine derived class, spawned to start IO operations */
        adios2::Engine bpWriter = bpIO.Open(filename, adios2::Mode::Write);

        /** Write variable for buffering */
        bpWriter.Put<float>(bpFloats, myFloats.data());

        /** Create bp file, engine becomes unreachable after this*/
        bpWriter.Close();
        std::cout << "Wrote file " << filename
                  << " to disk. It can now be read by running "
                     "./bin/hello_bpReader.\n";
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
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception, STOPPING PROGRAM\n";
        std::cerr << e.what() << "\n";
    }

    return 0;
}
