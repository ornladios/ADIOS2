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

int validateCommonTestR64(int start, int length, size_t step)
{
    int failures = 0;
    for (int i = 0; i < length; i++)
    {
        if (in_R64[i] != (double)((i + start) * 10 + step))
        {
            std::cout << "Expected " << (double)((i + start) * 10 + step)
                      << ", got " << in_R64[i] << " for in_R64[" << i
                      << "](global[" << i + start << "])" << std::endl;
            failures++;
        }
    }
    return failures;
}

// ADIOS2 Common read
TEST_F(CommonReadTest, ADIOS2CommonRead)
{
    // Each process would write a 1x8 array and all processes would
    // form a mpiSize * Nx 1D array
    int mpiRank = 0, mpiSize = 1;

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

    adios2::Engine engine;

    if (ExpectOpenTimeout)
    {
        ASSERT_ANY_THROW(engine = io.Open(fname, adios2::Mode::Read));
        return;
    }
    else
    {
        engine = io.Open(fname, adios2::Mode::Read);
    }
    unsigned int t = 0;

    while (engine.BeginStep() == adios2::StepStatus::OK)
    {
        const size_t currentStep = engine.CurrentStep();
        EXPECT_EQ(currentStep, static_cast<size_t>(t));

        size_t writerSize;

        auto var_i8 = io.InquireVariable<int8_t>("i8");
        EXPECT_TRUE(var_i8);
        ASSERT_EQ(var_i8.ShapeID(), adios2::ShapeID::GlobalArray);
        /* must be a multiple of Nx */
        ASSERT_EQ(var_i8.Shape()[0] % Nx, 0);

        /* take the first size as something that gives us writer size */
        writerSize = var_i8.Shape()[0] / 10;

        auto var_r64 = io.InquireVariable<double>("r64");
        EXPECT_TRUE(var_r64);
        ASSERT_EQ(var_r64.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r64.Shape()[0], writerSize * Nx);

        long unsigned int myStart =
            (long unsigned int)(writerSize * Nx / mpiSize) * mpiRank;
        long unsigned int myLength =
            (long unsigned int)((writerSize * Nx + mpiSize - 1) / mpiSize);

        if (myStart + myLength > writerSize * Nx)
        {
            myLength = (long unsigned int)writerSize * (int)Nx - myStart;
        }
        const adios2::Dims start{myStart};
        const adios2::Dims count{myLength};

        const adios2::Box<adios2::Dims> sel(start, count);
        var_r64.SetSelection(sel);

        in_R64.resize(myLength);
        engine.Get(var_r64, in_R64.data());
        engine.EndStep();
        EXPECT_EQ(validateCommonTestR64(myStart, myLength, t), 0);

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
