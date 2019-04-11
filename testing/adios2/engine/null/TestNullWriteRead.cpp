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

class NullWriteReadTests : public ::testing::Test
{
public:
    NullWriteReadTests() = default;

    SmallTestData m_TestData;
};

TEST_F(NullWriteReadTests, NullWriteRead1D8)
{
    const std::string fname("NullWriteRead1D8.bp");

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const size_t Nx = 8;

    // Number of steps
    const size_t NSteps = 3;

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
        adios2::IO io = adios.DeclareIO("WriteNull");

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

        io.SetEngine("NULL");

        adios2::Engine nullWriter = io.Open(fname, adios2::Mode::Write);

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

            nullWriter.BeginStep();
            nullWriter.Put(var_iString, currentTestData.S1);
            nullWriter.Put(var_i8, currentTestData.I8.data());
            nullWriter.Put(var_i16, currentTestData.I16.data());
            nullWriter.Put(var_i32, currentTestData.I32.data());
            nullWriter.Put(var_i64, currentTestData.I64.data());
            nullWriter.Put(var_u8, currentTestData.U8.data());
            nullWriter.Put(var_u16, currentTestData.U16.data());
            nullWriter.Put(var_u32, currentTestData.U32.data());
            nullWriter.Put(var_u64, currentTestData.U64.data());
            nullWriter.Put(var_r32, currentTestData.R32.data());
            nullWriter.Put(var_r64, currentTestData.R64.data());
            nullWriter.PerformPuts();
            nullWriter.EndStep();
        }

        nullWriter.Close();
    }

    {
        adios2::IO io = adios.DeclareIO("ReadIO");
        io.SetEngine("NULL");

        adios2::Engine nullReader = io.Open(fname, adios2::Mode::Read);

        std::string IString;
        std::vector<int8_t> I8;
        std::vector<int16_t> I16;
        std::vector<int32_t> I32;
        std::vector<int64_t> I64;
        std::vector<uint8_t> U8;
        std::vector<uint16_t> U16;
        std::vector<uint32_t> U32;
        std::vector<uint64_t> U64;
        std::vector<float> R32;
        std::vector<double> R64;

        EXPECT_EQ(I8.empty(), true);

        for (size_t t = 0; t < NSteps; ++t)
        {
            const adios2::StepStatus status = nullReader.BeginStep();
            EXPECT_EQ(status, adios2::StepStatus::EndOfStream);

            const size_t currentStep = nullReader.CurrentStep();
            EXPECT_EQ(currentStep, adios2::MaxSizeT);

            auto var_iString = io.InquireVariable<std::string>("iString");
            EXPECT_FALSE(var_iString);

            auto var_i8 = io.InquireVariable<int8_t>("i8");
            auto var_i8_info = nullReader.BlocksInfo(var_i8, currentStep);
            EXPECT_FALSE(var_i8);
            EXPECT_EQ(var_i8_info.empty(), true);

            auto var_i16 = io.InquireVariable<int16_t>("i16");
            auto var_i16_info = nullReader.BlocksInfo(var_i8, currentStep);
            EXPECT_FALSE(var_i16);
            EXPECT_EQ(var_i16_info.empty(), true);

            auto var_i32 = io.InquireVariable<int32_t>("i32");
            EXPECT_FALSE(var_i32);

            auto var_i64 = io.InquireVariable<int64_t>("i64");
            EXPECT_FALSE(var_i64);

            auto var_u8 = io.InquireVariable<uint8_t>("u8");
            EXPECT_FALSE(var_u8);

            auto var_u16 = io.InquireVariable<uint16_t>("u16");
            EXPECT_FALSE(var_u16);

            auto var_u32 = io.InquireVariable<uint32_t>("u32");
            EXPECT_FALSE(var_u32);

            auto var_u64 = io.InquireVariable<uint64_t>("u64");
            EXPECT_FALSE(var_u64);

            auto var_r32 = io.InquireVariable<float>("r32");
            EXPECT_FALSE(var_r32);

            auto var_r64 = io.InquireVariable<double>("r64");
            EXPECT_FALSE(var_r64);

            nullReader.Get(var_iString, IString);

            nullReader.Get(var_i8, I8.data());
            nullReader.Get(var_i16, I16.data());
            nullReader.Get(var_i32, I32.data());
            nullReader.Get(var_i64, I64.data());

            nullReader.Get(var_u8, U8.data());
            nullReader.Get(var_u16, U16.data());
            nullReader.Get(var_u32, U32.data());
            nullReader.Get(var_u64, U64.data());

            nullReader.Get(var_r32, R32.data());
            nullReader.Get(var_r64, R64.data());

            nullReader.PerformGets();
            nullReader.EndStep();

            EXPECT_TRUE(I8.empty());
            EXPECT_TRUE(I16.empty());
            EXPECT_TRUE(I32.empty());
            EXPECT_TRUE(I64.empty());
            EXPECT_TRUE(U8.empty());
            EXPECT_TRUE(U16.empty());
            EXPECT_TRUE(U32.empty());
            EXPECT_TRUE(U64.empty());
            EXPECT_TRUE(R32.empty());
            EXPECT_TRUE(R64.empty());
        }
        nullReader.Close();
    }
}

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
