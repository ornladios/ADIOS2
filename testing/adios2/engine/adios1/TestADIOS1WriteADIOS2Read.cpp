/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include <cstdint>
#include <cstring>

#include <iostream>
#include <stdexcept>

#include <adios.h>
#include <adios2.h>
#include <adios_read.h>

#include <gtest/gtest.h>

#include "../SmallTestData.h"

#ifdef ADIOS2_HAVE_MPI
#include "mpi.h"
#endif

class ADIOS1WriteADIOS2ReadTest : public ::testing::Test
{
public:
    ADIOS1WriteADIOS2ReadTest() = default;

    SmallTestData m_TestData;
};

//******************************************************************************
// 1D 1x8 test data
//******************************************************************************

// ADIOS2 write, native ADIOS1 read
TEST_F(ADIOS1WriteADIOS2ReadTest, ADIOS1WriteADIOS2Read1D8)
{
    // Each process would write a 1x8 array and all processes would
    // form a mpiSize * Nx 1D array
    std::string fname = "ADIOS1WriteADIOS2Read1D8.bp";

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const std::size_t Nx = 8;

    // Number of steps
    const std::size_t NSteps = 3;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

// Write test data using ADIOS2

#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(true);
#endif

    {
        adios2::IO &io = adios.DeclareIO("ADIOS1Write");

        // Declare 1D variables (NumOfProcesses * Nx)
        // The local process' part (start, count) can be defined now or later
        // before Write().
        {
            adios2::Dims shape{static_cast<size_t>(Nx * mpiSize)};
            adios2::Dims start{static_cast<size_t>(Nx * mpiRank)};
            adios2::Dims count{Nx};
            auto &var_i8 = io.DefineVariable<int8_t>("i8", shape, start, count);
            auto &var_i16 =
                io.DefineVariable<int16_t>("i16", shape, start, count);
            auto &var_i32 =
                io.DefineVariable<int32_t>("i32", shape, start, count);
            auto &var_i64 =
                io.DefineVariable<int64_t>("i64", shape, start, count);
            auto &var_u8 =
                io.DefineVariable<uint8_t>("u8", shape, start, count);
            auto &var_u16 =
                io.DefineVariable<uint16_t>("u16", shape, start, count);
            auto &var_u32 =
                io.DefineVariable<uint32_t>("u32", shape, start, count);
            auto &var_u64 =
                io.DefineVariable<uint64_t>("u64", shape, start, count);
            auto &var_r32 =
                io.DefineVariable<float>("r32", shape, start, count);
            auto &var_r64 =
                io.DefineVariable<double>("r64", shape, start, count);
        }

        // Create the ADIOS 1 Engine
        io.SetEngine("ADIOS1");
        io.AddTransport("file");

        adios2::Engine &engine = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            // Generate test data for each process uniquely
            SmallTestData currentTestData =
                generateNewSmallTestData(m_TestData, step, mpiRank, mpiSize);

            // Retrieve the variables that previously went out of scope
            auto &var_i8 = *io.InquireVariable<int8_t>("i8");
            auto &var_i16 = *io.InquireVariable<int16_t>("i16");
            auto &var_i32 = *io.InquireVariable<int32_t>("i32");
            auto &var_i64 = *io.InquireVariable<int64_t>("i64");
            auto &var_u8 = *io.InquireVariable<uint8_t>("u8");
            auto &var_u16 = *io.InquireVariable<uint16_t>("u16");
            auto &var_u32 = *io.InquireVariable<uint32_t>("u32");
            auto &var_u64 = *io.InquireVariable<uint64_t>("u64");
            auto &var_r32 = *io.InquireVariable<float>("r32");
            auto &var_r64 = *io.InquireVariable<double>("r64");

            // Make a 1D selection to describe the local dimensions of the
            // variable we write and its offsets in the global spaces
            adios2::Box<adios2::Dims> sel({mpiRank * Nx}, {Nx});
            var_i8.SetSelection(sel);
            var_i16.SetSelection(sel);
            var_i32.SetSelection(sel);
            var_i64.SetSelection(sel);
            var_u8.SetSelection(sel);
            var_u16.SetSelection(sel);
            var_u32.SetSelection(sel);
            var_u64.SetSelection(sel);
            var_r32.SetSelection(sel);
            var_r64.SetSelection(sel);

            // Write each one
            // fill in the variable with values from starting index to
            // starting index + count
            engine.PutSync(var_i8, currentTestData.I8.data());
            engine.PutSync(var_i16, currentTestData.I16.data());
            engine.PutSync(var_i32, currentTestData.I32.data());
            engine.PutSync(var_i64, currentTestData.I64.data());
            engine.PutSync(var_u8, currentTestData.U8.data());
            engine.PutSync(var_u16, currentTestData.U16.data());
            engine.PutSync(var_u32, currentTestData.U32.data());
            engine.PutSync(var_u64, currentTestData.U64.data());
            engine.PutSync(var_r32, currentTestData.R32.data());
            engine.PutSync(var_r64, currentTestData.R64.data());

            // Advance to the next time step
            engine.EndStep();
        }

        // Close the file
        engine.Close();

        adios_finalize(0);
    }

    {
        adios2::IO &io = adios.DeclareIO("ADIOS2Read");
        adios2::Engine &engine = io.Open(fname, adios2::Mode::Read);

        auto var_i8 = io.InquireVariable<int8_t>("i8");
        ASSERT_NE(var_i8, nullptr);
        ASSERT_EQ(var_i8->m_Shape.size(), 1);
        ASSERT_EQ(var_i8->m_Shape[0], mpiSize * Nx);
        ASSERT_EQ(var_i8->GetAvailableStepsCount(), NSteps);

        auto var_i16 = io.InquireVariable<int16_t>("i16");
        ASSERT_NE(var_i16, nullptr);
        ASSERT_EQ(var_i16->m_Shape.size(), 1);
        ASSERT_EQ(var_i16->m_Shape[0], mpiSize * Nx);
        ASSERT_EQ(var_i16->GetAvailableStepsCount(), NSteps);

        auto var_i32 = io.InquireVariable<int32_t>("i32");
        ASSERT_NE(var_i32, nullptr);
        ASSERT_EQ(var_i32->m_Shape.size(), 1);
        ASSERT_EQ(var_i32->m_Shape[0], mpiSize * Nx);
        ASSERT_EQ(var_i32->GetAvailableStepsCount(), NSteps);

        auto var_i64 = io.InquireVariable<int64_t>("i64");
        ASSERT_NE(var_i64, nullptr);
        ASSERT_EQ(var_i64->m_Shape.size(), 1);
        ASSERT_EQ(var_i64->m_Shape[0], mpiSize * Nx);
        ASSERT_EQ(var_i64->GetAvailableStepsCount(), NSteps);

        auto var_u8 = io.InquireVariable<uint8_t>("u8");
        ASSERT_NE(var_u8, nullptr);
        ASSERT_EQ(var_u8->m_Shape.size(), 1);
        ASSERT_EQ(var_u8->m_Shape[0], mpiSize * Nx);
        ASSERT_EQ(var_u8->GetAvailableStepsCount(), NSteps);

        auto var_u16 = io.InquireVariable<uint16_t>("u16");
        ASSERT_NE(var_u16, nullptr);
        ASSERT_EQ(var_u16->m_Shape.size(), 1);
        ASSERT_EQ(var_u16->m_Shape[0], mpiSize * Nx);
        ASSERT_EQ(var_u16->GetAvailableStepsCount(), NSteps);

        auto var_u32 = io.InquireVariable<uint32_t>("u32");
        ASSERT_NE(var_u32, nullptr);
        ASSERT_EQ(var_u32->m_Shape.size(), 1);
        ASSERT_EQ(var_u32->m_Shape[0], mpiSize * Nx);
        ASSERT_EQ(var_u32->GetAvailableStepsCount(), NSteps);

        auto var_u64 = io.InquireVariable<uint64_t>("u64");
        ASSERT_NE(var_u64, nullptr);
        ASSERT_EQ(var_u64->m_Shape.size(), 1);
        ASSERT_EQ(var_u64->m_Shape[0], mpiSize * Nx);
        ASSERT_EQ(var_u64->GetAvailableStepsCount(), NSteps);

        auto var_r32 = io.InquireVariable<float>("r32");
        ASSERT_NE(var_r32, nullptr);
        ASSERT_EQ(var_r32->m_Shape.size(), 1);
        ASSERT_EQ(var_r32->m_Shape[0], mpiSize * Nx);
        ASSERT_EQ(var_r32->GetAvailableStepsCount(), NSteps);

        auto var_r64 = io.InquireVariable<double>("r64");
        ASSERT_NE(var_r64, nullptr);
        ASSERT_EQ(var_r64->m_Shape.size(), 1);
        ASSERT_EQ(var_r64->m_Shape[0], mpiSize * Nx);
        ASSERT_EQ(var_r64->GetAvailableStepsCount(), NSteps);

        adios2::Box<adios2::Dims> selection({{mpiRank * Nx}, {Nx}});

        var_i8->SetSelection(selection);
        var_i16->SetSelection(selection);
        var_i32->SetSelection(selection);
        var_i64->SetSelection(selection);
        var_u8->SetSelection(selection);
        var_u16->SetSelection(selection);
        var_u32->SetSelection(selection);
        var_u64->SetSelection(selection);
        var_r32->SetSelection(selection);
        var_r64->SetSelection(selection);

        std::array<int8_t, Nx> I8;
        std::array<int16_t, Nx> I16;
        std::array<int32_t, Nx> I32;
        std::array<int64_t, Nx> I64;
        std::array<uint8_t, Nx> U8;
        std::array<uint16_t, Nx> U16;
        std::array<uint32_t, Nx> U32;
        std::array<uint64_t, Nx> U64;
        std::array<float, Nx> R32;
        std::array<double, Nx> R64;

        // Read stuff
        for (size_t t = 0; t < NSteps; ++t)
        {
            // Generate test data for each rank uniquely
            SmallTestData currentTestData =
                generateNewSmallTestData(m_TestData, t, mpiRank, mpiSize);
            // Read the current step
            engine.BeginStep();
            engine.GetDeferred(*var_i8, I8.data());
            engine.GetDeferred(*var_i16, I16.data());
            engine.GetDeferred(*var_i32, I32.data());
            engine.GetDeferred(*var_i64, I64.data());

            engine.GetDeferred(*var_u8, U8.data());
            engine.GetDeferred(*var_u16, U16.data());
            engine.GetDeferred(*var_u32, U32.data());
            engine.GetDeferred(*var_u64, U64.data());

            engine.GetDeferred(*var_r32, R32.data());
            engine.GetDeferred(*var_r64, R64.data());

            engine.PerformGets();

            engine.EndStep();

            // Check if it's correct
            for (size_t i = 0; i < Nx; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i << " rank=" << mpiRank;
                std::string msg = ss.str();

                EXPECT_EQ(I8[i], currentTestData.I8[i]) << msg;
                EXPECT_EQ(I16[i], currentTestData.I16[i]) << msg;
                EXPECT_EQ(I32[i], currentTestData.I32[i]) << msg;
                EXPECT_EQ(I64[i], currentTestData.I64[i]) << msg;
                EXPECT_EQ(U8[i], currentTestData.U8[i]) << msg;
                EXPECT_EQ(U16[i], currentTestData.U16[i]) << msg;
                EXPECT_EQ(U32[i], currentTestData.U32[i]) << msg;
                EXPECT_EQ(U64[i], currentTestData.U64[i]) << msg;
                EXPECT_EQ(R32[i], currentTestData.R32[i]) << msg;
                EXPECT_EQ(R64[i], currentTestData.R64[i]) << msg;
            }
        }
        engine.Close();
    }
}

//******************************************************************************
// 2D 2x4 test data
//******************************************************************************

// ADIOS2 write, native ADIOS1 read
TEST_F(ADIOS1WriteADIOS2ReadTest, ADIOS1WriteADIOS2Read2D2x4)
{
    // Each process would write a 2x4 array and all processes would
    // form a 2D 2 * (numberOfProcess*Nx) matrix where Nx is 4 here
    std::string fname = "ADIOS1WriteADIOS2Read2D2x4Test.bp";

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const std::size_t Nx = 4;

    // Number of rows
    const std::size_t Ny = 2;

    // Number of steps
    const std::size_t NSteps = 3;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

// Write test data using ADIOS2
#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(true);
#endif
    {
        adios2::IO &io = adios.DeclareIO("TestIO");

        // Declare 2D variables (Ny * (NumOfProcesses * Nx))
        // The local process' part (start, count) can be defined now or later
        // before Write().
        {
            adios2::Dims shape{Ny, static_cast<unsigned int>(mpiSize * Nx)};
            adios2::Dims start{0, static_cast<unsigned int>(mpiRank * Nx)};
            adios2::Dims count{Ny, Nx};
            auto &var_i8 = io.DefineVariable<int8_t>("i8", shape, start, count);
            auto &var_i16 =
                io.DefineVariable<int16_t>("i16", shape, start, count);
            auto &var_i32 =
                io.DefineVariable<int32_t>("i32", shape, start, count);
            auto &var_i64 =
                io.DefineVariable<int64_t>("i64", shape, start, count);
            auto &var_u8 =
                io.DefineVariable<uint8_t>("u8", shape, start, count);
            auto &var_u16 =
                io.DefineVariable<uint16_t>("u16", shape, start, count);
            auto &var_u32 =
                io.DefineVariable<uint32_t>("u32", shape, start, count);
            auto &var_u64 =
                io.DefineVariable<uint64_t>("u64", shape, start, count);
            auto &var_r32 =
                io.DefineVariable<float>("r32", shape, start, count);
            auto &var_r64 =
                io.DefineVariable<double>("r64", shape, start, count);
        }

        // Create the ADIOS 1 Engine
        io.SetEngine("ADIOS1");
        io.AddTransport("file");

        adios2::Engine &engine = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            // Generate test data for each process uniquely
            SmallTestData currentTestData =
                generateNewSmallTestData(m_TestData, step, mpiRank, mpiSize);

            // Retrieve the variables that previously went out of scope
            auto &var_i8 = *io.InquireVariable<int8_t>("i8");
            auto &var_i16 = *io.InquireVariable<int16_t>("i16");
            auto &var_i32 = *io.InquireVariable<int32_t>("i32");
            auto &var_i64 = *io.InquireVariable<int64_t>("i64");
            auto &var_u8 = *io.InquireVariable<uint8_t>("u8");
            auto &var_u16 = *io.InquireVariable<uint16_t>("u16");
            auto &var_u32 = *io.InquireVariable<uint32_t>("u32");
            auto &var_u64 = *io.InquireVariable<uint64_t>("u64");
            auto &var_r32 = *io.InquireVariable<float>("r32");
            auto &var_r64 = *io.InquireVariable<double>("r64");

            // Make a 2D selection to describe the local dimensions of the
            // variable we write and its offsets in the global spaces
            adios2::Box<adios2::Dims> sel(
                {0, static_cast<unsigned int>(mpiRank * Nx)}, {Ny, Nx});
            var_i8.SetSelection(sel);
            var_i16.SetSelection(sel);
            var_i32.SetSelection(sel);
            var_i64.SetSelection(sel);
            var_u8.SetSelection(sel);
            var_u16.SetSelection(sel);
            var_u32.SetSelection(sel);
            var_u64.SetSelection(sel);
            var_r32.SetSelection(sel);
            var_r64.SetSelection(sel);

            // Write each one
            // fill in the variable with values from starting index to
            // starting index + count
            engine.PutSync(var_i8, currentTestData.I8.data());
            engine.PutSync(var_i16, currentTestData.I16.data());
            engine.PutSync(var_i32, currentTestData.I32.data());
            engine.PutSync(var_i64, currentTestData.I64.data());
            engine.PutSync(var_u8, currentTestData.U8.data());
            engine.PutSync(var_u16, currentTestData.U16.data());
            engine.PutSync(var_u32, currentTestData.U32.data());
            engine.PutSync(var_u64, currentTestData.U64.data());
            engine.PutSync(var_r32, currentTestData.R32.data());
            engine.PutSync(var_r64, currentTestData.R64.data());

            engine.EndStep();
        }

        // Close the file
        engine.Close();

        adios_finalize(0);
    }

    {
        adios2::IO &io = adios.DeclareIO("ADIOS2Read");
        adios2::Engine &engine = io.Open(fname, adios2::Mode::Read);

        auto var_i8 = io.InquireVariable<int8_t>("i8");
        ASSERT_NE(var_i8, nullptr);
        ASSERT_EQ(var_i8->m_Shape.size(), 2);
        ASSERT_EQ(var_i8->m_Shape[0], Ny);
        ASSERT_EQ(var_i8->m_Shape[1], mpiSize * Nx);
        ASSERT_EQ(var_i8->GetAvailableStepsCount(), NSteps);

        auto var_i16 = io.InquireVariable<int16_t>("i16");
        ASSERT_NE(var_i16, nullptr);
        ASSERT_EQ(var_i16->m_Shape.size(), 2);
        ASSERT_EQ(var_i16->m_Shape[0], Ny);
        ASSERT_EQ(var_i16->m_Shape[1], mpiSize * Nx);
        ASSERT_EQ(var_i16->GetAvailableStepsCount(), NSteps);

        auto var_i32 = io.InquireVariable<int32_t>("i32");
        ASSERT_NE(var_i32, nullptr);
        ASSERT_EQ(var_i32->m_Shape.size(), 2);
        ASSERT_EQ(var_i32->m_Shape[0], Ny);
        ASSERT_EQ(var_i32->m_Shape[1], mpiSize * Nx);
        ASSERT_EQ(var_i32->GetAvailableStepsCount(), NSteps);

        auto var_i64 = io.InquireVariable<int64_t>("i64");
        ASSERT_NE(var_i64, nullptr);
        ASSERT_EQ(var_i64->m_Shape.size(), 2);
        ASSERT_EQ(var_i64->m_Shape[0], Ny);
        ASSERT_EQ(var_i64->m_Shape[1], mpiSize * Nx);
        ASSERT_EQ(var_i64->GetAvailableStepsCount(), NSteps);

        auto var_u8 = io.InquireVariable<uint8_t>("u8");
        ASSERT_NE(var_u8, nullptr);
        ASSERT_EQ(var_u8->m_Shape.size(), 2);
        ASSERT_EQ(var_u8->m_Shape[0], Ny);
        ASSERT_EQ(var_u8->m_Shape[1], mpiSize * Nx);
        ASSERT_EQ(var_u8->GetAvailableStepsCount(), NSteps);

        auto var_u16 = io.InquireVariable<uint16_t>("u16");
        ASSERT_NE(var_u16, nullptr);
        ASSERT_EQ(var_u16->m_Shape.size(), 2);
        ASSERT_EQ(var_u16->m_Shape[0], Ny);
        ASSERT_EQ(var_u16->m_Shape[1], mpiSize * Nx);
        ASSERT_EQ(var_u16->GetAvailableStepsCount(), NSteps);

        auto var_u32 = io.InquireVariable<uint32_t>("u32");
        ASSERT_NE(var_u32, nullptr);
        ASSERT_EQ(var_u32->m_Shape.size(), 2);
        ASSERT_EQ(var_u32->m_Shape[0], Ny);
        ASSERT_EQ(var_u32->m_Shape[1], mpiSize * Nx);
        ASSERT_EQ(var_u32->GetAvailableStepsCount(), NSteps);

        auto var_u64 = io.InquireVariable<uint64_t>("u64");
        ASSERT_NE(var_u64, nullptr);
        ASSERT_EQ(var_u64->m_Shape.size(), 2);
        ASSERT_EQ(var_u64->m_Shape[0], Ny);
        ASSERT_EQ(var_u64->m_Shape[1], mpiSize * Nx);
        ASSERT_EQ(var_u64->GetAvailableStepsCount(), NSteps);

        auto var_r32 = io.InquireVariable<float>("r32");
        ASSERT_NE(var_r32, nullptr);
        ASSERT_EQ(var_r32->m_Shape.size(), 2);
        ASSERT_EQ(var_r32->m_Shape[0], Ny);
        ASSERT_EQ(var_r32->m_Shape[1], mpiSize * Nx);
        ASSERT_EQ(var_r32->GetAvailableStepsCount(), NSteps);

        auto var_r64 = io.InquireVariable<double>("r64");
        ASSERT_NE(var_r64, nullptr);
        ASSERT_EQ(var_r64->m_Shape.size(), 2);
        ASSERT_EQ(var_r64->m_Shape[0], Ny);
        ASSERT_EQ(var_r64->m_Shape[1], mpiSize * Nx);
        ASSERT_EQ(var_r64->GetAvailableStepsCount(), NSteps);

        adios2::Box<adios2::Dims> selection({{0, mpiRank * Nx}, {Ny, Nx}});

        var_i8->SetSelection(selection);
        var_i16->SetSelection(selection);
        var_i32->SetSelection(selection);
        var_i64->SetSelection(selection);
        var_u8->SetSelection(selection);
        var_u16->SetSelection(selection);
        var_u32->SetSelection(selection);
        var_u64->SetSelection(selection);
        var_r32->SetSelection(selection);
        var_r64->SetSelection(selection);

        std::array<int8_t, Nx * Ny> I8;
        std::array<int16_t, Nx * Ny> I16;
        std::array<int32_t, Nx * Ny> I32;
        std::array<int64_t, Nx * Ny> I64;
        std::array<uint8_t, Nx * Ny> U8;
        std::array<uint16_t, Nx * Ny> U16;
        std::array<uint32_t, Nx * Ny> U32;
        std::array<uint64_t, Nx * Ny> U64;
        std::array<float, Nx * Ny> R32;
        std::array<double, Nx * Ny> R64;

        // Read stuff
        for (size_t t = 0; t < NSteps; ++t)
        {
            // Generate test data for each rank uniquely
            SmallTestData currentTestData =
                generateNewSmallTestData(m_TestData, t, mpiRank, mpiSize);
            // Read the current step
            engine.BeginStep();
            engine.GetDeferred(*var_i8, I8.data());
            engine.GetDeferred(*var_i16, I16.data());
            engine.GetDeferred(*var_i32, I32.data());
            engine.GetDeferred(*var_i64, I64.data());

            engine.GetDeferred(*var_u8, U8.data());
            engine.GetDeferred(*var_u16, U16.data());
            engine.GetDeferred(*var_u32, U32.data());
            engine.GetDeferred(*var_u64, U64.data());

            engine.GetDeferred(*var_r32, R32.data());
            engine.GetDeferred(*var_r64, R64.data());

            engine.PerformGets();

            engine.EndStep();

            // Check if it's correct
            for (size_t i = 0; i < Nx * Ny; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i << " rank=" << mpiRank;
                std::string msg = ss.str();

                EXPECT_EQ(I8[i], currentTestData.I8[i]) << msg;
                EXPECT_EQ(I16[i], currentTestData.I16[i]) << msg;
                EXPECT_EQ(I32[i], currentTestData.I32[i]) << msg;
                EXPECT_EQ(I64[i], currentTestData.I64[i]) << msg;
                EXPECT_EQ(U8[i], currentTestData.U8[i]) << msg;
                EXPECT_EQ(U16[i], currentTestData.U16[i]) << msg;
                EXPECT_EQ(U32[i], currentTestData.U32[i]) << msg;
                EXPECT_EQ(U64[i], currentTestData.U64[i]) << msg;
                EXPECT_EQ(R32[i], currentTestData.R32[i]) << msg;
                EXPECT_EQ(R64[i], currentTestData.R64[i]) << msg;
            }
        }
        engine.Close();
    }
}

//******************************************************************************
// 2D 4x2 test data
//******************************************************************************

// ADIOS2 write using ADIOS1 Writer, native ADIOS1 read
TEST_F(ADIOS1WriteADIOS2ReadTest, _ADIOS2ADIOS1WriteADIOS1Read2D4x2)
{
    // Each process would write a 4x2 array and all processes would
    // form a 2D 4 * (NumberOfProcess * Nx) matrix where Nx is 2 here
    std::string fname = "ADIOS1WriteADIOS2Read2D4x2Test.bp";

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const std::size_t Nx = 2;
    // Number of cols
    const std::size_t Ny = 4;

    // Number of steps
    const std::size_t NSteps = 3;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(true);
#endif
    {
        adios2::IO &io = adios.DeclareIO("TestIO");

        // Declare 2D variables (4 * (NumberOfProcess * Nx))
        // The local process' part (start, count) can be defined now or later
        // before Write().
        {
            adios2::Dims shape{static_cast<unsigned int>(Ny),
                               static_cast<unsigned int>(mpiSize * Nx)};
            adios2::Dims start{static_cast<unsigned int>(0),
                               static_cast<unsigned int>(mpiRank * Nx)};
            adios2::Dims count{static_cast<unsigned int>(Ny),
                               static_cast<unsigned int>(Nx)};
            auto &var_i8 = io.DefineVariable<int8_t>("i8", shape, start, count);
            auto &var_i16 =
                io.DefineVariable<int16_t>("i16", shape, start, count);
            auto &var_i32 =
                io.DefineVariable<int32_t>("i32", shape, start, count);
            auto &var_i64 =
                io.DefineVariable<int64_t>("i64", shape, start, count);
            auto &var_u8 =
                io.DefineVariable<uint8_t>("u8", shape, start, count);
            auto &var_u16 =
                io.DefineVariable<uint16_t>("u16", shape, start, count);
            auto &var_u32 =
                io.DefineVariable<uint32_t>("u32", shape, start, count);
            auto &var_u64 =
                io.DefineVariable<uint64_t>("u64", shape, start, count);
            auto &var_r32 =
                io.DefineVariable<float>("r32", shape, start, count);
            auto &var_r64 =
                io.DefineVariable<double>("r64", shape, start, count);
        }

        // Create the ADIOS 1 Engine
        io.SetEngine("ADIOS1");
        io.AddTransport("file");

        adios2::Engine &engine = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            // Generate test data for each process uniquely
            SmallTestData currentTestData =
                generateNewSmallTestData(m_TestData, step, mpiRank, mpiSize);

            // Retrieve the variables that previously went out of scope
            auto &var_i8 = *io.InquireVariable<int8_t>("i8");
            auto &var_i16 = *io.InquireVariable<int16_t>("i16");
            auto &var_i32 = *io.InquireVariable<int32_t>("i32");
            auto &var_i64 = *io.InquireVariable<int64_t>("i64");
            auto &var_u8 = *io.InquireVariable<uint8_t>("u8");
            auto &var_u16 = *io.InquireVariable<uint16_t>("u16");
            auto &var_u32 = *io.InquireVariable<uint32_t>("u32");
            auto &var_u64 = *io.InquireVariable<uint64_t>("u64");
            auto &var_r32 = *io.InquireVariable<float>("r32");
            auto &var_r64 = *io.InquireVariable<double>("r64");

            // Make a 2D selection to describe the local dimensions of the
            // variable we write and its offsets in the global spaces
            adios2::Box<adios2::Dims> sel(
                {0, static_cast<unsigned int>(mpiRank * Nx)}, {Ny, Nx});
            var_i8.SetSelection(sel);
            var_i16.SetSelection(sel);
            var_i32.SetSelection(sel);
            var_i64.SetSelection(sel);
            var_u8.SetSelection(sel);
            var_u16.SetSelection(sel);
            var_u32.SetSelection(sel);
            var_u64.SetSelection(sel);
            var_r32.SetSelection(sel);
            var_r64.SetSelection(sel);

            // Write each one
            // fill in the variable with values from starting index to
            // starting index + count
            engine.PutSync(var_i8, currentTestData.I8.data());
            engine.PutSync(var_i16, currentTestData.I16.data());
            engine.PutSync(var_i32, currentTestData.I32.data());
            engine.PutSync(var_i64, currentTestData.I64.data());
            engine.PutSync(var_u8, currentTestData.U8.data());
            engine.PutSync(var_u16, currentTestData.U16.data());
            engine.PutSync(var_u32, currentTestData.U32.data());
            engine.PutSync(var_u64, currentTestData.U64.data());
            engine.PutSync(var_r32, currentTestData.R32.data());
            engine.PutSync(var_r64, currentTestData.R64.data());

            // Advance to the next time step
            engine.EndStep();
        }

        // Close the file
        engine.Close();

        adios_finalize(0);
    }
    {
        adios2::IO &io = adios.DeclareIO("ADIOS2Read");
        adios2::Engine &engine = io.Open(fname, adios2::Mode::Read);

        auto var_i8 = io.InquireVariable<int8_t>("i8");
        ASSERT_NE(var_i8, nullptr);
        ASSERT_EQ(var_i8->m_Shape.size(), 2);
        ASSERT_EQ(var_i8->m_Shape[0], Ny);
        ASSERT_EQ(var_i8->m_Shape[1], mpiSize * Nx);
        ASSERT_EQ(var_i8->GetAvailableStepsCount(), NSteps);

        auto var_i16 = io.InquireVariable<int16_t>("i16");
        ASSERT_NE(var_i16, nullptr);
        ASSERT_EQ(var_i16->m_Shape.size(), 2);
        ASSERT_EQ(var_i16->m_Shape[0], Ny);
        ASSERT_EQ(var_i16->m_Shape[1], mpiSize * Nx);
        ASSERT_EQ(var_i16->GetAvailableStepsCount(), NSteps);

        auto var_i32 = io.InquireVariable<int32_t>("i32");
        ASSERT_NE(var_i32, nullptr);
        ASSERT_EQ(var_i32->m_Shape.size(), 2);
        ASSERT_EQ(var_i32->m_Shape[0], Ny);
        ASSERT_EQ(var_i32->m_Shape[1], mpiSize * Nx);
        ASSERT_EQ(var_i32->GetAvailableStepsCount(), NSteps);

        auto var_i64 = io.InquireVariable<int64_t>("i64");
        ASSERT_NE(var_i64, nullptr);
        ASSERT_EQ(var_i64->m_Shape.size(), 2);
        ASSERT_EQ(var_i64->m_Shape[0], Ny);
        ASSERT_EQ(var_i64->m_Shape[1], mpiSize * Nx);
        ASSERT_EQ(var_i64->GetAvailableStepsCount(), NSteps);

        auto var_u8 = io.InquireVariable<uint8_t>("u8");
        ASSERT_NE(var_u8, nullptr);
        ASSERT_EQ(var_u8->m_Shape.size(), 2);
        ASSERT_EQ(var_u8->m_Shape[0], Ny);
        ASSERT_EQ(var_u8->m_Shape[1], mpiSize * Nx);
        ASSERT_EQ(var_u8->GetAvailableStepsCount(), NSteps);

        auto var_u16 = io.InquireVariable<uint16_t>("u16");
        ASSERT_NE(var_u16, nullptr);
        ASSERT_EQ(var_u16->m_Shape.size(), 2);
        ASSERT_EQ(var_u16->m_Shape[0], Ny);
        ASSERT_EQ(var_u16->m_Shape[1], mpiSize * Nx);
        ASSERT_EQ(var_u16->GetAvailableStepsCount(), NSteps);

        auto var_u32 = io.InquireVariable<uint32_t>("u32");
        ASSERT_NE(var_u32, nullptr);
        ASSERT_EQ(var_u32->m_Shape.size(), 2);
        ASSERT_EQ(var_u32->m_Shape[0], Ny);
        ASSERT_EQ(var_u32->m_Shape[1], mpiSize * Nx);
        ASSERT_EQ(var_u32->GetAvailableStepsCount(), NSteps);

        auto var_u64 = io.InquireVariable<uint64_t>("u64");
        ASSERT_NE(var_u64, nullptr);
        ASSERT_EQ(var_u64->m_Shape.size(), 2);
        ASSERT_EQ(var_u64->m_Shape[0], Ny);
        ASSERT_EQ(var_u64->m_Shape[1], mpiSize * Nx);
        ASSERT_EQ(var_u64->GetAvailableStepsCount(), NSteps);

        auto var_r32 = io.InquireVariable<float>("r32");
        ASSERT_NE(var_r32, nullptr);
        ASSERT_EQ(var_r32->m_Shape.size(), 2);
        ASSERT_EQ(var_r32->m_Shape[0], Ny);
        ASSERT_EQ(var_r32->m_Shape[1], mpiSize * Nx);
        ASSERT_EQ(var_r32->GetAvailableStepsCount(), NSteps);

        auto var_r64 = io.InquireVariable<double>("r64");
        ASSERT_NE(var_r64, nullptr);
        ASSERT_EQ(var_r64->m_Shape.size(), 2);
        ASSERT_EQ(var_r64->m_Shape[0], Ny);
        ASSERT_EQ(var_r64->m_Shape[1], mpiSize * Nx);
        ASSERT_EQ(var_r64->GetAvailableStepsCount(), NSteps);

        adios2::Box<adios2::Dims> selection({{0, mpiRank * Nx}, {Ny, Nx}});

        var_i8->SetSelection(selection);
        var_i16->SetSelection(selection);
        var_i32->SetSelection(selection);
        var_i64->SetSelection(selection);
        var_u8->SetSelection(selection);
        var_u16->SetSelection(selection);
        var_u32->SetSelection(selection);
        var_u64->SetSelection(selection);
        var_r32->SetSelection(selection);
        var_r64->SetSelection(selection);

        std::array<int8_t, Nx * Ny> I8;
        std::array<int16_t, Nx * Ny> I16;
        std::array<int32_t, Nx * Ny> I32;
        std::array<int64_t, Nx * Ny> I64;
        std::array<uint8_t, Nx * Ny> U8;
        std::array<uint16_t, Nx * Ny> U16;
        std::array<uint32_t, Nx * Ny> U32;
        std::array<uint64_t, Nx * Ny> U64;
        std::array<float, Nx * Ny> R32;
        std::array<double, Nx * Ny> R64;

        // Read stuff
        for (size_t t = 0; t < NSteps; ++t)
        {
            // Generate test data for each rank uniquely
            SmallTestData currentTestData =
                generateNewSmallTestData(m_TestData, t, mpiRank, mpiSize);
            // Read the current step
            engine.BeginStep();
            engine.GetDeferred(*var_i8, I8.data());
            engine.GetDeferred(*var_i16, I16.data());
            engine.GetDeferred(*var_i32, I32.data());
            engine.GetDeferred(*var_i64, I64.data());

            engine.GetDeferred(*var_u8, U8.data());
            engine.GetDeferred(*var_u16, U16.data());
            engine.GetDeferred(*var_u32, U32.data());
            engine.GetDeferred(*var_u64, U64.data());

            engine.GetDeferred(*var_r32, R32.data());
            engine.GetDeferred(*var_r64, R64.data());

            engine.PerformGets();

            engine.EndStep();

            // Check if it's correct
            for (size_t i = 0; i < Nx * Ny; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i << " rank=" << mpiRank;
                std::string msg = ss.str();

                EXPECT_EQ(I8[i], currentTestData.I8[i]) << msg;
                EXPECT_EQ(I16[i], currentTestData.I16[i]) << msg;
                EXPECT_EQ(I32[i], currentTestData.I32[i]) << msg;
                EXPECT_EQ(I64[i], currentTestData.I64[i]) << msg;
                EXPECT_EQ(U8[i], currentTestData.U8[i]) << msg;
                EXPECT_EQ(U16[i], currentTestData.U16[i]) << msg;
                EXPECT_EQ(U32[i], currentTestData.U32[i]) << msg;
                EXPECT_EQ(U64[i], currentTestData.U64[i]) << msg;
                EXPECT_EQ(R32[i], currentTestData.R32[i]) << msg;
                EXPECT_EQ(R64[i], currentTestData.R64[i]) << msg;
            }
        }
        engine.Close();
    }
}

//******************************************************************************
// main
//******************************************************************************

int main(int argc, char **argv)
{
#ifdef ADIOS2_HAVE_MPI
    MPI_Init(nullptr, nullptr);
    adios_init_noxml(MPI_COMM_WORLD);
#endif

    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return result;
}
