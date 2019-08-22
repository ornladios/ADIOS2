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
    /* we won't share IOs on the reader side */
    adios2::IO io1 = adios.DeclareIO("TestIO");
    adios2::IO io2 = adios.DeclareIO("TestIO2");

    // Create the Engines
    io1.SetEngine(engine);
    io1.SetParameters(engineParams);
    io2.SetEngine(engine);
    io2.SetParameters(engineParams);

    std::string fname1 = fname + "1";
    std::string fname2 = fname + "2";

    adios2::Engine engine1 = io1.Open(fname1, adios2::Mode::Read);
    adios2::Engine engine2 = io2.Open(fname2, adios2::Mode::Read);

    unsigned int t = 0;

    std::vector<std::time_t> write_times;

    std::string varname1 = "r64";
    std::string varname2 = "r64_2";

    if (SharedVar)
    {
        varname2 = "r64";
    }
    while (engine1.BeginStep() == adios2::StepStatus::OK &&
           engine2.BeginStep() == adios2::StepStatus::OK)
    {
        size_t writerSize;

        auto var1 = io1.InquireVariable<double>(varname1);
        auto var2 = io2.InquireVariable<double>(varname2);

        EXPECT_TRUE(var1);
        EXPECT_TRUE(var2);
        ASSERT_EQ(var1.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var2.ShapeID(), adios2::ShapeID::GlobalArray);

        /* take the first size as something that gives us writer size */
        writerSize = var1.Shape()[0] / 10 - 1;

        ASSERT_EQ(var2.Shape()[0], (writerSize + 1) * Nx);

        long unsigned int myStart =
            (long unsigned int)((writerSize + 1) * Nx / mpiSize) * mpiRank;
        long unsigned int myLength =
            (long unsigned int)(((writerSize + 1) * Nx + mpiSize - 1) /
                                mpiSize);

        if (myStart + myLength > writerSize * Nx)
        {
            myLength = (long unsigned int)(writerSize + 1) * (int)Nx - myStart;
        }
        const adios2::Dims start{myStart};
        const adios2::Dims count{myLength};
        std::vector<double> in_R64_1;
        std::vector<double> in_R64_2;

        const adios2::Box<adios2::Dims> sel(start, count);

        var1.SetSelection(sel);
        var2.SetSelection(sel);

        in_R64_1.resize(myLength);
        in_R64_2.resize(myLength);
        engine1.Get(var1, in_R64_1.data());
        engine2.Get(var2, in_R64_2.data());
        engine1.EndStep();
        engine2.EndStep();

        EXPECT_EQ(validateSimpleForwardData(in_R64_1, 0, myStart, myLength,
                                            (writerSize + 1) * Nx),
                  0);
        EXPECT_EQ(validateSimpleReverseData(in_R64_2, 0, myStart, myLength,
                                            (writerSize + 1) * Nx),
                  0);

        ++t;
    }

    // Close the file
    engine1.Close();
    engine2.Close();
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
