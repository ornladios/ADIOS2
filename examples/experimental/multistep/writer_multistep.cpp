/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * writer.cpp
 *
 *  Created on: Feb 13, 2017
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
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(adios2::DebugON);
#endif

    // Application variables for output
    // 1. Global value, constant across processes, constant over time
    const unsigned int Nx = 10;
    // 2. Local value, varying across processes, constant over time
    const int ProcessID = rank; // = rank
    // 3. Global array, global dimensions, local dimensions and offsets are
    // constant over time
    std::vector<double> GlobalArrayFixedDims(Nx);

    // 4. Local array, local dimensions are
    // constant over time (but different across processors here)
    std::vector<float> LocalArrayFixedDims(nproc - rank + 1, rank);

    // 5. Global value, constant across processes, VARYING value over time
    unsigned int Ny = 0;
    // 6. Local value, varying across processes, VARYING over time
    unsigned int Nparts; // random size per process, 5..10 each
    // 7. Global array, dimensions and offsets are VARYING over time
    std::vector<double> GlobalArray;
    // 8. Local array, dimensions and offsets are VARYING over time
    std::vector<float> IrregularArray;

    try
    {

        // Define method for engine creation
        // 1. Get method def from config file or define new one
        adios2::IO &bpWriterSettings = adios.DeclareIO("output");
        if (!bpWriterSettings.InConfigFile())
        {
            // if not defined by user, we can change the default settings
            bpWriterSettings.SetEngine("ADIOS1Writer");
            // ISO-POSIX file is the default transport
            // Passing parameters to the transport
            bpWriterSettings.AddTransport("file"
#ifdef ADIOS2_HAVE_MPI
                                          ,
                                          {
                                              {
                                                  "library", "MPI-IO"
                                              }
                                          }
#endif
                                          );
            // Passing parameters to the engine
            bpWriterSettings.SetParameters({{"have_metadata_file", "yes"}});
            // number of aggregators
            // bpWriterSettings.SetParameters("Aggregation", (nproc + 1) / 2);
        }
        /*
         * Define variables
         */
        // 1. Global value, constant across processes, constant over time
        adios2::Variable<unsigned int> &varNX =
            bpWriterSettings.DefineVariable<unsigned int>("NX");
        adios2::Variable<int> &varNproc =
            bpWriterSettings.DefineVariable<int>("Nproc");

        // 2. Local value, varying across processes, constant over time
        adios2::Variable<int> &varProcessID =
            bpWriterSettings.DefineVariable<int>("ProcessID",
                                                 {adios2::LocalValueDim});

        // 3. Global array, global dimensions (shape), offsets (start) and
        // local
        // dimensions (count)  are  constant over time
        adios2::Variable<double> &varGlobalArrayFixedDims =
            bpWriterSettings.DefineVariable<double>("GlobalArrayFixedDims",
                                                    {nproc * Nx});

        // 4. Local array, local dimensions and offsets are
        // constant over time.
        // 4.a. Want to see this at reading as a bunch of local arrays
        adios2::Variable<float> &varLocalArrayFixedDims =
            bpWriterSettings.DefineVariable<float>(
                "LocalArrayFixedDims", {}, {}, {LocalArrayFixedDims.size()});
        // 4.b. Joined array, a 1D array, with global dimension and offsets
        // calculated at read time
        adios2::Variable<float> &varLocalArrayFixedDimsJoined =
            bpWriterSettings.DefineVariable<float>(
                "LocalArrayFixedDimsJoined", {adios2::JoinedDim}, {},
                {LocalArrayFixedDims.size()});

        // 5. Global value, constant across processes, VARYING value over
        // time
        adios2::Variable<unsigned int> &varNY =
            bpWriterSettings.DefineVariable<unsigned int>("NY");

        // 6. Local value, varying across processes, VARYING over time
        adios2::Variable<unsigned int> &varNparts =
            bpWriterSettings.DefineVariable<unsigned int>(
                "Nparts", {adios2::LocalValueDim});

        // 7. Global array, dimensions and offsets are VARYING over time
        adios2::Variable<double> &varGlobalArray =
            bpWriterSettings.DefineVariable<double>("GlobalArray",
                                                    {adios2::UnknownDim});

        // 8. Local array, dimensions and offsets are VARYING over time
        adios2::Variable<float> &varIrregularArray =
            bpWriterSettings.DefineVariable<float>("Irregular", {}, {},
                                                   {adios2::UnknownDim});

        // add transform to variable in group...not executed (just testing
        // API)
        // adios2::Transform bzip2 = adios2::transform::BZIP2();
        // varNice->AddTransform(bzip2, 1);

        // Open returns a smart pointer to Engine containing the Derived class
        // Writer
        // "w" means we overwrite any existing file on disk, but AdvanceStep
        // will
        // append steps later.
        auto bpWriter =
            bpWriterSettings.Open("myNumbers.bp", adios2::Mode::Write);

        if (!bpWriter)
        {
            throw std::ios_base::failure(
                "ERROR: failed to open ADIOS bpWriter\n");
        }

        for (int step = 0; step < NSTEPS; step++)
        {
            for (int i = 0; i < Nx; i++)
            {
                GlobalArrayFixedDims[i] =
                    step * Nx * nproc * 1.0 + rank * Nx * 1.0 + (double)i;
            }

            // Create and fill the arrays whose dimensions change over time
            Ny = Nx + step;
            GlobalArray.reserve(Ny);
            for (int i = 0; i < Ny; i++)
            {
                GlobalArray[i] = rank * Ny + (double)i;
            }

            // random size per process, 5..10 each
            Nparts = rand() % 6 + 5;
            IrregularArray.reserve(Nparts);
            for (int i = 0; i < Nparts; i++)
            {
                IrregularArray[i] = rank * Nx + (float)i;
            }

            // 1. and 5. Writing a global scalar from only one process
            if (rank == 0)
            {
                // 5. Writing a global constant scalar only once
                if (step == 0)
                {
                    bpWriter->Write<unsigned int>(varNX, Nx);
                    bpWriter->Write<int>("Nproc", nproc);
                }
                bpWriter->Write<unsigned int>(varNY, Ny);
            }

            // 2. and 6. Writing a local scalar on every process. Will be shown
            // at reading as a 1D array
            if (step == 0)
            {
                bpWriter->Write<int>(varProcessID, ProcessID);
            }
            bpWriter->Write<unsigned int>(varNparts, Nparts);

            // 3.
            // Make a 1D selection to describe the local dimensions of the
            // variable we write and its offsets in the global spaces
            adios2::SelectionBoundingBox sel({rank * Nx}, {Nx});
            varGlobalArrayFixedDims.SetSelection(sel);
            bpWriter->Write<double>(varGlobalArrayFixedDims,
                                    GlobalArrayFixedDims.data());

            // 4.a Local array that will be shown at read as an irregular 2D
            // global array
            bpWriter->Write<float>(varLocalArrayFixedDims,
                                   LocalArrayFixedDims.data());
            // 4.b Local array that will be shown at read as a joined 1D
            // global array
            bpWriter->Write<float>(varLocalArrayFixedDimsJoined,
                                   LocalArrayFixedDims.data());

            // 7.
            // Make a 1D selection to describe the local dimensions of the
            // variable we write and its offsets in the global spaces
            // Also set the global dimension of the array now

            // TODO: varGlobalArray.SetGlobalDimension({nproc * Ny});
            varGlobalArray.m_Shape = adios2::Dims({nproc * Ny});
            varGlobalArray.SetSelection({rank * Ny}, {Ny});
            bpWriter->Write<double>(varGlobalArray, GlobalArray.data());

            // 8. Local array that will be shown at read as a ragged 2D global
            // array with ranks in slow dimension
            varIrregularArray.SetSelection({}, {1, Nparts});
            bpWriter->Write<float>(varIrregularArray, IrregularArray.data());

            // Indicate we are done for this step
            // N-to-M Aggregation, disk I/O will be performed during this call,
            // unless
            // time aggregation postpones all of that to some later step
            bpWriter->Advance();
        }

        // Called once: indicate that we are done with this output for the run
        bpWriter->Close();
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

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return 0;
}
