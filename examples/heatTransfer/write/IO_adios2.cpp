/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * IO_ADIOS2.cpp
 *
 *  Created on: Feb 2017
 *      Author: Norbert Podhorszki
 */

#include "IO.h"

#include <string>

#include <adios2.h>

static int rank_saved;
adios::ADIOS *ad = nullptr;
std::shared_ptr<adios::Engine> bpWriter;
adios::Variable<double> *varT = nullptr;
adios::Variable<unsigned int> *varGndx = nullptr;

IO::IO(const Settings &s, MPI_Comm comm)
{
    rank_saved = s.rank;
    m_outputfilename = s.outputfile + ".bp";
    ad = new adios::ADIOS("adios2.xml", comm, adios::Verbose::INFO);

    // Define method for engine creation
    // 1. Get method def from config file or define new one

    adios::Method &bpWriterSettings = ad->DeclareMethod("output");
    if (!bpWriterSettings.IsUserDefined())
    {
        // if not defined by user, we can change the default settings
        // BPFileWriter is the default engine
        bpWriterSettings.SetEngine("ADIOS1Writer");
        // Allow an extra thread for data processing
        bpWriterSettings.AllowThreads(1);
        // ISO-POSIX file is the default transport
        // Passing parameters to the transport
        bpWriterSettings.AddTransport("File", "library=MPI-IO");

        const std::string aggregatorsParam("Aggregators=" +
                                           std::to_string((s.nproc + 1) / 2));
        bpWriterSettings.SetParameters("have_metadata_file=yes",
                                       aggregatorsParam);
    }

    //    ad->DefineScalar<unsigned int>("gndx", true);
    varGndx = &ad->DefineVariable<unsigned int>("gndx");
    ad->DefineVariable<unsigned int>("gndy");

    // define T as 2D global array
    varT = &ad->DefineArray<double>(
        "T",
        // Global dimensions
        {s.gndx, s.gndy},
        // starting offset of the local array in the global space
        {s.offsx, s.offsy},
        // local size, could be defined later using SetSelection()
        {s.ndx, s.ndy});

    // add transform to variable
    // adios::Transform tr = adios::transform::BZIP2( );
    // varT.AddTransform( tr, "" );
    // varT.AddTransform( tr,"accuracy=0.001" );  // for ZFP

    bpWriter = ad->Open(m_outputfilename, "w", comm, bpWriterSettings);

    if (bpWriter == nullptr)
        throw std::ios_base::failure("ERROR: failed to open ADIOS bpWriter\n");
}

IO::~IO()
{
    bpWriter->Close();
    delete ad;
}

void IO::write(int step, const HeatTransfer &ht, const Settings &s,
               MPI_Comm comm)
{
#if 1

    /* This selection is redundant and not required, since we defined
     * the selection already in DefineVariable(). It is here just as an example.
     */
    // Make a selection to describe the local dimensions of the variable we
    // write and its offsets in the global spaces. This could have been done in
    // adios.DefineVariable()
    adios::SelectionBoundingBox sel({s.offsx, s.offsy}, {s.ndx, s.ndy});
    varT->SetSelection(sel);

    /* Select the area that we want to write from the data pointer we pass to
       the
       writer.
       Think HDF5 memspace, just not hyperslabs, only a bounding box selection.
       Engine will copy this bounding box from the data pointer into the output
       buffer.
       Size of the bounding box should match the "space" selection which was
       given
       above.
       Default memspace is always the full selection.
    */
    adios::SelectionBoundingBox memspace =
        adios::SelectionBoundingBox({1, 1}, {s.ndx, s.ndy});
    varT->SetMemorySelection(memspace);

    bpWriter->Write<unsigned int>(*varGndx, s.gndx);
    bpWriter->Write<unsigned int>("gndy", s.gndy);
    bpWriter->Write<double>(*varT, ht.data_noghost().data());
    bpWriter->Advance();

#else

    bpWriter->Write<double>(*varT, ht.data_noghost().data());
    bpWriter->Advance();

#endif
}
