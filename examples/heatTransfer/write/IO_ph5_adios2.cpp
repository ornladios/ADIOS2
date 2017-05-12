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
std::shared_ptr<adios::Engine> h5writer;
adios::Variable<double> *varT = nullptr;
adios::Variable<unsigned int> *varGndx = nullptr;

IO::IO(const Settings &s, MPI_Comm comm)
{
    rank_saved = s.rank;
    m_outputfilename = s.outputfile + ".h5";
    //adios::ADIOS adios(comm, adios::Verbose::INFO, false);
    ad = new adios::ADIOS(comm, adios::Verbose::INFO, false);

    // Define method for engine creation
    // 1. Get method def from config file or define new one

    adios::Method &h5writerSettings = ad->DeclareMethod("output");
    if (!h5writerSettings.IsUserDefined())
    {
        // if not defined by user, we can change the default settings
        // BPFileWriter is the default engine
        h5writerSettings.SetEngine("HDF5Writer");
        // Allow an extra thread for data processing       

        const std::string aggregatorsParam("Aggregators=" +
                                           std::to_string((s.nproc + 1) / 2));
        h5writerSettings.SetParameters("have_metadata_file=yes",
                                       aggregatorsParam);
    }

    //    ad->DefineScalar<unsigned int>("gndx", true);
    varGndx = &(ad->DefineVariable<unsigned int>("gndx"));
    ad->DefineVariable<unsigned int>("gndy");

    // define T as 2D global array
    varT = &(ad->DefineArray<double>(
        "T",
        // Global dimensions
        {s.gndx, s.gndy},
        // starting offset of the local array in the global space
        {s.offsx, s.offsy},
        // local size, could be defined later using SetSelection()
      {s.ndx, s.ndy}));

    // add transform to variable
    // adios::Transform tr = adios::transform::BZIP2( );
    // varT.AddTransform( tr, "" );
    // varT.AddTransform( tr,"accuracy=0.001" );  // for ZFP

    h5writer = ad->Open(m_outputfilename, "w", comm, h5writerSettings);

    if (h5writer == nullptr)
        throw std::ios_base::failure("ERROR: failed to open ADIOS h5writer\n");
}

IO::~IO()
{
    h5writer->Close();
    //delete ad;
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
    //adios::SelectionBoundingBox sel({s.offsx, s.offsy}, {s.ndx, s.ndy});
    //varT->SetSelection(sel);

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
    //adios::SelectionBoundingBox memspace =
    //    adios::SelectionBoundingBox({1, 1}, {s.ndx, s.ndy});
    //varT->SetMemorySelection(memspace);

    h5writer->Write<double>(*varT, ht.data_noghost().data());
    //h5writer->Write(*varT, ht.data_noghost().data());
    h5writer->Write<unsigned int>(*varGndx, s.gndx);
    h5writer->Write<unsigned int>("gndy", s.gndy);

    h5writer->Advance();

#else

    h5writer->Write<double>(*varT, ht.data_noghost().data());
    h5writer->Advance();

#endif
}
