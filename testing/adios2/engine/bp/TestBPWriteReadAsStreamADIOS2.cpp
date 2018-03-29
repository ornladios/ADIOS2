/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include <cstdint>
#include <cstring>

#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#include "../SmallTestData.h"

class BPWriteReadAsStreamTestADIOS2 : public ::testing::Test
{
public:
    BPWriteReadAsStreamTestADIOS2() = default;

    SmallTestData m_TestData;
    SmallTestData m_OriginalData;
};

//******************************************************************************
// 1D 1x8 test data
//******************************************************************************

// ADIOS2 BP write, native ADIOS1 read
TEST_F(BPWriteReadAsStreamTestADIOS2, ADIOS2BPWriteRead1D8)
{
    // Each process would write a 1x8 array and all processes would
    // form a mpiSize * Nx 1D array
    const std::string fname("ADIOS2BPWriteReadAsStream1D8.bp");

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
        adios2::IO &io = adios.DeclareIO("TestIO");

        // Declare 1D variables (NumOfProcesses * Nx)
        // The local process' part (start, count) can be defined now or later
        // before Write().
        {
            const adios2::Dims shape{static_cast<size_t>(Nx * mpiSize)};
            const adios2::Dims start{static_cast<size_t>(Nx * mpiRank)};
            const adios2::Dims count{Nx};

            io.DefineVariable<int8_t>("i8", shape, start, count,
                                      adios2::ConstantDims,
                                      m_TestData.I8.data());
            io.DefineVariable<int16_t>("i16", shape, start, count,
                                       adios2::ConstantDims,
                                       m_TestData.I16.data());
            io.DefineVariable<int32_t>("i32", shape, start, count,
                                       adios2::ConstantDims,
                                       m_TestData.I32.data());
            io.DefineVariable<int64_t>("i64", shape, start, count,
                                       adios2::ConstantDims,
                                       m_TestData.I64.data());

            io.DefineVariable<uint8_t>("u8", shape, start, count,
                                       adios2::ConstantDims,
                                       m_TestData.U8.data());

            io.DefineVariable<uint16_t>("u16", shape, start, count,
                                        adios2::ConstantDims,
                                        m_TestData.U16.data());
            io.DefineVariable<uint32_t>("u32", shape, start, count,
                                        adios2::ConstantDims,
                                        m_TestData.U32.data());
            io.DefineVariable<uint64_t>("u64", shape, start, count,
                                        adios2::ConstantDims,
                                        m_TestData.U64.data());

            io.DefineVariable<float>("r32", shape, start, count,
                                     adios2::ConstantDims,
                                     m_TestData.R32.data());
            io.DefineVariable<double>("r64", shape, start, count,
                                      adios2::ConstantDims,
                                      m_TestData.R64.data());
        }

        // Create the BP Engine
        io.SetEngine("BPFile");

        io.AddTransport("file");

        adios2::Engine &bpWriter = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            UpdateSmallTestData(m_TestData, static_cast<int>(step), mpiRank,
                                mpiSize);
            EXPECT_EQ(bpWriter.CurrentStep(), step);
            bpWriter.WriteStep();
        }

        bpWriter.Close();
    }

    {
        adios2::IO &io = adios.DeclareIO("ReadIO");

        adios2::Engine &bpReader = io.Open(fname, adios2::Mode::Read);

        auto var_i8 = io.InquireVariable<int8_t>("i8");
        ASSERT_NE(var_i8, nullptr);
        ASSERT_EQ(var_i8->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i8->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_i8->m_Shape[0], mpiSize * Nx);

        auto var_i16 = io.InquireVariable<int16_t>("i16");
        ASSERT_NE(var_i16, nullptr);
        ASSERT_EQ(var_i16->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i16->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_i16->m_Shape[0], mpiSize * Nx);

        auto var_i32 = io.InquireVariable<int32_t>("i32");
        ASSERT_NE(var_i32, nullptr);
        ASSERT_EQ(var_i32->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i32->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_i32->m_Shape[0], mpiSize * Nx);

        auto var_i64 = io.InquireVariable<int64_t>("i64");
        ASSERT_NE(var_i64, nullptr);
        ASSERT_EQ(var_i64->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i64->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_i64->m_Shape[0], mpiSize * Nx);

        auto var_u8 = io.InquireVariable<uint8_t>("u8");
        ASSERT_NE(var_u8, nullptr);
        ASSERT_EQ(var_u8->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u8->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_u8->m_Shape[0], mpiSize * Nx);

        auto var_u16 = io.InquireVariable<uint16_t>("u16");
        ASSERT_NE(var_u16, nullptr);
        ASSERT_EQ(var_u16->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u16->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_u16->m_Shape[0], mpiSize * Nx);

        auto var_u32 = io.InquireVariable<uint32_t>("u32");
        ASSERT_NE(var_u32, nullptr);
        ASSERT_EQ(var_u32->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u32->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_u32->m_Shape[0], mpiSize * Nx);

        auto var_u64 = io.InquireVariable<uint64_t>("u64");
        ASSERT_NE(var_u64, nullptr);
        ASSERT_EQ(var_u64->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u64->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_u64->m_Shape[0], mpiSize * Nx);

        auto var_r32 = io.InquireVariable<float>("r32");
        ASSERT_NE(var_r32, nullptr);
        ASSERT_EQ(var_r32->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r32->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_r32->m_Shape[0], mpiSize * Nx);

        auto var_r64 = io.InquireVariable<double>("r64");
        ASSERT_NE(var_r64, nullptr);
        ASSERT_EQ(var_r64->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r64->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_r64->m_Shape[0], mpiSize * Nx);

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

        var_i8->SetSelection(sel);
        var_i16->SetSelection(sel);
        var_i32->SetSelection(sel);
        var_i64->SetSelection(sel);

        var_u8->SetSelection(sel);
        var_u16->SetSelection(sel);
        var_u32->SetSelection(sel);
        var_u64->SetSelection(sel);

        var_r32->SetSelection(sel);
        var_r64->SetSelection(sel);

        unsigned int t = 0;

        while (bpReader.BeginStep() == adios2::StepStatus::OK)
        {
            const size_t currentStep = bpReader.CurrentStep();
            EXPECT_EQ(currentStep, static_cast<size_t>(t));

            bpReader.GetDeferred(*var_i8, I8.data());
            bpReader.GetDeferred(*var_i16, I16.data());
            bpReader.GetDeferred(*var_i32, I32.data());
            bpReader.GetDeferred(*var_i64, I64.data());

            bpReader.GetDeferred(*var_u8, U8.data());
            bpReader.GetDeferred(*var_u16, U16.data());
            bpReader.GetDeferred(*var_u32, U32.data());
            bpReader.GetDeferred(*var_u64, U64.data());

            bpReader.GetDeferred(*var_r32, R32.data());
            bpReader.GetDeferred(*var_r64, R64.data());

            bpReader.PerformGets();

            bpReader.EndStep();

            UpdateSmallTestData(m_OriginalData, static_cast<int>(t), mpiRank,
                                mpiSize);

            for (size_t i = 0; i < Nx; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i << " rank=" << mpiRank;
                std::string msg = ss.str();

                EXPECT_EQ(I8[i], m_OriginalData.I8[i]) << msg;
                EXPECT_EQ(I16[i], m_OriginalData.I16[i]) << msg;
                EXPECT_EQ(I32[i], m_OriginalData.I32[i]) << msg;
                EXPECT_EQ(I64[i], m_OriginalData.I64[i]) << msg;
                EXPECT_EQ(U8[i], m_OriginalData.U8[i]) << msg;
                EXPECT_EQ(U16[i], m_OriginalData.U16[i]) << msg;
                EXPECT_EQ(U32[i], m_OriginalData.U32[i]) << msg;
                EXPECT_EQ(U64[i], m_OriginalData.U64[i]) << msg;
                EXPECT_EQ(R32[i], m_OriginalData.R32[i]) << msg;
                EXPECT_EQ(R64[i], m_OriginalData.R64[i]) << msg;
            }
            ++t;
        }

        EXPECT_EQ(t, NSteps);

        bpReader.Close();
    }
}

//******************************************************************************
// 2D 2x4 test data
//******************************************************************************

// ADIOS2 BP write, native ADIOS1 read
TEST_F(BPWriteReadAsStreamTestADIOS2, ADIOS2BPWriteRead2D2x4)
{
    // Each process would write a 2x4 array and all processes would
    // form a 2D 2 * (numberOfProcess*Nx) matrix where Nx is 4 here
    const std::string fname("ADIOS2BPWriteReadAsStream2D2x4Test.bp");

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
            const adios2::Dims shape{Ny, static_cast<size_t>(Nx * mpiSize)};
            const adios2::Dims start{0, static_cast<size_t>(mpiRank * Nx)};
            const adios2::Dims count{Ny, Nx};

            io.DefineVariable<int8_t>("i8", shape, start, count,
                                      adios2::ConstantDims,
                                      m_TestData.I8.data());
            io.DefineVariable<int16_t>("i16", shape, start, count,
                                       adios2::ConstantDims,
                                       m_TestData.I16.data());
            io.DefineVariable<int32_t>("i32", shape, start, count,
                                       adios2::ConstantDims,
                                       m_TestData.I32.data());
            io.DefineVariable<int64_t>("i64", shape, start, count,
                                       adios2::ConstantDims,
                                       m_TestData.I64.data());

            io.DefineVariable<uint8_t>("u8", shape, start, count,
                                       adios2::ConstantDims,
                                       m_TestData.U8.data());

            io.DefineVariable<uint16_t>("u16", shape, start, count,
                                        adios2::ConstantDims,
                                        m_TestData.U16.data());
            io.DefineVariable<uint32_t>("u32", shape, start, count,
                                        adios2::ConstantDims,
                                        m_TestData.U32.data());
            io.DefineVariable<uint64_t>("u64", shape, start, count,
                                        adios2::ConstantDims,
                                        m_TestData.U64.data());

            io.DefineVariable<float>("r32", shape, start, count,
                                     adios2::ConstantDims,
                                     m_TestData.R32.data());
            io.DefineVariable<double>("r64", shape, start, count,
                                      adios2::ConstantDims,
                                      m_TestData.R64.data());
        }

        // Create the BP Engine
        io.SetEngine("BPFile");
        io.AddTransport("file");

        adios2::Engine &bpWriter = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            UpdateSmallTestData(m_TestData, static_cast<int>(step), mpiRank,
                                mpiSize);
            EXPECT_EQ(bpWriter.CurrentStep(), step);
            bpWriter.WriteStep();
        }

        bpWriter.Close();
    }

    {
        adios2::IO &io = adios.DeclareIO("ReadIO");

        adios2::Engine &bpReader = io.Open(fname, adios2::Mode::Read);

        auto var_i8 = io.InquireVariable<int8_t>("i8");
        ASSERT_NE(var_i8, nullptr);
        ASSERT_EQ(var_i8->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i8->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_i8->m_Shape[0], Ny);
        ASSERT_EQ(var_i8->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_i16 = io.InquireVariable<int16_t>("i16");
        ASSERT_NE(var_i16, nullptr);
        ASSERT_EQ(var_i16->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i16->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_i16->m_Shape[0], Ny);
        ASSERT_EQ(var_i16->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_i32 = io.InquireVariable<int32_t>("i32");
        ASSERT_NE(var_i32, nullptr);
        ASSERT_EQ(var_i32->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i32->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_i32->m_Shape[0], Ny);
        ASSERT_EQ(var_i32->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_i64 = io.InquireVariable<int64_t>("i64");
        ASSERT_NE(var_i64, nullptr);
        ASSERT_EQ(var_i64->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i64->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_i64->m_Shape[0], Ny);
        ASSERT_EQ(var_i64->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_u8 = io.InquireVariable<uint8_t>("u8");
        ASSERT_NE(var_u8, nullptr);
        ASSERT_EQ(var_u8->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u8->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_u8->m_Shape[0], Ny);
        ASSERT_EQ(var_u8->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_u16 = io.InquireVariable<uint16_t>("u16");
        ASSERT_NE(var_u16, nullptr);
        ASSERT_EQ(var_u16->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u16->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_u16->m_Shape[0], Ny);
        ASSERT_EQ(var_u16->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_u32 = io.InquireVariable<uint32_t>("u32");
        ASSERT_NE(var_u32, nullptr);
        ASSERT_EQ(var_u32->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u32->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_u32->m_Shape[0], Ny);
        ASSERT_EQ(var_u32->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_u64 = io.InquireVariable<uint64_t>("u64");
        ASSERT_NE(var_u64, nullptr);
        ASSERT_EQ(var_u64->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u64->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_u64->m_Shape[0], Ny);
        ASSERT_EQ(var_u64->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_r32 = io.InquireVariable<float>("r32");
        ASSERT_NE(var_r32, nullptr);
        ASSERT_EQ(var_r32->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r32->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_r32->m_Shape[0], Ny);
        ASSERT_EQ(var_r32->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_r64 = io.InquireVariable<double>("r64");
        ASSERT_NE(var_r64, nullptr);
        ASSERT_EQ(var_r64->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r64->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_r64->m_Shape[0], Ny);
        ASSERT_EQ(var_r64->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

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

        var_i8->SetSelection(sel);
        var_i16->SetSelection(sel);
        var_i32->SetSelection(sel);
        var_i64->SetSelection(sel);

        var_u8->SetSelection(sel);
        var_u16->SetSelection(sel);
        var_u32->SetSelection(sel);
        var_u64->SetSelection(sel);

        var_r32->SetSelection(sel);
        var_r64->SetSelection(sel);

        unsigned int t = 0;

        while (bpReader.BeginStep() == adios2::StepStatus::OK)
        {
            const size_t currentStep = bpReader.CurrentStep();
            EXPECT_EQ(currentStep, static_cast<size_t>(t));

            bpReader.GetDeferred(*var_i8, I8.data());
            bpReader.GetDeferred(*var_i16, I16.data());
            bpReader.GetDeferred(*var_i32, I32.data());
            bpReader.GetDeferred(*var_i64, I64.data());

            bpReader.GetDeferred(*var_u8, U8.data());
            bpReader.GetDeferred(*var_u16, U16.data());
            bpReader.GetDeferred(*var_u32, U32.data());
            bpReader.GetDeferred(*var_u64, U64.data());

            bpReader.GetDeferred(*var_r32, R32.data());
            bpReader.GetDeferred(*var_r64, R64.data());

            bpReader.PerformGets();
            bpReader.EndStep();

            // Generate test data for each rank uniquely
            UpdateSmallTestData(m_OriginalData, static_cast<int>(t), mpiRank,
                                mpiSize);

            for (size_t i = 0; i < Nx * Ny; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i << " rank=" << mpiRank;
                std::string msg = ss.str();

                EXPECT_EQ(I8[i], m_OriginalData.I8[i]) << msg;
                EXPECT_EQ(I16[i], m_OriginalData.I16[i]) << msg;
                EXPECT_EQ(I32[i], m_OriginalData.I32[i]) << msg;
                EXPECT_EQ(I64[i], m_OriginalData.I64[i]) << msg;
                EXPECT_EQ(U8[i], m_OriginalData.U8[i]) << msg;
                EXPECT_EQ(U16[i], m_OriginalData.U16[i]) << msg;
                EXPECT_EQ(U32[i], m_OriginalData.U32[i]) << msg;
                EXPECT_EQ(U64[i], m_OriginalData.U64[i]) << msg;
                EXPECT_EQ(R32[i], m_OriginalData.R32[i]) << msg;
                EXPECT_EQ(R64[i], m_OriginalData.R64[i]) << msg;
            }
            ++t;
        }
        EXPECT_EQ(t, NSteps);

        bpReader.Close();
    }
}

//******************************************************************************
// 2D 4x2 test data
//******************************************************************************

TEST_F(BPWriteReadAsStreamTestADIOS2, ADIOS2BPWriteRead2D4x2)
{
    // Each process would write a 4x2 array and all processes would
    // form a 2D 4 * (NumberOfProcess * Nx) matrix where Nx is 2 here
    const std::string fname("ADIOS2BPWriteReadAsStream2D4x2Test.bp");

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

            io.DefineVariable<int8_t>("i8", shape, start, count,
                                      adios2::ConstantDims,
                                      m_TestData.I8.data());
            io.DefineVariable<int16_t>("i16", shape, start, count,
                                       adios2::ConstantDims,
                                       m_TestData.I16.data());
            io.DefineVariable<int32_t>("i32", shape, start, count,
                                       adios2::ConstantDims,
                                       m_TestData.I32.data());
            io.DefineVariable<int64_t>("i64", shape, start, count,
                                       adios2::ConstantDims,
                                       m_TestData.I64.data());

            io.DefineVariable<uint8_t>("u8", shape, start, count,
                                       adios2::ConstantDims,
                                       m_TestData.U8.data());

            io.DefineVariable<uint16_t>("u16", shape, start, count,
                                        adios2::ConstantDims,
                                        m_TestData.U16.data());
            io.DefineVariable<uint32_t>("u32", shape, start, count,
                                        adios2::ConstantDims,
                                        m_TestData.U32.data());
            io.DefineVariable<uint64_t>("u64", shape, start, count,
                                        adios2::ConstantDims,
                                        m_TestData.U64.data());

            io.DefineVariable<float>("r32", shape, start, count,
                                     adios2::ConstantDims,
                                     m_TestData.R32.data());
            io.DefineVariable<double>("r64", shape, start, count,
                                      adios2::ConstantDims,
                                      m_TestData.R64.data());
        }

        // Create the BP Engine
        io.SetEngine("BPFile");

        io.AddTransport("file");

        adios2::Engine &bpWriter = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            // Generate test data for each process uniquely
            UpdateSmallTestData(m_TestData, static_cast<int>(step), mpiRank,
                                mpiSize);
            EXPECT_EQ(bpWriter.CurrentStep(), step);
            bpWriter.WriteStep();
        }

        bpWriter.Close();
    }

    {
        adios2::IO &io = adios.DeclareIO("ReadIO");

        adios2::Engine &bpReader = io.Open(fname, adios2::Mode::Read);

        auto var_i8 = io.InquireVariable<int8_t>("i8");
        ASSERT_NE(var_i8, nullptr);
        ASSERT_EQ(var_i8->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i8->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_i8->m_Shape[0], Ny);
        ASSERT_EQ(var_i8->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_i16 = io.InquireVariable<int16_t>("i16");
        ASSERT_NE(var_i16, nullptr);
        ASSERT_EQ(var_i16->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i16->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_i16->m_Shape[0], Ny);
        ASSERT_EQ(var_i16->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_i32 = io.InquireVariable<int32_t>("i32");
        ASSERT_NE(var_i32, nullptr);
        ASSERT_EQ(var_i32->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i32->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_i32->m_Shape[0], Ny);
        ASSERT_EQ(var_i32->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_i64 = io.InquireVariable<int64_t>("i64");
        ASSERT_NE(var_i64, nullptr);
        ASSERT_EQ(var_i64->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i64->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_i64->m_Shape[0], Ny);
        ASSERT_EQ(var_i64->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_u8 = io.InquireVariable<uint8_t>("u8");
        ASSERT_NE(var_u8, nullptr);
        ASSERT_EQ(var_u8->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u8->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_u8->m_Shape[0], Ny);
        ASSERT_EQ(var_u8->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_u16 = io.InquireVariable<uint16_t>("u16");
        ASSERT_NE(var_u16, nullptr);
        ASSERT_EQ(var_u16->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u16->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_u16->m_Shape[0], Ny);
        ASSERT_EQ(var_u16->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_u32 = io.InquireVariable<uint32_t>("u32");
        ASSERT_NE(var_u32, nullptr);
        ASSERT_EQ(var_u32->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u32->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_u32->m_Shape[0], Ny);
        ASSERT_EQ(var_u32->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_u64 = io.InquireVariable<uint64_t>("u64");
        ASSERT_NE(var_u64, nullptr);
        ASSERT_EQ(var_u64->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u64->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_u64->m_Shape[0], Ny);
        ASSERT_EQ(var_u64->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_r32 = io.InquireVariable<float>("r32");
        ASSERT_NE(var_r32, nullptr);
        ASSERT_EQ(var_r32->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r32->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_r32->m_Shape[0], Ny);
        ASSERT_EQ(var_r32->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_r64 = io.InquireVariable<double>("r64");
        ASSERT_NE(var_r64, nullptr);
        ASSERT_EQ(var_r64->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r64->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_r64->m_Shape[0], Ny);
        ASSERT_EQ(var_r64->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

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

        var_i8->SetSelection(sel);
        var_i16->SetSelection(sel);
        var_i32->SetSelection(sel);
        var_i64->SetSelection(sel);

        var_u8->SetSelection(sel);
        var_u16->SetSelection(sel);
        var_u32->SetSelection(sel);
        var_u64->SetSelection(sel);

        var_r32->SetSelection(sel);
        var_r64->SetSelection(sel);

        unsigned int t = 0;

        while (bpReader.BeginStep() == adios2::StepStatus::OK)
        {
            const size_t currentStep = bpReader.CurrentStep();
            EXPECT_EQ(currentStep, static_cast<size_t>(t));

            bpReader.GetDeferred(*var_i8, I8.data());
            bpReader.GetDeferred(*var_i16, I16.data());
            bpReader.GetDeferred(*var_i32, I32.data());
            bpReader.GetDeferred(*var_i64, I64.data());

            bpReader.GetDeferred(*var_u8, U8.data());
            bpReader.GetDeferred(*var_u16, U16.data());
            bpReader.GetDeferred(*var_u32, U32.data());
            bpReader.GetDeferred(*var_u64, U64.data());

            bpReader.GetDeferred(*var_r32, R32.data());
            bpReader.GetDeferred(*var_r64, R64.data());

            bpReader.PerformGets();

            bpReader.EndStep();

            // Generate test data for each rank uniquely
            UpdateSmallTestData(m_OriginalData, static_cast<int>(t), mpiRank,
                                mpiSize);

            for (size_t i = 0; i < Nx * Ny; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i << " rank=" << mpiRank;
                std::string msg = ss.str();

                EXPECT_EQ(I8[i], m_OriginalData.I8[i]) << msg;
                EXPECT_EQ(I16[i], m_OriginalData.I16[i]) << msg;
                EXPECT_EQ(I32[i], m_OriginalData.I32[i]) << msg;
                EXPECT_EQ(I64[i], m_OriginalData.I64[i]) << msg;
                EXPECT_EQ(U8[i], m_OriginalData.U8[i]) << msg;
                EXPECT_EQ(U16[i], m_OriginalData.U16[i]) << msg;
                EXPECT_EQ(U32[i], m_OriginalData.U32[i]) << msg;
                EXPECT_EQ(U64[i], m_OriginalData.U64[i]) << msg;
                EXPECT_EQ(R32[i], m_OriginalData.R32[i]) << msg;
                EXPECT_EQ(R64[i], m_OriginalData.R64[i]) << msg;
            }
            ++t;
        }
        EXPECT_EQ(t, NSteps);
        bpReader.Close();
    }
}

TEST_F(BPWriteReadAsStreamTestADIOS2, ADIOS2BPWriteRead1D8MissingPerformGets)
{
    // Each process would write a 1x8 array and all processes would
    // form a mpiSize * Nx 1D array
    const std::string fname(
        "ADIOS2BPWriteReadAsStream1D8MissingPerformGetsTest.bp");

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
        adios2::IO &io = adios.DeclareIO("TestIO");

        // Declare 1D variables (NumOfProcesses * Nx)
        // The local process' part (start, count) can be defined now or later
        // before Write().
        {
            const adios2::Dims shape{static_cast<size_t>(Nx * mpiSize)};
            const adios2::Dims start{static_cast<size_t>(Nx * mpiRank)};
            const adios2::Dims count{Nx};

            io.DefineVariable<int8_t>("i8", shape, start, count,
                                      adios2::ConstantDims,
                                      m_TestData.I8.data());
            io.DefineVariable<int16_t>("i16", shape, start, count,
                                       adios2::ConstantDims,
                                       m_TestData.I16.data());
            io.DefineVariable<int32_t>("i32", shape, start, count,
                                       adios2::ConstantDims,
                                       m_TestData.I32.data());
            io.DefineVariable<int64_t>("i64", shape, start, count,
                                       adios2::ConstantDims,
                                       m_TestData.I64.data());

            io.DefineVariable<uint8_t>("u8", shape, start, count,
                                       adios2::ConstantDims,
                                       m_TestData.U8.data());

            io.DefineVariable<uint16_t>("u16", shape, start, count,
                                        adios2::ConstantDims,
                                        m_TestData.U16.data());
            io.DefineVariable<uint32_t>("u32", shape, start, count,
                                        adios2::ConstantDims,
                                        m_TestData.U32.data());
            io.DefineVariable<uint64_t>("u64", shape, start, count,
                                        adios2::ConstantDims,
                                        m_TestData.U64.data());

            io.DefineVariable<float>("r32", shape, start, count,
                                     adios2::ConstantDims,
                                     m_TestData.R32.data());
            io.DefineVariable<double>("r64", shape, start, count,
                                      adios2::ConstantDims,
                                      m_TestData.R64.data());
        }

        // Create the BP Engine
        io.SetEngine("BPFile");

        io.AddTransport("file");

        // QUESTION: It seems that BPFilterWriter cannot overwrite existing
        // files
        // Ex. if you tune Nx and NSteps, the test would fail. But if you clear
        // the cache in
        // ${adios2Build}/testing/adios2/engine/bp/ADIOS2BPWriteADIOS1Read1D8.bp.dir,
        // then it works
        adios2::Engine &bpWriter = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            UpdateSmallTestData(m_TestData, static_cast<int>(step), mpiRank,
                                mpiSize);
            EXPECT_EQ(bpWriter.CurrentStep(), step);
            bpWriter.WriteStep();
        }

        bpWriter.Close();
    }

    {
        adios2::IO &io = adios.DeclareIO("ReadIO");

        adios2::Engine &bpReader = io.Open(fname, adios2::Mode::Read);

        auto var_i8 = io.InquireVariable<int8_t>("i8");
        ASSERT_NE(var_i8, nullptr);
        ASSERT_EQ(var_i8->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i8->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_i8->m_Shape[0], mpiSize * Nx);

        auto var_i16 = io.InquireVariable<int16_t>("i16");
        ASSERT_NE(var_i16, nullptr);
        ASSERT_EQ(var_i16->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i16->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_i16->m_Shape[0], mpiSize * Nx);

        auto var_i32 = io.InquireVariable<int32_t>("i32");
        ASSERT_NE(var_i32, nullptr);
        ASSERT_EQ(var_i32->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i32->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_i32->m_Shape[0], mpiSize * Nx);

        auto var_i64 = io.InquireVariable<int64_t>("i64");
        ASSERT_NE(var_i64, nullptr);
        ASSERT_EQ(var_i64->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i64->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_i64->m_Shape[0], mpiSize * Nx);

        auto var_u8 = io.InquireVariable<uint8_t>("u8");
        ASSERT_NE(var_u8, nullptr);
        ASSERT_EQ(var_u8->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u8->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_u8->m_Shape[0], mpiSize * Nx);

        auto var_u16 = io.InquireVariable<uint16_t>("u16");
        ASSERT_NE(var_u16, nullptr);
        ASSERT_EQ(var_u16->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u16->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_u16->m_Shape[0], mpiSize * Nx);

        auto var_u32 = io.InquireVariable<uint32_t>("u32");
        ASSERT_NE(var_u32, nullptr);
        ASSERT_EQ(var_u32->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u32->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_u32->m_Shape[0], mpiSize * Nx);

        auto var_u64 = io.InquireVariable<uint64_t>("u64");
        ASSERT_NE(var_u64, nullptr);
        ASSERT_EQ(var_u64->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u64->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_u64->m_Shape[0], mpiSize * Nx);

        auto var_r32 = io.InquireVariable<float>("r32");
        ASSERT_NE(var_r32, nullptr);
        ASSERT_EQ(var_r32->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r32->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_r32->m_Shape[0], mpiSize * Nx);

        auto var_r64 = io.InquireVariable<double>("r64");
        ASSERT_NE(var_r64, nullptr);
        ASSERT_EQ(var_r64->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r64->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_r64->m_Shape[0], mpiSize * Nx);

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

        var_i8->SetSelection(sel);
        var_i16->SetSelection(sel);
        var_i32->SetSelection(sel);
        var_i64->SetSelection(sel);

        var_u8->SetSelection(sel);
        var_u16->SetSelection(sel);
        var_u32->SetSelection(sel);
        var_u64->SetSelection(sel);

        var_r32->SetSelection(sel);
        var_r64->SetSelection(sel);

        unsigned int t = 0;

        while (bpReader.BeginStep() == adios2::StepStatus::OK)
        {
            const size_t currentStep = bpReader.CurrentStep();
            EXPECT_EQ(currentStep, static_cast<size_t>(t));

            bpReader.GetDeferred(*var_i8, I8.data());
            bpReader.GetDeferred(*var_i16, I16.data());
            bpReader.GetDeferred(*var_i32, I32.data());
            bpReader.GetDeferred(*var_i64, I64.data());

            bpReader.GetDeferred(*var_u8, U8.data());
            bpReader.GetDeferred(*var_u16, U16.data());
            bpReader.GetDeferred(*var_u32, U32.data());
            bpReader.GetDeferred(*var_u64, U64.data());

            bpReader.GetDeferred(*var_r32, R32.data());
            bpReader.GetDeferred(*var_r64, R64.data());

            bpReader.EndStep();

            UpdateSmallTestData(m_OriginalData, static_cast<int>(t), mpiRank,
                                mpiSize);

            for (size_t i = 0; i < Nx; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i << " rank=" << mpiRank;
                std::string msg = ss.str();

                EXPECT_EQ(I8[i], m_OriginalData.I8[i]) << msg;
                EXPECT_EQ(I16[i], m_OriginalData.I16[i]) << msg;
                EXPECT_EQ(I32[i], m_OriginalData.I32[i]) << msg;
                EXPECT_EQ(I64[i], m_OriginalData.I64[i]) << msg;
                EXPECT_EQ(U8[i], m_OriginalData.U8[i]) << msg;
                EXPECT_EQ(U16[i], m_OriginalData.U16[i]) << msg;
                EXPECT_EQ(U32[i], m_OriginalData.U32[i]) << msg;
                EXPECT_EQ(U64[i], m_OriginalData.U64[i]) << msg;
                EXPECT_EQ(R32[i], m_OriginalData.R32[i]) << msg;
                EXPECT_EQ(R64[i], m_OriginalData.R64[i]) << msg;
            }
            ++t;
        }

        EXPECT_EQ(t, NSteps);

        bpReader.Close();
    }
}

//******************************************************************************
// 2D 2x4 test data
//******************************************************************************

// ADIOS2 BP write, native ADIOS1 read
TEST_F(BPWriteReadAsStreamTestADIOS2, ADIOS2BPWriteRead2D2x4MissingPerformGets)
{
    // Each process would write a 2x4 array and all processes would
    // form a 2D 2 * (numberOfProcess*Nx) matrix where Nx is 4 here
    const std::string fname(
        "ADIOS2BPWriteReadAsStream2D2x4MissingPerformGetsTest.bp");

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
            const adios2::Dims shape{Ny, static_cast<size_t>(Nx * mpiSize)};
            const adios2::Dims start{0, static_cast<size_t>(mpiRank * Nx)};
            const adios2::Dims count{Ny, Nx};

            io.DefineVariable<int8_t>("i8", shape, start, count,
                                      adios2::ConstantDims,
                                      m_TestData.I8.data());
            io.DefineVariable<int16_t>("i16", shape, start, count,
                                       adios2::ConstantDims,
                                       m_TestData.I16.data());
            io.DefineVariable<int32_t>("i32", shape, start, count,
                                       adios2::ConstantDims,
                                       m_TestData.I32.data());
            io.DefineVariable<int64_t>("i64", shape, start, count,
                                       adios2::ConstantDims,
                                       m_TestData.I64.data());

            io.DefineVariable<uint8_t>("u8", shape, start, count,
                                       adios2::ConstantDims,
                                       m_TestData.U8.data());

            io.DefineVariable<uint16_t>("u16", shape, start, count,
                                        adios2::ConstantDims,
                                        m_TestData.U16.data());
            io.DefineVariable<uint32_t>("u32", shape, start, count,
                                        adios2::ConstantDims,
                                        m_TestData.U32.data());
            io.DefineVariable<uint64_t>("u64", shape, start, count,
                                        adios2::ConstantDims,
                                        m_TestData.U64.data());

            io.DefineVariable<float>("r32", shape, start, count,
                                     adios2::ConstantDims,
                                     m_TestData.R32.data());
            io.DefineVariable<double>("r64", shape, start, count,
                                      adios2::ConstantDims,
                                      m_TestData.R64.data());
        }

        // Create the BP Engine
        io.SetEngine("BPFile");
        io.AddTransport("file");

        adios2::Engine &bpWriter = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            UpdateSmallTestData(m_TestData, static_cast<int>(step), mpiRank,
                                mpiSize);
            EXPECT_EQ(bpWriter.CurrentStep(), step);
            bpWriter.WriteStep();
        }

        bpWriter.Close();
    }

    {
        adios2::IO &io = adios.DeclareIO("ReadIO");

        adios2::Engine &bpReader = io.Open(fname, adios2::Mode::Read);

        auto var_i8 = io.InquireVariable<int8_t>("i8");
        ASSERT_NE(var_i8, nullptr);
        ASSERT_EQ(var_i8->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i8->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_i8->m_Shape[0], Ny);
        ASSERT_EQ(var_i8->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_i16 = io.InquireVariable<int16_t>("i16");
        ASSERT_NE(var_i16, nullptr);
        ASSERT_EQ(var_i16->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i16->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_i16->m_Shape[0], Ny);
        ASSERT_EQ(var_i16->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_i32 = io.InquireVariable<int32_t>("i32");
        ASSERT_NE(var_i32, nullptr);
        ASSERT_EQ(var_i32->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i32->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_i32->m_Shape[0], Ny);
        ASSERT_EQ(var_i32->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_i64 = io.InquireVariable<int64_t>("i64");
        ASSERT_NE(var_i64, nullptr);
        ASSERT_EQ(var_i64->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i64->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_i64->m_Shape[0], Ny);
        ASSERT_EQ(var_i64->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_u8 = io.InquireVariable<uint8_t>("u8");
        ASSERT_NE(var_u8, nullptr);
        ASSERT_EQ(var_u8->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u8->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_u8->m_Shape[0], Ny);
        ASSERT_EQ(var_u8->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_u16 = io.InquireVariable<uint16_t>("u16");
        ASSERT_NE(var_u16, nullptr);
        ASSERT_EQ(var_u16->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u16->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_u16->m_Shape[0], Ny);
        ASSERT_EQ(var_u16->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_u32 = io.InquireVariable<uint32_t>("u32");
        ASSERT_NE(var_u32, nullptr);
        ASSERT_EQ(var_u32->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u32->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_u32->m_Shape[0], Ny);
        ASSERT_EQ(var_u32->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_u64 = io.InquireVariable<uint64_t>("u64");
        ASSERT_NE(var_u64, nullptr);
        ASSERT_EQ(var_u64->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u64->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_u64->m_Shape[0], Ny);
        ASSERT_EQ(var_u64->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_r32 = io.InquireVariable<float>("r32");
        ASSERT_NE(var_r32, nullptr);
        ASSERT_EQ(var_r32->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r32->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_r32->m_Shape[0], Ny);
        ASSERT_EQ(var_r32->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_r64 = io.InquireVariable<double>("r64");
        ASSERT_NE(var_r64, nullptr);
        ASSERT_EQ(var_r64->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r64->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_r64->m_Shape[0], Ny);
        ASSERT_EQ(var_r64->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

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

        var_i8->SetSelection(sel);
        var_i16->SetSelection(sel);
        var_i32->SetSelection(sel);
        var_i64->SetSelection(sel);

        var_u8->SetSelection(sel);
        var_u16->SetSelection(sel);
        var_u32->SetSelection(sel);
        var_u64->SetSelection(sel);

        var_r32->SetSelection(sel);
        var_r64->SetSelection(sel);

        unsigned int t = 0;

        while (bpReader.BeginStep() == adios2::StepStatus::OK)
        {
            const size_t currentStep = bpReader.CurrentStep();
            EXPECT_EQ(currentStep, static_cast<size_t>(t));

            bpReader.GetDeferred(*var_i8, I8.data());
            bpReader.GetDeferred(*var_i16, I16.data());
            bpReader.GetDeferred(*var_i32, I32.data());
            bpReader.GetDeferred(*var_i64, I64.data());

            bpReader.GetDeferred(*var_u8, U8.data());
            bpReader.GetDeferred(*var_u16, U16.data());
            bpReader.GetDeferred(*var_u32, U32.data());
            bpReader.GetDeferred(*var_u64, U64.data());

            bpReader.GetDeferred(*var_r32, R32.data());
            bpReader.GetDeferred(*var_r64, R64.data());

            bpReader.EndStep();

            // Generate test data for each rank uniquely
            UpdateSmallTestData(m_OriginalData, static_cast<int>(t), mpiRank,
                                mpiSize);

            for (size_t i = 0; i < Nx * Ny; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i << " rank=" << mpiRank;
                std::string msg = ss.str();

                EXPECT_EQ(I8[i], m_OriginalData.I8[i]) << msg;
                EXPECT_EQ(I16[i], m_OriginalData.I16[i]) << msg;
                EXPECT_EQ(I32[i], m_OriginalData.I32[i]) << msg;
                EXPECT_EQ(I64[i], m_OriginalData.I64[i]) << msg;
                EXPECT_EQ(U8[i], m_OriginalData.U8[i]) << msg;
                EXPECT_EQ(U16[i], m_OriginalData.U16[i]) << msg;
                EXPECT_EQ(U32[i], m_OriginalData.U32[i]) << msg;
                EXPECT_EQ(U64[i], m_OriginalData.U64[i]) << msg;
                EXPECT_EQ(R32[i], m_OriginalData.R32[i]) << msg;
                EXPECT_EQ(R64[i], m_OriginalData.R64[i]) << msg;
            }
            ++t;
        }
        EXPECT_EQ(t, NSteps);

        bpReader.Close();
    }
}

//******************************************************************************
// 2D 4x2 test data
//******************************************************************************

TEST_F(BPWriteReadAsStreamTestADIOS2, ADIOS2BPWriteRead2D4x2MissingPerformGets)
{
    // Each process would write a 4x2 array and all processes would
    // form a 2D 4 * (NumberOfProcess * Nx) matrix where Nx is 2 here
    const std::string fname(
        "ADIOS2BPWriteReadAsStream2D4x2MissingPerformGetsTest.bp");

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

            io.DefineVariable<int8_t>("i8", shape, start, count,
                                      adios2::ConstantDims,
                                      m_TestData.I8.data());
            io.DefineVariable<int16_t>("i16", shape, start, count,
                                       adios2::ConstantDims,
                                       m_TestData.I16.data());
            io.DefineVariable<int32_t>("i32", shape, start, count,
                                       adios2::ConstantDims,
                                       m_TestData.I32.data());
            io.DefineVariable<int64_t>("i64", shape, start, count,
                                       adios2::ConstantDims,
                                       m_TestData.I64.data());

            io.DefineVariable<uint8_t>("u8", shape, start, count,
                                       adios2::ConstantDims,
                                       m_TestData.U8.data());

            io.DefineVariable<uint16_t>("u16", shape, start, count,
                                        adios2::ConstantDims,
                                        m_TestData.U16.data());
            io.DefineVariable<uint32_t>("u32", shape, start, count,
                                        adios2::ConstantDims,
                                        m_TestData.U32.data());
            io.DefineVariable<uint64_t>("u64", shape, start, count,
                                        adios2::ConstantDims,
                                        m_TestData.U64.data());

            io.DefineVariable<float>("r32", shape, start, count,
                                     adios2::ConstantDims,
                                     m_TestData.R32.data());
            io.DefineVariable<double>("r64", shape, start, count,
                                      adios2::ConstantDims,
                                      m_TestData.R64.data());
        }

        // Create the BP Engine
        io.SetEngine("BPFile");

        io.AddTransport("file");

        adios2::Engine &bpWriter = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            // Generate test data for each process uniquely
            UpdateSmallTestData(m_TestData, static_cast<int>(step), mpiRank,
                                mpiSize);
            EXPECT_EQ(bpWriter.CurrentStep(), step);
            bpWriter.WriteStep();
        }

        bpWriter.Close();
    }

    {
        adios2::IO &io = adios.DeclareIO("ReadIO");

        adios2::Engine &bpReader = io.Open(fname, adios2::Mode::Read);

        auto var_i8 = io.InquireVariable<int8_t>("i8");
        ASSERT_NE(var_i8, nullptr);
        ASSERT_EQ(var_i8->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i8->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_i8->m_Shape[0], Ny);
        ASSERT_EQ(var_i8->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_i16 = io.InquireVariable<int16_t>("i16");
        ASSERT_NE(var_i16, nullptr);
        ASSERT_EQ(var_i16->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i16->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_i16->m_Shape[0], Ny);
        ASSERT_EQ(var_i16->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_i32 = io.InquireVariable<int32_t>("i32");
        ASSERT_NE(var_i32, nullptr);
        ASSERT_EQ(var_i32->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i32->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_i32->m_Shape[0], Ny);
        ASSERT_EQ(var_i32->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_i64 = io.InquireVariable<int64_t>("i64");
        ASSERT_NE(var_i64, nullptr);
        ASSERT_EQ(var_i64->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i64->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_i64->m_Shape[0], Ny);
        ASSERT_EQ(var_i64->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_u8 = io.InquireVariable<uint8_t>("u8");
        ASSERT_NE(var_u8, nullptr);
        ASSERT_EQ(var_u8->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u8->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_u8->m_Shape[0], Ny);
        ASSERT_EQ(var_u8->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_u16 = io.InquireVariable<uint16_t>("u16");
        ASSERT_NE(var_u16, nullptr);
        ASSERT_EQ(var_u16->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u16->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_u16->m_Shape[0], Ny);
        ASSERT_EQ(var_u16->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_u32 = io.InquireVariable<uint32_t>("u32");
        ASSERT_NE(var_u32, nullptr);
        ASSERT_EQ(var_u32->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u32->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_u32->m_Shape[0], Ny);
        ASSERT_EQ(var_u32->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_u64 = io.InquireVariable<uint64_t>("u64");
        ASSERT_NE(var_u64, nullptr);
        ASSERT_EQ(var_u64->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_u64->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_u64->m_Shape[0], Ny);
        ASSERT_EQ(var_u64->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_r32 = io.InquireVariable<float>("r32");
        ASSERT_NE(var_r32, nullptr);
        ASSERT_EQ(var_r32->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r32->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_r32->m_Shape[0], Ny);
        ASSERT_EQ(var_r32->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

        auto var_r64 = io.InquireVariable<double>("r64");
        ASSERT_NE(var_r64, nullptr);
        ASSERT_EQ(var_r64->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r64->m_AvailableStepsCount, NSteps);
        ASSERT_EQ(var_r64->m_Shape[0], Ny);
        ASSERT_EQ(var_r64->m_Shape[1], static_cast<size_t>(mpiSize * Nx));

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

        var_i8->SetSelection(sel);
        var_i16->SetSelection(sel);
        var_i32->SetSelection(sel);
        var_i64->SetSelection(sel);

        var_u8->SetSelection(sel);
        var_u16->SetSelection(sel);
        var_u32->SetSelection(sel);
        var_u64->SetSelection(sel);

        var_r32->SetSelection(sel);
        var_r64->SetSelection(sel);

        unsigned int t = 0;

        while (bpReader.BeginStep() == adios2::StepStatus::OK)
        {
            const size_t currentStep = bpReader.CurrentStep();
            EXPECT_EQ(currentStep, static_cast<size_t>(t));

            bpReader.GetDeferred(*var_i8, I8.data());
            bpReader.GetDeferred(*var_i16, I16.data());
            bpReader.GetDeferred(*var_i32, I32.data());
            bpReader.GetDeferred(*var_i64, I64.data());

            bpReader.GetDeferred(*var_u8, U8.data());
            bpReader.GetDeferred(*var_u16, U16.data());
            bpReader.GetDeferred(*var_u32, U32.data());
            bpReader.GetDeferred(*var_u64, U64.data());

            bpReader.GetDeferred(*var_r32, R32.data());
            bpReader.GetDeferred(*var_r64, R64.data());

            bpReader.EndStep();

            // Generate test data for each rank uniquely
            UpdateSmallTestData(m_OriginalData, static_cast<int>(t), mpiRank,
                                mpiSize);

            for (size_t i = 0; i < Nx * Ny; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i << " rank=" << mpiRank;
                std::string msg = ss.str();

                EXPECT_EQ(I8[i], m_OriginalData.I8[i]) << msg;
                EXPECT_EQ(I16[i], m_OriginalData.I16[i]) << msg;
                EXPECT_EQ(I32[i], m_OriginalData.I32[i]) << msg;
                EXPECT_EQ(I64[i], m_OriginalData.I64[i]) << msg;
                EXPECT_EQ(U8[i], m_OriginalData.U8[i]) << msg;
                EXPECT_EQ(U16[i], m_OriginalData.U16[i]) << msg;
                EXPECT_EQ(U32[i], m_OriginalData.U32[i]) << msg;
                EXPECT_EQ(U64[i], m_OriginalData.U64[i]) << msg;
                EXPECT_EQ(R32[i], m_OriginalData.R32[i]) << msg;
                EXPECT_EQ(R64[i], m_OriginalData.R64[i]) << msg;
            }
            ++t;
        }
        EXPECT_EQ(t, NSteps);
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

    int result = -1;
    try
    {
        ::testing::InitGoogleTest(&argc, argv);
        result = RUN_ALL_TESTS();
    }
    catch (std::exception &e)
    {
        result = 1;
    }

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return result;
}
