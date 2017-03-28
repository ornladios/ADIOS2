/*
 * IO_ADIOS2.cpp
 *
 *  Created on: Feb 2017
 *      Author: Norbert Podhorszki
 */

#include "IO.h"
#include "ADIOS_CPP.h"

#include <string>

static int rank_saved;
adios::ADIOS *ad = nullptr;
std::shared_ptr<adios::Engine> bpWriter;
adios::Variable<double> *varT = nullptr;

IO::IO(const Settings &s, MPI_Comm comm)
{
  rank_saved = s.rank;
  m_outputfilename = s.outputfile + ".bp";
  ad = new adios::ADIOS("adios2.xml", comm, adios::Verbose::INFO);

  // Define method for engine creation
  // 1. Get method def from config file or define new one

  adios::Method &bpWriterSettings = ad->DeclareMethod("output");
  if (!bpWriterSettings.isUserDefined())
  {
    // if not defined by user, we can change the default settings
    bpWriterSettings.SetEngine("BP"); // BP is the default engine
    bpWriterSettings.AllowThreads(
        1); // allow 1 extra thread for data processing
    bpWriterSettings.AddTransport(
        "File", "lucky=yes"); // ISO-POSIX file is the default transport
                              // Passing parameters to the transport
    bpWriterSettings.SetParameters("have_metadata_file",
                                   "yes"); // Passing parameters to the engine
    bpWriterSettings.SetParameters(
        "Aggregation",
        std::to_string((s.nproc + 1) / 2)); // number of aggregators
  }

  // define T as 2D global array
  varT = &ad->DefineVariable<double>(
      "T", {s.gndx, s.gndy}, // Global dimensions
      {s.ndx, s.ndy}, // local size, could be defined later using SetSelection()
      {s.offsx, s.offsy} // offset of the local array in the global space
      );

  // add transform to variable
  // adios::Transform tr = adios::transform::BZIP2( );
  // varT.AddTransform( tr, "" );
  // varT.AddTransform( tr,"accuracy=0.001" );  // for ZFP

  bpWriter = ad->Open(m_outputfilename, "w", comm, bpWriterSettings,
                      adios::IOMode::COLLECTIVE);

  if (bpWriter == nullptr)
    throw std::ios_base::failure("ERROR: failed to open ADIOS bpWriter\n");
}

IO::~IO()
{
  bpWriter->Close();
  delete ad;
}

void /*IO::*/ old_style_write(int step, const HeatTransfer &ht,
                              const Settings &s, MPI_Comm comm)
{
  bpWriter->Write<double>(*varT, ht.data_noghost().data());
  bpWriter->Advance();
}

void IO::write(int step, const HeatTransfer &ht, const Settings &s,
               MPI_Comm comm)
{
  /* This selection is redundant and not required, since we defined
   * the selection already in DefineVariable(). It is here just as an example.
   */
  // Make a selection to describe the local dimensions of the variable we write
  // and
  // its offsets in the global spaces. This could have been done in
  // adios.DefineVariable()
  adios::Selection sel = adios.SelectionBoundingBox(
      {s.ndx, s.ndy},
      {s.offsx, s.offsy}); // local dims and offsets; both as list
  var2D.SetSelection(sel);

  /* Select the area that we want to write from the data pointer we pass to the
     writer.
     Think HDF5 memspace, just not hyperslabs, only a bounding box selection.
     Engine will copy this bounding box from the data pointer into the output
     buffer.
     Size of the bounding box should match the "space" selection which was given
     above.
     Default memspace is always the full selection.
  */
  adios::Selection memspace =
      adios.SelectionBoundingBox({s.ndx, s.ndy}, {1, 1});
  var2D.SetMemorySelection(memspace);

  bpWriter->Write<double>(*varT, ht.data());
  bpWriter->Advance();
}
