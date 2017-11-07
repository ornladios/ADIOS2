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

#define str_helper(X) #X
#define str(X) str_helper(X)
#ifndef DEFAULT_CONFIG
#define DEFAULT_CONFIG config.xml
#endif
#define DEFAULT_CONFIG_STR str(DEFAULT_CONFIG)

static int rank_saved;
adios2::ADIOS *ad = nullptr;
adios2::Engine *bpWriter = nullptr;
adios2::Variable<double> *varT = nullptr;
adios2::Variable<unsigned int> *varGndx = nullptr;

IO::IO(const Settings &s, MPI_Comm comm)
{
    rank_saved = s.rank;
    m_outputfilename = s.outputfile + ".bp";
    ad = new adios2::ADIOS(std::string(DEFAULT_CONFIG_STR), comm,
                           adios2::DebugON);

    // Define method for engine creation

    adios2::IO &bpio = *ad->InquireIO("output");
    if (!bpio.InConfigFile())
    {
        // if not defined by user, we can change the default settings
        // BPFileWriter is the default engine

        // Allow an extra thread for data processing
        // ISO-POSIX file is the default transport
        // Passing parameters to the transport
    }

    varGndx = &bpio.DefineVariable<unsigned int>("gndx");
    bpio.DefineVariable<unsigned int>("gndy");

    // define T as 2D global array
    varT = &bpio.DefineVariable<double>(
        "T",
        // Global dimensions
        {s.gndx, s.gndy},
        // starting offset of the local array in the global space
        {s.offsx, s.offsy},
        // local size, could be defined later using SetSelection()
        {s.ndx, s.ndy});

    // add transform to variable
    // adios2::Transform tr = adios2::transform::BZIP2( );
    // varT.AddTransform( tr, "" );
    // varT.AddTransform( tr,"accuracy=0.001" );  // for ZFP

    bpWriter = &bpio.Open(m_outputfilename, adios2::Mode::Write, comm);
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

    bpWriter->BeginStep();
    /* This selection is redundant and not required, since we defined
     * the selection already in DefineVariable(). It is here just as an example.
     */
    // Make a selection to describe the local dimensions of the variable we
    // write and its offsets in the global spaces. This could have been done in
    // adios.DefineVariable()
    varT->SetSelection(
        adios2::Box<adios2::Dims>({s.offsx, s.offsy}, {s.ndx, s.ndy}));

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
    varT->SetMemorySelection(adios2::Box<adios2::Dims>({1, 1}, {s.ndx, s.ndy}));

    bpWriter->PutSync<unsigned int>(*varGndx, s.gndx);
    bpWriter->PutSync<unsigned int>("gndy", s.gndy);
    bpWriter->PutSync<double>(*varT, ht.data_noghost().data());

    bpWriter->EndStep();

#else

    bpWriter->PutSync<double>(*varT, ht.data_noghost().data());
    bpWriter->EndStep();

#endif
}
