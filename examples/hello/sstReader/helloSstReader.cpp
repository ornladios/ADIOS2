/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloSstReader_nompi.cpp
 *
 *  Created on: Aug 17, 2017
v *      Author: Greg Eisenhauer
 */

#include <chrono>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

#include <adios2.h>

void UserCallBack(const void *data, std::string doid, std::string var,
                  std::string dtype, std::vector<std::size_t> varshape)
{
    std::cout << "data object ID = " << doid << "\n";
    std::cout << "variable name = " << var << "\n";
    std::cout << "data type = " << dtype << "\n";

    std::size_t varsize = std::accumulate(varshape.begin(), varshape.end(), 1,
                                          std::multiplies<std::size_t>());

    for (unsigned int i = 0; i < varsize; ++i)
        std::cout << ((float *)data)[i] << " ";
    std::cout << std::endl;
}

int main(int argc, char *argv[])
{
    // Application variable
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int timeout = 5;

    if (argc == 2)
    {
        timeout = atoi(argv[1]);
    }

    try
    {
        adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);

        adios2::IO &sstIO = adios.DeclareIO("WAN");
        sstIO.SetEngine("Sst");
        sstIO.SetParameters({{"real_time", "yes"},
                             {"method_type", "stream"},
                             {"method", "dump"}});

        adios2::Engine &sstReader =
            sstIO.Open("helloSst.bp", adios2::Mode::Read);

        std::this_thread::sleep_for(std::chrono::seconds(timeout));

        sstReader.Close();
    }
    catch (std::invalid_argument &e)
    {
        std::cout << "Invalid argument exception, STOPPING PROGRAM from rank "
                  << rank << "\n";
        std::cout << e.what() << "\n";
    }
    catch (std::ios_base::failure &e)
    {
        std::cout << "IO System base failure exception, STOPPING PROGRAM "
                     "from rank "
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
