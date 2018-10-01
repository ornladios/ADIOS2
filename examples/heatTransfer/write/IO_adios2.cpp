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

adios2::ADIOS ad;
adios2::Engine bpWriter;
adios2::Variable<double> varT;
adios2::Variable<unsigned int> varGndx;

IO::IO(const Settings &s, MPI_Comm comm)
{
    m_outputfilename = MakeFilename(s.outputfile, ".bp");

    ad = adios2::ADIOS(s.configfile, comm, adios2::DebugON);

    adios2::IO bpio = ad.DeclareIO("writer");
    if (!bpio.InConfigFile())
    {
        // if not defined by user, we can change the default settings
        // BPFile is the default engine
        bpio.SetEngine("BPFile");
        bpio.SetParameters({{"num_threads", "1"}});

        // ISO-POSIX file output is the default transport (called "File")
        // Passing parameters to the transport
        bpio.AddTransport("File", {{"Library", "POSIX"}});
    }

    // define T as 2D global array
    varT = bpio.DefineVariable<double>(
        "T",
        // Global dimensions
        {s.gndx, s.gndy},
        // starting offset of the local array in the global space
        {s.offsx, s.offsy},
        // local size, could be defined later using SetSelection()
        {s.ndx, s.ndy});

    // Promise that we are not going to change the variable sizes nor add new
    // variables
    bpio.LockDefinitions();

    bpWriter = bpio.Open(m_outputfilename, adios2::Mode::Write, comm);
}

IO::~IO() { bpWriter.Close(); }

void IO::write(int step, const HeatTransfer &ht, const Settings &s,
               MPI_Comm comm)
{
    bpWriter.BeginStep();
    // using PutDeferred() you promise the pointer to the data will be intact
    // until the end of the output step.
    // We need to have the vector object here not to destruct here until the end
    // of function.
    std::vector<double> v = ht.data_noghost();
    bpWriter.Put<double>(varT, v.data());
    bpWriter.EndStep();
}
