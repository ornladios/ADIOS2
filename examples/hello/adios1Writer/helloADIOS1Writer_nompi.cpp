/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloADIOS1Writer_nompi.cpp : non mpi version of helloADIOS1Writer
 *
 *  Created on: Jan 9, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#include <iostream>
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
        adios2::IO &adios1IO = adios.DeclareIO("ADIOS1IO");
        adios1IO.SetEngine("ADIOS1");
        adios1IO.AddTransport("file");

        /** global array : name, { shape (total) }, { start (local) }, { count
         * (local) }, all are constant dimensions */
        adios2::Variable<float> &bpFloats = adios1IO.DefineVariable<float>(
            "bpFloats", {}, {}, {Nx}, adios2::ConstantDims);

        /** Engine derived class, spawned to start IO operations */
        adios2::Engine &adios1Writer =
            adios1IO.Open("myVector.bp", adios2::Mode::Write);

        /** Write variable for buffering */
        adios1Writer.PutSync<float>(bpFloats, myFloats.data());

        /** Create bp file, engine becomes unreachable after this*/
        adios1Writer.Close();
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
