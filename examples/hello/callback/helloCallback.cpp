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

void UserCallBack(const float *data, const std::string &id,
                  const std::string &variableName, const std::string &type,
                  const std::vector<std::size_t> &count)
{
    std::cout << "Hello callback UserCallBack\n";
    std::cout << "Variable name : " << variableName << "\n";
    std::cout << "Variable type : " << type << "\n";
    std::cout << "Variable id : " << id << "\n";

    size_t totalSize = 1;
    for (const size_t n : count)
    {
        totalSize *= n;
    }

    std::cout << "Variable data :\n { ";
    for (size_t i = 0; i < totalSize; ++i)
    {
        std::cout << data[i] << ", ";
    }
    std::cout << " }\n";
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
            "Print Variable<float>",
            std::function<void(const float *, const std::string &,
                               const std::string &, const std::string &,
                               const std::vector<std::size_t> &)>(
                &UserCallBack));

        if (callback.m_Type == "Signature1")
        {
            callback.RunCallback1(myFloats.data(), "0", "bpFloats", "float",
                                  std::vector<std::size_t>{Nx});
        }
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
