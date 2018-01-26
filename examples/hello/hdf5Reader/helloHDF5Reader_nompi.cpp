/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloHDF5Reader_nompi.cpp
 *
 *  Created on: Jan 24, 2018
 *      Author: Junmin Gu
 */


#include <ios>       //std::ios_base::failure
#include <iostream>  //std::cout
#include <stdexcept> //std::invalid_argument std::exception
#include <vector>

#include <adios2.h>



int main(int argc, char *argv[])
{
    /** Application variable */
    const std::size_t Nx = 10;
    std::vector<float> myFloats(Nx);
    std::vector<int> myInts(Nx);

    try
    {
        /** ADIOS class factory of IO class objects, DebugON is recommended */
        adios2::ADIOS adios(adios2::DebugON);

        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines */
        adios2::IO &h5IO = adios.DeclareIO("ReadHDF5");

        /** Engine derived class, spawned to start IO operations */
        adios2::Engine &h5Reader =
            h5IO.Open("myVector_cpp.h5", adios2::Mode::Read);

	h5IO.SetEngine("HDF5");
        /** Write variable for buffering */
        adios2::Variable<float> *h5Floats =
            h5IO.InquireVariable<float>("h5Floats");

        adios2::Variable<int> *h5Ints = h5IO.InquireVariable<int>("h5Ints");

        if (h5Floats != nullptr)
        {
            h5Reader.GetSync<float>(*h5Floats, myFloats.data());
        }

        if (h5Floats != nullptr)
        {
            h5Reader.GetSync<int>(*h5Ints, myInts.data());
        }

        /** Close h5 file, engine becomes unreachable after this*/
        h5Reader.Close();
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


