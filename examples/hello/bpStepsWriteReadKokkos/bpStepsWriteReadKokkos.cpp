/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * bpStepsWriteReadKokkos.cpp  Simple example of writing and reading bpFloats through ADIOS2 BP
 * engine using Kokkos on any memory space
 */
#include <ios>
#include <iostream>
#include <vector>

#include <adios2.h>

#include <Kokkos_Core.hpp>

template <class MemSpace, class ExecSpace>
int BPWrite(const std::string fname, const size_t Nx, const size_t Ny, const size_t nSteps,
            const std::string engine)
{
    // Initialize the simulation data
    Kokkos::View<float **, MemSpace> gpuSimData("simBuffer", Nx, Ny);
    static_assert(Kokkos::SpaceAccessibility<ExecSpace, MemSpace>::accessible, "");
    Kokkos::parallel_for("initBuffer", Kokkos::RangePolicy<ExecSpace>(0, Nx), KOKKOS_LAMBDA(int i) {
        for (int j = 0; j < Ny; j++)
            gpuSimData(i, j) = static_cast<float>(i);
    });
    Kokkos::fence();

    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("WriteIO");
    io.SetEngine(engine);

    const adios2::Dims shape{Nx, Ny};
    const adios2::Dims start{0, 0};
    const adios2::Dims count{Nx, Ny};
    auto data = io.DefineVariable<float>("bpFloats", shape, start, count);

    adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);

    // Simulation steps
    for (unsigned int step = 0; step < nSteps; ++step)
    {
        adios2::Box<adios2::Dims> sel({0, 0}, {Nx, Ny});
        data.SetSelection(sel);

        bpWriter.BeginStep();
        bpWriter.Put(data, gpuSimData.data());
        bpWriter.EndStep();

        // Update values in the simulation data
        Kokkos::parallel_for("updateBuffer", Kokkos::RangePolicy<ExecSpace>(0, Nx),
                             KOKKOS_LAMBDA(int i) {
                                 for (int j = 0; j < Ny; j++)
                                     gpuSimData(i, j) += 10;
                             });
        Kokkos::fence();
    }

    bpWriter.Close();
    ExecSpace exe_space;
    std::cout << "Done writing on memory space: " << exe_space.name() << std::endl;
    return 0;
}

template <class MemSpace>
std::array<size_t, 2> GetDimenstions(adios2::Variable<float> data)
{
    return {data.Shape()[1], data.Shape()[0]};
}

template <>
std::array<size_t, 2> GetDimenstions<Kokkos::HostSpace>(adios2::Variable<float> data)
{
    return {data.Shape()[0], data.Shape()[1]};
}

template <class MemSpace, class ExecSpace>
int BPRead(const std::string fname, const std::string engine)
{
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("ReadIO");
    io.SetEngine(engine);

    ExecSpace exe_space;
    std::cout << "Read on memory space: " << exe_space.name() << std::endl;

    adios2::Engine bpReader = io.Open(fname, adios2::Mode::Read);

    unsigned int step = 0;
    for (; bpReader.BeginStep() == adios2::StepStatus::OK; ++step)
    {
        auto data = io.InquireVariable<float>("bpFloats");
        if (data.Shape().size() != 2)
        {
            std::cout << "Error, the bpFloats variable in the BP file "
                         " on step "
                      << step << "needs to have two dimensions" << std::endl;
            break;
        }

        const adios2::Dims start{0, 0};
        const adios2::Box<adios2::Dims> sel(start, data.Shape());
        data.SetSelection(sel);

        auto dims = GetDimenstions<MemSpace>(data);
        size_t Nx = dims[0], Ny = dims[1];
        Kokkos::View<float **, MemSpace> gpuSimData("simBuffer", Nx, Ny);
        bpReader.Get(data, gpuSimData.data());
        bpReader.EndStep();

        auto cpuData = Kokkos::create_mirror_view_and_copy(Kokkos::HostSpace{}, gpuSimData);
        for (size_t i = 0; i < Nx; i++)
        {
            for (size_t j = 0; j < Ny; j++)
                std::cout << cpuData(i, j) << " ";
            std::cout << std::endl;
        }
    }

    bpReader.Close();
    return 0;
}

int main(int argc, char **argv)
{
    const std::string engine = argv[1] ? argv[1] : "BPFile";
    std::cout << "Using engine " << engine << std::endl;
    const size_t Nx = 6, Ny = 10, nSteps = 1;
    const std::string fromMemSpace = "Default";
    const std::string toMemSpace = "Default";

    const std::string filename = engine + "StepsWriteReadKokkos";
    Kokkos::initialize(argc, argv);
    {
        std::cout << "Using engine " << engine << std::endl;

        if (fromMemSpace == "Default")
        {
            using mem_space = Kokkos::DefaultExecutionSpace::memory_space;
            std::cout << "Writing on memory space: DefaultMemorySpace" << std::endl;
            BPWrite<mem_space, Kokkos::DefaultExecutionSpace>(filename + ".bp", Nx, Ny, nSteps,
                                                              engine);
        }
        else
        {
            std::cout << "Writing on memory space: HostSpace" << std::endl;
            BPWrite<Kokkos::HostSpace, Kokkos::Serial>(filename + ".bp", nSteps, Nx, Ny, engine);
        }
        if (toMemSpace == "Default")
        {
            using mem_space = Kokkos::DefaultExecutionSpace::memory_space;
            std::cout << "Reading on memory space: DefaultMemorySpace" << std::endl;
            BPRead<mem_space, Kokkos::DefaultExecutionSpace>(filename + ".bp", engine);
        }
        else
        {
            std::cout << "Reading on memory space: HostSpace" << std::endl;
            BPRead<Kokkos::HostSpace, Kokkos::Serial>(filename + ".bp", engine);
        }
    }
    Kokkos::finalize();
    return 0;
}
