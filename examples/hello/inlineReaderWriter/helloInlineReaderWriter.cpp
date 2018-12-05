/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * helloInlineReaderWriter.cpp  example borrowed from helloBPTimeWriter, using the
 * inline engine. Writes a variable using the Advance
 * function for time aggregation. Time step is saved as an additional (global)
 * single value variable, just for tracking purposes.
 *
 *  Created on: Nov 16, 2018
 *      Author: Aron Helser aron.helser@kitware.com
 */

#include <algorithm> //std::for_each
#include <ios>       //std::ios_base::failure
#include <iostream>  //std::cout
#include <stdexcept> //std::invalid_argument std::exception
#include <vector>

#include <adios2.h>


void DoAnalysis(adios2::IO& bpIO, adios2::Engine& bpReader, int rank, unsigned int step)
{
    // Application variable
    std::vector<float> myFloats = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    const std::size_t Nx = myFloats.size();

    try
    {
        bpReader.BeginStep();
        /////////////////////READ
        adios2::Variable<float> bpFloats000 =
            bpIO.InquireVariable<float>("bpFloats000");

        adios2::Variable<std::string> bpString =
            bpIO.InquireVariable<std::string>("bpString");

        if (bpFloats000)
        {
            bpFloats000.SetSelection({{rank * Nx}, {Nx}});
            // bpFloats000.SetStepSelection({step, 1});

            std::vector<float> data(bpFloats000.SelectionSize());
            bpReader.Get(bpFloats000, data.data(), adios2::Mode::Sync);

            std::cout << "Data timestep " << bpFloats000.StepsStart()
                      << " from rank " << rank << ": ";
            for (const auto datum : data)
            {
                std::cout << datum << " ";
            }
            std::cout << "\n";
        }
        else
        {
            std::cout << "Variable bpFloats000 not found\n";
        }

        if (bpString)
        {
            // bpString.SetStepSelection({step, 1});

            std::string myString;
            bpReader.Get(bpString, myString, adios2::Mode::Sync);
            std::cout << "bpString: " << myString << "\n";
        }
        bpReader.EndStep();
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

}


int main(int argc, char *argv[])
{
    int rank = 0, size = 1;
#ifdef ADIOS2_HAVE_MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
#endif

    // Application variable
    std::vector<float> myFloats = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    const std::size_t Nx = myFloats.size();

    try
    {
        /** ADIOS class factory of IO class objects, DebugON (default) is
         * recommended */
#ifdef ADIOS2_HAVE_MPI
        adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
        adios2::ADIOS adios(adios2::DebugON);
#endif
        /*** IO class object: settings and factory of Settings: Variables,
         * Parameters, Transports, and Execution: Engines
         * Inline uses single IO for write/read */
        adios2::IO bpIO = adios.DeclareIO("InlineReadWrite");
        /// WRITE
        {
            bpIO.SetEngine("Inline");
            bpIO.SetParameters({{"verbose", "5"}});

            /** global array: name, { shape (total dimensions) }, { start
             * (local) },
             * { count (local) }, all are constant dimensions */
            const unsigned int variablesSize = 10;
            std::vector<adios2::Variable<float>> bpFloats(variablesSize);

            adios2::Variable<std::string> bpString =
                bpIO.DefineVariable<std::string>("bpString");

            for (unsigned int v = 0; v < variablesSize; ++v)
            {
                std::string namev("bpFloats");
                if (v < 10)
                {
                    namev += "00";
                }
                else if (v < 100)
                {
                    namev += "0";
                }
                namev += std::to_string(v);

                bpFloats[v] =
                    bpIO.DefineVariable<float>(namev, {size * Nx}, {rank * Nx},
                                               {Nx});
            }

            /** global single value variable: name */
            adios2::Variable<unsigned int> bpTimeStep =
                bpIO.DefineVariable<unsigned int>("timeStep");

            /** Engine derived class, spawned to start IO operations */
            adios2::Engine bpWriter =
                bpIO.Open("myWriteID", adios2::Mode::Write);

            bpIO.SetEngine("Inline");
            bpIO.SetParameters({{"verbose", "5"}, {"writerID", "myWriteID"}});

            adios2::Engine bpReader =
                bpIO.Open("myReadID", adios2::Mode::Read);

            for (unsigned int timeStep = 0; timeStep < 3; ++timeStep)
            {
                bpWriter.BeginStep();
                if (rank == 0) // global single value, only saved by rank 0
                {
                    bpWriter.Put<unsigned int>(bpTimeStep, timeStep);
                }

                // template type is optional, but recommended
                for (unsigned int v = 0; v < variablesSize; ++v)
                {
                    // Note: Put is deferred, so all variables will see v == 9
                    // and myFloats[0] == 9, 10, or 11
                    myFloats[0] = static_cast<float>(v + timeStep);
                    bpWriter.Put(bpFloats[v], myFloats.data());
                }

                const std::string myString(
                    "Hello from rank: " + std::to_string(rank) +
                    " and timestep: " + std::to_string(timeStep));

                if (rank == 0)
                {
                    bpWriter.Put(bpString, myString);
                }

                bpWriter.EndStep();

                DoAnalysis(bpIO, bpReader, rank, timeStep);
            }

            bpWriter.Close();
            bpReader.Close();
        }
        // MPI_Barrier(MPI_COMM_WORLD);

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

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return 0;
}
