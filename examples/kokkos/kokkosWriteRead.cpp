/*
 * Simple example of writing and reading data
 * through ADIOS2 BP engine with multiple simulations steps
 * for every IO step.
 */

#include <ios>
#include <iostream>
#include <vector>

#include <Kokkos_Core.hpp>
#include <adios2.h>
#include <adios2/cxx11/KokkosView.h>

#define ASSERT(condition, message)                                             \
    do                                                                         \
    {                                                                          \
        if (!(condition))                                                      \
        {                                                                      \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__   \
                      << " line " << __LINE__ << ": " << message << std::endl; \
            std::terminate();                                                  \
        }                                                                      \
    } while (false)

int BPWrite(const std::string fname, const size_t N, size_t nSteps,
            const std::string engine)
{
    int rank, size;
#if ADIOS2_USE_MPI
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
#else
    rank = 0;
    size = 1;
#endif
    // Initialize the simulation data
    using mem_space = Kokkos::DefaultExecutionSpace::memory_space;
    Kokkos::View<float *, mem_space> gpuSimData("simBuffer", N);
    if (rank == 0)
    {
        Kokkos::DefaultExecutionSpace exe_space;
        std::cout << "Write 6k floats with " << engine
                  << " on memory space: " << exe_space.name() << std::endl;
    }

#if ADIOS2_USE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    adios2::ADIOS adios;
#endif
    adios2::IO io = adios.DeclareIO("WriteKokkos");
    io.SetEngine(engine);

    // Declare an array for the ADIOS data of size (NumOfProcesses * N)
    const adios2::Dims shape{static_cast<size_t>(size * N)};
    const adios2::Dims start{static_cast<size_t>(rank * N)};
    const adios2::Dims count{N};
    auto data = io.DefineVariable<float>("data", shape, start, count);

    adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);

    // Simulation steps
    for (size_t step = 0; step < nSteps; ++step)
    {
        // Make a 1D selection to describe the local dimensions of the
        // variable we write and its offsets in the global spaces
        adios2::Box<adios2::Dims> sel({0}, {N});
        data.SetSelection(sel);

        // Start IO step every write step
        bpWriter.BeginStep();
        bpWriter.Put(data, gpuSimData);
        bpWriter.EndStep();

        // Update values in the simulation data
        Kokkos::parallel_for(
            "updateBuffer",
            Kokkos::RangePolicy<Kokkos::DefaultExecutionSpace>(0, N),
            KOKKOS_LAMBDA(int i) { gpuSimData(i) += 5; });
    }

    bpWriter.Close();
    return 0;
}

int BPRead(const std::string fname, const size_t N, size_t nSteps,
           std::string engine)
{
    int rank;
#if ADIOS2_USE_MPI
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#else
    rank = 0;
#endif
    // Create ADIOS structures
#if ADIOS2_USE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    adios2::ADIOS adios;
#endif
    adios2::IO io = adios.DeclareIO("ReadKokkos");
    io.SetEngine("BPFile");

    adios2::Engine bpReader = io.Open(fname, adios2::Mode::Read);

    unsigned int step = 0;
    if (rank == 0)
    {
        Kokkos::DefaultExecutionSpace exe_space;
        std::cout << "Read 6k floats with " << engine
                  << " on memory space: " << exe_space.name() << std::endl;
    }

    using mem_space = Kokkos::DefaultExecutionSpace::memory_space;
    Kokkos::View<float *, mem_space> gpuSimData("simBuffer", N);
    for (; bpReader.BeginStep() == adios2::StepStatus::OK; ++step)
    {
        auto data = io.InquireVariable<float>("data");
        const adios2::Dims start{rank * N};
        const adios2::Dims count{N};
        const adios2::Box<adios2::Dims> sel(start, count);
        data.SetSelection(sel);

        bpReader.Get(data, gpuSimData);
        bpReader.EndStep();
        auto simData = Kokkos::create_mirror_view_and_copy(Kokkos::HostSpace{},
                                                           gpuSimData);
        ASSERT(simData.size() == 6000,
               "Rank [" + std::to_string(rank) +
                   "] Received different data size than expected");
        ASSERT(simData[0] == step * 5,
               "[Rank " + std::to_string(rank) + "] Received incorrect data");
    }
    bpReader.Close();
    std::cout << "[Rank " << rank << "] Transfer ended successfully"
              << std::endl;
    return 0;
}

int main(int argc, char **argv)
{
    const std::vector<std::string> list_of_engines = {"BPFile", "BP5"};
    const size_t N = 6000;
    size_t nSteps = 10;
    int ret = 0, rank = 0;
#if ADIOS2_USE_MPI
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#endif
    Kokkos::initialize(argc, argv);
    {
        for (auto engine : list_of_engines)
        {
            const std::string fname(engine + "Kokkos.bp");
            ret += BPWrite(fname, N, nSteps, engine);
            ret += BPRead(fname, N, nSteps, engine);
        }
    }
    Kokkos::finalize();
    if (ret == 0 && rank == 0)
        std::cout << "All transfered finished successfully. Done" << std::endl;
#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif
    return ret;
}
