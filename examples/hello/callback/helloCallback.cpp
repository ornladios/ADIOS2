/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloCallback.cpp
 *
 *  Created on: Oct 18, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */
#include <mpi.h>

#include <cstdint>   //std::int32_t
#include <ios>       //std::ios_base::failure
#include <iostream>  //std::cout
#include <numeric>   //std::iota
#include <stdexcept> //std::invalid_argument std::exception
#include <vector>

#include <adios2.h>

void UserCallBack(const unsigned int userID)
{
    std::cout << "Hello callback UserCallBack " << userID << "\n";
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /** Application variable uints from 0 to 100 */
    std::vector<float> myFloats(100);
    std::iota(myFloats.begin(), myFloats.end(), 0.f);
    const std::size_t Nx = myFloats.size();
    const std::size_t inputBytes = Nx * sizeof(float);

    try
    {
        /** ADIOS class factory of IO class objects, DebugON is recommended */
        adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);

        adios2::Operator &callback = adios.DefineOperator(
            "UserCallback",
            std::function<void(const unsigned int)>(UserCallBack));

        //        std::function<void(const unsigned int)> myFunction =
        //            callback.GetCallback<void, const unsigned int>();

        // myFunction(1);
    }
    catch (std::invalid_argument &e)
    {
        std::cout << "Invalid argument exception, STOPPING PROGRAM from rank "
                  << rank << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::ios_base::failure &e)
    {
        std::cout
            << "IO System base failure exception, STOPPING PROGRAM from rank "
            << rank << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::exception &e)
    {
        std::cout << "Exception, STOPPING PROGRAM from rank " << rank << "\n";
        std::cout << e.what() << "\n";
    }

    MPI_Finalize();

    return 0;
}
