/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstdint>
#include <cstring>

#include <iostream>
#include <numeric> //std::iota
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#include "../../TestHelpers.h"

std::string engineName; // comes from command line

void SZAccuracy1D(const std::string accuracy)
{
    // Each process would write a 1x8 array and all processes would
    // form a mpiSize * Nx 1D array

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const size_t Nx = 1000;

    // Number of steps
    const size_t NSteps = 1;

    std::vector<float> r32s(Nx);
    std::vector<double> r64s(Nx);

    // range 0 to 999
    std::iota(r32s.begin(), r32s.end(), 0.f);
    std::iota(r64s.begin(), r64s.end(), 0.);

#if ADIOS2_USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    const std::string fname("BPWRSZ1D_" + accuracy + "_MPI.bp");
#else
    const std::string fname("BPWRSZ1D_" + accuracy + ".bp");
#endif

#if ADIOS2_USE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    adios2::ADIOS adios;
#endif
    {
        adios2::IO io = adios.DeclareIO("TestIO");

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            // Create the BP Engine
            io.SetEngine("BPFile");
        }

        const adios2::Dims shape{static_cast<size_t>(Nx * mpiSize)};
        const adios2::Dims start{static_cast<size_t>(Nx * mpiRank)};
        const adios2::Dims count{Nx};

        adios2::Variable<float> var_r32 =
            io.DefineVariable<float>("r32", shape, start, count, adios2::ConstantDims);
        adios2::Variable<double> var_r64 =
            io.DefineVariable<double>("r64", shape, start, count, adios2::ConstantDims);

        // add operations
        adios2::Operator szOp = adios.DefineOperator("szCompressor", adios2::ops::LossySZ);

        var_r32.AddOperation(szOp, {{adios2::ops::sz::key::accuracy, accuracy}});
        var_r64.AddOperation(szOp, {{adios2::ops::sz::key::accuracy, accuracy}});

        adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            bpWriter.BeginStep();
            bpWriter.Put<float>("r32", r32s.data());
            bpWriter.Put<double>("r64", r64s.data());
            bpWriter.EndStep();
        }

        bpWriter.Close();
    }

    {
        adios2::IO io = adios.DeclareIO("ReadIO");

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            // Create the BP Engine
            io.SetEngine("BPFile");
        }

        adios2::Engine bpReader = io.Open(fname, adios2::Mode::Read);

        unsigned int t = 0;
        std::vector<float> decompressedR32s;
        std::vector<double> decompressedR64s;

        while (bpReader.BeginStep() == adios2::StepStatus::OK)
        {
            auto var_r32 = io.InquireVariable<float>("r32");
            EXPECT_TRUE(var_r32);
            ASSERT_EQ(var_r32.ShapeID(), adios2::ShapeID::GlobalArray);
            ASSERT_EQ(var_r32.Steps(), NSteps);
            ASSERT_EQ(var_r32.Shape()[0], mpiSize * Nx);

            auto var_r64 = io.InquireVariable<double>("r64");
            EXPECT_TRUE(var_r64);
            ASSERT_EQ(var_r64.ShapeID(), adios2::ShapeID::GlobalArray);
            ASSERT_EQ(var_r64.Steps(), NSteps);
            ASSERT_EQ(var_r64.Shape()[0], mpiSize * Nx);

            const adios2::Dims start{mpiRank * Nx};
            const adios2::Dims count{Nx};
            const adios2::Box<adios2::Dims> sel(start, count);
            var_r32.SetSelection(sel);
            var_r64.SetSelection(sel);

            bpReader.Get(var_r32, decompressedR32s);
            bpReader.Get(var_r64, decompressedR64s);
            bpReader.EndStep();

            for (size_t i = 0; i < Nx; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i << " rank=" << mpiRank;
                std::string msg = ss.str();

                ASSERT_LT(std::abs(decompressedR32s[i] - r32s[i]), std::stod(accuracy)) << msg;
                ASSERT_LT(std::abs(decompressedR64s[i] - r64s[i]), std::stod(accuracy)) << msg;
            }
            ++t;
        }

        EXPECT_EQ(t, NSteps);

        bpReader.Close();
    }

#if ADIOS2_USE_MPI
    CleanupTestFilesMPI(fname, MPI_COMM_WORLD);
#else
    CleanupTestFiles(fname);
#endif
}

void SZAccuracy2D(const std::string accuracy)
{
    // Each process would write a 1x8 array and all processes would
    // form a mpiSize * Nx 1D array

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const size_t Nx = 100;
    const size_t Ny = 50;

    // Number of steps
    const size_t NSteps = 1;

    std::vector<float> r32s(Nx * Ny);
    std::vector<double> r64s(Nx * Ny);

    // range 0 to 100*50
    std::iota(r32s.begin(), r32s.end(), 0.f);
    std::iota(r64s.begin(), r64s.end(), 0.);

#if ADIOS2_USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    const std::string fname("BPWRSZ2D_" + accuracy + "_MPI.bp");
#else
    const std::string fname("BPWRSZ2D_" + accuracy + ".bp");
#endif

#if ADIOS2_USE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    adios2::ADIOS adios;
#endif
    {
        adios2::IO io = adios.DeclareIO("TestIO");

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            // Create the BP Engine
            io.SetEngine("BPFile");
        }

        const adios2::Dims shape{static_cast<size_t>(Nx * mpiSize), Ny};
        const adios2::Dims start{static_cast<size_t>(Nx * mpiRank), 0};
        const adios2::Dims count{Nx, Ny};

        auto var_r32 = io.DefineVariable<float>("r32", shape, start, count, adios2::ConstantDims);
        auto var_r64 = io.DefineVariable<double>("r64", shape, start, count, adios2::ConstantDims);

        // add operations
        adios2::Operator szOp = adios.DefineOperator("szCompressor", adios2::ops::LossySZ);

        var_r32.AddOperation(szOp, {{adios2::ops::sz::key::accuracy, accuracy}});
        var_r64.AddOperation(szOp, {{adios2::ops::sz::key::accuracy, accuracy}});

        adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            bpWriter.BeginStep();
            bpWriter.Put<float>("r32", r32s.data());
            bpWriter.Put<double>("r64", r64s.data());
            bpWriter.EndStep();
        }

        bpWriter.Close();
    }

    {
        adios2::IO io = adios.DeclareIO("ReadIO");

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            // Create the BP Engine
            io.SetEngine("BPFile");
        }

        adios2::Engine bpReader = io.Open(fname, adios2::Mode::Read);

        unsigned int t = 0;
        std::vector<float> decompressedR32s;
        std::vector<double> decompressedR64s;

        while (bpReader.BeginStep() == adios2::StepStatus::OK)
        {
            auto var_r32 = io.InquireVariable<float>("r32");
            EXPECT_TRUE(var_r32);
            ASSERT_EQ(var_r32.ShapeID(), adios2::ShapeID::GlobalArray);
            ASSERT_EQ(var_r32.Steps(), NSteps);
            ASSERT_EQ(var_r32.Shape()[0], mpiSize * Nx);
            ASSERT_EQ(var_r32.Shape()[1], Ny);

            auto var_r64 = io.InquireVariable<double>("r64");
            EXPECT_TRUE(var_r64);
            ASSERT_EQ(var_r64.ShapeID(), adios2::ShapeID::GlobalArray);
            ASSERT_EQ(var_r64.Steps(), NSteps);
            ASSERT_EQ(var_r64.Shape()[0], mpiSize * Nx);
            ASSERT_EQ(var_r64.Shape()[1], Ny);

            const adios2::Dims start{mpiRank * Nx, 0};
            const adios2::Dims count{Nx, Ny};
            const adios2::Box<adios2::Dims> sel(start, count);
            var_r32.SetSelection(sel);
            var_r64.SetSelection(sel);

            bpReader.Get(var_r32, decompressedR32s);
            bpReader.Get(var_r64, decompressedR64s);
            bpReader.EndStep();

            for (size_t i = 0; i < Nx * Ny; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i << " rank=" << mpiRank;
                std::string msg = ss.str();

                ASSERT_LT(std::abs(decompressedR32s[i] - r32s[i]), std::stod(accuracy)) << msg;
                ASSERT_LT(std::abs(decompressedR64s[i] - r64s[i]), std::stod(accuracy)) << msg;
            }
            ++t;
        }

        EXPECT_EQ(t, NSteps);

        bpReader.Close();
    }

#if ADIOS2_USE_MPI
    CleanupTestFilesMPI(fname, MPI_COMM_WORLD);
#else
    CleanupTestFiles(fname);
#endif
}

void SZAccuracy3D(const std::string accuracy)
{
    // Each process would write a 1x8 array and all processes would
    // form a mpiSize * Nx 1D array

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const size_t Nx = 10;
    const size_t Ny = 20;
    const size_t Nz = 15;

    // Number of steps
    const size_t NSteps = 1;

    std::vector<float> r32s(Nx * Ny * Nz);
    std::vector<double> r64s(Nx * Ny * Nz);

    // range 0 to 100*50
    std::iota(r32s.begin(), r32s.end(), 0.f);
    std::iota(r64s.begin(), r64s.end(), 0.);

#if ADIOS2_USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    const std::string fname("BPWRSZ3D_" + accuracy + "_MPI.bp");
#else
    const std::string fname("BPWRSZ3D_" + accuracy + ".bp");
#endif

#if ADIOS2_USE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    adios2::ADIOS adios;
#endif
    {
        adios2::IO io = adios.DeclareIO("TestIO");

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            // Create the BP Engine
            io.SetEngine("BPFile");
        }

        const adios2::Dims shape{static_cast<size_t>(Nx * mpiSize), Ny, Nz};
        const adios2::Dims start{static_cast<size_t>(Nx * mpiRank), 0, 0};
        const adios2::Dims count{Nx, Ny, Nz};

        auto var_r32 = io.DefineVariable<float>("r32", shape, start, count, adios2::ConstantDims);
        auto var_r64 = io.DefineVariable<double>("r64", shape, start, count, adios2::ConstantDims);

        // add operations
        adios2::Operator szOp = adios.DefineOperator("szCompressor", adios2::ops::LossySZ);

        var_r32.AddOperation(szOp, {{adios2::ops::sz::key::accuracy, accuracy}});
        var_r64.AddOperation(szOp, {{adios2::ops::sz::key::accuracy, accuracy}});

        adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            bpWriter.BeginStep();
            bpWriter.Put<float>("r32", r32s.data());
            bpWriter.Put<double>("r64", r64s.data());
            bpWriter.EndStep();
        }

        bpWriter.Close();
    }

    {
        adios2::IO io = adios.DeclareIO("ReadIO");

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            // Create the BP Engine
            io.SetEngine("BPFile");
        }

        adios2::Engine bpReader = io.Open(fname, adios2::Mode::Read);

        unsigned int t = 0;
        std::vector<float> decompressedR32s;
        std::vector<double> decompressedR64s;

        while (bpReader.BeginStep() == adios2::StepStatus::OK)
        {
            auto var_r32 = io.InquireVariable<float>("r32");
            EXPECT_TRUE(var_r32);
            ASSERT_EQ(var_r32.ShapeID(), adios2::ShapeID::GlobalArray);
            ASSERT_EQ(var_r32.Steps(), NSteps);
            ASSERT_EQ(var_r32.Shape()[0], mpiSize * Nx);
            ASSERT_EQ(var_r32.Shape()[1], Ny);
            ASSERT_EQ(var_r32.Shape()[2], Nz);

            auto var_r64 = io.InquireVariable<double>("r64");
            EXPECT_TRUE(var_r64);
            ASSERT_EQ(var_r64.ShapeID(), adios2::ShapeID::GlobalArray);
            ASSERT_EQ(var_r64.Steps(), NSteps);
            ASSERT_EQ(var_r64.Shape()[0], mpiSize * Nx);
            ASSERT_EQ(var_r64.Shape()[1], Ny);
            ASSERT_EQ(var_r64.Shape()[2], Nz);

            const adios2::Dims start{mpiRank * Nx, 0, 0};
            const adios2::Dims count{Nx, Ny, Nz};
            const adios2::Box<adios2::Dims> sel(start, count);
            var_r32.SetSelection(sel);
            var_r64.SetSelection(sel);

            bpReader.Get(var_r32, decompressedR32s);
            bpReader.Get(var_r64, decompressedR64s);
            bpReader.EndStep();

            for (size_t i = 0; i < Nx * Ny * Nz; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i << " rank=" << mpiRank;
                std::string msg = ss.str();

                ASSERT_LT(std::abs(decompressedR32s[i] - r32s[i]), std::stod(accuracy)) << msg;
                ASSERT_LT(std::abs(decompressedR64s[i] - r64s[i]), std::stod(accuracy)) << msg;
            }
            ++t;
        }

        EXPECT_EQ(t, NSteps);

        bpReader.Close();
    }

#if ADIOS2_USE_MPI
    CleanupTestFilesMPI(fname, MPI_COMM_WORLD);
#else
    CleanupTestFiles(fname);
#endif
}

void SZAccuracy1DSel(const std::string accuracy)
{
    // Each process would write a 1x8 array and all processes would
    // form a mpiSize * Nx 1D array

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const size_t Nx = 1000;

    // Number of steps
    const size_t NSteps = 1;

    std::vector<float> r32s(Nx);
    std::vector<double> r64s(Nx);

    // range 0 to 999
    std::iota(r32s.begin(), r32s.end(), 0.f);
    std::iota(r64s.begin(), r64s.end(), 0.);

#if ADIOS2_USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    const std::string fname("BPWRSZ1DSel_" + accuracy + "_MPI.bp");
#else
    const std::string fname("BPWRSZ1DSel_" + accuracy + ".bp");
#endif

#if ADIOS2_USE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    adios2::ADIOS adios;
#endif
    {
        adios2::IO io = adios.DeclareIO("TestIO");

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            // Create the BP Engine
            io.SetEngine("BPFile");
        }

        const adios2::Dims shape{static_cast<size_t>(Nx * mpiSize)};
        const adios2::Dims start{static_cast<size_t>(Nx * mpiRank)};
        const adios2::Dims count{Nx};

        auto var_r32 = io.DefineVariable<float>("r32", shape, start, count, adios2::ConstantDims);
        auto var_r64 = io.DefineVariable<double>("r64", shape, start, count, adios2::ConstantDims);

        // add operations
        adios2::Operator szOp = adios.DefineOperator("szCompressor", adios2::ops::LossySZ);

        var_r32.AddOperation(szOp, {{adios2::ops::sz::key::accuracy, accuracy}});
        var_r64.AddOperation(szOp, {{adios2::ops::sz::key::accuracy, accuracy}});

        adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            bpWriter.BeginStep();
            bpWriter.Put<float>("r32", r32s.data());
            bpWriter.Put<double>("r64", r64s.data());
            bpWriter.EndStep();
        }

        bpWriter.Close();
    }

    {
        adios2::IO io = adios.DeclareIO("ReadIO");

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            // Create the BP Engine
            io.SetEngine("BPFile");
        }

        adios2::Engine bpReader = io.Open(fname, adios2::Mode::Read);

        unsigned int t = 0;
        std::vector<float> decompressedR32s;
        std::vector<double> decompressedR64s;

        while (bpReader.BeginStep() == adios2::StepStatus::OK)
        {
            auto var_r32 = io.InquireVariable<float>("r32");
            EXPECT_TRUE(var_r32);
            ASSERT_EQ(var_r32.ShapeID(), adios2::ShapeID::GlobalArray);
            ASSERT_EQ(var_r32.Steps(), NSteps);
            ASSERT_EQ(var_r32.Shape()[0], mpiSize * Nx);

            auto var_r64 = io.InquireVariable<double>("r64");
            EXPECT_TRUE(var_r64);
            ASSERT_EQ(var_r64.ShapeID(), adios2::ShapeID::GlobalArray);
            ASSERT_EQ(var_r64.Steps(), NSteps);
            ASSERT_EQ(var_r64.Shape()[0], mpiSize * Nx);

            const adios2::Dims start{mpiRank * Nx + Nx / 2};
            const adios2::Dims count{Nx / 2};
            const adios2::Box<adios2::Dims> sel(start, count);
            var_r32.SetSelection(sel);
            var_r64.SetSelection(sel);

            bpReader.Get(var_r32, decompressedR32s);
            bpReader.Get(var_r64, decompressedR64s);
            bpReader.EndStep();

            for (size_t i = 0; i < Nx / 2; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i << " rank=" << mpiRank;
                std::string msg = ss.str();

                ASSERT_LT(std::abs(decompressedR32s[i] - r32s[Nx / 2 + i]), std::stod(accuracy))
                    << msg;
                ASSERT_LT(std::abs(decompressedR64s[i] - r64s[Nx / 2 + i]), std::stod(accuracy))
                    << msg;
            }
            ++t;
        }

        EXPECT_EQ(t, NSteps);

        bpReader.Close();
    }

#if ADIOS2_USE_MPI
    CleanupTestFilesMPI(fname, MPI_COMM_WORLD);
#else
    CleanupTestFiles(fname);
#endif
}

void SZAccuracy2DSel(const std::string accuracy)
{
    // Each process would write a 1x8 array and all processes would
    // form a mpiSize * Nx 1D array

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const size_t Nx = 100;
    const size_t Ny = 50;

    // Number of steps
    const size_t NSteps = 1;

    std::vector<float> r32s(Nx * Ny);
    std::vector<double> r64s(Nx * Ny);

    // range 0 to 100*50
    std::iota(r32s.begin(), r32s.end(), 0.f);
    std::iota(r64s.begin(), r64s.end(), 0.);

#if ADIOS2_USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    const std::string fname("BPWRSZ2DSel_" + accuracy + "_MPI.bp");
#else
    const std::string fname("BPWRSZ2DSel_" + accuracy + ".bp");
#endif

#if ADIOS2_USE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    adios2::ADIOS adios;
#endif
    {
        adios2::IO io = adios.DeclareIO("TestIO");

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            // Create the BP Engine
            io.SetEngine("BPFile");
        }

        const adios2::Dims shape{static_cast<size_t>(Nx * mpiSize), Ny};
        const adios2::Dims start{static_cast<size_t>(Nx * mpiRank), 0};
        const adios2::Dims count{Nx, Ny};

        auto var_r32 = io.DefineVariable<float>("r32", shape, start, count, adios2::ConstantDims);
        auto var_r64 = io.DefineVariable<double>("r64", shape, start, count, adios2::ConstantDims);

        // add operations
        adios2::Operator szOp = adios.DefineOperator("szCompressor", adios2::ops::LossySZ);

        var_r32.AddOperation(szOp, {{adios2::ops::sz::key::accuracy, accuracy}});
        var_r64.AddOperation(szOp, {{adios2::ops::sz::key::accuracy, accuracy}});

        adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            bpWriter.BeginStep();
            bpWriter.Put<float>("r32", r32s.data());
            bpWriter.Put<double>("r64", r64s.data());
            bpWriter.EndStep();
        }

        bpWriter.Close();
    }

    {
        adios2::IO io = adios.DeclareIO("ReadIO");

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            // Create the BP Engine
            io.SetEngine("BPFile");
        }

        adios2::Engine bpReader = io.Open(fname, adios2::Mode::Read);

        unsigned int t = 0;
        std::vector<float> decompressedR32s;
        std::vector<double> decompressedR64s;

        while (bpReader.BeginStep() == adios2::StepStatus::OK)
        {
            auto var_r32 = io.InquireVariable<float>("r32");
            EXPECT_TRUE(var_r32);
            ASSERT_EQ(var_r32.ShapeID(), adios2::ShapeID::GlobalArray);
            ASSERT_EQ(var_r32.Steps(), NSteps);
            ASSERT_EQ(var_r32.Shape()[0], mpiSize * Nx);
            ASSERT_EQ(var_r32.Shape()[1], Ny);

            auto var_r64 = io.InquireVariable<double>("r64");
            EXPECT_TRUE(var_r64);
            ASSERT_EQ(var_r64.ShapeID(), adios2::ShapeID::GlobalArray);
            ASSERT_EQ(var_r64.Steps(), NSteps);
            ASSERT_EQ(var_r64.Shape()[0], mpiSize * Nx);
            ASSERT_EQ(var_r64.Shape()[1], Ny);

            const adios2::Dims start{mpiRank * Nx + Nx / 2, 0};
            const adios2::Dims count{Nx / 2, Ny};
            const adios2::Box<adios2::Dims> sel(start, count);
            var_r32.SetSelection(sel);
            var_r64.SetSelection(sel);

            bpReader.Get(var_r32, decompressedR32s);
            bpReader.Get(var_r64, decompressedR64s);
            bpReader.EndStep();

            for (size_t i = 0; i < Nx / 2 * Ny; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i << " rank=" << mpiRank;
                std::string msg = ss.str();

                ASSERT_LT(std::abs(decompressedR32s[i] - r32s[Nx / 2 * Ny + i]),
                          std::stod(accuracy))
                    << msg;
                ASSERT_LT(std::abs(decompressedR64s[i] - r64s[Nx / 2 * Ny + i]),
                          std::stod(accuracy))
                    << msg;
            }
            ++t;
        }

        EXPECT_EQ(t, NSteps);

        bpReader.Close();
    }

#if ADIOS2_USE_MPI
    CleanupTestFilesMPI(fname, MPI_COMM_WORLD);
#else
    CleanupTestFiles(fname);
#endif
}

void SZAccuracy3DSel(const std::string accuracy)
{
    // Each process would write a 1x8 array and all processes would
    // form a mpiSize * Nx 1D array

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const size_t Nx = 10;
    const size_t Ny = 20;
    const size_t Nz = 15;

    // Number of steps
    const size_t NSteps = 1;

    std::vector<float> r32s(Nx * Ny * Nz);
    std::vector<double> r64s(Nx * Ny * Nz);

    // range 0 to 100*50
    std::iota(r32s.begin(), r32s.end(), 0.f);
    std::iota(r64s.begin(), r64s.end(), 0.);

#if ADIOS2_USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    const std::string fname("BPWRSZ3DSel_" + accuracy + "_MPI.bp");
#else
    const std::string fname("BPWRSZ3DSel_" + accuracy + ".bp");
#endif

#if ADIOS2_USE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    adios2::ADIOS adios;
#endif
    {
        adios2::IO io = adios.DeclareIO("TestIO");

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            // Create the BP Engine
            io.SetEngine("BPFile");
        }

        const adios2::Dims shape{static_cast<size_t>(Nx * mpiSize), Ny, Nz};
        const adios2::Dims start{static_cast<size_t>(Nx * mpiRank), 0, 0};
        const adios2::Dims count{Nx, Ny, Nz};

        auto var_r32 = io.DefineVariable<float>("r32", shape, start, count, adios2::ConstantDims);
        auto var_r64 = io.DefineVariable<double>("r64", shape, start, count, adios2::ConstantDims);

        // add operations
        adios2::Operator szOp = adios.DefineOperator("szCompressor", adios2::ops::LossySZ);

        var_r32.AddOperation(szOp, {{adios2::ops::sz::key::accuracy, accuracy}});
        var_r64.AddOperation(szOp, {{adios2::ops::sz::key::accuracy, accuracy}});

        adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            bpWriter.BeginStep();
            bpWriter.Put<float>("r32", r32s.data());
            bpWriter.Put<double>("r64", r64s.data());
            bpWriter.EndStep();
        }

        bpWriter.Close();
    }

    {
        adios2::IO io = adios.DeclareIO("ReadIO");

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            // Create the BP Engine
            io.SetEngine("BPFile");
        }

        adios2::Engine bpReader = io.Open(fname, adios2::Mode::Read);

        unsigned int t = 0;
        std::vector<float> decompressedR32s;
        std::vector<double> decompressedR64s;

        while (bpReader.BeginStep() == adios2::StepStatus::OK)
        {
            auto var_r32 = io.InquireVariable<float>("r32");
            EXPECT_TRUE(var_r32);
            ASSERT_EQ(var_r32.ShapeID(), adios2::ShapeID::GlobalArray);
            ASSERT_EQ(var_r32.Steps(), NSteps);
            ASSERT_EQ(var_r32.Shape()[0], mpiSize * Nx);
            ASSERT_EQ(var_r32.Shape()[1], Ny);
            ASSERT_EQ(var_r32.Shape()[2], Nz);

            auto var_r64 = io.InquireVariable<double>("r64");
            EXPECT_TRUE(var_r64);
            ASSERT_EQ(var_r64.ShapeID(), adios2::ShapeID::GlobalArray);
            ASSERT_EQ(var_r64.Steps(), NSteps);
            ASSERT_EQ(var_r64.Shape()[0], mpiSize * Nx);
            ASSERT_EQ(var_r64.Shape()[1], Ny);
            ASSERT_EQ(var_r64.Shape()[2], Nz);

            const adios2::Dims start{mpiRank * Nx + Nx / 2, 0, 0};
            const adios2::Dims count{Nx / 2, Ny, Nz};
            const adios2::Box<adios2::Dims> sel(start, count);
            var_r32.SetSelection(sel);
            var_r64.SetSelection(sel);

            bpReader.Get(var_r32, decompressedR32s);
            bpReader.Get(var_r64, decompressedR64s);
            bpReader.EndStep();

            for (size_t i = 0; i < Nx / 2 * Ny * Nz; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i << " rank=" << mpiRank;
                std::string msg = ss.str();

                ASSERT_LT(std::abs(decompressedR32s[i] - r32s[Nx / 2 * Ny * Nz + i]),
                          std::stod(accuracy))
                    << msg;
                ASSERT_LT(std::abs(decompressedR64s[i] - r64s[Nx / 2 * Ny * Nz + i]),
                          std::stod(accuracy))
                    << msg;
            }
            ++t;
        }

        EXPECT_EQ(t, NSteps);

        bpReader.Close();
    }

#if ADIOS2_USE_MPI
    CleanupTestFilesMPI(fname, MPI_COMM_WORLD);
#else
    CleanupTestFiles(fname);
#endif
}

void SZAccuracy2DSmallSel(const std::string accuracy)
{
    // Each process would write a 1x8 array and all processes would
    // form a mpiSize * Nx 1D array

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const size_t Nx = 5;
    const size_t Ny = 5;

    // Number of steps
    const size_t NSteps = 1;

    std::vector<float> r32s = {0.00, 0.01, 0.02, 0.03, 0.04, 0.05, 0.06, 0.07, 0.08,
                               0.09, 0.10, 0.11, 0.12, 0.13, 0.14, 0.15, 0.16, 0.17,
                               0.18, 0.19, 0.20, 0.21, 0.22, 0.23, 0.24};
    std::vector<double> r64s = {0.00, 0.01, 0.02, 0.03, 0.04, 0.05, 0.06, 0.07, 0.08,
                                0.09, 0.10, 0.11, 0.12, 0.13, 0.14, 0.15, 0.16, 0.17,
                                0.18, 0.19, 0.20, 0.21, 0.22, 0.23, 0.24};
#if ADIOS2_USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    const std::string fname("BPWRSZ2DSmallSel_" + accuracy + "_MPI.bp");
#else
    const std::string fname("BPWRSZ2DSmallSel_" + accuracy + ".bp");
#endif

#if ADIOS2_USE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    adios2::ADIOS adios;
#endif
    {
        adios2::IO io = adios.DeclareIO("TestIO");

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            // Create the BP Engine
            io.SetEngine("BPFile");
        }

        const adios2::Dims shape{static_cast<size_t>(Nx * mpiSize), Ny};
        const adios2::Dims start{static_cast<size_t>(Nx * mpiRank), 0};
        const adios2::Dims count{Nx, Ny};

        auto var_r32 = io.DefineVariable<float>("r32", shape, start, count, adios2::ConstantDims);
        auto var_r64 = io.DefineVariable<double>("r64", shape, start, count, adios2::ConstantDims);

        // add operations
        adios2::Operator szOp = adios.DefineOperator("szCompressor", adios2::ops::LossySZ);

        var_r32.AddOperation(szOp, {{adios2::ops::sz::key::accuracy, accuracy}});
        var_r64.AddOperation(szOp, {{adios2::ops::sz::key::accuracy, accuracy}});

        adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            bpWriter.BeginStep();
            bpWriter.Put<float>("r32", r32s.data());
            bpWriter.Put<double>("r64", r64s.data());
            bpWriter.EndStep();
        }

        bpWriter.Close();
    }

    {
        adios2::IO io = adios.DeclareIO("ReadIO");

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            // Create the BP Engine
            io.SetEngine("BPFile");
        }

        adios2::Engine bpReader = io.Open(fname, adios2::Mode::Read);

        unsigned int t = 0;
        std::vector<float> decompressedR32s;
        std::vector<double> decompressedR64s;

        while (bpReader.BeginStep() == adios2::StepStatus::OK)
        {
            auto var_r32 = io.InquireVariable<float>("r32");
            EXPECT_TRUE(var_r32);
            ASSERT_EQ(var_r32.ShapeID(), adios2::ShapeID::GlobalArray);
            ASSERT_EQ(var_r32.Steps(), NSteps);
            ASSERT_EQ(var_r32.Shape()[0], mpiSize * Nx);
            ASSERT_EQ(var_r32.Shape()[1], Ny);

            auto var_r64 = io.InquireVariable<double>("r64");
            EXPECT_TRUE(var_r64);
            ASSERT_EQ(var_r64.ShapeID(), adios2::ShapeID::GlobalArray);
            ASSERT_EQ(var_r64.Steps(), NSteps);
            ASSERT_EQ(var_r64.Shape()[0], mpiSize * Nx);
            ASSERT_EQ(var_r64.Shape()[1], Ny);

            const adios2::Dims start{static_cast<std::size_t>(mpiRank) * Nx + 1, 1};
            const adios2::Dims count{2, 2};
            const adios2::Box<adios2::Dims> sel(start, count);
            var_r32.SetSelection(sel);
            var_r64.SetSelection(sel);

            bpReader.Get(var_r32, decompressedR32s);
            bpReader.Get(var_r64, decompressedR64s);
            bpReader.EndStep();

            ASSERT_LT(std::abs(decompressedR32s[0] - 0.06), std::stod(accuracy));
            ASSERT_LT(std::abs(decompressedR64s[0] - 0.06), std::stod(accuracy));

            ASSERT_LT(std::abs(decompressedR32s[1] - 0.07), std::stod(accuracy));
            ASSERT_LT(std::abs(decompressedR64s[1] - 0.07), std::stod(accuracy));

            ASSERT_LT(std::abs(decompressedR32s[2] - 0.11), std::stod(accuracy));
            ASSERT_LT(std::abs(decompressedR64s[2] - 0.11), std::stod(accuracy));

            ASSERT_LT(std::abs(decompressedR32s[3] - 0.12), std::stod(accuracy));
            ASSERT_LT(std::abs(decompressedR64s[3] - 0.12), std::stod(accuracy));

            ++t;
        }

        EXPECT_EQ(t, NSteps);

        bpReader.Close();
    }

#if ADIOS2_USE_MPI
    CleanupTestFilesMPI(fname, MPI_COMM_WORLD);
#else
    CleanupTestFiles(fname);
#endif
}

void SZMemorySelection3D(const std::string accuracy)
{
    // BP3/BP4 don't support compression + MemorySelection
    if (engineName == "BP3" || engineName == "BP4")
        GTEST_SKIP();

    // Write a 3D sub-region from a larger in-memory buffer (with ghost cells)
    // using SetMemorySelection + compression. Verifies that compression
    // works correctly with strided memory layouts (GitHub issue #4965).

    const size_t Nx = 10, Ny = 12, Nz = 8;
    const size_t ghost = 1;
    const size_t Mx = Nx + 2 * ghost;
    const size_t My = Ny + 2 * ghost;
    const size_t Mz = Nz + 2 * ghost;

    // Fill the interior of a halo'd buffer with known values.
    // Buffer layout is row-major with dims {Mx, My, Mz}: index = (i*My + j)*Mz + k
    std::vector<double> full(Mx * My * Mz, -1.0);
    for (size_t i = ghost; i < Nx + ghost; ++i)
        for (size_t j = ghost; j < Ny + ghost; ++j)
            for (size_t k = ghost; k < Nz + ghost; ++k)
                full[(i * My + j) * Mz + k] = static_cast<double>(i + j * Nx + k * Nx * Ny);

#if ADIOS2_USE_MPI
    const std::string fname("BPWRSZ3DMemSel_" + accuracy + "_MPI.bp");
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    const std::string fname("BPWRSZ3DMemSel_" + accuracy + ".bp");
    adios2::ADIOS adios;
#endif
    {
        adios2::IO io = adios.DeclareIO("TestIO");
        if (!engineName.empty())
            io.SetEngine(engineName);

        auto var = io.DefineVariable<double>("data", {Nx, Ny, Nz}, {0, 0, 0}, {Nx, Ny, Nz},
                                             adios2::ConstantDims);

        adios2::Operator szOp = adios.DefineOperator("szCompressor", adios2::ops::LossySZ);
        var.AddOperation(szOp, {{adios2::ops::sz::key::accuracy, accuracy}});

        var.SetMemorySelection({{ghost, ghost, ghost}, {Mx, My, Mz}});

        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        writer.BeginStep();
        writer.Put(var, full.data(), adios2::Mode::Sync);
        writer.EndStep();
        writer.Close();
    }

    auto checkInterior = [&](const std::vector<double> &result, const double tol) {
        for (size_t i = 0; i < Nx; ++i)
            for (size_t j = 0; j < Ny; ++j)
                for (size_t k = 0; k < Nz; ++k)
                {
                    double expected =
                        static_cast<double>((i + ghost) + (j + ghost) * Nx + (k + ghost) * Nx * Ny);
                    size_t idx = (i * Ny + j) * Nz + k;
                    ASSERT_NEAR(result[idx], expected, tol)
                        << "Mismatch at (" << i << "," << j << "," << k << ")";
                }
    };

    // Read back into a contiguous buffer (verify write path)
    {
        adios2::IO io = adios.DeclareIO("ReadIO");
        if (!engineName.empty())
            io.SetEngine(engineName);

        adios2::Engine reader = io.Open(fname, adios2::Mode::Read);
        reader.BeginStep();
        auto var = io.InquireVariable<double>("data");
        ASSERT_TRUE(var);
        ASSERT_EQ(var.Shape().size(), 3);
        ASSERT_EQ(var.Shape()[0], Nx);
        ASSERT_EQ(var.Shape()[1], Ny);
        ASSERT_EQ(var.Shape()[2], Nz);

        std::vector<double> result(Nx * Ny * Nz);
        reader.Get(var, result.data(), adios2::Mode::Sync);
        reader.EndStep();
        reader.Close();

        checkInterior(result, std::stod(accuracy));
    }

    // Read back with MemorySelection into a halo'd buffer (verify read path)
    {
        adios2::IO io = adios.DeclareIO("ReadMemSelIO");
        if (!engineName.empty())
            io.SetEngine(engineName);

        adios2::Engine reader = io.Open(fname, adios2::Mode::Read);
        reader.BeginStep();
        auto var = io.InquireVariable<double>("data");
        ASSERT_TRUE(var);

        const size_t Rx = Nx + 2 * ghost;
        const size_t Ry = Ny + 2 * ghost;
        const size_t Rz = Nz + 2 * ghost;
        std::vector<double> readBuf(Rx * Ry * Rz, -1.0);
        var.SetMemorySelection({{ghost, ghost, ghost}, {Rx, Ry, Rz}});
        reader.Get(var, readBuf.data(), adios2::Mode::Sync);
        reader.EndStep();
        reader.Close();

        // Extract interior from the halo'd read buffer into a contiguous buffer
        std::vector<double> result(Nx * Ny * Nz);
        for (size_t i = 0; i < Nx; ++i)
            for (size_t j = 0; j < Ny; ++j)
                for (size_t k = 0; k < Nz; ++k)
                    result[(i * Ny + j) * Nz + k] =
                        readBuf[((i + ghost) * Ry + (j + ghost)) * Rz + (k + ghost)];

        checkInterior(result, std::stod(accuracy));
    }
}

class BPWriteReadSZ : public ::testing::TestWithParam<std::string>
{
public:
    BPWriteReadSZ() = default;
    virtual void SetUp(){};
    virtual void TearDown(){};
};

TEST_P(BPWriteReadSZ, BPWRSZ1D) { SZAccuracy1D(GetParam()); }
TEST_P(BPWriteReadSZ, BPWRSZ2D) { SZAccuracy2D(GetParam()); }
TEST_P(BPWriteReadSZ, BPWRSZ3D) { SZAccuracy3D(GetParam()); }
TEST_P(BPWriteReadSZ, BPWRSZ1DSel) { SZAccuracy1DSel(GetParam()); }
TEST_P(BPWriteReadSZ, BPWRSZ2DSel) { SZAccuracy2DSel(GetParam()); }
TEST_P(BPWriteReadSZ, BPWRSZ3DSel) { SZAccuracy3DSel(GetParam()); }
TEST_F(BPWriteReadSZ, BPWRSZ2DSmallSel) { SZAccuracy2DSmallSel("0.01"); }
TEST_F(BPWriteReadSZ, BPWRSZ3DMemSel) { SZMemorySelection3D("0.01"); }

INSTANTIATE_TEST_SUITE_P(SZAccuracy, BPWriteReadSZ,
                         ::testing::Values("0.01", "0.001", "0.0001", "0.00001"));

int main(int argc, char **argv)
{
#if ADIOS2_USE_MPI
    int provided;

    // MPI_THREAD_MULTIPLE is only required if you enable the SST MPI_DP
    MPI_Init_thread(nullptr, nullptr, MPI_THREAD_MULTIPLE, &provided);
#endif

    int result;
    ::testing::InitGoogleTest(&argc, argv);

    if (argc > 1)
    {
        engineName = std::string(argv[1]);
    }
    result = RUN_ALL_TESTS();

#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif

    return result;
}
