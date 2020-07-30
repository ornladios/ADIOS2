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
#include <string>
#include <vector>

#include <adios2.h>

int main(int argc, char *argv[])
{
    /** Application variable */
    std::vector<float> myFloats = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    const std::size_t Nx = myFloats.size();
    std::cout << "No mpi version" << std::endl;
    try
    {
        /** ADIOS class factory of IO class objects */
        adios2::ADIOS adios;

        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines */
        adios2::IO bpIO = adios.DeclareIO("BPFile_N2N");

        /** global array: name, { shape (total dimensions) }, { start (local) },
         * { count (local) }, all are constant dimensions */
        adios2::Variable<float> bpFloats = bpIO.DefineVariable<float>(
            "bpFloats", {}, {}, {Nx}, adios2::ConstantDims);

        bpIO.DefineAttribute<std::string>("Single_String",
                                          "File generated with ADIOS2");

        std::vector<std::string> myStrings = {"one", "two", "three"};
        bpIO.DefineAttribute<std::string>("Array_of_Strings", myStrings.data(),
                                          myStrings.size());

        bpIO.DefineAttribute<double>("Attr_Double", 0.f);
        std::vector<double> myDoubles = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        bpIO.DefineAttribute<double>("Array_of_Doubles", myDoubles.data(),
                                     myDoubles.size());

        /** Engine derived class, spawned to start IO operations */
        adios2::Engine bpWriter = bpIO.Open("myVector.bp", adios2::Mode::Write);

        /** Write variable for buffering */
        bpWriter.Put<float>(bpFloats, myFloats.data());

        /** Create bp file, engine becomes unreachable after this*/
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
