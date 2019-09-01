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

#include "TestData.h"

#include "ParseArgs.h"

class CommonReadTest : public ::testing::Test
{
public:
    CommonReadTest() = default;
};

#ifdef ADIOS2_HAVE_MPI
MPI_Comm testComm;
#endif

// ADIOS2 Common read
TEST_F(CommonReadTest, ADIOS2CommonRead1D8)
{
    // Each process would write a 1x8 array and all processes would
    // form a mpiSize * Nx 1D array
    int mpiRank = 0, mpiSize = 1;

    int TimeGapDetected = 0;
#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(testComm, &mpiRank);
    MPI_Comm_size(testComm, &mpiSize);
#endif

    // Write test data using ADIOS2

#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(testComm, adios2::DebugON);
#else
    adios2::ADIOS adios(true);
#endif
    adios2::IO io = adios.DeclareIO("TestIO");

    // Create the Engine
    io.SetEngine(engine);
    io.SetParameters(engineParams);

    adios2::Engine engine = io.Open(fname, adios2::Mode::Read);

    unsigned int t = 0;

    std::vector<std::time_t> write_times;

    while (engine.BeginStep() == adios2::StepStatus::OK)
    {
        const size_t currentStep = engine.CurrentStep();
        EXPECT_EQ(currentStep, static_cast<size_t>(t));

        size_t writerSize;

        auto scalar_r64 = io.InquireVariable<double>("scalar_r64");
        EXPECT_TRUE(scalar_r64);

        auto var_time = io.InquireVariable<int64_t>("time");
        EXPECT_TRUE(var_time);
        ASSERT_EQ(var_time.ShapeID(), adios2::ShapeID::GlobalArray);

        writerSize = var_time.Shape()[0];

        //	std::cout << "Writer size is " << writerSize << std::endl;

        int rankToRead = mpiRank;
        if (writerSize < static_cast<size_t>(mpiSize))
        {
            rankToRead = mpiRank % writerSize;
        }

        auto var_i8 = io.InquireVariable<int8_t>("i8");
        EXPECT_TRUE(var_i8);

        ASSERT_EQ(var_i8.ShapeID(), adios2::ShapeID::LocalArray);

        auto var_i16 = io.InquireVariable<int16_t>("i16");
        EXPECT_TRUE(var_i16);
        ASSERT_EQ(var_i16.ShapeID(), adios2::ShapeID::LocalArray);

        auto var_i32 = io.InquireVariable<int32_t>("i32");
        EXPECT_TRUE(var_i32);
        ASSERT_EQ(var_i32.ShapeID(), adios2::ShapeID::LocalArray);

        auto var_i64 = io.InquireVariable<int64_t>("i64");
        EXPECT_TRUE(var_i64);
        ASSERT_EQ(var_i64.ShapeID(), adios2::ShapeID::LocalArray);

        auto var_r32 = io.InquireVariable<float>("r32");
        EXPECT_TRUE(var_r32);
        ASSERT_EQ(var_r32.ShapeID(), adios2::ShapeID::LocalArray);

        auto var_r64 = io.InquireVariable<double>("r64");
        EXPECT_TRUE(var_r64);
        ASSERT_EQ(var_r64.ShapeID(), adios2::ShapeID::LocalArray);

        auto var_c32 = io.InquireVariable<std::complex<float>>("c32");
        auto var_c64 = io.InquireVariable<std::complex<double>>("c64");
        auto var_r64_2d = io.InquireVariable<double>("r64_2d");
        auto var_r64_2d_rev = io.InquireVariable<double>("r64_2d_rev");
        if (var_c32)
        {
            EXPECT_TRUE(var_c32);
            ASSERT_EQ(var_c32.ShapeID(), adios2::ShapeID::LocalArray);

            EXPECT_TRUE(var_c64);
            ASSERT_EQ(var_c64.ShapeID(), adios2::ShapeID::LocalArray);

            EXPECT_TRUE(var_r64_2d);
            ASSERT_EQ(var_r64_2d.ShapeID(), adios2::ShapeID::LocalArray);
            ASSERT_EQ(var_r64_2d.Count()[0], Nx);
            ASSERT_EQ(var_r64_2d.Count()[1], 2);

            EXPECT_TRUE(var_r64_2d_rev);
            ASSERT_EQ(var_r64_2d_rev.ShapeID(), adios2::ShapeID::LocalArray);
            ASSERT_EQ(var_r64_2d_rev.Count()[0], 2);
            ASSERT_EQ(var_r64_2d_rev.Count()[1], Nx);
        }
        else
        {
            EXPECT_FALSE(var_c32);
            EXPECT_FALSE(var_c64);
            EXPECT_FALSE(var_r64_2d);
            EXPECT_FALSE(var_r64_2d_rev);
        }

        long unsigned int hisStart = rankToRead * (int)Nx;
        long unsigned int hisLength = (long unsigned int)Nx;

        var_i8.SetBlockSelection(rankToRead);
        var_i16.SetBlockSelection(rankToRead);
        var_i32.SetBlockSelection(rankToRead);
        var_i64.SetBlockSelection(rankToRead);

        var_r32.SetBlockSelection(rankToRead);
        var_r64.SetBlockSelection(rankToRead);
        if (var_c32)
            var_c32.SetBlockSelection(rankToRead);
        if (var_c64)
            var_c64.SetBlockSelection(rankToRead);
        if (var_r64_2d)
            var_r64_2d.SetBlockSelection(rankToRead);
        if (var_r64_2d_rev)
            var_r64_2d_rev.SetBlockSelection(rankToRead);

        const adios2::Dims start_time{0};
        const adios2::Dims count_time{1};
        const adios2::Box<adios2::Dims> sel_time(start_time, count_time);
        var_time.SetSelection(sel_time);

        in_I8.resize(hisLength);
        in_I16.resize(hisLength);
        in_I32.resize(hisLength);
        in_I64.resize(hisLength);
        in_R32.resize(hisLength);
        in_R64.resize(hisLength);
        in_C32.resize(hisLength);
        in_C64.resize(hisLength);
        in_R64_2d.resize(hisLength * 2);
        in_R64_2d_rev.resize(hisLength * 2);
        engine.Get(var_i8, in_I8.data());
        engine.Get(var_i16, in_I16.data());
        engine.Get(var_i32, in_I32.data());
        engine.Get(var_i64, in_I64.data());

        engine.Get(scalar_r64, in_scalar_R64);

        engine.Get(var_r32, in_R32.data());
        engine.Get(var_r64, in_R64.data());
        if (var_c32)
            engine.Get(var_c32, in_C32.data());
        if (var_c64)
            engine.Get(var_c64, in_C64.data());
        if (var_r64_2d)
            engine.Get(var_r64_2d, in_R64_2d.data());
        if (var_r64_2d_rev)
            engine.Get(var_r64_2d_rev, in_R64_2d_rev.data());
        std::time_t write_time;
        engine.Get(var_time, (int64_t *)&write_time);
        engine.EndStep();

        EXPECT_EQ(validateCommonTestData(hisStart, hisLength, t, !var_c32), 0);
        write_times.push_back(write_time);
        ++t;
    }

    if ((write_times.back() - write_times.front()) > 1)
    {
        TimeGapDetected++;
    }

    if (!IgnoreTimeGap)
    {
        if (TimeGapExpected)
        {
            EXPECT_TRUE(TimeGapDetected);
        }
        else
        {
            EXPECT_FALSE(TimeGapDetected);
        }
    }

    EXPECT_EQ(t, NSteps);

    // Close the file
    engine.Close();
}

//******************************************************************************
// main
//******************************************************************************

int main(int argc, char **argv)
{
#ifdef ADIOS2_HAVE_MPI
    MPI_Init(nullptr, nullptr);

    int key;
    MPI_Comm_rank(MPI_COMM_WORLD, &key);

    const unsigned int color = 2;
    MPI_Comm_split(MPI_COMM_WORLD, color, key, &testComm);
#endif

    int result;
    ::testing::InitGoogleTest(&argc, argv);

    ParseArgs(argc, argv);

    result = RUN_ALL_TESTS();

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return result;
}
