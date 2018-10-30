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

void BPSteps1D(const size_t ghostCells)
{
    const std::string fname("BPSteps1D_" + std::to_string(ghostCells) + ".bp");

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const size_t Nx = 10;

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
        adios2::IO io = adios.DeclareIO("WriteIO");

        const adios2::Dims shape{static_cast<size_t>(Nx * mpiSize)};
        const adios2::Dims start{static_cast<size_t>(Nx * mpiRank)};
        const adios2::Dims count{Nx};

        auto var_i8 = io.DefineVariable<int8_t>("i8", shape, start, count);
        auto var_i16 = io.DefineVariable<int16_t>("i16", shape, start, count);
        auto var_i32 = io.DefineVariable<int32_t>("i32", shape, start, count);
        auto var_i64 = io.DefineVariable<int64_t>("i64", shape, start, count);
        auto var_r32 = io.DefineVariable<int64_t>("r32", shape, start, count);
        auto var_r64 = io.DefineVariable<int64_t>("r64", shape, start, count);

        var_i8.SetMemoryStart({ghostCells});

        adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);

        for (auto i = 0; i < NSteps; ++i)
        {
            std::vector<int8_t> dataI8(Nx + 2 * ghostCells,
                                       static_cast<int8_t>(i));
            dataI8[0] = -1;
            dataI8[Nx + 1] = -1;

            bpWriter.BeginStep();
            bpWriter.Put(var_i8, dataI8.data(), adios2::Mode::Sync);
            bpWriter.EndStep();
        }
        bpWriter.Close();
    }
}

void BPWriteMemorySelectionRead1D10(const size_t ghostCells)
{
    const std::string fname("BPWriteMemSelRead1D10_" +
                            std::to_string(ghostCells) + ".bp");

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const size_t Nx = 10 - ghostCells;

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
        adios2::IO io = adios.DeclareIO("WriteIO");

        const adios2::Dims shape{static_cast<size_t>(Nx * mpiSize) +
                                 2 * ghostCells};
        const adios2::Dims start{static_cast<size_t>(Nx * mpiRank)};
        const adios2::Dims count{Nx};

        auto var_i8 = io.DefineVariable<int8_t>("i8", shape, start, count);
        auto var_i16 = io.DefineVariable<int16_t>("i16", shape, start, count);
        auto var_i32 = io.DefineVariable<int32_t>("i32", shape, start, count);
        auto var_i64 = io.DefineVariable<int64_t>("i64", shape, start, count);
        auto var_u8 = io.DefineVariable<uint8_t>("u8", shape, start, count);
        auto var_u16 = io.DefineVariable<uint16_t>("u16", shape, start, count);
        auto var_u32 = io.DefineVariable<uint32_t>("u32", shape, start, count);
        auto var_u64 = io.DefineVariable<uint64_t>("u64", shape, start, count);
        auto var_r32 = io.DefineVariable<float>("r32", shape, start, count);
        auto var_r64 = io.DefineVariable<double>("r64", shape, start, count);
        auto var_cr32 =
            io.DefineVariable<std::complex<float>>("cr32", shape, start, count);
        auto var_cr64 = io.DefineVariable<std::complex<double>>("cr64", shape,
                                                                start, count);

        const adios2::Dims memoryStart{ghostCells};
        var_i8.SetMemoryStart(memoryStart);
        var_i16.SetMemoryStart(memoryStart);
        var_i32.SetMemoryStart(memoryStart);
        var_i64.SetMemoryStart(memoryStart);
        var_u8.SetMemoryStart(memoryStart);
        var_u16.SetMemoryStart(memoryStart);
        var_u32.SetMemoryStart(memoryStart);
        var_u64.SetMemoryStart(memoryStart);
        var_r32.SetMemoryStart(memoryStart);
        var_r64.SetMemoryStart(memoryStart);
        var_cr32.SetMemoryStart(memoryStart);
        var_cr64.SetMemoryStart(memoryStart);

        adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);

        SmallTestData testData;

        for (size_t step = 0; step < NSteps; ++step)
        {
            // Generate test data for each process uniquely
            SmallTestData currentTestData = generateNewSmallTestData(
                testData, static_cast<int>(step), mpiRank, mpiSize);

            bpWriter.BeginStep();
            bpWriter.Put(var_i8, currentTestData.I8.data(), adios2::Mode::Sync);
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

    if (false)
    {
        adios2::IO io = adios.DeclareIO("ReadIO");

        adios2::Engine bpReader = io.Open(fname, adios2::Mode::Read);

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

        SmallTestData testData;

        while (bpReader.BeginStep() == adios2::StepStatus::OK)
        {
            const size_t t = bpReader.CurrentStep();

            // Generate test data for each rank uniquely
            SmallTestData currentTestData = generateNewSmallTestData(
                testData, static_cast<int>(t), mpiRank, mpiSize);

            bpReader.Get(var_i8, I8);
            bpReader.Get(var_i16, I16);
            bpReader.Get(var_i32, I32);
            bpReader.Get(var_i64, I64);

            bpReader.Get(var_u8, U8);
            bpReader.Get(var_u16, U16);
            bpReader.Get(var_u32, U32);
            bpReader.Get(var_u64, U64);

            bpReader.Get(var_r32, R32);
            bpReader.Get(var_r64, R64);

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

class BPWriteMemSelReadVector : public ::testing::TestWithParam<size_t>
{
public:
    BPWriteMemSelReadVector() = default;
    virtual void SetUp() {}
    virtual void TearDown() {}
};

TEST_P(BPWriteMemSelReadVector, BPMemorySelectionSteps1D)
{
    BPSteps1D(GetParam());
}

// TEST_P(BPWriteMemSelReadVector, BPWriteMemorySelectionRead1D10)
//{
//    BPWriteMemorySelectionRead1D10(GetParam());
//}

INSTANTIATE_TEST_CASE_P(ghostCells, BPWriteMemSelReadVector,
                        ::testing::Values(1));

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
