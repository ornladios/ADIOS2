/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloDataManReader.cpp
 *
 *  Created on: Feb 16, 2017
 *      Author: wfg
 */

#include <functional> //std::multiplies
#include <iostream>   //std::cout, std::endl
#include <numeric>    //std::accumulate
#include <string>
#include <vector>

#include <mpi.h>

#include <adios2.h>

void getcb(const void *data, std::string doid, std::string var,
           std::string dtype, std::vector<std::size_t> varshape)
{
    std::cout << "data object ID = " << doid << "\n"; // do you need to flush?
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
    MPI_Init(&argc, &argv);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    const bool adiosDebug = true;
    adios::ADIOS adios(MPI_COMM_WORLD, adios::Verbose::WARN, adiosDebug);

    try
    {
        // Define method for engine creation, it is basically straight-forward
        // parameters
        adios::Method &datamanSettings = adios.DeclareMethod("WAN");
        if (!datamanSettings.IsUserDefined())
        {
            // if not defined by user, we can change the default settings
            datamanSettings.SetEngine("DataManReader");
            datamanSettings.SetParameters("peer-to-peer=yes");
            datamanSettings.AddTransport("Mdtm", "localIP=127.0.0.1",
                                         "remoteIP=127.0.0.1",
                                         "tolerances=1,2,3");
            // datamanSettings.AddTransport( "ZeroMQ", "localIP=127.0.0.1",
            // "remoteIP=127.0.0.1", "tolerances=1,2,3" ); not yet supported
            // ,
            // will throw an exception
        }

        // Create engine smart pointer to DataManReader Engine due to
        // polymorphism,
        // Open returns a smart pointer to Engine containing the Derived class
        // DataManReader
        auto datamanReader = adios.Open("myDoubles.bp", "r", datamanSettings);

        if (datamanReader == nullptr)
            throw std::ios_base::failure(
                "ERROR: failed to create DataMan I/O engine at Open\n");

        datamanReader->SetCallBack(getcb);

        adios::Variable<double> *ioMyDoubles =
            datamanReader->InquireVariableDouble("ioMyDoubles");
        if (ioMyDoubles == nullptr)
            std::cout << "Variable ioMyDoubles not read...yet\n";

        datamanReader->Close();
    }
    catch (std::invalid_argument &e)
    {
        if (rank == 0)
        {
            std::cout << "Invalid argument exception, STOPPING PROGRAM\n";
            std::cout << e.what() << "\n";
        }
    }
    catch (std::ios_base::failure &e)
    {
        if (rank == 0)
        {
            std::cout << "System exception, STOPPING PROGRAM\n";
            std::cout << e.what() << "\n";
        }
    }
    catch (std::exception &e)
    {
        if (rank == 0)
        {
            std::cout << "Exception, STOPPING PROGRAM\n";
            std::cout << e.what() << "\n";
        }
    }

    MPI_Finalize();

    return 0;
}
