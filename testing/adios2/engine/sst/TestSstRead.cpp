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
};

//******************************************************************************
// 1D 1x8 test data
//******************************************************************************

// ADIOS2 Sst read
TEST_F(SstReadTest, ADIOS2SstRead1D8)
{
    // Each process would write a 1x8 array and all processes would
    // form a mpiSize * Nx 1D array
    const std::string fname = "ADIOS2Sst1D8.sst";

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const std::size_t Nx = 8;

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
    adios2::IO &io = adios.DeclareIO("TestIO");

    // Declare 1D variables (NumOfProcesses * Nx)
    // The local process' part (start, count) can be defined now or later
    // before Write().
    {
        adios2::Dims shape{static_cast<unsigned int>(Nx * mpiSize)};
        adios2::Dims start{static_cast<unsigned int>(Nx * mpiRank)};
        adios2::Dims count{static_cast<unsigned int>(Nx)};
        io.DefineVariable<int8_t>("i8", shape, start, count);
        io.DefineVariable<int16_t>("i16", shape, start, count);
        io.DefineVariable<int32_t>("i32", shape, start, count);
        io.DefineVariable<int64_t>("i64", shape, start, count);
        io.DefineVariable<uint8_t>("u8", shape, start, count);
        io.DefineVariable<uint16_t>("u16", shape, start, count);
        io.DefineVariable<uint32_t>("u32", shape, start, count);
        io.DefineVariable<uint64_t>("u64", shape, start, count);
        io.DefineVariable<float>("r32", shape, start, count);
        io.DefineVariable<double>("r64", shape, start, count);
    }

    // Create the ADIOS 1 Engine
    io.SetEngine("SstReader");

    adios2::Engine &engine = io.Open(fname, adios2::Mode::Read);

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

    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return result;
}
