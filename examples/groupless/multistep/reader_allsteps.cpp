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

template <class T>
T **Make2DArray(int nRows, int nCols)
{
    T **ptr = new T *[nRows];
    ptr[0] = new T[nRows * nCols];
    for (int i = 1; i < nRows; i++)
    {
        ptr[i] = ptr[i - 1] + nCols;
    }
    return ptr;
}

template <class T>
void Delete2DArray(T **ptr)
{
    delete[] ptr[0];
    delete[] ptr;
}

template <class T>
void Print2DArray(T **ptr, int nRows, int nCols, std::string name)
{
    std::cout << name << " = { \n";
    for (int step = 0; step < nRows; step++)
    {
        std::cout << "    { ";
        for (int col = 0; col < nCols; col++)
        {
            std::cout << ptr[step][col] << " ";
        }
        std::cout << "}\n";
    }
    std::cout << "}\n";
}

int main(int argc, char *argv[])
{
    int rank, nproc;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    const bool adiosDebug = true;

    adios::ADIOS adios(MPI_COMM_WORLD, adios::Verbose::WARN);

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
    std::vector<unsigned int> Nys;
    // 6. Local value, varying across processes, VARYING over time
    // Read as a 2D array, time as first dimension

    // 7. Global array, dimensions and offsets are VARYING over time
    std::vector<double> GlobalArray;
    // 8. Local array, dimensions and offsets are VARYING over time
    std::vector<float> IrregularArray;

    try
    {
        // Define method for engine creation
        // 1. Get method def from config file or define new one
        adios::Method &bpReaderSettings = adios.DeclareMethod("input");
        if (!bpReaderSettings.IsUserDefined())
        {
            // if not defined by user, we can change the default settings
            bpReaderSettings.SetEngine(
                "ADIOS1Reader"); // BP is the default engine
            // see only one step at a time
            bpReaderSettings.SetParameters("OpenAsFile");
        }

        // Create engine smart pointer due to polymorphism,
        // Default behavior
        // auto bpReader = adios.Open( "myNumbers.bp", "r" );
        // this would just open with a default transport, which is "BP"
        auto bpReader = adios.Open("myNumbers.bp", "r", bpReaderSettings);

        if (bpReader == nullptr)
            throw std::ios_base::failure(
                "ERROR: failed to open ADIOS bpReader\n");

        /* Note: there is no global number of steps. Each variable has its
         * own
         * number of steps */

        adios::Variable<int> *vNproc = bpReader->InquireVariableInt("Nproc");
        Nwriters = vNproc->m_Data[0];
        std::cout << "# of writers = " << Nwriters << std::endl;

        /* NX */
        /* There is a single value written once.
         */
        // read a Global scalar which has a single value in a step
        adios::Variable<unsigned int> *vNX =
            bpReader->InquireVariableUInt("NX");
        Nx = vNX->m_Data[0];
        // bpReader->Read<unsigned int>("NX", &Nx);
        std::cout << "NX = " << Nx << std::endl;

        /* NY */
        /* We can read all into a 1D array with a step selection.
           Steps are not automatically presented as an array dimension
           and read does not read it as array by default.
        */
        adios::Variable<unsigned int> *vNY =
            bpReader->InquireVariableUInt("NY");
        Nys.resize(vNY->GetNSteps()); // number of steps available
        // make a StepSelection to select multiple steps. Args: From, #of
        // consecutive steps
        // ? How do we make a selection for an arbitrary list of steps ?
        vNY->SetStepSelection(0, vNY->GetNSteps());
        bpReader->Read<unsigned int>(*vNY, Nys.data());

        std::cout << "NY = { ";
        for (const auto &it : Nys)
        {
            std::cout << it << " ";
        }
        std::cout << "}\n";

        /* Nparts */
        // Nparts local scalar is presented as a 1D array of Nwriters
        // elements.
        // We can read all steps into a 2D array of nproc * Nwriters
        adios::Variable<unsigned int> *vNparts =
            bpReader->InquireVariableUInt("Nparts");
        unsigned int **Nparts =
            Make2DArray<unsigned int>(vNparts->GetNSteps(), Nwriters);
        vNparts->SetStepSelection(0, vNparts->GetNSteps());
        bpReader->Read<unsigned int>(*vNparts, Nparts[0]);
        Print2DArray(Nparts, vNparts->GetNSteps(), Nwriters, "Nparts");
        Delete2DArray(Nparts);

        /* GlobalArrayFixedDims */
        // inquiry about a variable, whose name we know
        std::shared_ptr<adios::Variable<void *>> vGlobalArrayFixedDims =
            bpReader->InquireVariable("GlobalArrayFixedDims");

        if (vGlobalArrayFixedDims == nullptr)
            throw std::ios_base::failure(
                "ERROR: failed to find variable "
                "'GlobalArrayFixedDims' in input file\n");

        // ? how do we know about the type? std::string varNice->m_Type
        std::size_t gdim =
            vGlobalArrayFixedDims->m_Shape[0]; // ?member var or member func?
        std::size_t count = gdim / nproc;
        std::size_t start = rank * count;
        if (rank == nproc - 1)
        {
            count = gdim - (count * gdim);
        }

        GlobalArrayFixedDims.resize(count);

        // Make a 1D selection to describe the local dimensions of the variable
        // we READ and its offsets in the global spaces
        vGlobalArrayFixedDims->SetSelection({start}, {count});
        bpReader->ScheduleRead<void>(vGlobalArrayFixedDims,
                                     GlobalArrayFixedDims.data());
        bpReader->PerformReads(adios::PerformReadMode::BLOCKINGREAD);

// overloaded Read from Derived
#if 0
        /* Ragged */
        // inquiry about a variable, whose name we know
        std::shared_ptr<adios::Variable<void>> varRagged =
            bpReader.InquiryVariable("Ragged");
        if (varRagged->m_Shape[1] != adios::VARYING_DIMENSION)
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

            unsigned long long int count =
                varRagged->blockinfo[rank].m_Dimensions[0];
            RaggedArray.resize(count);

            std::unique_ptr<adios::Selection> wbsel =
                adios.SelectionWriteblock(rank);
            bpReader->Read<float>("Ragged", wbsel, RaggedArray.data());

            // We can use bounding box selection as well
            std::unique_ptr<adios::Selection> rbbsel =
                adios.SelectionBoundingBox({1, count}, {rank, 0});
            bpReader->Read<float>("Ragged", rbbsel, RaggedArray.data());
        }

        /* Extra help to process Ragged */
        int maxRaggedDim =
            varRagged->GetMaxGlobalDimensions(1); // contains the largest
        std::vector<int> raggedDims = varRagged->GetVaryingGlobalDimensions(
            1); // contains all individual sizes in that dimension

#endif

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
