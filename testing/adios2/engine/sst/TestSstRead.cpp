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

class SstReadTest : public ::testing::Test
{
public:
    SstReadTest() = default;

    SmallTestData m_TestData;
    SmallTestData m_OriginalData;
    std::array<double, 20> Original_R64_2d = {{0, 1, 2, 3, 4, 5, 6, 7, 1000,
                                               1001, 1002, 1003, 1004, 1005,
                                               1006, 1007}};
};

//******************************************************************************
// 1D 1x8 test data
//******************************************************************************

// ADIOS2 Sst read
TEST_F(SstReadTest, ADIOS2SstRead1D8)
{
    // Each process would write a 1x8 array and all processes would
    // form a mpiSize * Nx 1D array
    const std::string fname = "ADIOS2Sst";

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const std::size_t Nx = 8;

    // Number of steps
    const std::size_t NSteps = 1;

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
    adios2::IO io = adios.DeclareIO("TestIO");

    // Create the Engine
    io.SetEngine("Sst");

    adios2::Engine engine = io.Open(fname, adios2::Mode::Read);

    unsigned int t = 0;

    while (engine.BeginStep() == adios2::StepStatus::OK)
    {
        const size_t currentStep = engine.CurrentStep();
        EXPECT_EQ(currentStep, static_cast<size_t>(t));

        auto var_i8 = io.InquireVariable<int8_t>("i8");
        EXPECT_TRUE(var_i8);
        ASSERT_EQ(var_i8.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i8.Shape()[0], mpiSize * Nx);

        auto var_i16 = io.InquireVariable<int16_t>("i16");
        EXPECT_TRUE(var_i16);
        ASSERT_EQ(var_i16.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i16.Shape()[0], mpiSize * Nx);

        auto var_i32 = io.InquireVariable<int32_t>("i32");
        EXPECT_TRUE(var_i32);
        ASSERT_EQ(var_i32.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i32.Shape()[0], mpiSize * Nx);

        auto var_i64 = io.InquireVariable<int64_t>("i64");
        EXPECT_TRUE(var_i64);
        ASSERT_EQ(var_i64.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i64.Shape()[0], mpiSize * Nx);

        auto var_r32 = io.InquireVariable<float>("r32");
        EXPECT_TRUE(var_r32);
        ASSERT_EQ(var_r32.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r32.Shape()[0], mpiSize * Nx);

        auto var_r64 = io.InquireVariable<double>("r64");
        EXPECT_TRUE(var_r64);
        ASSERT_EQ(var_r64.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r64.Shape()[0], mpiSize * Nx);

        auto var_r64_2d = io.InquireVariable<double>("r64_2d");
        ASSERT_NE(var_r64_2d, nullptr);
        ASSERT_EQ(var_r64_2d->m_ShapeID, adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r64_2d->m_Shape[0], mpiSize * Nx);
        ASSERT_EQ(var_r64_2d->m_Shape[1], 2);

        std::string IString;
        std::array<int8_t, Nx> I8;
        std::array<int16_t, Nx> I16;
        std::array<int32_t, Nx> I32;
        std::array<int64_t, Nx> I64;
        std::array<float, Nx> R32;
        std::array<double, Nx> R64;
        std::array<double, Nx * 2> R64_2d;

        const adios2::Dims start{mpiRank * Nx};
        const adios2::Dims count{Nx};
        const adios2::Dims start2{mpiRank * Nx, 0};
        const adios2::Dims count2{Nx, 2};

        const adios2::Box<adios2::Dims> sel(start, count);
        const adios2::Box<adios2::Dims> sel2(start2, count2);

        var_i8.SetSelection(sel);
        var_i16.SetSelection(sel);
        var_i32.SetSelection(sel);
        var_i64.SetSelection(sel);

        var_r32.SetSelection(sel);
        var_r64.SetSelection(sel);
        var_r64_2d.SetSelection(sel2);

        engine.Get(var_i8, I8.data());
        engine.Get(var_i16, I16.data());
        engine.Get(var_i32, I32.data());
        engine.Get(var_i64, I64.data());

        engine.Get(var_r32, R32.data());
        engine.Get(var_r64, R64.data());
        engine.Get(var_r64_2d, R64_2d.data());

        engine.EndStep();

        UpdateSmallTestData(m_OriginalData, static_cast<int>(t), mpiRank,
                            mpiSize);
        int j = mpiRank + 1 + t * mpiSize;
        std::for_each(Original_R64_2d.begin(), Original_R64_2d.end(),
                      [&](double &v) { v += j; });

        for (size_t i = 0; i < Nx; ++i)
        {
            std::stringstream ss;
            ss << "t=" << t << " i=" << i << " rank=" << mpiRank;
            std::string msg = ss.str();

            EXPECT_EQ(I8[i], m_OriginalData.I8[i]) << msg;
            EXPECT_EQ(I16[i], m_OriginalData.I16[i]) << msg;
            EXPECT_EQ(I32[i], m_OriginalData.I32[i]) << msg;
            EXPECT_EQ(I64[i], m_OriginalData.I64[i]) << msg;
            EXPECT_EQ(R32[i], m_OriginalData.R32[i]) << msg;
            EXPECT_EQ(R64[i], m_OriginalData.R64[i]) << msg;
            EXPECT_EQ(R64_2d[i], Original_R64_2d[i]) << msg;
            EXPECT_EQ(R64_2d[i + 8], Original_R64_2d[i + 8]) << msg;
        }
        ++t;
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
#endif

    int result;
    ::testing::InitGoogleTest(&argc, argv);
    result = RUN_ALL_TESTS();

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return result;
}
