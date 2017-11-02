/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloBPReaderHeatMap.cpp : Writes a regular heat map in a regular 2D mesh,
 * values grow from 0 in increments of 1
 *
 * temperature[gNx, gNy]
 * where: gNx = MPI_size_x * Nx and gNy = MPI_size_y * Ny
 *
 * 0               1       2   ...    gNy-1
 * gNy           gNy+1  gNy+2  ...   2*gNy-1
 * ...
 * ...
 * (gNx-1)*gNy   ...                  gNx*gNy-1
 *
 *
 *  Created on: Nov 1, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include <algorithm>
#include <ios>      //std::ios_base::failure
#include <iostream> //std::cout
#include <mpi.h>
#include <stdexcept> //std::invalid_argument std::exception
#include <vector>

#include <adios2.h>

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /** Application variable dimensions */
    constexpr std::size_t Nx = 10;
    constexpr std::size_t Ny = 10;

    const adios2::Dims count{Nx, Ny};
    const adios2::Dims start{rank * Nx, rank * Ny};
    const adios2::Dims shape{size * Nx, size * Ny};

    // populate local temperature values
    std::vector<unsigned int> temperatures(Nx * Ny);
    for (unsigned int i = 0; i < Nx; ++i)
    {
        const unsigned int iGlobal = start[0] + i;

        for (unsigned int j = 0; j < Ny; ++j)
        {
            const unsigned int jGlobal = start[1] + j;
            const unsigned int value = iGlobal * shape[1] + jGlobal;
            temperatures[i * Ny + j] = value;
        }
    }

    try
    {
        /** ADIOS class factory of IO class objects, Debug is ON by default */
        adios2::ADIOS adios(MPI_COMM_WORLD);

        // ************************** WRITE
        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines */
        adios2::IO &putHeatMap = adios.DeclareIO("HeatMapWriter");

        adios2::Variable<unsigned int> &outTemperature =
            putHeatMap.DefineVariable<unsigned int>(
                "temperature", shape, start, count, adios2::ConstantDims);

        /** Will create HeatMap.bp */
        adios2::Engine &bpWriter =
            putHeatMap.Open("HeatMap.bp", adios2::Mode::Write);

        bpWriter.PutSync(outTemperature, temperatures.data());
        bpWriter.Close();

        // ************************** READ
        if (rank == 0)
        {
            adios2::IO &getHeatMap = adios.DeclareIO("HeatMapReader");
            adios2::Engine &bpReader =
                getHeatMap.Open("HeatMap.bp", adios2::Mode::Read);

            // this just discovers in the metadata file that the variable exists
            adios2::Variable<unsigned int> *inTemperature =
                getHeatMap.InquireVariable<unsigned int>("temperature");
            inTemperature->SetSelection({{2, 2}, {4, 4}});

            // now read the variable
            if (inTemperature != nullptr)
            {
                std::vector<unsigned int> inTemperatures(16);
                bpReader.GetSync(*inTemperature, inTemperatures.data());

                std::cout << "Incoming temperature map:\n";

                for (auto i = 0; i < inTemperatures.size(); ++i)
                {
                    std::cout << inTemperatures[i] << " ";
                    if ((i + 1) % 4 == 0)
                    {
                        std::cout << "\n";
                    }
                }
                std::cout << "\n";
            }

            bpReader.Close();
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
