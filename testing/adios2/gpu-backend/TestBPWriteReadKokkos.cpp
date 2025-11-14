/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#include <adios2.h>

#include <Kokkos_Core.hpp>
#include <adios2/cxx/KokkosView.h>
#include <algorithm>
#include <array>
#include <gtest/gtest.h>
#include <iostream>
#include <numeric>

std::string engineName;

const float EPSILON = std::numeric_limits<float>::epsilon();
const float INCREMENT = 10.0f;

void KokkosDetectMemSpace(const std::string mode)
{
    const std::string fname("BPWRKokkosDetect" + mode + ".bp");
    adios2::Mode ioMode = adios2::Mode::Deferred;
    if (mode == "Sync")
        ioMode = adios2::Mode::Sync;

    const size_t Nx = 5;
    const size_t NSteps = 2;

    adios2::ADIOS adios;
    { // write
        Kokkos::View<float *, Kokkos::HostSpace> cpuData("simBuffer", Nx);
        Kokkos::parallel_for(
            "initBuffer", Kokkos::RangePolicy<Kokkos::Serial>(0, Nx),
            KOKKOS_LAMBDA(int i) { cpuData(i) = static_cast<float>(i); });
        Kokkos::fence();
        auto gpuData = Kokkos::create_mirror_view_and_copy(
            Kokkos::DefaultExecutionSpace::memory_space{}, cpuData);

        adios2::IO io = adios.DeclareIO("TestIO");
        const adios2::Dims shape{Nx};
        const adios2::Dims start{0};
        const adios2::Dims count{Nx};
        auto var_r32 = io.DefineVariable<float>("r32", shape, start, count);
        auto var_gpur32 = io.DefineVariable<float>("gpur32", shape, start, count);
        EXPECT_TRUE(var_r32);
        EXPECT_TRUE(var_gpur32);

        io.SetEngine("BP5");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            // Update values in the simulation data
            bpWriter.BeginStep();
            bpWriter.Put(var_gpur32, gpuData, ioMode);
            bpWriter.Put(var_r32, cpuData, ioMode);
            bpWriter.EndStep();
            Kokkos::parallel_for(
                "updateCPUBuffer", Kokkos::RangePolicy<Kokkos::Serial>(0, Nx),
                KOKKOS_LAMBDA(int i) { cpuData(i) += INCREMENT; });
            Kokkos::parallel_for(
                "updateGPUBuffer", Kokkos::RangePolicy<Kokkos::DefaultExecutionSpace>(0, Nx),
                KOKKOS_LAMBDA(int i) { gpuData(i) += INCREMENT; });
            Kokkos::fence();
        }

        bpWriter.Close();
    }
    { // read
        adios2::IO io = adios.DeclareIO("ReadIO");
        io.SetEngine("BP5");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }

        adios2::Engine bpReader = io.Open(fname, adios2::Mode::Read);
        unsigned int t = 0;
        for (; bpReader.BeginStep() == adios2::StepStatus::OK; ++t)
        {
            auto var_r32 = io.InquireVariable<float>("r32");
            auto var_gpur32 = io.InquireVariable<float>("gpur32");
            EXPECT_TRUE(var_r32);
            EXPECT_TRUE(var_gpur32);

            std::vector<float> r32o(Nx);
            Kokkos::View<float *, Kokkos::DefaultExecutionSpace::memory_space> gpuData("readBuffer",
                                                                                       Nx);

            bpReader.Get(var_r32, r32o.data(), ioMode);
            bpReader.Get(var_gpur32, gpuData, ioMode);
            bpReader.EndStep();
            auto cpuData =
                Kokkos::create_mirror_view_and_copy(Kokkos::HostSpace::memory_space{}, gpuData);

            for (size_t i = 0; i < Nx; i++)
            {
                char msg[1 << 8] = {0};
                snprintf(msg, sizeof(msg), "t=%d i=%zu cpu=%f gpu=%f", t, i, r32o[i], cpuData(i));
                ASSERT_LT(std::abs(r32o[i] - cpuData(i)), EPSILON) << msg;
            }
        }
        EXPECT_EQ(t, NSteps);

        bpReader.Close();
    }
}

void KokkosWriteReadMemorySelection()
{
    adios2::MemorySpace adiosMemSpace = adios2::MemorySpace::Host;
#ifdef ADIOS2_HAVE_GPU_SUPPORT
    if (!std::is_same<Kokkos::DefaultExecutionSpace::memory_space, Kokkos::HostSpace>::value)
        adiosMemSpace = adios2::MemorySpace::GPU;
#endif

    int mpiRank = 0, mpiSize = 1;
#if ADIOS2_USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    const std::string fname("BPWRKokkosMemSel2D_MPI.bp");
#else
    const std::string fname("BPWRKokkosMemSel2D.bp");
#endif

#if ADIOS2_USE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    adios2::ADIOS adios;
#endif
    const size_t Nx = 3, Ny = 2;
    const size_t NSteps = 2;
    const size_t ghostCells = 2;
    const size_t totalNx = Nx + 2 * ghostCells, totalNy = Ny + 2 * ghostCells;

    Kokkos::View<float **, Kokkos::DefaultExecutionSpace::memory_space> inputData("inBuffer",
                                                                                  totalNx, totalNy);
    // initialize all data to 0 and update values in the active selection
    Kokkos::parallel_for(
        "zeroBuffer", Kokkos::MDRangePolicy<Kokkos::Rank<2>>({0, 0}, {totalNx, totalNy}),
        KOKKOS_LAMBDA(int x, int y) { inputData(x, y) = 0; });
    Kokkos::parallel_for(
        "initBuffer",
        Kokkos::MDRangePolicy<Kokkos::Rank<2>>({ghostCells, ghostCells},
                                               {totalNx - ghostCells, totalNy - ghostCells}),
        KOKKOS_LAMBDA(int x, int y) { inputData(x, y) = x * (mpiRank + 1); });
    Kokkos::fence();

    { // write

        adios2::IO io = adios.DeclareIO("TestIO");
        io.SetEngine("BP5");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }

        const adios2::Dims shape{static_cast<size_t>(Nx * mpiSize), Ny};
        const adios2::Dims start{static_cast<size_t>(Nx * mpiRank), 0};
        const adios2::Dims count{Nx, Ny};
        auto var_r32 = io.DefineVariable<float>("r32", shape, start, count);

        const adios2::Dims memoryStart = {ghostCells, ghostCells};
        const adios2::Dims memoryCount = {totalNx, totalNy};
        var_r32.SetMemorySelection({memoryStart, memoryCount});

        adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);
        for (size_t step = 0; step < NSteps; ++step)
        {
            bpWriter.BeginStep();
            var_r32.SetMemorySpace(adiosMemSpace);
            bpWriter.Put(var_r32, inputData);
            bpWriter.EndStep();
            // Update values in the simulation data
            Kokkos::parallel_for(
                "updateBuffer",
                Kokkos::MDRangePolicy<Kokkos::Rank<2>>(
                    {ghostCells, ghostCells}, {totalNx - ghostCells, totalNy - ghostCells}),
                KOKKOS_LAMBDA(int x, int y) { inputData(x, y) += INCREMENT; });
            Kokkos::fence();
        }

        bpWriter.Close();
    }
    {
        adios2::IO io = adios.DeclareIO("ReadIO");
        io.SetEngine("BP5");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }

        adios2::Engine bpReader = io.Open(fname, adios2::Mode::Read);
        unsigned int t = 0;
        for (; bpReader.BeginStep() == adios2::StepStatus::OK; ++t)
        {
            auto var_r32 = io.InquireVariable<float>("r32");
            var_r32.SetSelection({{Nx * mpiRank, 0}, {Nx, Ny}});
            EXPECT_TRUE(var_r32);
            var_r32.SetMemorySpace(adiosMemSpace);
            EXPECT_EQ(var_r32.Min(), t * INCREMENT + ghostCells);
            EXPECT_EQ(var_r32.Max(), t * INCREMENT + (Nx + ghostCells - 1) * mpiSize);

            Kokkos::View<float **, Kokkos::DefaultExecutionSpace::memory_space> outputData(
                "outBuffer", Nx, Ny);
            bpReader.Get(var_r32, outputData);
            bpReader.EndStep();

            auto cpuData =
                Kokkos::create_mirror_view_and_copy(Kokkos::HostSpace::memory_space{}, outputData);

            for (size_t x = 0; x < Nx; x++)
                for (size_t y = 0; y < Ny; y++)
                {
                    char msg[1 << 8] = {0};
                    snprintf(msg, sizeof(msg), "t=%d i=(%zu %zu) rank=%d r32o=%f r32i=%f", t, x, y,
                             mpiRank, cpuData(x, y),
                             INCREMENT * t + (x + ghostCells) * (mpiRank + 1));
                    ASSERT_LT(std::abs(cpuData(x, y) -
                                       (INCREMENT * t + (x + ghostCells) * (mpiRank + 1))),
                              EPSILON)
                        << msg;
                }
        }
        EXPECT_EQ(t, NSteps);
        bpReader.Close();
    }
}

void KokkosWriteReadMPI2D()
{
    const size_t Nx = 5;
    const size_t Ny = 2;
    const size_t NSteps = 3;

    adios2::MemorySpace adiosMemSpace = adios2::MemorySpace::Host;
#ifdef ADIOS2_HAVE_GPU_SUPPORT
    if (!std::is_same<Kokkos::DefaultExecutionSpace::memory_space, Kokkos::HostSpace>::value)
        adiosMemSpace = adios2::MemorySpace::GPU;
#endif

    int mpiRank = 0, mpiSize = 1;
#if ADIOS2_USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    const std::string fname("BPWRKokkos2D_MPI.bp");
#else
    const std::string fname("BPWRKokkos2D.bp");
#endif

#if ADIOS2_USE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    adios2::ADIOS adios;
#endif

    // Initialize the simulation data
    Kokkos::View<float **, Kokkos::DefaultExecutionSpace::memory_space> inputData("inBuffer", Nx,
                                                                                  Ny);
    Kokkos::parallel_for(
        "initBuffer", Kokkos::MDRangePolicy<Kokkos::Rank<2>>({0, 0}, {Nx, Ny}),
        KOKKOS_LAMBDA(int x, int y) { inputData(x, y) = x * (mpiRank + 1); });
    Kokkos::fence();

    { // write
        adios2::IO io = adios.DeclareIO("TestIO");
        io.SetEngine("BP5");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }

        const adios2::Dims shape{static_cast<size_t>(Nx * mpiSize), Ny};
        const adios2::Dims start{static_cast<size_t>(Nx * mpiRank), 0};
        const adios2::Dims count{Nx, Ny};

        auto var_r32 = io.DefineVariable<float>("r32", shape, start, count);
        var_r32.SetMemorySpace(adiosMemSpace);

        adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);
        for (size_t step = 0; step < NSteps; ++step)
        {
            bpWriter.BeginStep();
            bpWriter.Put(var_r32, inputData);
            bpWriter.EndStep();
            // Update values in the simulation data
            Kokkos::parallel_for(
                "updateBuffer", Kokkos::MDRangePolicy<Kokkos::Rank<2>>({0, 0}, {Nx, Ny}),
                KOKKOS_LAMBDA(int x, int y) { inputData(x, y) += INCREMENT; });
            Kokkos::fence();
        }

        bpWriter.Close();
    }

#if ADIOS2_USE_MPI
    MPI_Barrier(MPI_COMM_WORLD);
#endif

    { // read
        adios2::IO io = adios.DeclareIO("ReadIO");
        io.SetEngine("BP5");

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }

        adios2::Engine bpReader = io.Open(fname, adios2::Mode::Read);

        unsigned int t = 0;
        for (; bpReader.BeginStep() == adios2::StepStatus::OK; ++t)
        {
            auto var_r32 = io.InquireVariable<float>("r32");
            EXPECT_TRUE(var_r32);
            var_r32.SetMemorySpace(adiosMemSpace);
            var_r32.SetSelection({{Nx * mpiRank, 0}, {Nx, Ny}});

            EXPECT_EQ(var_r32.Min(), INCREMENT * t);
            EXPECT_EQ(var_r32.Max(), INCREMENT * t + (Nx - 1) * mpiSize);

            Kokkos::View<float **, Kokkos::DefaultExecutionSpace::memory_space> outputData(
                "outBuffer", Nx, Ny);
            bpReader.Get(var_r32, outputData);
            bpReader.EndStep();

            auto cpuData =
                Kokkos::create_mirror_view_and_copy(Kokkos::HostSpace::memory_space{}, outputData);

            for (size_t x = 0; x < Nx; x++)
                for (size_t y = 0; y < Ny; y++)
                {
                    char msg[1 << 8] = {0};
                    snprintf(msg, sizeof(msg), "t=%d i=(%zu %zu) rank=%d r32o=%f r32i=%f", t, x, y,
                             mpiRank, cpuData(x, y), INCREMENT * t + x * (mpiRank + 1));
                    ASSERT_LT(std::abs(cpuData(x, y) - (INCREMENT * t + x * (mpiRank + 1))),
                              EPSILON)
                        << msg;
                }
        }
        EXPECT_EQ(t, NSteps);

        bpReader.Close();
    }
}

bool compareSelection2D(
    const Kokkos::View<double **, Kokkos::DefaultExecutionSpace::memory_space> a,
    const adios2::Dims &shape, const adios2::Dims &start, const adios2::Dims &count,
    Kokkos::View<double **, Kokkos::DefaultExecutionSpace::memory_space> b)
{
    std::cout << " compare Block: shape = " << adios2::ToString(shape)
              << " start = " << adios2::ToString(start) << " count = " << adios2::ToString(count)
              << std::endl;
    size_t match = 0;
    auto const start_0 = start[0];
    auto const start_1 = start[1];
    Kokkos::parallel_reduce(
        "compareBuffers", Kokkos::MDRangePolicy<Kokkos::Rank<2>>({0, 0}, {count[0], count[1]}),
        KOKKOS_LAMBDA(int x, int y, size_t &lmatch) {
            if (b(x, y) != a(start_0 + x, start_1 + y))
            {
                lmatch++;
            }
        },
        match);
    Kokkos::fence();
    return (match == 0);
}

void KokkosWriteReadSelection2D()
{
    adios2::MemorySpace adiosMemSpace = adios2::MemorySpace::Host;
#ifdef ADIOS2_HAVE_GPU_SUPPORT
    if (!std::is_same<Kokkos::DefaultExecutionSpace::memory_space, Kokkos::HostSpace>::value)
        adiosMemSpace = adios2::MemorySpace::GPU;
#endif

    constexpr size_t C1 = 5;
    constexpr size_t C2 = 4;
    constexpr size_t DIM1 = 3 * C1;
    constexpr size_t DIM2 = 3 * C2;
    const std::string filename = "BPWRKokkosSel2D.bp";
    Kokkos::View<double **, Kokkos::DefaultExecutionSpace::memory_space> inputData("inBuffer", DIM1,
                                                                                   DIM2);
    Kokkos::parallel_for(
        "initBuffer", Kokkos::MDRangePolicy<Kokkos::Rank<2>>({0, 0}, {DIM1, DIM2}),
        KOKKOS_LAMBDA(int x, int y) { inputData(x, y) = x * 1.0 + y / 100.0; });
    Kokkos::fence();

    adios2::ADIOS adios;
    { // write
        adios2::IO ioWrite = adios.DeclareIO("TestIO");
        ioWrite.SetEngine("BP5");
        if (!engineName.empty())
        {
            ioWrite.SetEngine(engineName);
        }
        adios2::Engine engine = ioWrite.Open(filename, adios2::Mode::Write);
        const adios2::Dims shape = {DIM1, DIM2};
        const adios2::Dims count = {C1, C2};
        adios2::Dims start{0, 0};
        adios2::Variable<double> var =
            ioWrite.DefineVariable<double>("selDouble", shape, start, count);

        engine.BeginStep();
        for (size_t i = 0; i < DIM1; i += count[0])
        {
            for (size_t j = 0; j < DIM2; j += count[1])
            {
                start = {i, j};
                Kokkos::View<double **, Kokkos::DefaultExecutionSpace::memory_space> selData(
                    "selBuffer", C1, C2);
                Kokkos::parallel_for(
                    "createSelBuffer", Kokkos::MDRangePolicy<Kokkos::Rank<2>>({0, 0}, {C1, C2}),
                    KOKKOS_LAMBDA(int x, int y) { selData(x, y) = inputData(i + x, j + y); });
                Kokkos::fence();
                var.SetSelection({start, count});
                var.SetMemorySpace(adiosMemSpace);
                engine.Put(var, selData, adios2::Mode::Sync);
            }
        }

        engine.EndStep();
        engine.Close();
    }
    { // read
        adios2::ADIOS adios;
        adios2::IO ioRead = adios.DeclareIO("TestIORead");
        ioRead.SetEngine("BP5");
        if (!engineName.empty())
        {
            ioRead.SetEngine(engineName);
        }
        adios2::Engine engine = ioRead.Open(filename, adios2::Mode::Read);
        EXPECT_TRUE(engine);
        engine.BeginStep();
        adios2::Variable<double> var = ioRead.InquireVariable<double>("selDouble");
        EXPECT_TRUE(var);
        const adios2::Dims shape = {DIM1, DIM2};
        const adios2::Dims count = {C1, C2};
        adios2::Dims s{0, 0};
        adios2::Dims c = shape;
        adios2::Dims firstNonMatch{0, 0};

        /* Entire array */
        {
            Kokkos::View<double **, Kokkos::DefaultExecutionSpace::memory_space> selOutData(
                "selOutBuffer", DIM1, DIM2);
            var.SetSelection({s, c});
            var.SetMemorySpace(adiosMemSpace);
            engine.Get<double>(var, selOutData, adios2::Mode::Sync);
            EXPECT_TRUE(compareSelection2D(inputData, shape, s, c, selOutData));
        }
        /* Single block in the center */
        {
            s = {5, 4};
            c = count;
            Kokkos::View<double **, Kokkos::DefaultExecutionSpace::memory_space> selOutData(
                "selOutBuffer", c[0], c[1]);
            var.SetSelection({s, c});
            var.SetMemorySpace(adiosMemSpace);
            engine.Get<double>(var, selOutData, adios2::Mode::Sync);
            EXPECT_TRUE(compareSelection2D(inputData, shape, s, c, selOutData));
        }
        /* Four blocks in X-Y direction */
        {
            s = {5, 4};
            c = {2 * count[0], 2 * count[1]};
            Kokkos::View<double **, Kokkos::DefaultExecutionSpace::memory_space> selOutData(
                "selOutBuffer", c[0], c[1]);
            var.SetSelection({s, c});
            var.SetMemorySpace(adiosMemSpace);
            engine.Get<double>(var, selOutData, adios2::Mode::Sync);
            EXPECT_TRUE(compareSelection2D(inputData, shape, s, c, selOutData));
        }
        /* Partial blocks : center part of single block in center */
        {
            s = {6, 5};
            c = {count[0] - 2, count[1] - 2};
            Kokkos::View<double **, Kokkos::DefaultExecutionSpace::memory_space> selOutData(
                "selOutBuffer", c[0], c[1]);
            var.SetSelection({s, c});
            var.SetMemorySpace(adiosMemSpace);
            engine.Get<double>(var, selOutData, adios2::Mode::Sync);
            EXPECT_TRUE(compareSelection2D(inputData, shape, s, c, selOutData));
        }
    }
}

void KokkosWriteReadStruct()
{
    constexpr size_t DIM1 = 2;
    constexpr size_t DIM2 = 3;
    adios2::MemorySpace adiosMemSpace = adios2::MemorySpace::Host;
#ifdef ADIOS2_HAVE_GPU_SUPPORT
    if (!std::is_same<Kokkos::DefaultExecutionSpace::memory_space, Kokkos::HostSpace>::value)
        adiosMemSpace = adios2::MemorySpace::GPU;
#endif
    const std::string filename = "BPWRKokkosStruct.bp";
    struct particle
    {
        double a;
        int b[2];
    };

    { // write
        Kokkos::View<particle **, Kokkos::DefaultExecutionSpace::memory_space> inputData(
            "inBuffer", DIM1, DIM2);
        Kokkos::parallel_for(
            "initBuffer", Kokkos::MDRangePolicy<Kokkos::Rank<2>>({0, 0}, {DIM1, DIM2}),
            KOKKOS_LAMBDA(int x, int y) {
                inputData(x, y) = {x * 1.5, {x, y}};
            });
        Kokkos::fence();
        adios2::ADIOS adios;
        adios2::IO ioWrite = adios.DeclareIO("TestIO");
        ioWrite.SetEngine("BP5");
        if (!engineName.empty())
        {
            ioWrite.SetEngine(engineName);
        }
        adios2::Engine engine = ioWrite.Open(filename, adios2::Mode::Write);
        const adios2::Dims shape = {DIM1, DIM2};
        const adios2::Dims count = {DIM1, DIM2};
        const adios2::Dims start = {0, 0};
        auto particleDef = ioWrite.DefineStruct("particle", sizeof(particle));
        particleDef.AddField("a", offsetof(struct particle, a), adios2::DataType::Double);
        particleDef.AddField("b", offsetof(struct particle, b), adios2::DataType::Int32, 2);
        auto varStruct =
            ioWrite.DefineStructVariable("particles", particleDef, shape, start, count);

        engine.BeginStep();
        varStruct.SetMemorySpace(adiosMemSpace);
        engine.Put(varStruct, inputData.data());
        engine.EndStep();
        engine.Close();
    }

    { // read
        adios2::ADIOS adios;
        adios2::IO ioRead = adios.DeclareIO("TestIORead");
        ioRead.SetEngine("BP5");
        if (!engineName.empty())
        {
            ioRead.SetEngine(engineName);
        }
        adios2::Engine engine = ioRead.Open(filename, adios2::Mode::Read);
        Kokkos::View<particle **, Kokkos::DefaultExecutionSpace::memory_space> myParticles(
            "outParticles", DIM1, DIM2);

        engine.BeginStep();
        auto varStruct = ioRead.InquireStructVariable("particles");
        adios2::StructDefinition ReadStruct = varStruct.GetReadStructDef();
        if (varStruct && !ReadStruct)
        {
            varStruct.SetMemorySpace(adiosMemSpace);
            adios2::StructDefinition WriteStruct = varStruct.GetWriteStructDef();
            ASSERT_TRUE(WriteStruct);
            std::cout << "Writer side structure was named \"" << WriteStruct.StructName()
                      << "\" and has size " << WriteStruct.StructSize() << std::endl;
            for (size_t i = 0; i < WriteStruct.Fields(); i++)
            {
                std::cout << "\tField " << i << " - Name: \"" << WriteStruct.Name(i)
                          << "\", Offset: " << WriteStruct.Offset(i)
                          << ", Type: " << WriteStruct.Type(i)
                          << ", ElementCount : " << WriteStruct.ElementCount(i) << std::endl;
            }
            std::cout << std::endl;
            auto particleDef1 = ioRead.DefineStruct("particle", sizeof(particle));
            particleDef1.AddField("a", offsetof(particle, a), adios2::DataType::Double);
            particleDef1.AddField("b", offsetof(particle, b), adios2::DataType::Int32, 2);
            varStruct.SetReadStructDef(particleDef1);
        }
        else if (varStruct)
        {
            // set this already, but try something else
            static bool first = true;
            if (first)
            {
                first = false;
                adios2::StructDefinition SaveDef = varStruct.GetReadStructDef();
                ASSERT_TRUE(SaveDef);
                auto particleDef2 = ioRead.DefineStruct("particle", sizeof(particle));
                particleDef2.AddField("a", offsetof(particle, a), adios2::DataType::Double);
                particleDef2.AddField("b", offsetof(particle, b), adios2::DataType::Int32, 2);
                varStruct.SetReadStructDef(particleDef2);

                // restore the old one so we can succeed below
                varStruct.SetReadStructDef(SaveDef);
            }
        }
        const adios2::Dims count = {DIM1, DIM2};
        const adios2::Dims start = {0, 0};
        varStruct.SetSelection({start, count});
        ASSERT_TRUE(varStruct);
        engine.Get(varStruct, myParticles.data(), adios2::Mode::Sync);
        engine.EndStep();
        // move the data to the CPU
        auto cpuData = Kokkos::create_mirror_view_and_copy(Kokkos::HostSpace{}, myParticles);
        for (size_t i = 0; i < DIM1; ++i)
            for (size_t j = 0; j < DIM2; ++j)
            {
                ASSERT_EQ(cpuData(i, j).a, i * 1.5);
                ASSERT_EQ(cpuData(i, j).b[0], i);
                ASSERT_EQ(cpuData(i, j).b[1], j);
            }
    }
}

class BPWRKokkos : public ::testing::TestWithParam<std::string>
{
public:
    BPWRKokkos() = default;

    virtual void SetUp() {}
    virtual void TearDown() {}
};

TEST_P(BPWRKokkos, ADIOS2BPKokkosDetect) { KokkosDetectMemSpace(GetParam()); }
TEST_P(BPWRKokkos, ADIOS2BPKokkosMemSel) { KokkosWriteReadMemorySelection(); }
TEST_P(BPWRKokkos, ADIOS2BPWRKokkos2D) { KokkosWriteReadMPI2D(); }
TEST_P(BPWRKokkos, ADIOS2BPWRKokkosSel2D) { KokkosWriteReadSelection2D(); }
TEST_P(BPWRKokkos, ADIOS2BPWRKokkosStruct) { KokkosWriteReadStruct(); }

INSTANTIATE_TEST_SUITE_P(KokkosRW, BPWRKokkos, ::testing::Values("deferred"));

int main(int argc, char **argv)
{
#if ADIOS2_USE_MPI
    int provided;

    // MPI_THREAD_MULTIPLE is only required if you enable the SST MPI_DP
    MPI_Init_thread(nullptr, nullptr, MPI_THREAD_MULTIPLE, &provided);
#endif
    Kokkos::initialize(argc, argv);

    Kokkos::DefaultExecutionSpace exe_space;
    std::cout << "Testing on memory space: " << exe_space.name() << std::endl;

    int result;
    ::testing::InitGoogleTest(&argc, argv);
    if (argc > 1)
    {
        engineName = std::string(argv[1]);
    }
    result = RUN_ALL_TESTS();

    Kokkos::finalize();
#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif

    return result;
}
