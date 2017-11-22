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

class SstWriteTest : public ::testing::Test
{
public:
    SstWriteTest() = default;

    SmallTestData m_TestData;
};

//******************************************************************************
// 1D 1x8 test data
//******************************************************************************

// ADIOS2 SST write
TEST_F(SstWriteTest, ADIOS2SstWrite)
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
    io.SetEngine("SstWriter");

    adios2::Engine &engine = io.Open(fname, adios2::Mode::Write);

    for (size_t step = 0; step < NSteps; ++step)
    {
        // Generate test data for each process uniquely
        SmallTestData currentTestData =
            generateNewSmallTestData(m_TestData, step, mpiRank, mpiSize);

        // Retrieve the variables that previously went out of scope
        auto &var_i8 = *io.InquireVariable<int8_t>("i8");
        auto &var_i16 = *io.InquireVariable<int16_t>("i16");
        auto &var_i32 = *io.InquireVariable<int32_t>("i32");
        auto &var_i64 = *io.InquireVariable<int64_t>("i64");
        auto &var_u8 = *io.InquireVariable<uint8_t>("u8");
        auto &var_u16 = *io.InquireVariable<uint16_t>("u16");
        auto &var_u32 = *io.InquireVariable<uint32_t>("u32");
        auto &var_u64 = *io.InquireVariable<uint64_t>("u64");
        auto &var_r32 = *io.InquireVariable<float>("r32");
        auto &var_r64 = *io.InquireVariable<double>("r64");

        // Make a 1D selection to describe the local dimensions of the
        // variable we write and its offsets in the global spaces
        adios2::Box<adios2::Dims> sel({mpiRank * Nx}, {Nx});
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

        // Write each one
        // fill in the variable with values from starting index to
        // starting index + count
        // engine.BeginStep();
        engine.PutSync(var_i8, currentTestData.I8.data());
        engine.PutSync(var_i16, currentTestData.I16.data());
        engine.PutSync(var_i32, currentTestData.I32.data());
        engine.PutSync(var_i64, currentTestData.I64.data());
        engine.PutSync(var_u8, currentTestData.U8.data());
        engine.PutSync(var_u16, currentTestData.U16.data());
        engine.PutSync(var_u32, currentTestData.U32.data());
        engine.PutSync(var_u64, currentTestData.U64.data());
        engine.PutSync(var_r32, currentTestData.R32.data());
        engine.PutSync(var_r64, currentTestData.R64.data());
        // Advance to the next time step
        engine.EndStep();
    }

    // Close the file
    engine.Close();
}

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
