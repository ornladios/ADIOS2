/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include <cstdint>
#include <cstring>

#include <iostream>
#include <numeric> //std::iota
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#include "../SmallTestData.h"

std::string engineName; // comes from command line

class BPWriteReadTestADIOS2 : public ::testing::Test
{
public:
    BPWriteReadTestADIOS2() = default;

    SmallTestData m_TestData;
};

//******************************************************************************
// 1D 1x8 test data
//******************************************************************************

// ADIOS2 BP write, native ADIOS1 read
TEST_F(BPWriteReadTestADIOS2, ADIOS2BPWriteRead1D8)
{
    // Each process would write a 1x8 array and all processes would
    // form a mpiSize * Nx 1D array
    const std::string fname("ADIOS2BPWriteRead1D8.bp");

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const size_t Nx = 8;

    // Number of steps
    const size_t NSteps = 3;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

    // Write test data using BP

#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(true);
#endif
    {
        adios2::IO io = adios.DeclareIO("TestIO");

        // Test setting parameters
        {
            io.SetParameter("ProfileUnits", "Microseconds");
            io.SetParameters("Threads=2, CollectiveMetadata = OFF");
            adios2::Params parameters = io.Parameters();

            auto ProfileUnits = parameters.find("ProfileUnits");
            EXPECT_NE(ProfileUnits, parameters.end());
            EXPECT_EQ(ProfileUnits->second, "Microseconds");

            auto Threads = parameters.find("Threads");
            EXPECT_NE(Threads, parameters.end());
            EXPECT_EQ(Threads->second, "2");

            auto CollectiveMetadata = parameters.find("CollectiveMetadata");
            EXPECT_NE(CollectiveMetadata, parameters.end());
            EXPECT_EQ(CollectiveMetadata->second, "OFF");

            io.ClearParameters();

            // should not find parameters anymore
            parameters = io.Parameters();
            CollectiveMetadata = parameters.find("CollectiveMetadata");
            EXPECT_EQ(CollectiveMetadata, parameters.end());
        }

        // Declare 1D variables (NumOfProcesses * Nx)
        // The local process' part (start, count) can be defined now or later
        // before Write().
        {
            const adios2::Dims shape{static_cast<size_t>(Nx * mpiSize)};
            const adios2::Dims start{static_cast<size_t>(Nx * mpiRank)};
            const adios2::Dims count{Nx};

            auto var_iString = io.DefineVariable<std::string>("iString");
            auto var_i8 = io.DefineVariable<int8_t>("i8", shape, start, count);
            auto var_i16 =
                io.DefineVariable<int16_t>("i16", shape, start, count);
            auto var_i32 =
                io.DefineVariable<int32_t>("i32", shape, start, count);
            auto var_i64 =
                io.DefineVariable<int64_t>("i64", shape, start, count);
            auto var_u8 = io.DefineVariable<uint8_t>("u8", shape, start, count);
            auto var_u16 =
                io.DefineVariable<uint16_t>("u16", shape, start, count);
            auto var_u32 =
                io.DefineVariable<uint32_t>("u32", shape, start, count);
            auto var_u64 =
                io.DefineVariable<uint64_t>("u64", shape, start, count);
            auto var_r32 = io.DefineVariable<float>("r32", shape, start, count);
            auto var_r64 =
                io.DefineVariable<double>("r64", shape, start, count);
        }

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            // Create the BP Engine
            io.SetEngine("BPFile");
        }

        io.AddTransport("file");

        // QUESTION: It seems that BPFilterWriter cannot overwrite existing
        // files
        // Ex. if you tune Nx and NSteps, the test would fail. But if you clear
        // the cache in
        // ${adios2Build}/testing/adios2/engine/bp/ADIOS2BPWriteADIOS1Read1D8.bp.dir,
        // then it works
        adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            // Generate test data for each process uniquely
            SmallTestData currentTestData = generateNewSmallTestData(
                m_TestData, static_cast<int>(step), mpiRank, mpiSize);

            // Retrieve the variables that previously went out of scope
            auto var_iString = io.InquireVariable<std::string>("iString");
            auto var_i8 = io.InquireVariable<int8_t>("i8");
            auto var_i16 = io.InquireVariable<int16_t>("i16");
            auto var_i32 = io.InquireVariable<int32_t>("i32");
            auto var_i64 = io.InquireVariable<int64_t>("i64");
            auto var_u8 = io.InquireVariable<uint8_t>("u8");
            auto var_u16 = io.InquireVariable<uint16_t>("u16");
            auto var_u32 = io.InquireVariable<uint32_t>("u32");
            auto var_u64 = io.InquireVariable<uint64_t>("u64");
            auto var_r32 = io.InquireVariable<float>("r32");
            auto var_r64 = io.InquireVariable<double>("r64");

            // Make a 1D selection to describe the local dimensions of the
            // variable we write and its offsets in the global spaces
            adios2::Box<adios2::Dims> sel({mpiRank * Nx}, {Nx});

            EXPECT_THROW(var_iString.SetSelection(sel), std::invalid_argument);
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
            bpWriter.BeginStep();

            bpWriter.Put(var_iString, currentTestData.S1);
            bpWriter.Put(var_i8, currentTestData.I8.data());
            bpWriter.Put(var_i16, currentTestData.I16.data());
            bpWriter.Put(var_i32, currentTestData.I32.data());
            bpWriter.Put(var_i64, currentTestData.I64.data());
            bpWriter.Put(var_u8, currentTestData.U8.data());
            bpWriter.Put(var_u16, currentTestData.U16.data());
            bpWriter.Put(var_u32, currentTestData.U32.data());
            bpWriter.Put(var_u64, currentTestData.U64.data());
            bpWriter.Put(var_r32, currentTestData.R32.data());
            bpWriter.Put(var_r64, currentTestData.R64.data());
            bpWriter.PerformPuts();

            bpWriter.EndStep();
        }

        // Close the file
        bpWriter.Close();
    }

    {
        adios2::IO io = adios.DeclareIO("ReadIO");

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }

        adios2::Engine bpReader = io.Open(fname, adios2::Mode::Read);

        EXPECT_EQ(bpReader.Steps(), NSteps);

        auto var_iString = io.InquireVariable<std::string>("iString");
        EXPECT_TRUE(var_iString);
        ASSERT_EQ(var_iString.Shape().size(), 0);
        ASSERT_EQ(var_iString.Steps(), NSteps);

        auto var_i8 = io.InquireVariable<int8_t>("i8");
        EXPECT_TRUE(var_i8);
        ASSERT_EQ(var_i8.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i8.Steps(), NSteps);
        ASSERT_EQ(var_i8.Shape()[0], mpiSize * Nx);

        auto var_i16 = io.InquireVariable<int16_t>("i16");
        EXPECT_TRUE(var_i16);
        ASSERT_EQ(var_i16.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i16.Steps(), NSteps);
        ASSERT_EQ(var_i16.Shape()[0], mpiSize * Nx);

        auto var_i32 = io.InquireVariable<int32_t>("i32");
        EXPECT_TRUE(var_i32);
        ASSERT_EQ(var_i32.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i32.Steps(), NSteps);
        ASSERT_EQ(var_i32.Shape()[0], mpiSize * Nx);

        auto var_i64 = io.InquireVariable<int64_t>("i64");
        EXPECT_TRUE(var_i64);
        ASSERT_EQ(var_i64.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i64.Steps(), NSteps);
        ASSERT_EQ(var_i64.Shape()[0], mpiSize * Nx);

        auto var_u8 = io.InquireVariable<uint8_t>("u8");
        EXPECT_TRUE(var_u8);
        ASSERT_EQ(var_u8.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u8.Steps(), NSteps);
        ASSERT_EQ(var_u8.Shape()[0], mpiSize * Nx);

        auto var_u16 = io.InquireVariable<uint16_t>("u16");
        EXPECT_TRUE(var_u16);
        ASSERT_EQ(var_u16.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u16.Steps(), NSteps);
        ASSERT_EQ(var_u16.Shape()[0], mpiSize * Nx);

        auto var_u32 = io.InquireVariable<uint32_t>("u32");
        EXPECT_TRUE(var_u32);
        ASSERT_EQ(var_u32.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u32.Steps(), NSteps);
        ASSERT_EQ(var_u32.Shape()[0], mpiSize * Nx);

        auto var_u64 = io.InquireVariable<uint64_t>("u64");
        EXPECT_TRUE(var_u64);
        ASSERT_EQ(var_u64.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u64.Steps(), NSteps);
        ASSERT_EQ(var_u64.Shape()[0], mpiSize * Nx);

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

        // TODO: other types

        SmallTestData testData;

        std::string IString;
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

        const adios2::Dims start{mpiRank * Nx};
        const adios2::Dims count{Nx};

        const adios2::Box<adios2::Dims> sel(start, count);

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

        for (size_t t = 0; t < NSteps; ++t)
        {
            var_i8.SetStepSelection({t, 1});
            var_i16.SetStepSelection({t, 1});
            var_i32.SetStepSelection({t, 1});
            var_i64.SetStepSelection({t, 1});

            var_u8.SetStepSelection({t, 1});
            var_u16.SetStepSelection({t, 1});
            var_u32.SetStepSelection({t, 1});
            var_u64.SetStepSelection({t, 1});

            var_r32.SetStepSelection({t, 1});
            var_r64.SetStepSelection({t, 1});

            // Generate test data for each rank uniquely
            SmallTestData currentTestData = generateNewSmallTestData(
                m_TestData, static_cast<int>(t), mpiRank, mpiSize);

            bpReader.Get(var_iString, IString);

            bpReader.Get(var_i8, I8.data());
            bpReader.Get(var_i16, I16.data());
            bpReader.Get(var_i32, I32.data());
            bpReader.Get(var_i64, I64.data());

            bpReader.Get(var_u8, U8.data());
            bpReader.Get(var_u16, U16.data());
            bpReader.Get(var_u32, U32.data());
            bpReader.Get(var_u64, U64.data());

            bpReader.Get(var_r32, R32.data());
            bpReader.Get(var_r64, R64.data());

            bpReader.PerformGets();

            EXPECT_EQ(IString, currentTestData.S1);

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
        bpReader.Close();
    }
}

//******************************************************************************
// 2D 2x4 test data
//******************************************************************************

// ADIOS2 BP write, native ADIOS1 read
TEST_F(BPWriteReadTestADIOS2, ADIOS2BPWriteRead2D2x4)
{
    // Each process would write a 2x4 array and all processes would
    // form a 2D 2 * (numberOfProcess*Nx) matrix where Nx is 4 here
    const std::string fname("ADIOS2BPWriteRead2D2x4Test.bp");

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
        adios2::IO io = adios.DeclareIO("TestIO");

        // Declare 2D variables (Ny * (NumOfProcesses * Nx))
        // The local process' part (start, count) can be defined now or later
        // before Write().
        {
            const adios2::Dims shape{Ny, static_cast<size_t>(Nx * mpiSize)};
            const adios2::Dims start{0, static_cast<size_t>(mpiRank * Nx)};
            const adios2::Dims count{Ny, Nx};

            auto var_iString = io.DefineVariable<std::string>("iString");
            auto var_i8 = io.DefineVariable<int8_t>("i8", shape, start, count);
            auto var_i16 =
                io.DefineVariable<int16_t>("i16", shape, start, count);
            auto var_i32 =
                io.DefineVariable<int32_t>("i32", shape, start, count);
            auto var_i64 =
                io.DefineVariable<int64_t>("i64", shape, start, count);
            auto var_u8 = io.DefineVariable<uint8_t>("u8", shape, start, count);
            auto var_u16 =
                io.DefineVariable<uint16_t>("u16", shape, start, count);
            auto var_u32 =
                io.DefineVariable<uint32_t>("u32", shape, start, count);
            auto var_u64 =
                io.DefineVariable<uint64_t>("u64", shape, start, count);
            auto var_r32 = io.DefineVariable<float>("r32", shape, start, count);
            auto var_r64 =
                io.DefineVariable<double>("r64", shape, start, count);
        }

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            // Create the BP Engine
            io.SetEngine("BPFile");
        }
        io.AddTransport("file");

        adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            // Generate test data for each process uniquely
            SmallTestData currentTestData = generateNewSmallTestData(
                m_TestData, static_cast<int>(step), mpiRank, mpiSize);

            // Retrieve the variables that previously went out of scope
            auto var_iString = io.InquireVariable<std::string>("iString");
            auto var_i8 = io.InquireVariable<int8_t>("i8");
            auto var_i16 = io.InquireVariable<int16_t>("i16");
            auto var_i32 = io.InquireVariable<int32_t>("i32");
            auto var_i64 = io.InquireVariable<int64_t>("i64");
            auto var_u8 = io.InquireVariable<uint8_t>("u8");
            auto var_u16 = io.InquireVariable<uint16_t>("u16");
            auto var_u32 = io.InquireVariable<uint32_t>("u32");
            auto var_u64 = io.InquireVariable<uint64_t>("u64");
            auto var_r32 = io.InquireVariable<float>("r32");
            auto var_r64 = io.InquireVariable<double>("r64");

            // Make a 2D selection to describe the local dimensions of the
            // variable we write and its offsets in the global spaces
            adios2::Box<adios2::Dims> sel(
                {0, static_cast<size_t>(mpiRank * Nx)}, {Ny, Nx});
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
            bpWriter.BeginStep();
            bpWriter.Put(var_iString, currentTestData.S1);
            bpWriter.Put(var_i8, currentTestData.I8.data());
            bpWriter.Put(var_i16, currentTestData.I16.data());
            bpWriter.Put(var_i32, currentTestData.I32.data());
            bpWriter.Put(var_i64, currentTestData.I64.data());
            bpWriter.Put(var_u8, currentTestData.U8.data());
            bpWriter.Put(var_u16, currentTestData.U16.data());
            bpWriter.Put(var_u32, currentTestData.U32.data());
            bpWriter.Put(var_u64, currentTestData.U64.data());
            bpWriter.Put(var_r32, currentTestData.R32.data());
            bpWriter.Put(var_r64, currentTestData.R64.data());
            bpWriter.PerformPuts();

            bpWriter.EndStep();
        }

        // Close the file
        bpWriter.Close();
    }

    {
        adios2::IO io = adios.DeclareIO("ReadIO");

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }

        adios2::Engine bpReader = io.Open(fname, adios2::Mode::Read);

        EXPECT_EQ(bpReader.Steps(), NSteps);
        auto var_iString = io.InquireVariable<std::string>("iString");
        EXPECT_TRUE(var_iString);
        ASSERT_EQ(var_iString.Shape().size(), 0);
        ASSERT_EQ(var_iString.Steps(), NSteps);

        auto var_i8 = io.InquireVariable<int8_t>("i8");
        EXPECT_TRUE(var_i8);
        ASSERT_EQ(var_i8.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i8.Steps(), NSteps);
        ASSERT_EQ(var_i8.Shape()[0], Ny);
        ASSERT_EQ(var_i8.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        auto var_i16 = io.InquireVariable<int16_t>("i16");
        EXPECT_TRUE(var_i16);
        ASSERT_EQ(var_i16.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i16.Steps(), NSteps);
        ASSERT_EQ(var_i16.Shape()[0], Ny);
        ASSERT_EQ(var_i16.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        auto var_i32 = io.InquireVariable<int32_t>("i32");
        EXPECT_TRUE(var_i32);
        ASSERT_EQ(var_i32.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i32.Steps(), NSteps);
        ASSERT_EQ(var_i32.Shape()[0], Ny);
        ASSERT_EQ(var_i32.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        auto var_i64 = io.InquireVariable<int64_t>("i64");
        EXPECT_TRUE(var_i64);
        ASSERT_EQ(var_i64.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i64.Steps(), NSteps);
        ASSERT_EQ(var_i64.Shape()[0], Ny);
        ASSERT_EQ(var_i64.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        auto var_u8 = io.InquireVariable<uint8_t>("u8");
        EXPECT_TRUE(var_u8);
        ASSERT_EQ(var_u8.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u8.Steps(), NSteps);
        ASSERT_EQ(var_u8.Shape()[0], Ny);
        ASSERT_EQ(var_u8.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        auto var_u16 = io.InquireVariable<uint16_t>("u16");
        EXPECT_TRUE(var_u16);
        ASSERT_EQ(var_u16.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u16.Steps(), NSteps);
        ASSERT_EQ(var_u16.Shape()[0], Ny);
        ASSERT_EQ(var_u16.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        auto var_u32 = io.InquireVariable<uint32_t>("u32");
        EXPECT_TRUE(var_u32);
        ASSERT_EQ(var_u32.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u32.Steps(), NSteps);
        ASSERT_EQ(var_u32.Shape()[0], Ny);
        ASSERT_EQ(var_u32.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        auto var_u64 = io.InquireVariable<uint64_t>("u64");
        EXPECT_TRUE(var_u64);
        ASSERT_EQ(var_u64.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u64.Steps(), NSteps);
        ASSERT_EQ(var_u64.Shape()[0], Ny);
        ASSERT_EQ(var_u64.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        auto var_r32 = io.InquireVariable<float>("r32");
        EXPECT_TRUE(var_r32);
        ASSERT_EQ(var_r32.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r32.Steps(), NSteps);
        ASSERT_EQ(var_r32.Shape()[0], Ny);
        ASSERT_EQ(var_r32.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        auto var_r64 = io.InquireVariable<double>("r64");
        EXPECT_TRUE(var_r64);
        ASSERT_EQ(var_r64.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r64.Steps(), NSteps);
        ASSERT_EQ(var_r64.Shape()[0], Ny);
        ASSERT_EQ(var_r64.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        std::string IString;
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

        const adios2::Dims start{0, static_cast<size_t>(mpiRank * Nx)};
        const adios2::Dims count{Ny, Nx};

        const adios2::Box<adios2::Dims> sel(start, count);

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

        for (size_t t = 0; t < NSteps; ++t)
        {
            var_i8.SetStepSelection({t, 1});
            var_i16.SetStepSelection({t, 1});
            var_i32.SetStepSelection({t, 1});
            var_i64.SetStepSelection({t, 1});

            var_u8.SetStepSelection({t, 1});
            var_u16.SetStepSelection({t, 1});
            var_u32.SetStepSelection({t, 1});
            var_u64.SetStepSelection({t, 1});

            var_r32.SetStepSelection({t, 1});
            var_r64.SetStepSelection({t, 1});

            bpReader.Get(var_iString, IString);

            bpReader.Get(var_i8, I8.data());
            bpReader.Get(var_i16, I16.data());
            bpReader.Get(var_i32, I32.data());
            bpReader.Get(var_i64, I64.data());

            bpReader.Get(var_u8, U8.data());
            bpReader.Get(var_u16, U16.data());
            bpReader.Get(var_u32, U32.data());
            bpReader.Get(var_u64, U64.data());

            bpReader.Get(var_r32, R32.data());
            bpReader.Get(var_r64, R64.data());

            bpReader.PerformGets();

            // Generate test data for each rank uniquely
            SmallTestData currentTestData = generateNewSmallTestData(
                m_TestData, static_cast<int>(t), mpiRank, mpiSize);

            EXPECT_EQ(IString, currentTestData.S1);

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
        bpReader.Close();
    }
}

//******************************************************************************
// 2D 4x2 test data
//******************************************************************************

TEST_F(BPWriteReadTestADIOS2, ADIOS2BPWriteRead2D4x2)
{
    // Each process would write a 4x2 array and all processes would
    // form a 2D 4 * (NumberOfProcess * Nx) matrix where Nx is 2 here
    const std::string fname("ADIOS2BPWriteRead2D4x2Test.bp");

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

    // Write test data using ADIOS2

#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(true);
#endif
    {
        adios2::IO io = adios.DeclareIO("TestIO");

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
            auto var_i8 = io.DefineVariable<int8_t>("i8", shape, start, count);
            auto var_i16 =
                io.DefineVariable<int16_t>("i16", shape, start, count);
            auto var_i32 =
                io.DefineVariable<int32_t>("i32", shape, start, count);
            auto var_i64 =
                io.DefineVariable<int64_t>("i64", shape, start, count);
            auto var_u8 = io.DefineVariable<uint8_t>("u8", shape, start, count);
            auto var_u16 =
                io.DefineVariable<uint16_t>("u16", shape, start, count);
            auto var_u32 =
                io.DefineVariable<uint32_t>("u32", shape, start, count);
            auto var_u64 =
                io.DefineVariable<uint64_t>("u64", shape, start, count);
            auto var_r32 = io.DefineVariable<float>("r32", shape, start, count);
            auto var_r64 =
                io.DefineVariable<double>("r64", shape, start, count);
        }

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            // Create the BP Engine
            io.SetEngine("BPFile");
        }

        io.AddTransport("file");

        adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            // Generate test data for each process uniquely
            SmallTestData currentTestData = generateNewSmallTestData(
                m_TestData, static_cast<int>(step), mpiRank, mpiSize);

            // Retrieve the variables that previously went out of scope
            auto var_i8 = io.InquireVariable<int8_t>("i8");
            auto var_i16 = io.InquireVariable<int16_t>("i16");
            auto var_i32 = io.InquireVariable<int32_t>("i32");
            auto var_i64 = io.InquireVariable<int64_t>("i64");
            auto var_u8 = io.InquireVariable<uint8_t>("u8");
            auto var_u16 = io.InquireVariable<uint16_t>("u16");
            auto var_u32 = io.InquireVariable<uint32_t>("u32");
            auto var_u64 = io.InquireVariable<uint64_t>("u64");
            auto var_r32 = io.InquireVariable<float>("r32");
            auto var_r64 = io.InquireVariable<double>("r64");

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
            bpWriter.BeginStep();
            bpWriter.Put(var_i8, currentTestData.I8.data());
            bpWriter.Put(var_i16, currentTestData.I16.data());
            bpWriter.Put(var_i32, currentTestData.I32.data());
            bpWriter.Put(var_i64, currentTestData.I64.data());
            bpWriter.Put(var_u8, currentTestData.U8.data());
            bpWriter.Put(var_u16, currentTestData.U16.data());
            bpWriter.Put(var_u32, currentTestData.U32.data());
            bpWriter.Put(var_u64, currentTestData.U64.data());
            bpWriter.Put(var_r32, currentTestData.R32.data());
            bpWriter.Put(var_r64, currentTestData.R64.data());
            bpWriter.EndStep();
        }

        // Close the file
        bpWriter.Close();
    }

    {
        adios2::IO io = adios.DeclareIO("ReadIO");

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }

        adios2::Engine bpReader = io.Open(fname, adios2::Mode::Read);

        EXPECT_EQ(bpReader.Steps(), NSteps);

        auto var_i8 = io.InquireVariable<int8_t>("i8");
        EXPECT_TRUE(var_i8);
        ASSERT_EQ(var_i8.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i8.Steps(), NSteps);
        ASSERT_EQ(var_i8.Shape()[0], Ny);
        ASSERT_EQ(var_i8.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        auto var_i16 = io.InquireVariable<int16_t>("i16");
        EXPECT_TRUE(var_i16);
        ASSERT_EQ(var_i16.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i16.Steps(), NSteps);
        ASSERT_EQ(var_i16.Shape()[0], Ny);
        ASSERT_EQ(var_i16.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        auto var_i32 = io.InquireVariable<int32_t>("i32");
        EXPECT_TRUE(var_i32);
        ASSERT_EQ(var_i32.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i32.Steps(), NSteps);
        ASSERT_EQ(var_i32.Shape()[0], Ny);
        ASSERT_EQ(var_i32.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        auto var_i64 = io.InquireVariable<int64_t>("i64");
        EXPECT_TRUE(var_i64);
        ASSERT_EQ(var_i64.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i64.Steps(), NSteps);
        ASSERT_EQ(var_i64.Shape()[0], Ny);
        ASSERT_EQ(var_i64.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        auto var_u8 = io.InquireVariable<uint8_t>("u8");
        EXPECT_TRUE(var_u8);
        ASSERT_EQ(var_u8.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u8.Steps(), NSteps);
        ASSERT_EQ(var_u8.Shape()[0], Ny);
        ASSERT_EQ(var_u8.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        auto var_u16 = io.InquireVariable<uint16_t>("u16");
        EXPECT_TRUE(var_u16);
        ASSERT_EQ(var_u16.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u16.Steps(), NSteps);
        ASSERT_EQ(var_u16.Shape()[0], Ny);
        ASSERT_EQ(var_u16.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        auto var_u32 = io.InquireVariable<uint32_t>("u32");
        EXPECT_TRUE(var_u32);
        ASSERT_EQ(var_u32.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u32.Steps(), NSteps);
        ASSERT_EQ(var_u32.Shape()[0], Ny);
        ASSERT_EQ(var_u32.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        auto var_u64 = io.InquireVariable<uint64_t>("u64");
        EXPECT_TRUE(var_u64);
        ASSERT_EQ(var_u64.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u64.Steps(), NSteps);
        ASSERT_EQ(var_u64.Shape()[0], Ny);
        ASSERT_EQ(var_u64.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        auto var_r32 = io.InquireVariable<float>("r32");
        EXPECT_TRUE(var_r32);
        ASSERT_EQ(var_r32.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r32.Steps(), NSteps);
        ASSERT_EQ(var_r32.Shape()[0], Ny);
        ASSERT_EQ(var_r32.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        auto var_r64 = io.InquireVariable<double>("r64");
        EXPECT_TRUE(var_r64);
        ASSERT_EQ(var_r64.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r64.Steps(), NSteps);
        ASSERT_EQ(var_r64.Shape()[0], Ny);
        ASSERT_EQ(var_r64.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        // If the size of the array is smaller than the data
        // the result is weird... double and uint64_t would get
        // completely garbage data
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

        const adios2::Dims start{0, static_cast<size_t>(mpiRank * Nx)};
        const adios2::Dims count{Ny, Nx};

        const adios2::Box<adios2::Dims> sel(start, count);

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

        for (size_t t = 0; t < NSteps; ++t)
        {
            var_i8.SetStepSelection({t, 1});
            var_i16.SetStepSelection({t, 1});
            var_i32.SetStepSelection({t, 1});
            var_i64.SetStepSelection({t, 1});

            var_u8.SetStepSelection({t, 1});
            var_u16.SetStepSelection({t, 1});
            var_u32.SetStepSelection({t, 1});
            var_u64.SetStepSelection({t, 1});

            var_r32.SetStepSelection({t, 1});
            var_r64.SetStepSelection({t, 1});

            bpReader.Get(var_i8, I8.data());
            bpReader.Get(var_i16, I16.data());
            bpReader.Get(var_i32, I32.data());
            bpReader.Get(var_i64, I64.data());

            bpReader.Get(var_u8, U8.data());
            bpReader.Get(var_u16, U16.data());
            bpReader.Get(var_u32, U32.data());
            bpReader.Get(var_u64, U64.data());

            bpReader.Get(var_r32, R32.data());
            bpReader.Get(var_r64, R64.data());

            bpReader.PerformGets();

            // Generate test data for each rank uniquely
            SmallTestData currentTestData = generateNewSmallTestData(
                m_TestData, static_cast<int>(t), mpiRank, mpiSize);

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
        bpReader.Close();
    }
}

TEST_F(BPWriteReadTestADIOS2, ADIOS2BPWriteRead2D4x2_ReadMultiSteps)
{
    // Each process would write a 4x2 array and all processes would
    // form a 2D 4 * (NumberOfProcess * Nx) matrix where Nx is 2 here
    const std::string fname("ADIOS2BPWriteRead2D4x2Test_ReadMultiSteps.bp");

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const std::size_t Nx = 2;
    // Number of cols
    const std::size_t Ny = 4;

    // Number of steps
    const std::size_t NSteps = 5;
    const std::size_t tInitial = 2;

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
        adios2::IO io = adios.DeclareIO("TestIO");

        // Declare 2D variables (4 * (NumberOfProcess * Nx))
        // The local process' part (start, count) can be defined now or later
        // before Write().
        {
            adios2::Dims shape{Ny, static_cast<size_t>(mpiSize * Nx)};
            adios2::Dims start{0, static_cast<size_t>(mpiRank * Nx)};
            adios2::Dims count{Ny, Nx};
            auto var_i8 = io.DefineVariable<int8_t>("i8", shape, start, count);
            auto var_i16 =
                io.DefineVariable<int16_t>("i16", shape, start, count);
            auto var_i32 =
                io.DefineVariable<int32_t>("i32", shape, start, count);
            auto var_i64 =
                io.DefineVariable<int64_t>("i64", shape, start, count);
            auto var_u8 = io.DefineVariable<uint8_t>("u8", shape, start, count);
            auto var_u16 =
                io.DefineVariable<uint16_t>("u16", shape, start, count);
            auto var_u32 =
                io.DefineVariable<uint32_t>("u32", shape, start, count);
            auto var_u64 =
                io.DefineVariable<uint64_t>("u64", shape, start, count);
            auto var_r32 = io.DefineVariable<float>("r32", shape, start, count);
            auto var_r64 =
                io.DefineVariable<double>("r64", shape, start, count);
        }

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            // Create the BP Engine
            io.SetEngine("BPFile");
        }

        io.AddTransport("file");

        adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            // Generate test data for each process uniquely
            SmallTestData currentTestData = generateNewSmallTestData(
                m_TestData, static_cast<int>(step), mpiRank, mpiSize);

            // Retrieve the variables that previously went out of scope
            auto var_i8 = io.InquireVariable<int8_t>("i8");
            auto var_i16 = io.InquireVariable<int16_t>("i16");
            auto var_i32 = io.InquireVariable<int32_t>("i32");
            auto var_i64 = io.InquireVariable<int64_t>("i64");
            auto var_u8 = io.InquireVariable<uint8_t>("u8");
            auto var_u16 = io.InquireVariable<uint16_t>("u16");
            auto var_u32 = io.InquireVariable<uint32_t>("u32");
            auto var_u64 = io.InquireVariable<uint64_t>("u64");
            auto var_r32 = io.InquireVariable<float>("r32");
            auto var_r64 = io.InquireVariable<double>("r64");

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
            bpWriter.BeginStep();
            bpWriter.Put(var_i8, currentTestData.I8.data());
            bpWriter.Put(var_i16, currentTestData.I16.data());
            bpWriter.Put(var_i32, currentTestData.I32.data());
            bpWriter.Put(var_i64, currentTestData.I64.data());
            bpWriter.Put(var_u8, currentTestData.U8.data());
            bpWriter.Put(var_u16, currentTestData.U16.data());
            bpWriter.Put(var_u32, currentTestData.U32.data());
            bpWriter.Put(var_u64, currentTestData.U64.data());
            bpWriter.Put(var_r32, currentTestData.R32.data());
            bpWriter.Put(var_r64, currentTestData.R64.data());
            bpWriter.EndStep();
        }

        // Close the file
        bpWriter.Close();
    }

    {
        adios2::IO io = adios.DeclareIO("ReadIO");

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }

        adios2::Engine bpReader = io.Open(fname, adios2::Mode::Read);

        EXPECT_EQ(bpReader.Steps(), NSteps);

        auto var_i8 = io.InquireVariable<int8_t>("i8");
        EXPECT_TRUE(var_i8);
        ASSERT_EQ(var_i8.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i8.Steps(), NSteps);
        ASSERT_EQ(var_i8.Shape()[0], Ny);
        ASSERT_EQ(var_i8.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        auto var_i16 = io.InquireVariable<int16_t>("i16");
        EXPECT_TRUE(var_i16);
        ASSERT_EQ(var_i16.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i16.Steps(), NSteps);
        ASSERT_EQ(var_i16.Shape()[0], Ny);
        ASSERT_EQ(var_i16.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        auto var_i32 = io.InquireVariable<int32_t>("i32");
        EXPECT_TRUE(var_i32);
        ASSERT_EQ(var_i32.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i32.Steps(), NSteps);
        ASSERT_EQ(var_i32.Shape()[0], Ny);
        ASSERT_EQ(var_i32.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        auto var_i64 = io.InquireVariable<int64_t>("i64");
        EXPECT_TRUE(var_i64);
        ASSERT_EQ(var_i64.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i64.Steps(), NSteps);
        ASSERT_EQ(var_i64.Shape()[0], Ny);
        ASSERT_EQ(var_i64.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        auto var_u8 = io.InquireVariable<uint8_t>("u8");
        EXPECT_TRUE(var_u8);
        ASSERT_EQ(var_u8.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u8.Steps(), NSteps);
        ASSERT_EQ(var_u8.Shape()[0], Ny);
        ASSERT_EQ(var_u8.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        auto var_u16 = io.InquireVariable<uint16_t>("u16");
        EXPECT_TRUE(var_u16);
        ASSERT_EQ(var_u16.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u16.Steps(), NSteps);
        ASSERT_EQ(var_u16.Shape()[0], Ny);
        ASSERT_EQ(var_u16.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        auto var_u32 = io.InquireVariable<uint32_t>("u32");
        EXPECT_TRUE(var_u32);
        ASSERT_EQ(var_u32.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u32.Steps(), NSteps);
        ASSERT_EQ(var_u32.Shape()[0], Ny);
        ASSERT_EQ(var_u32.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        auto var_u64 = io.InquireVariable<uint64_t>("u64");
        EXPECT_TRUE(var_u64);
        ASSERT_EQ(var_u64.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u64.Steps(), NSteps);
        ASSERT_EQ(var_u64.Shape()[0], Ny);
        ASSERT_EQ(var_u64.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        auto var_r32 = io.InquireVariable<float>("r32");
        EXPECT_TRUE(var_r32);
        ASSERT_EQ(var_r32.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r32.Steps(), NSteps);
        ASSERT_EQ(var_r32.Shape()[0], Ny);
        ASSERT_EQ(var_r32.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        auto var_r64 = io.InquireVariable<double>("r64");
        EXPECT_TRUE(var_r64);
        ASSERT_EQ(var_r64.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r64.Steps(), NSteps);
        ASSERT_EQ(var_r64.Shape()[0], Ny);
        ASSERT_EQ(var_r64.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        // If the size of the array is smaller than the data
        // the result is weird... double and uint64_t would get
        // completely garbage data
        std::array<int8_t, NSteps * Nx * Ny> I8;
        std::array<int16_t, NSteps * Nx * Ny> I16;
        std::array<int32_t, NSteps * Nx * Ny> I32;
        std::array<int64_t, NSteps * Nx * Ny> I64;
        std::array<uint8_t, NSteps * Nx * Ny> U8;
        std::array<uint16_t, NSteps * Nx * Ny> U16;
        std::array<uint32_t, NSteps * Nx * Ny> U32;
        std::array<uint64_t, NSteps * Nx * Ny> U64;
        std::array<float, NSteps * Nx * Ny> R32;
        std::array<double, NSteps * Nx * Ny> R64;

        const adios2::Dims start{0, static_cast<size_t>(mpiRank * Nx)};
        const adios2::Dims count{Ny, Nx};

        const adios2::Box<adios2::Dims> sel(start, count);

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

        var_i8.SetStepSelection({tInitial, NSteps - tInitial});
        var_i16.SetStepSelection({tInitial, NSteps - tInitial});
        var_i32.SetStepSelection({tInitial, NSteps - tInitial});
        var_i64.SetStepSelection({tInitial, NSteps - tInitial});

        var_u8.SetStepSelection({tInitial, NSteps - tInitial});
        var_u16.SetStepSelection({tInitial, NSteps - tInitial});
        var_u32.SetStepSelection({tInitial, NSteps - tInitial});
        var_u64.SetStepSelection({tInitial, NSteps - tInitial});

        var_r32.SetStepSelection({tInitial, NSteps - tInitial});
        var_r64.SetStepSelection({tInitial, NSteps - tInitial});

        bpReader.Get(var_i8, I8.data());
        bpReader.Get(var_i16, I16.data());
        bpReader.Get(var_i32, I32.data());
        bpReader.Get(var_i64, I64.data());

        bpReader.Get(var_u8, U8.data());
        bpReader.Get(var_u16, U16.data());
        bpReader.Get(var_u32, U32.data());
        bpReader.Get(var_u64, U64.data());

        bpReader.Get(var_r32, R32.data());
        bpReader.Get(var_r64, R64.data());

        bpReader.PerformGets();

        for (size_t t = tInitial; t < NSteps; ++t)
        {
            // Generate test data for each rank uniquely
            SmallTestData currentTestData = generateNewSmallTestData(
                m_TestData, static_cast<int>(t), mpiRank, mpiSize);

            for (size_t i = 0; i < Nx * Ny; ++i)
            {
                const size_t index = (t - tInitial) * Nx * Ny + i;
                std::stringstream ss;
                ss << "t=" << t << " i=" << i << " rank=" << mpiRank;
                std::string msg = ss.str();

                EXPECT_EQ(I8[index], currentTestData.I8[i]) << msg;
                EXPECT_EQ(I16[index], currentTestData.I16[i]) << msg;
                EXPECT_EQ(I32[index], currentTestData.I32[i]) << msg;
                EXPECT_EQ(I64[index], currentTestData.I64[i]) << msg;
                EXPECT_EQ(U8[index], currentTestData.U8[i]) << msg;
                EXPECT_EQ(U16[index], currentTestData.U16[i]) << msg;
                EXPECT_EQ(U32[index], currentTestData.U32[i]) << msg;
                EXPECT_EQ(U64[index], currentTestData.U64[i]) << msg;
                EXPECT_EQ(R32[index], currentTestData.R32[i]) << msg;
                EXPECT_EQ(R64[index], currentTestData.R64[i]) << msg;
            }
        }

        bpReader.Close();
    }
}

TEST_F(BPWriteReadTestADIOS2, ADIOS2BPWriteRead2D4x2_MultiStepsOverflow)
{
    // Each process would write a 4x2 array and all processes would
    // form a 2D 4 * (NumberOfProcess * Nx) matrix where Nx is 2 here
    const std::string fname("ADIOS2BPWriteRead2D4x2Test_Overflow.bp");

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const std::size_t Nx = 2;
    // Number of cols
    const std::size_t Ny = 4;

    // Number of steps
    const std::size_t NSteps = 5;
    const std::size_t tInitial = 2;

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
        adios2::IO io = adios.DeclareIO("TestIO");

        // Declare 2D variables (4 * (NumberOfProcess * Nx))
        // The local process' part (start, count) can be defined now or later
        // before Write().
        {
            adios2::Dims shape{Ny, static_cast<size_t>(mpiSize * Nx)};
            adios2::Dims start{0, static_cast<size_t>(mpiRank * Nx)};
            adios2::Dims count{Ny, Nx};
            auto var_i8 = io.DefineVariable<int8_t>("i8", shape, start, count);
            auto var_i16 =
                io.DefineVariable<int16_t>("i16", shape, start, count);
            auto var_i32 =
                io.DefineVariable<int32_t>("i32", shape, start, count);
            auto var_i64 =
                io.DefineVariable<int64_t>("i64", shape, start, count);
            auto var_u8 = io.DefineVariable<uint8_t>("u8", shape, start, count);
            auto var_u16 =
                io.DefineVariable<uint16_t>("u16", shape, start, count);
            auto var_u32 =
                io.DefineVariable<uint32_t>("u32", shape, start, count);
            auto var_u64 =
                io.DefineVariable<uint64_t>("u64", shape, start, count);
            auto var_r32 = io.DefineVariable<float>("r32", shape, start, count);
            auto var_r64 =
                io.DefineVariable<double>("r64", shape, start, count);
        }

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            // Create the BP Engine
            io.SetEngine("BPFile");
        }

        io.AddTransport("file");

        adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            // Generate test data for each process uniquely
            SmallTestData currentTestData = generateNewSmallTestData(
                m_TestData, static_cast<int>(step), mpiRank, mpiSize);

            // Retrieve the variables that previously went out of scope
            auto var_i8 = io.InquireVariable<int8_t>("i8");
            auto var_i16 = io.InquireVariable<int16_t>("i16");
            auto var_i32 = io.InquireVariable<int32_t>("i32");
            auto var_i64 = io.InquireVariable<int64_t>("i64");
            auto var_u8 = io.InquireVariable<uint8_t>("u8");
            auto var_u16 = io.InquireVariable<uint16_t>("u16");
            auto var_u32 = io.InquireVariable<uint32_t>("u32");
            auto var_u64 = io.InquireVariable<uint64_t>("u64");
            auto var_r32 = io.InquireVariable<float>("r32");
            auto var_r64 = io.InquireVariable<double>("r64");

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
            bpWriter.BeginStep();
            bpWriter.Put(var_i8, currentTestData.I8.data());
            bpWriter.Put(var_i16, currentTestData.I16.data());
            bpWriter.Put(var_i32, currentTestData.I32.data());
            bpWriter.Put(var_i64, currentTestData.I64.data());
            bpWriter.Put(var_u8, currentTestData.U8.data());
            bpWriter.Put(var_u16, currentTestData.U16.data());
            bpWriter.Put(var_u32, currentTestData.U32.data());
            bpWriter.Put(var_u64, currentTestData.U64.data());
            bpWriter.Put(var_r32, currentTestData.R32.data());
            bpWriter.Put(var_r64, currentTestData.R64.data());
            bpWriter.EndStep();
        }

        // Close the file
        bpWriter.Close();
    }

    {
        adios2::IO io = adios.DeclareIO("ReadIO");

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }

        adios2::Engine bpReader = io.Open(fname, adios2::Mode::Read);

        auto var_i8 = io.InquireVariable<int8_t>("i8");
        auto var_i16 = io.InquireVariable<int16_t>("i16");
        auto var_i32 = io.InquireVariable<int32_t>("i32");
        auto var_i64 = io.InquireVariable<int64_t>("i64");

        auto var_u8 = io.InquireVariable<uint8_t>("u8");
        auto var_u16 = io.InquireVariable<uint16_t>("u16");
        auto var_u32 = io.InquireVariable<uint32_t>("u32");
        auto var_u64 = io.InquireVariable<uint64_t>("u64");

        auto var_r32 = io.InquireVariable<float>("r32");
        auto var_r64 = io.InquireVariable<double>("r64");
        // If the size of the array is smaller than the data
        // the result is weird... double and uint64_t would get
        // completely garbage data
        std::array<int8_t, NSteps * Nx * Ny> I8;
        std::array<int16_t, NSteps * Nx * Ny> I16;
        std::array<int32_t, NSteps * Nx * Ny> I32;
        std::array<int64_t, NSteps * Nx * Ny> I64;
        std::array<uint8_t, NSteps * Nx * Ny> U8;
        std::array<uint16_t, NSteps * Nx * Ny> U16;
        std::array<uint32_t, NSteps * Nx * Ny> U32;
        std::array<uint64_t, NSteps * Nx * Ny> U64;
        std::array<float, NSteps * Nx * Ny> R32;
        std::array<double, NSteps * Nx * Ny> R64;

        const adios2::Dims start{0, static_cast<size_t>(mpiRank * Nx)};
        const adios2::Dims count{Ny, Nx};

        const adios2::Box<adios2::Dims> sel(start, count);

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

        var_i8.SetStepSelection({tInitial, NSteps - tInitial + 1});
        var_i16.SetStepSelection({tInitial + 1, NSteps - tInitial + 1});
        var_i32.SetStepSelection({tInitial + 1, NSteps - tInitial + 1});
        var_i64.SetStepSelection({tInitial + 1, NSteps - tInitial + 1});

        var_u8.SetStepSelection({tInitial + 1, NSteps - tInitial + 1});
        var_u16.SetStepSelection({tInitial + 1, NSteps - tInitial + 1});
        var_u32.SetStepSelection({tInitial + 1, NSteps - tInitial + 1});
        var_u64.SetStepSelection({tInitial + 1, NSteps - tInitial + 1});

        var_r32.SetStepSelection({tInitial + 1, NSteps - tInitial + 1});
        var_r64.SetStepSelection({tInitial + 1, NSteps - tInitial + 1});

        EXPECT_THROW(bpReader.Get(var_i8, I8.data()), std::invalid_argument);
        EXPECT_THROW(bpReader.Get(var_i16, I16.data()), std::invalid_argument);
        EXPECT_THROW(bpReader.Get(var_i32, I32.data()), std::invalid_argument);
        EXPECT_THROW(bpReader.Get(var_i64, I64.data()), std::invalid_argument);

        EXPECT_THROW(bpReader.Get(var_u8, U8.data()), std::invalid_argument);
        EXPECT_THROW(bpReader.Get(var_u16, U16.data()), std::invalid_argument);
        EXPECT_THROW(bpReader.Get(var_u32, U32.data()), std::invalid_argument);
        EXPECT_THROW(bpReader.Get(var_u64, U64.data()), std::invalid_argument);

        EXPECT_THROW(bpReader.Get(var_r32, R32.data()), std::invalid_argument);
        EXPECT_THROW(bpReader.Get(var_r64, R64.data()), std::invalid_argument);
    }
}

TEST_F(BPWriteReadTestADIOS2, OpenEngineTwice)
{
    // Each process would write a 4x2 array and all processes would
    // form a 2D 4 * (NumberOfProcess * Nx) matrix where Nx is 2 here
    const std::string fname("OpenTwice.bp");

#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(true);
#endif
    {
        adios2::IO io = adios.DeclareIO("TwoOpens");

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            // Create the BP Engine
            io.SetEngine("BPFile");
        }

        adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);

        EXPECT_THROW(io.Open(fname, adios2::Mode::Write),
                     std::invalid_argument);

        bpWriter.Close();

        EXPECT_NO_THROW(io.Open(fname, adios2::Mode::Write));
        EXPECT_THROW(io.Open(fname, adios2::Mode::Read), std::invalid_argument);
    }
}

TEST_F(BPWriteReadTestADIOS2, ReadStartCount)
{
    // Each process would write a 4x2 array and all processes would
    // form a 2D 4 * (NumberOfProcess * Nx) matrix where Nx is 2 here
    const std::string fname("ReadStartCount.bp");

    int mpiRank = 0, mpiSize = 1;

    const std::size_t Nx = 10;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

    std::vector<int64_t> localData(Nx);
    std::iota(localData.begin(), localData.end(), mpiRank * Nx);

#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(true);
#endif
    {
        adios2::IO io = adios.DeclareIO("StartCountWrite");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }

        io.DefineVariable<int64_t>(
            "range", {static_cast<std::size_t>(Nx * mpiSize)},
            {static_cast<std::size_t>(Nx * mpiRank)}, {Nx});

        adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);

        bpWriter.Put<int64_t>("range", localData.data());
        bpWriter.Close();
    }
    // Reader
    {
        adios2::IO io = adios.DeclareIO("StartCountRead");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }

        adios2::Engine bpReader = io.Open(fname, adios2::Mode::Read);
        adios2::Variable<int64_t> varRange =
            io.InquireVariable<int64_t>("range");

        const std::size_t gNx = static_cast<std::size_t>(Nx * mpiSize);
        std::vector<int64_t> globalData(gNx);
        bpReader.Get(varRange, globalData);
        bpReader.PerformGets();

        std::vector<int64_t> iStartEndData;
        iStartEndData.reserve(gNx); // maximum possible

        for (size_t i = 1; i < gNx; ++i)
        {
            varRange.SetSelection({{i}, {gNx - i}});

            bpReader.Get("range", iStartEndData);
            bpReader.PerformGets();

            for (size_t j = i; j < gNx; ++j)
            {
                EXPECT_EQ(globalData[j], iStartEndData[j - i]);
            }
        }
        bpReader.Close();
    }
}

//******************************************************************************
// main
//******************************************************************************

int main(int argc, char **argv)
{
#ifdef ADIOS2_HAVE_MPI
    MPI_Init(nullptr, nullptr);
#endif

    int result;
    ::testing::InitGoogleTest(&argc, argv);

    if (argc > 1)
    {
        engineName = std::string(argv[1]);
    }
    result = RUN_ALL_TESTS();

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return result;
}
