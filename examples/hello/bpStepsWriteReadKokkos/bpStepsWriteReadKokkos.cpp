/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * bpStepsWriteReadKokkos.cpp  Simple example of writing and reading bpFloats through ADIOS2 BP
 * engine with multiple simulations steps for every IO step using Kokkos
 */
#include <ios>
#include <iostream>
#include <stdexcept> //std::invalid_argument std::exception
#include <vector>

#include <adios2.h>

#include <Kokkos_Core.hpp>

void writer(adios2::ADIOS &adios, const std::string &engine, const std::string &fname,
            const size_t Nx, unsigned int nSteps)
{
    // Initialize the simulation bpFloats with the default memory space
    using mem_space = Kokkos::DefaultExecutionSpace::memory_space;
    Kokkos::View<float *, mem_space> gpuSimData("simBuffer", Nx);
    Kokkos::parallel_for(
        "initBuffer", Kokkos::RangePolicy<Kokkos::DefaultExecutionSpace>(0, Nx),
        KOKKOS_LAMBDA(int i) { gpuSimData(i) = static_cast<float>(i); });
    Kokkos::fence();

    // Set up the ADIOS structures
    adios2::IO bpIO = adios.DeclareIO("WriteIO");
    bpIO.SetEngine(engine);

    const adios2::Dims shape{static_cast<size_t>(Nx)};
    const adios2::Dims start{static_cast<size_t>(0)};
    const adios2::Dims count{Nx};
    auto bpFloats = bpIO.DefineVariable<float>("bpFloats", shape, start, count);
    auto bpStep = bpIO.DefineVariable<unsigned int>("bpStep");

    adios2::Engine bpWriter = bpIO.Open(fname, adios2::Mode::Write);

    // Simulation steps
    for (unsigned int step = 0; step < nSteps; ++step)
    {
        // Make a 1D selection to describe the local dimensions of the
        // variable we write and its offsets in the global spaces
        adios2::Box<adios2::Dims> sel({0}, {Nx});
        bpFloats.SetSelection(sel);

        // Start IO step every write step
        bpWriter.BeginStep();
        bpWriter.Put(bpFloats, gpuSimData.data());
        bpWriter.Put(bpStep, step);
        bpWriter.EndStep();

        // Update values in the simulation bpFloats using the default
        // execution space
        Kokkos::parallel_for(
            "updateBuffer", Kokkos::RangePolicy<Kokkos::DefaultExecutionSpace>(0, Nx),
            KOKKOS_LAMBDA(int i) { gpuSimData(i) += 10; });
        Kokkos::fence();
    }

    bpWriter.Close();
    Kokkos::DefaultExecutionSpace exe_space;
    std::cout << "Done writing on memory space: " << exe_space.name() << std::endl;
}

void reader(adios2::ADIOS &adios, const std::string &engine, const std::string &fname,
            const size_t Nx, unsigned int /*nSteps*/)
{
    // Create ADIOS structures
    adios2::IO bpIO = adios.DeclareIO("ReadIO");
    bpIO.SetEngine(engine);

    Kokkos::DefaultExecutionSpace exe_space;
    std::cout << "Read on memory space: " << exe_space.name() << std::endl;

    adios2::Engine bpReader = bpIO.Open(fname, adios2::Mode::Read);

    using mem_space = Kokkos::DefaultExecutionSpace::memory_space;
    Kokkos::View<float *, mem_space> gpuSimData("simBuffer", Nx);
    unsigned int inStep = 0;
    for (unsigned int step = 0; bpReader.BeginStep() == adios2::StepStatus::OK; ++step)
    {
        auto bpFloats = bpIO.InquireVariable<float>("bpFloats");
        if (bpFloats)
        {
            const adios2::Dims start{0};
            const adios2::Dims count{Nx};
            const adios2::Box<adios2::Dims> sel(start, count);
            bpFloats.SetSelection(sel);

            bpReader.Get(bpFloats, gpuSimData.data());
        }
        auto bpStep = bpIO.InquireVariable<unsigned int>("bpStep");
        if (bpStep)
        {
            bpReader.Get(bpStep, &inStep);
        }
        bpReader.EndStep();
        if (inStep != step)
        {
            std::cout << "ERROR: step mismatch\n";
            return;
        }
    }

    bpReader.Close();
}

int main(int argc, char **argv)
{
    Kokkos::initialize(argc, argv);

    const std::string engine = argv[1] ? argv[1] : "BPFile";
    std::cout << "Using engine " << engine << std::endl;

    const std::string filename = engine + "StepsWriteReadCuda.bp";
    const unsigned int nSteps = 10;
    const unsigned int Nx = 6000;
    try
    {
        /** ADIOS class factory of IO class objects */
        adios2::ADIOS adios;

        writer(adios, engine, filename, Nx, nSteps);
        reader(adios, engine, filename, Nx, nSteps);
    }
    catch (std::invalid_argument &e)
    {
        std::cout << "Invalid argument exception, STOPPING PROGRAM\n";
        std::cout << e.what() << "\n";
    }
    catch (std::ios_base::failure &e)
    {
        std::cout << "IO System base failure exception, STOPPING PROGRAM\n";
        std::cout << e.what() << "\n";
    }
    catch (std::exception &e)
    {
        std::cout << "Exception, STOPPING PROGRAM\n";
        std::cout << e.what() << "\n";
    }
    Kokkos::finalize();

    return 0;
}
