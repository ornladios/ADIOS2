/*
 * Simple example of writing and reading data
 * through ADIOS2 BP engine with multiple simulations steps
 * for every IO step.
 */

#include <ios>
#include <vector>
#include <iostream>

#include <adios2.h>

#if ADIOS2_USE_MPI
  #include <mpi.h>
#endif

int BPWrite(const std::string fname, const size_t N,
            int mpiRank, int mpiSize, int nSteps, float startVal){
  // Initialize the simulation data
  std::vector<float> simData(N, startVal);
 
  // Set up the ADIOS structures
  #if ADIOS2_USE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD);
  #else
    adios2::ADIOS adios;
  #endif
  adios2::IO io = adios.DeclareIO("WriteIO");
  io.SetEngine("CamBP");
  // Optionally add parameters
  // io.SetParameter("AsyncThreads", "0");
  // io.AddTransport("file");

  // Declare an array for the ADIOS data of size (NumOfProcesses * N)
  const adios2::Dims shape{static_cast<size_t>(N * mpiSize)};
  const adios2::Dims start{static_cast<size_t>(N * mpiRank)};
  const adios2::Dims count{N};
  auto data = io.DefineVariable<float>("data", shape, start, count);

  adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);
  
  // Simulation steps
  for (size_t step = 0; step < nSteps; ++step)
  {
      // Make a 1D selection to describe the local dimensions of the
      // variable we write and its offsets in the global spaces
      adios2::Box<adios2::Dims> sel({mpiRank * N}, {N});
      data.SetSelection(sel);

      // Start IO step every 10 simulation step
      if (step % 10 == 0){
        bpWriter.BeginStep();
        bpWriter.Put(data, simData.data());
        bpWriter.EndStep();
      }

      // Compute new values for the data
      // for (auto * x: simData) 
      for (int i = 0; i < N; i++)
        simData[i] += i;
  }

  bpWriter.Close();
  return 0;
}

int BPRead(const std::string fname, const size_t N,
            int mpiRank, int mpiSize, int nSteps){
  // Create ADIOS structures
  #if ADIOS2_USE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD);
  #else
    adios2::ADIOS adios;
  #endif
  adios2::IO io = adios.DeclareIO("ReadIO");
  io.SetEngine("CamBP");

  adios2::Engine bpReader = io.Open(fname, adios2::Mode::Read);

  auto data = io.InquireVariable<float>("data");
  std::cout << "Steps expected by the reader: " << bpReader.Steps() << std::endl;
  std::cout << "Rank " << mpiRank << " expects " << data.Shape()[0];
  std::cout  << " elements" << std::endl;

  // Create the local buffer and initialize the access point in the ADIOS file
  std::vector<float> simData(N); //set size to N
  const adios2::Dims start{mpiRank * N};
  const adios2::Dims count{N};
  const adios2::Box<adios2::Dims> sel(start, count);
  data.SetSelection(sel);
  
  // Read the data in each of the ADIOS steps
  for (size_t step = 0; step < nSteps / 10; step++)
  {
      data.SetStepSelection({step, 1});
      bpReader.Get(data, simData.data());
      bpReader.PerformGets();
      std::cout << "Simualation step " << step * 10 << " : ";
      std::cout << simData.size() << " elements: " << simData[1] << std::endl;
  }
  bpReader.Close();
  return 0;
}

int main(int argc, char **argv){
  #if ADIOS2_USE_MPI
      MPI_Init(nullptr, nullptr);
  #endif

  const std::string fname("BPAnaWriteRead.bp");
  int mpiRank = 0, mpiSize = 1;
  const size_t N = 100;
  int nSteps = 100, ret = 0;

  #if ADIOS2_USE_MPI
      MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
      MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
  #endif

  ret = BPWrite(fname, N, mpiRank, mpiSize, nSteps, 5);
  ret = BPWrite(fname, N, mpiRank, mpiSize, nSteps, 10);
  ret = BPRead(fname, N, mpiRank, mpiSize, nSteps);

  #if ADIOS2_USE_MPI
      MPI_Finalize();
  #endif
  return ret;
}
