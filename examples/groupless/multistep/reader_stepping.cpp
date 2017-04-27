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
#include <mpi.h>

int main(int argc, char *argv[])
{
    int rank, nproc;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    const bool adiosDebug = true;

    adios::ADIOS adios(MPI_COMM_WORLD, adiosDebug);

    // Application variable
    std::vector<double> NiceArray;
    std::vector<float> RaggedArray;
    unsigned int Nx;
    int Nparts;
    int Nwriters;
    int Nsteps;

    try
    {
        // Define method for engine creation
        // 1. Get method def from config file or define new one
        adios::Method &bpReaderSettings = adios.GetMethod("input");
        if (bpReaderSettings.undeclared())
        {
            // if not defined by user, we can change the default settings
            bpReaderSettings.SetEngine("BP"); // BP is the default engine
            bpReaderSettings.SetParameters("Stepping",
                                           true); // see only one step at a time
        }

        // Create engine smart pointer due to polymorphism,
        // Default behavior
        // auto bpReader = adios.Open( "myNumbers.bp", "r" );
        // this would just open with a default transport, which is "BP"
        try
        {
            auto bpReader = adios.Open("myNumbers.bp", "r", bpReaderSettings);

            while (true)
            {
                /* NX */
                bpReader->Read<unsigned int>("NX",
                                             &Nx); // read a Global scalar which
                                                   // has a single value in a
                                                   // step

                /* nproc */
                bpReader->Read<int>("nproc", &Nwriters); // also a global scalar

                /* Nparts */
                // Nparts local scalar is presented as a 1D array of Nwriters
                // elements.
                // We can read all as a 1D array
                std::vector<int> partsV(Nwriters);
                bpReader->Read<int>("Nparts",
                                    &partsV); // read with string name, no
                                              // selection => read whole array

                /* Nice */
                // inquiry about a variable, whose name we know
                std::shared_ptr<adios::Variable<void>> varNice =
                    bpReader.InquiryVariable("Nice");

                if (varNice == nullptr)
                    throw std::ios_base::failure("ERROR: failed to find "
                                                 "variable 'myDoubles' in "
                                                 "input file\n");

                // ? how do we know about the type? std::string varNice->m_Type
                unsigned long long int gdim =
                    varMyDoubles->m_Shape[0]; // ?member var or
                                                         // member func?
                unsigned long long int ldim = gdim / nproc;
                unsigned long long int offs = rank * ldim;
                if (rank == nproc - 1)
                {
                    ldim = gdim - (ldim * gdim);
                }

                NiceArray.reserve(ldim);

                // Make a 1D selection to describe the local dimensions of the
                // variable
                // we READ and
                // its offsets in the global spaces
                std::unique_ptr<adios::Selection> bbsel =
                    adios.SelectionBoundingBox(
                        {ldim}, {offs}); // local dims and offsets; both as list
                varNice->SetSelection(bbsel);
                bpReader->Read<double>(varNice, NiceArray.data());

                /* Ragged */
                // inquiry about a variable, whose name we know
                std::shared_ptr<adios::Variable<void>> varRagged =
                    bpReader.InquiryVariable("Ragged");
                if (varRagged->m_Shape[1] !=
                    adios::VARYING_DIMENSION)
                {
                    throw std::ios_base::failure(
                        "Unexpected condition: Ragged array's fast "
                        "dimension "
                        "is supposed to be VARYING_DIMENSION\n");
                }
                // We have here varRagged->sum_nblocks, nsteps, nblocks[],
                // global
                if (rank < varRagged->nblocks[0]) // same as rank < Nwriters in
                                                  // this example
                {
                    // get per-writer size information
                    varRagged->InquiryBlocks();
                    // now we have the dimensions per block

                    unsigned long long int ldim =
                        varRagged->blockinfo[rank].m_Dimensions[0];
                    RaggedArray.resize(ldim);

                    std::unique_ptr<adios::Selection> wbsel =
                        adios.SelectionWriteblock(rank);
                    varRagged->SetSelection(wbsel);
                    bpReader->Read<float>(varRagged, RaggedArray.data());

                    // We can use bounding box selection as well
                    std::unique_ptr<adios::Selection> rbbsel =
                        adios.SelectionBoundingBox({1, ldim}, {rank, 0});
                    varRagged->SetSelection(rbbsel);
                    bpReader->Read<float>(varRagged, RaggedArray.data());
                }

                /* Extra help to process Ragged */
                int maxRaggedDim = varRagged->GetMaxGlobalDimensions(
                    1); // contains the largest
                std::vector<int> raggedDims =
                    varRagged->GetVaryingGlobalDimensions(
                        1); // contains all individual sizes in that
                            // dimension

                // promise to not read more from this step
                bpReader->Release();

                // want to move on to the next available step
                // bpReader->Advance(adios::NextStep);
                // bpReader->Advance(adios::LatestStep);
                bpReader->Advance(); // default is adios::NextStep
            }

            // Close file/stream
            bpReader->Close();
        }
        catch (adios::end_of_stream &e)
        {
            if (rank == 0)
            {
                std::cout << "Reached end of stream, end processing loop.\n";
            }
            // Close file/stream
            bpReader->Close();
        }
        catch (adios::file_not_found &e)
        {
            if (rank == 0)
            {
                std::cout << "File/stream does not exist, quit.\n";
            }
        }
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
