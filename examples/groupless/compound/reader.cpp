/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * reader.cpp
 *
 *  Created on: Feb 21, 2017
 *      Author: eisen, modified from basic by pnorbert
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

    //  Application variable
    //  GSE === user-defined structure
    //
    //  Like HDF5 example, read with a subset of the structure that was written.
    //  Easily handled by FFS and HDF5, but ambitious for ADIOS
    typedef struct s2_t
    {
        double c;
        int a;
    };
    unsigned int Nx;
    int Nparts;
    int Nwriters;

    try
    {
        // Define method for engine creation
        // 1. Get method def from config file or define new one
        adios::Method &bpReaderSettings = adios.GetMethod("input");
        if (bpReaderSettings.undeclared())
        {
            // if not defined by user, we can change the default settings
            bpReaderSettings.SetEngine("BP"); // BP is the default engine
        }

        // Create engine smart pointer due to polymorphism,
        // Default behavior
        // auto bpReader = adios.Open( "myNumbers.bp", "r" );
        // this would just open with a default transport, which is "BP"
        auto bpReader = adios.Open("myNumbers.bp", "r", bpReaderSettings);

        // All the above is same as default use:
        // auto bpReader = adios.Open( "myNumbers.bp", "r");

        if (bpReader == nullptr)
            throw std::ios_base::failure(
                "ERROR: failed to open ADIOS bpReader\n");

        /* Variable names are available as a vector of strings */
        std::cout << "List of variables in file: " << bpReader->VariableNames
                  << "\n";

        /* NX */
        bpReader->Read<unsigned int>(
            "NX",
            Nx); // read a Global scalar which has a single value in a step

        /* nproc */
        bpReader->Read<int>("nproc", Nwriters); // also a global scalar

        /* Nparts */
        // Nparts local scalar is presented as a 1D array of Nwriters elements.
        // We need to read a specific value the same way as reading from any 1D
        // array.
        // Make a single-value selection to describe our rank's position in the
        // 1D array of Nwriters values.
        if (rank < Nwriters)
        {
            std::shared_ptr<adios::Variable> varNparts =
                bpReader.InquiryVariable("Nparts");
            std::unique_ptr<adios::Selection> selNparts =
                adios.SelectionBoundingBox({1}, {rank});
            varNparts->SetSelection(selNparts);
            bpReader->Read<int>(varNparts, Nparts);
        }
        // or we could just read the whole array by every process
        std::vector<int> partsV(Nwriters);
        bpReader->Read<int>("Nparts",
                            partsV.data()); // read with string name, no
                                            // selection => read whole array

        std::vector<int> partsV;
        bpReader->Read<int>("Nparts",
                            partsV); // read with string name, no selection =>
                                     // read whole array
        (Nwriters == partsV.size())

            /* Nice */
            // inquiry about a variable, whose name we know
            /* GSE === compound type declaration borrowed heavily from HDF5
               style */
            adios::CompType mtype(sizeof(s2_t));
        mtype.insertMember("c_name", OFFSET(s2_t, c), PredType::NATIVE_DOUBLE);
        mtype.insertMember("a_name", OFFSET(s2_t, a), PredType::NATIVE_INT);

        /*
         *   GSE === this is a bit conceptually different.  There was no real
         *   check in the prior API that the variable in the file was in any way
         *   "compatible" with how we planned to read it.  This could be done
         *   here, by providing the details of the structure that we are
         *   prepared to read in the inquiryVariable, or if we had an API that
         *   allowed more introspection, we could perhaps support a query on the
         *   adios::Variable and a later operation that informed it of the data
         *   type that we were prepared to extract from it.
         */
        std::shared_ptr<adios::Variable> varNice =
            bpReader.InquiryVariable("Nice", mtype);

        if (varNice == nullptr)
            throw std::ios_base::failure("ERROR: failed to find variable "
                                         "'myDoubles' in input file\n");

        // ? how do we know about the type? std::string varNice->m_Type
        uint64_t gdim =
            varNice->m_GlobalDimensions[0]; // ?member var or member func?
        uint64_t ldim = gdim / nproc;
        uint64_t offs = rank * ldim;
        if (rank == nproc - 1)
        {
            ldim = gdim - (ldim * gdim);
        }

        NiceArray.reserve(ldim);

        // Make a 1D selection to describe the local dimensions of the variable
        // we
        // READ and
        // its offsets in the global spaces
        std::unique_ptr<adios::Selection> bbsel = adios.SelectionBoundingBox(
            {ldim}, {offs}); // local dims and offsets; both as list
        varNice->SetSelection(bbsel);
        // GSE ===   Again, templated here?
        bpReader->Read<s2_t>(varNice, NiceArray.data());

        /* Ragged */
        // inquiry about a variable, whose name we know
        std::shared_ptr<adios::Variable<void>> varRagged =
            bpReader.InquiryVariable("Ragged");
        if (varRagged->m_GlobalDimensions[1] != adios::VARYING_DIMENSION)
        {
            throw std::ios_base::failure(
                "Unexpected condition: Ragged array's fast dimension "
                "is supposed to be VARYING_DIMENSION\n");
        }
        // We have here varRagged->sum_nblocks, nsteps, nblocks[], global
        if (rank <
            varRagged->nblocks[0]) // same as rank < Nwriters in this example
        {
            // get per-writer size information
            varRagged->InquiryBlocks();
            // now we have the dimensions per block

            unsigned long long int ldim =
                varRagged->blockinfo[rank].m_Dimensions[1];
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
        int maxRaggedDim =
            varRagged->GetMaxGlobalDimensions(1); // contains the largest
        std::vector<int> raggedDims = varRagged->GetVaryingGlobalDimensions(
            1); // contains all individual sizes in that dimension

        // Close file/stream
        bpReader->Close();
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
