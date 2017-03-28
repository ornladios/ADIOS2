/*
 * reader.cpp
 *
 *  Created on: Feb 13, 2017
 *      Author: pnorbert
 */

#include <iostream>
#include <vector>

#include "ADIOS_CPP.h"
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
      // By default we see all steps available in a file, so the next line is
      // not needed
      bpReaderSettings.SetParameters("Stepping", false);
    }

    // Create engine smart pointer due to polymorphism,
    // Default behavior
    // auto bpReader = adios.Open( "myNumbers.bp", "r" );
    // this would just open with a default transport, which is "BP"
    auto bpReader = adios.Open("myNumbers.bp", "r", bpReaderSettings);

    // All the above is same as default use:
    // auto bpReader = adios.Open( "myNumbers.bp", "r");

    if (bpReader == nullptr)
      throw std::ios_base::failure("ERROR: failed to open ADIOS bpReader\n");

    /* Note: there is no global number of steps. Each variable has its own
     * number of steps */

    /* NX */
    /* There is a single value for each step. We can read all into a 1D array
     * with a step selection.
     * We can also just conveniently get the first with a simple read statement.
     * Steps are not automatically presented as an array dimension and read does
     * not read it as array.
     */
    unsigned int Nx;
    bpReader->Read<unsigned int>(
        "NX", &Nx); // read a Global scalar which has a single value in a step

    std::shared_ptr<adios::Variable<void>> varNx =
        bpReader.InquiryVariable("Nx");
    std::vector<int> Nxs(varNx->nsteps()); // number of steps available
    // make a StepSelection to select multiple steps. Args: From, #of
    // consecutive steps
    std::unique_ptr<adios::StepSelection> stepsNx =
        adios.StepSelection(0, varNx->nsteps());
    // ? How do we make a selection for an arbitrary list of steps ?
    varNX.SetStepSelection(stepsNx);
    bpReader->Read<unsigned int>(varNx, Nxs.data());

    auto itmax = std::max_element(std::begin(Nxs), std::end(Nxs));
    auto itmin = std::min_element(std::begin(Nxs), std::end(Nxs));
    if (*itmin != *itmax)
    {
      throw std::ios_base::failure("ERROR: NX is not the same at all steps!\n");
    }

    /* nproc */
    bpReader->Read<int>("nproc", &Nwriters); // also a global scalar

    /* Nparts */
    // Nparts local scalar is presented as a 1D array of Nwriters elements.
    // We can read all steps into a 2D array of nproc * Nwriters
    std::shared_ptr<adios::Variable<void>> varNparts =
        bpReader.InquiryVariable("Nparts");
    std::vector<int> partsV(Nproc * Nwriters);
    varNparts->SetStepSelection(adios.StepSelection(0, varNparts->nsteps()));
    bpReader->Read<int>(
        varNparts,
        partsV.data()); // missing spatial selection = whole array at each step

    /* Nice */
    // inquiry about a variable, whose name we know
    std::shared_ptr<adios::Variable<void>> varNice =
        bpReader.InquiryVariable("Nice");

    if (varNice == nullptr)
      throw std::ios_base::failure(
          "ERROR: failed to find variable 'myDoubles' in input file\n");

    // ? how do we know about the type? std::string varNice->m_Type
    unsigned long long int gdim =
        varMyDoubles->m_GlobalDimensions[0]; // ?member var or member func?
    unsigned long long int ldim = gdim / nproc;
    unsigned long long int offs = rank * ldim;
    if (rank == nproc - 1)
    {
      ldim = gdim - (ldim * gdim);
    }

    NiceArray.reserve(ldim);

    // Make a 1D selection to describe the local dimensions of the variable we
    // READ and
    // its offsets in the global spaces
    std::unique_ptr<adios::Selection> bbsel = adios.SelectionBoundingBox(
        {ldim}, {offs}); // local dims and offsets; both as list
    bpReader->Read<double>("Nice", bbsel,
                           NiceArray.data()); // Base class Engine own the
                                              // Read<T> that will call
                                              // overloaded Read from Derived

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
    if (rank < varRagged->nblocks[0]) // same as rank < Nwriters in this example
    {
      // get per-writer size information
      varRagged->InquiryBlocks();
      // now we have the dimensions per block

      unsigned long long int ldim = varRagged->blockinfo[rank].m_Dimensions[0];
      RaggedArray.resize(ldim);

      std::unique_ptr<adios::Selection> wbsel = adios.SelectionWriteblock(rank);
      bpReader->Read<float>("Ragged", wbsel, RaggedArray.data());

      // We can use bounding box selection as well
      std::unique_ptr<adios::Selection> rbbsel =
          adios.SelectionBoundingBox({1, ldim}, {rank, 0});
      bpReader->Read<float>("Ragged", rbbsel, RaggedArray.data());
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
