/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * writer.cpp
 *
 *  Created on: Feb 13, 2017
 *      Author: pnorbert
 */

#include <iostream>
#include <vector>

#include "ADIOS_CPP.h"
#include <mpi.h>

namespace adios
{
typedef enum { VARYING_DIMENSION = -1, LOCAL_VALUE = 0, GLOBAL_VALUE = 1 };
}

int main(int argc, char *argv[])
{
  int rank, nproc;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);
  const bool adiosDebug = true;

  adios::ADIOS adios(MPI_COMM_WORLD, adiosDebug);

  // Application variable
  const unsigned int Nx = 10;
  //  GSE === user-defined structure
  typedef struct s1_t
  {
    int a;
    float b;
    double c;
  };
  const int Nparts = rand() % 6 + 5; // random size per process, 5..10 each

  std::vector<s1_t> NiceArray(Nx);
  for (int i = 0; i < Nx; i++)
  {
    NiceArray.push_back(ss1_t());
    NiceArray[i].a = rank * Nx + (double)i;
    NiceArray[i].b = 100.0 * rank * Nx + 100.0 * (double)i;
    NiceArray[i].c = 10000.0 * rank * Nx + 10000.0 * (double)i;
  }

  try
  {
    /* GSE === compound type declaration borrowed heavily from HDF5 style */
    adios::CompType mtype(sizeof(s1_t));
    mtype.insertMember("a_name", OFFSET(s1_t, a), PredType::NATIVE_INT);
    mtype.insertMember("c_name", OFFSET(s1_t, c), PredType::NATIVE_DOUBLE);
    mtype.insertMember("b_name", OFFSET(s1_t, b), PredType::NATIVE_FLOAT);

    // Define group and variables with transforms, variables don't have
    // functions, only group can access variables
    adios::Variable<unsigned int> &varNX = adios.DefineVariable<unsigned int>(
        "NX"); // global single-value across processes
    adios::Variable<int> &varNproc = adios.DefineVariable<int>(
        "nproc", adios::GLOBAL_VALUE); // same def for global value
    adios::Variable<int> &varNparts = adios.DefineVariable<int>(
        "Nparts",
        adios::LOCAL_VALUE); // a single-value different on every process

    // GSE === template necessary or useful here? Extra argument for
    // previously-built compound type information */
    adios::Variable<s1_t> &varNice = adios.DefineVariable<s1_t>(
        "Nice", {nproc * Nx}, mtype); // 1D global array

    // add transform to variable in group...not executed (just testing API)
    adios::Transform bzip2 = adios::transform::BZIP2();
    varNice->AddTransform(bzip2, 1);

    // Define method for engine creation
    // 1. Get method def from config file or define new one
    adios::Method &bpWriterSettings = adios.GetMethod("output");
    if (bpWriterSettings.undeclared())
    {
      // if not defined by user, we can change the default settings
      bpWriterSettings.SetEngine("BP"); // BP is the default engine
      bpWriterSettings.AddTransport(
          "File", "lucky=yes"); // ISO-POSIX file is the default transport
                                // Passing parameters to the transport
      bpWriterSettings.SetParameters("have_metadata_file",
                                     "yes"); // Passing parameters to the engine
      bpWriterSettings.SetParameters("Aggregation",
                                     (nproc + 1) / 2); // number of aggregators
    }

    // Open returns a smart pointer to Engine containing the Derived class
    // Writer
    // "w" means we overwrite any existing file on disk, but AdvanceStep will
    // append steps later.
    auto bpWriter = adios.Open("myNumbers.bp", "w", bpWriterSettings);

    if (bpWriter == nullptr)
      throw std::ios_base::failure("ERROR: failed to open ADIOS bpWriter\n");

    if (rank == 0)
    {
      // Writing a global scalar from only one process
      bpWriter->Write<unsigned int>(varNX, Nx);
    }
    // Writing a local scalar on every process. Will be shown at reading as a 1D
    // array
    bpWriter->Write<int>(varNparts, Nparts);

    // Writing a global scalar on every process is useless. Information will be
    // thrown away
    // and only rank 0's data will be in the output
    bpWriter->Write<int>(varNproc, nproc);

    // Make a 1D selection to describe the local dimensions of the variable we
    // write and
    // its offsets in the global spaces
    adios::Selection &sel = adios.SelectionBoundingBox(
        {Nx}, {rank * Nx}); // local dims and offsets; both as list
    varNice.SetSelection(sel);

    // GSE === Template useful or necessary here?   We have to treat this as a
    // void* inside ADIOS...
    bpWriter->Write<sl_t>(varNice, NiceArray.data());

    // Indicate we are done for this step
    // N-to-M Aggregation, disk I/O will be performed during this call, unless
    // time aggregation postpones all of that to some later step
    bpWriter->Advance();
    bpWriter->AdvanceAsync(callback_func_to_notify_me);

    // Called once: indicate that we are done with this output for the run
    bpWriter->Close();
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
