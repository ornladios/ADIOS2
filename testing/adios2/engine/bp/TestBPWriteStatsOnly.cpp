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

std::string engineName;       // comes from command line
std::string engineParameters; // comes from command line

class BPWriteStatsOnly : public ::testing::Test
{
public:
    BPWriteStatsOnly() = default;

    SmallTestData m_TestData;
};

//******************************************************************************
// Write only stats without data
//******************************************************************************

TEST_F(BPWriteStatsOnly, BPReadException)
{
    int mpiRank = 0, mpiSize = 1;
    const size_t Nx = 8;

#if ADIOS2_USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    const std::string fname("ADIOS2BPWriteReadStatsOnly_mpi.bp");
#else
    const std::string fname("ADIOS2BPWriteReadStatsOnly.bp");
#endif

#if ADIOS2_USE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    adios2::ADIOS adios;
#endif
    {
        adios2::IO io = adios.DeclareIO("WriteIO");
        const adios2::Dims shape{static_cast<size_t>(Nx * mpiSize)};
        const adios2::Dims start{static_cast<size_t>(Nx * mpiRank)};
        const adios2::Dims count{Nx};

        auto r32 = io.DefineVariable<float>("r32", shape, start, count, adios2::ConstantDims);
        r32.StoreStatsOnly(true);
        adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);

        SmallTestData currentTestData = generateNewSmallTestData(m_TestData, 0, mpiRank, mpiSize);

        bpWriter.BeginStep();
        bpWriter.Put<float>(r32, currentTestData.R32.data());
        bpWriter.EndStep();
        bpWriter.Close();
    }

    {
        adios2::IO io = adios.DeclareIO("ReadIO");

        adios2::Engine bpReader = io.Open(fname, adios2::Mode::Read);

        bpReader.BeginStep();
        std::array<float, Nx> R32;
        auto var_r32 = io.InquireVariable<float>("r32");
        EXPECT_TRUE(var_r32);
        ASSERT_EQ(var_r32.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r32.Steps(), 1);
        ASSERT_EQ(var_r32.Shape()[0], mpiSize * Nx);
        ASSERT_THROW(bpReader.Get(var_r32, R32.data(), adios2::Mode::Sync), std::runtime_error);
        bpReader.Close();
    }
}

//******************************************************************************
// main
//******************************************************************************

int main(int argc, char **argv)
{
#if ADIOS2_USE_MPI
    int provided;

    // MPI_THREAD_MULTIPLE is only required if you enable the SST MPI_DP
    MPI_Init_thread(nullptr, nullptr, MPI_THREAD_MULTIPLE, &provided);
#endif

    int result;
    ::testing::InitGoogleTest(&argc, argv);

    if (argc > 1)
    {
        engineName = std::string(argv[1]);
    }
    if (argc > 2)
    {
        engineParameters = std::string(argv[2]);
    }
    result = RUN_ALL_TESTS();

#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif

    return result;
}
