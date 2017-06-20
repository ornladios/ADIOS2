/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * reader.cpp
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

#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(adios2::DebugON);
#endif

    // Info variables from the file
    int Nwriters;
    int Nsteps;
    // Data variables from the file
    // 1. Global value, constant across processes, constant over time
    unsigned int Nx;
    // 2. Local value, varying across processes, constant over time
    std::vector<int> ProcessID;
    // 3. Global array, global dimensions, local dimensions and offsets are
    // constant over time
    std::vector<double> GlobalArrayFixedDims;

    // 4. Local array, local dimensions are
    // constant over time (but different across processors here)
    std::vector<float> LocalArrayFixedDims;

    // 5. Global value, constant across processes, VARYING value over time
    unsigned int Ny;
    // 6. Local value, varying across processes, VARYING over time
    std::vector<unsigned int> Nparts;
    // 7. Global array, dimensions and offsets are VARYING over time
    std::vector<double> GlobalArray;
    // 8. Local array, dimensions and offsets are VARYING over time
    std::vector<float> IrregularArray;

    try
    {
        // Define method for engine creation
        // 1. Get method def from config file or define new one
        adios2::IO &bpReaderSettings = adios.DeclareIO("input");
        if (!bpReaderSettings.InConfigFile())
        {
            // if not defined by user, we can change the default settings
            bpReaderSettings.SetEngine(
                "ADIOS1Reader"); // BP is the default engine
            // see only one step at a time
            // this is default, nothing to be done
        }

        auto bpReader =
            bpReaderSettings.Open("myNumbers.bp", adios2::OpenMode::Read);
        if (!bpReader)
        {
            int step = 0;
            while (bpReader->GetAdvanceStatus() == adios2::AdvanceStatus::OK)
            {
                std::cout << "Process step " << std::to_string(step)
                          << std::endl;
                /* NX */
                if (step == 0)
                {
                    // read a Global scalar which has a single value in a step
                    adios2::Variable<int> *vNproc =
                        bpReader->InquireVariable<int>("Nproc");
                    Nwriters = vNproc->m_Data[0];
                    std::cout << "# of writers = " << Nwriters << std::endl;

                    adios2::Variable<unsigned int> *vNX =
                        bpReader->InquireVariable<unsigned int>("NX");
                    Nx = vNX->m_Data[0];
                    // bpReader->Read<unsigned int>("NX", &Nx);
                    std::cout << "NX = " << Nx << std::endl;
                }

                adios2::Variable<unsigned int> *vNY =
                    bpReader->InquireVariable<unsigned int>("NY");
                Ny = vNY->m_Data[0];
                std::cout << "NY = " << Ny << std::endl;

                // promise to not read more from this step
                bpReader->Release();

                // want to move on to the next available step
                // bpReader->Advance(adios2::NextStep);
                // bpReader->Advance(adios2::LatestStep);
                bpReader->Advance(); // default is adios2::NextStep
                ++step;
            }

            // Close file/stream
            bpReader->Close();
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

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return 0;
}
