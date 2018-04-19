/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include <cstdint>
#include <cstring>

#include <iostream>
#include <stdexcept>

#include <gtest/gtest.h>

#include "SmallTestData.h"

#include <adios2_cxx98.h>

class BPWriteReadTestADIOS2_cxx98 : public ::testing::Test
{
public:
    BPWriteReadTestADIOS2_cxx98() = default;

    SmallTestData m_TestData;
};

//******************************************************************************
// 1D 1x8 test data
//******************************************************************************

// ADIOS2 BP write, native ADIOS1 read
TEST_F(BPWriteReadTestADIOS2_cxx98, ADIOS2BPWriteRead1D8)
{
    // Each process would write a 1x8 array and all processes would
    // form a mpiSize * Nx 1D array
    const std::string fname("ADIOS2BPWriteRead1D8_cxx98.bp");

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
    adios2::cxx98::ADIOS adios(MPI_COMM_WORLD);
#else
    adios2::cxx98::ADIOS adios(true);
#endif
    {
        adios2::cxx98::IO io = adios.DeclareIO("TestIO");

        // Declare 1D variables (NumOfProcesses * Nx)
        // The local process' part (start, count) can be defined now or later
        // before Write().
        {
            const adios2::cxx98::Dims shape{static_cast<size_t>(Nx * mpiSize)};
            const adios2::cxx98::Dims start{static_cast<size_t>(Nx * mpiRank)};
            const adios2::cxx98::Dims count{Nx};

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

        // QUESTION: It seems that BPFilterWriter cannot overwrite existing
        // files
        // Ex. if you tune Nx and NSteps, the test would fail. But if you clear
        // the cache in
        // ${adios2Build}/testing/adios2/engine/bp/ADIOS2BPWriteADIOS1Read1D8.bp.dir,
        // then it works
        adios2::cxx98::Engine bpWriter = io.Open(fname, adios2::cxx98::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            // Generate test data for each process uniquely
            SmallTestData currentTestData = generateNewSmallTestData(
                m_TestData, static_cast<int>(step), mpiRank, mpiSize);

            // Retrieve the variables that previously went out of scope
            adios2::cxx98::Variable<std::string> var_iString =
                io.InquireVariable<std::string>("iString");
            adios2::cxx98::Variable<int8_t> var_i8 =
                io.InquireVariable<int8_t>("i8");
            adios2::cxx98::Variable<int16_t> var_i16 =
                io.InquireVariable<int16_t>("i16");
            adios2::cxx98::Variable<int32_t> var_i32 =
                io.InquireVariable<int32_t>("i32");
            adios2::cxx98::Variable<int64_t> var_i64 =
                io.InquireVariable<int64_t>("i64");

            adios2::cxx98::Variable<uint8_t> var_u8 =
                io.InquireVariable<uint8_t>("u8");
            adios2::cxx98::Variable<uint16_t> var_u16 =
                io.InquireVariable<uint16_t>("u16");
            adios2::cxx98::Variable<uint32_t> var_u32 =
                io.InquireVariable<uint32_t>("u32");
            adios2::cxx98::Variable<uint64_t> var_u64 =
                io.InquireVariable<uint64_t>("u64");
            adios2::cxx98::Variable<float> var_r32 =
                io.InquireVariable<float>("r32");
            adios2::cxx98::Variable<double> var_r64 =
                io.InquireVariable<double>("r64");

            // Make a 1D selection to describe the local dimensions of the
            // variable we write and its offsets in the global spaces
            adios2::cxx98::Dims start(1);
            start[0] = mpiRank * Nx;

            adios2::cxx98::Dims count(1);
            count[0] = Nx;

            EXPECT_THROW(var_iString.SetSelection(start, count),
                         std::invalid_argument);
            var_i8.SetSelection(start, count);
            var_i16.SetSelection(start, count);
            var_i32.SetSelection(start, count);
            var_i64.SetSelection(start, count);
            var_u8.SetSelection(start, count);
            var_u16.SetSelection(start, count);
            var_u32.SetSelection(start, count);
            var_u64.SetSelection(start, count);
            var_r32.SetSelection(start, count);
            var_r64.SetSelection(start, count);

            // Write each one
            // fill in the variable with values from starting index to
            // starting index + count
            bpWriter.BeginStep();

            // bpWriter.PutDeferred(var_iString, currentTestData.S1);
            bpWriter.PutDeferred(var_i8, currentTestData.I8.data());
            bpWriter.PutDeferred(var_i16, currentTestData.I16.data());
            bpWriter.PutDeferred(var_i32, currentTestData.I32.data());
            bpWriter.PutDeferred(var_i64, currentTestData.I64.data());
            bpWriter.PutDeferred(var_u8, currentTestData.U8.data());
            bpWriter.PutDeferred(var_u16, currentTestData.U16.data());
            bpWriter.PutDeferred(var_u32, currentTestData.U32.data());
            bpWriter.PutDeferred(var_u64, currentTestData.U64.data());
            bpWriter.PutDeferred(var_r32, currentTestData.R32.data());
            bpWriter.PutDeferred(var_r64, currentTestData.R64.data());
            bpWriter.PerformPuts();

            bpWriter.EndStep();
        }

        // Close the file
        bpWriter.Close();
    }

    {
        adios2::cxx98::IO io = adios.DeclareIO("ReadIO");

        adios2::cxx98::Engine bpReader =
            io.Open(fname, adios2::cxx98::Mode::Read);

        //        auto var_iString = io.InquireVariable<std::string>("iString");
        //        ASSERT_NE(var_iString, nullptr);
        //        ASSERT_EQ(var_iString->m_Shape.size(), 0);
        //        ASSERT_EQ(var_iString->m_AvailableStepsCount, NSteps);
        //
        auto var_i8 = io.InquireVariable<int8_t>("i8");
        //        ASSERT_NE(var_i8, nullptr);
        //        ASSERT_EQ(var_i8->m_ShapeID, adios2::ShapeID::GlobalArray);
        //        ASSERT_EQ(var_i8->m_AvailableStepsCount, NSteps);
        //        ASSERT_EQ(var_i8->m_Shape[0], mpiSize * Nx);
        //
        auto var_i16 = io.InquireVariable<int16_t>("i16");
        //        ASSERT_NE(var_i16, nullptr);
        //        ASSERT_EQ(var_i16->m_ShapeID, adios2::ShapeID::GlobalArray);
        //        ASSERT_EQ(var_i16->m_AvailableStepsCount, NSteps);
        //        ASSERT_EQ(var_i16->m_Shape[0], mpiSize * Nx);
        //
        auto var_i32 = io.InquireVariable<int32_t>("i32");
        //        ASSERT_NE(var_i32, nullptr);
        //        ASSERT_EQ(var_i32->m_ShapeID, adios2::ShapeID::GlobalArray);
        //        ASSERT_EQ(var_i32->m_AvailableStepsCount, NSteps);
        //        ASSERT_EQ(var_i32->m_Shape[0], mpiSize * Nx);
        //
        auto var_i64 = io.InquireVariable<int64_t>("i64");
        //        ASSERT_NE(var_i64, nullptr);
        //        ASSERT_EQ(var_i64->m_ShapeID, adios2::ShapeID::GlobalArray);
        //        ASSERT_EQ(var_i64->m_AvailableStepsCount, NSteps);
        //        ASSERT_EQ(var_i64->m_Shape[0], mpiSize * Nx);
        //
        auto var_u8 = io.InquireVariable<uint8_t>("u8");
        //        ASSERT_NE(var_u8, nullptr);
        //        ASSERT_EQ(var_u8->m_ShapeID, adios2::ShapeID::GlobalArray);
        //        ASSERT_EQ(var_u8->m_AvailableStepsCount, NSteps);
        //        ASSERT_EQ(var_u8->m_Shape[0], mpiSize * Nx);
        //
        auto var_u16 = io.InquireVariable<uint16_t>("u16");
        //        ASSERT_NE(var_u16, nullptr);
        //        ASSERT_EQ(var_u16->m_ShapeID, adios2::ShapeID::GlobalArray);
        //        ASSERT_EQ(var_u16->m_AvailableStepsCount, NSteps);
        //        ASSERT_EQ(var_u16->m_Shape[0], mpiSize * Nx);
        //
        auto var_u32 = io.InquireVariable<uint32_t>("u32");
        //        ASSERT_NE(var_u32, nullptr);
        //        ASSERT_EQ(var_u32->m_ShapeID, adios2::ShapeID::GlobalArray);
        //        ASSERT_EQ(var_u32->m_AvailableStepsCount, NSteps);
        //        ASSERT_EQ(var_u32->m_Shape[0], mpiSize * Nx);
        //
        auto var_u64 = io.InquireVariable<uint64_t>("u64");
        //        ASSERT_NE(var_u64, nullptr);
        //        ASSERT_EQ(var_u64->m_ShapeID, adios2::ShapeID::GlobalArray);
        //        ASSERT_EQ(var_u64->m_AvailableStepsCount, NSteps);
        //        ASSERT_EQ(var_u64->m_Shape[0], mpiSize * Nx);
        //
        auto var_r32 = io.InquireVariable<float>("r32");
        //        ASSERT_NE(var_r32, nullptr);
        //        ASSERT_EQ(var_r32->m_ShapeID, adios2::ShapeID::GlobalArray);
        //        ASSERT_EQ(var_r32->m_AvailableStepsCount, NSteps);
        //        ASSERT_EQ(var_r32->m_Shape[0], mpiSize * Nx);
        //
        auto var_r64 = io.InquireVariable<double>("r64");
        //        ASSERT_NE(var_r64, nullptr);
        //        ASSERT_EQ(var_r64->m_ShapeID, adios2::ShapeID::GlobalArray);
        //        ASSERT_EQ(var_r64->m_AvailableStepsCount, NSteps);
        //        ASSERT_EQ(var_r64->m_Shape[0], mpiSize * Nx);

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

        adios2::cxx98::Dims start(1);
        start[0] = mpiRank * Nx;
        adios2::cxx98::Dims count(1);
        count[0] = Nx;

        var_i8.SetSelection(start, count);
        var_i16.SetSelection(start, count);
        var_i32.SetSelection(start, count);
        var_i64.SetSelection(start, count);

        var_u8.SetSelection(start, count);
        var_u16.SetSelection(start, count);
        var_u32.SetSelection(start, count);
        var_u64.SetSelection(start, count);

        var_r32.SetSelection(start, count);
        var_r64.SetSelection(start, count);

        for (size_t t = 0; t < NSteps; ++t)
        {
            var_i8.SetStepSelection(t, 1);
            var_i16.SetStepSelection(t, 1);
            var_i32.SetStepSelection(t, 1);
            var_i64.SetStepSelection(t, 1);

            var_u8.SetStepSelection(t, 1);
            var_u16.SetStepSelection(t, 1);
            var_u32.SetStepSelection(t, 1);
            var_u64.SetStepSelection(t, 1);

            var_r32.SetStepSelection(t, 1);
            var_r64.SetStepSelection(t, 1);

            // Generate test data for each rank uniquely
            SmallTestData currentTestData = generateNewSmallTestData(
                m_TestData, static_cast<int>(t), mpiRank, mpiSize);

            // bpReader.GetSync(*var_iString, IString);

            bpReader.GetDeferred(var_i8, I8.data());
            bpReader.GetDeferred(var_i16, I16.data());
            bpReader.GetDeferred(var_i32, I32.data());
            bpReader.GetDeferred(var_i64, I64.data());

            bpReader.GetDeferred(var_u8, U8.data());
            bpReader.GetDeferred(var_u16, U16.data());
            bpReader.GetDeferred(var_u32, U32.data());
            bpReader.GetDeferred(var_u64, U64.data());

            bpReader.GetDeferred(var_r32, R32.data());
            bpReader.GetDeferred(var_r64, R64.data());

            bpReader.PerformGets();

            // EXPECT_EQ(IString, currentTestData.S1);

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
    result = RUN_ALL_TESTS();

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return result;
}
