/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Write local arrays from multiple processors.
 *
 * If one cannot or does not want to organize arrays present on each process
 * as one global array, still one can write them out with the same name.
 * Reading, however, needs to be handled differently: each process' array has
 * to be read separately, using Writeblock selections. The size of each process
 * block should be discovered by the reading application by inquiring per-block
 * size information of the variable, and allocate memory for reading
 * accordingly.
 *
 * bpls can show the size of each block of the variable:
 * bpls -D <file> <variable>
 *
 *
 * Created on: Jun 2, 2017
 *      Author: pnorbert
 */

#include <iostream>
#include <vector>

#include <adios2.h>
#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#endif

int main(int argc, char *argv[])
{
    int rank = 0, nproc = 1;
#ifdef ADIOS2_HAVE_MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
#endif
    const bool adiosDebug = true;
    const int NSTEPS = 5;

    // generate different random numbers on each process,
    // but always the same sequence at each run
    srand(rank * 32767);

#ifdef ADIOS2_HAVE_MPI
    adios::ADIOS adios("localArray.xml", MPI_COMM_WORLD);
#else
    adios::ADIOS adios("localArray.xml", adios::Verbose::WARN);
#endif

    // Application variables for output
    // random size per process, 5..10 each
    unsigned int Nx = rand() % 6 + 5;
    // Local array, size is fixed over time on each process
    std::vector<double> v1(Nx);

    // random size per process, a different size at each step
    unsigned int Nelems;
    // Local array, size is changing over time on each process
    std::vector<double> v2;

    try
    {
        // Get io settings from the config file or
        // create one with default settings here
        adios::IO &io = adios.DeclareIO("Output");

        /*
         * Define local array: type, name, local size
         * Global dimension and starting offset must be an empty vector
         */
        adios::Variable<double> &varV1 =
            io.DefineVariable<double>("v1", {}, {}, {Nx});

        /*
         * Define local array: type, name
         * Global dimension and starting offset must be an empty vector
         * but local size must NOT be an empty vector.
         * We can use {adios::UnknownDim} for this purpose or any number
         * but we will modify it before writing
         */
        adios::Variable<double> &varV2 =
            io.DefineVariable<double>("v2", {}, {}, {adios::UnknownDim});

        // Open file. "w" means we overwrite any existing file on disk,
        // but Advance() will append steps to the same file.
        auto writer = io.Open("localArray.bp", adios::OpenMode::Write);

        if (writer == nullptr)
            throw std::ios_base::failure(
                "ERROR: failed to open file with ADIOS\n");

        for (int step = 0; step < NSTEPS; step++)
        {
            for (int i = 0; i < Nx; i++)
            {
                v1[i] = rank * 1.0 + step * 0.1;
            }

            writer->Write<double>(varV1, v1.data());

            // random size per process per step, 5..10 each
            Nelems = rand() % 6 + 5;
            v2.reserve(Nelems);
            for (int i = 0; i < Nelems; i++)
            {
                v2[i] = rank * 1.0 + step * 0.1;
            }

            // Set the size of the array now because we did not know
            // the size at the time of definition
            varV2.SetSelection({}, {Nelems});
            writer->Write<double>(varV2, v2.data());

            writer->Advance();
        }

        writer->Close();
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

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return 0;
}
