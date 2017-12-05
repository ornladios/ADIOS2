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

adios2::ADIOS *ad = nullptr;
adios2::Engine *h5writer;
adios2::Variable<double> *varT = nullptr;
adios2::Variable<unsigned int> *varGndx = nullptr;

IO::IO(const Settings &s, MPI_Comm comm)
{
    std::string suffix = ".h5";
    m_outputfilename = s.outputfile + suffix;

    int ss = s.outputfile.size();
    int pos = s.outputfile.find(suffix);
    if ((ss > suffix.size()) && (pos == ss - suffix.size()))
    {
        // Your code here
      m_outputfilename = s.outputfile;
    }

    ad = new adios2::ADIOS(comm, adios2::DebugOFF);

    // Define method for engine creation
    // 1. Get method def from config file or define new one

    adios2::IO &h5io = ad->DeclareIO("output");
    if (!h5io.InConfigFile())
    {
        // if not defined by user, we can change the default settings
        // BPFileWriter is the default engine
        h5io.SetEngine("HDF5Writer");
    }

    varGndx = &h5io.DefineVariable<unsigned int>("gndx");
    h5io.DefineVariable<unsigned int>("gndy");

    // define T as 2D global array
    varT = &h5io.DefineVariable<double>(
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

    h5writer = &(h5io.Open(m_outputfilename, adios2::Mode::Write, comm));
}

IO::~IO()
{
    h5writer->Close();
    // delete ad;
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
    // adios2::SelectionBoundingBox sel({s.offsx, s.offsy}, {s.ndx, s.ndy});
    // varT->SetSelection(sel);

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
    // adios2::SelectionBoundingBox memspace =
    //    adios2::SelectionBoundingBox({1, 1}, {s.ndx, s.ndy});
    // varT->SetMemorySelection(memspace);

    h5writer->BeginStep();
    h5writer->PutSync<double>(*varT, ht.data_noghost().data());
    // h5writer->Write(*varT, ht.data_noghost().data());
    h5writer->PutSync<unsigned int>(*varGndx, &(s.gndx));
    h5writer->PutSync("gndy", &(s.gndy));

    h5writer->EndStep();

#else
    h5writer->BeginStep();
    h5writer->PutSync<double>(*varT, ht.data_noghost().data());
    h5writer->EndStep();

#endif
}
