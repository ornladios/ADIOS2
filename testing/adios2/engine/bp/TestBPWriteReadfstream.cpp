/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include <cstdint>
#include <cstring>

#include <iostream>
#include <stdexcept>

#include <adios2.h>
#include <adios_read.h>

#include <gtest/gtest.h>

#include "../SmallTestData.h"

std::string engineName; // comes from command line

class BPWriteReadTest : public ::testing::Test
{
public:
    BPWriteReadTest() = default;

    SmallTestData m_TestData;
};

//******************************************************************************
// 1D 1x8 test data
//******************************************************************************

// ADIOS2 BP write, native ADIOS1 read
TEST_F(BPWriteReadTest, ADIOS2BPWriteADIOS1Read1D8fstream)
{
    // Each process would write a 1x8 array and all processes would
    // form a mpiSize * Nx 1D array
    std::string fname = "ADIOS2BPWriteADIOS1Read1D8fstream.bp";

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const std::size_t Nx = 8;

    // Number of steps
    const std::size_t NSteps = 3;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

    // Write test data using BP
    {
#ifdef ADIOS2_HAVE_MPI
        adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
        adios2::ADIOS adios(true);
#endif
        adios2::IO io = adios.DeclareIO("TestIO");

        // Declare 1D variables (NumOfProcesses * Nx)
        // The local process' part (start, count) can be defined now or later
        // before Write().
        {
            adios2::Dims shape{static_cast<unsigned int>(Nx * mpiSize)};
            adios2::Dims start{static_cast<unsigned int>(Nx * mpiRank)};
            adios2::Dims count{static_cast<unsigned int>(Nx)};
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

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            // Create the BP Engine
            io.SetEngine("BPFile");
        }
        io.AddTransport("file", {{"Library", "fstream"}});

        // QUESTION: It seems that BPFilterWriter cannot overwrite existing
        // files
        // Ex. if you tune Nx and NSteps, the test would fail. But if you clear
        // the cache in
        // ${adios2Build}/testing/adios2/engine/bp/ADIOS2BPWriteADIOS1Read1D8.bp.dir,
        // then it works
        adios2::Engine engine = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            // Generate test data for each process uniquely
            SmallTestData currentTestData =
                generateNewSmallTestData(m_TestData, step, mpiRank, mpiSize);

            // Retrieve the variables that previously went out of scope
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
            engine.BeginStep();
            engine.Put(var_i8, currentTestData.I8.data());
            engine.Put(var_i16, currentTestData.I16.data());
            engine.Put(var_i32, currentTestData.I32.data());
            engine.Put(var_i64, currentTestData.I64.data());
            engine.Put(var_u8, currentTestData.U8.data());
            engine.Put(var_u16, currentTestData.U16.data());
            engine.Put(var_u32, currentTestData.U32.data());
            engine.Put(var_u64, currentTestData.U64.data());
            engine.Put(var_r32, currentTestData.R32.data());
            engine.Put(var_r64, currentTestData.R64.data());
            engine.EndStep();
        }

        // Close the file
        engine.Close();
    }

    {
        adios_read_init_method(ADIOS_READ_METHOD_BP, MPI_COMM_SELF,
                               "verbose=3");

        // Open the file for reading
        // Note: Since collective metadata generation is not implemented yet,
        // SO for now we read each subfile instead of a single bp file with all
        // metadata.
        // Meanwhile if we open file with MPI_COMM_WORLD, then the selection
        // bounding box should be [0, Nx]
        std::string index = std::to_string(mpiRank);
        ADIOS_FILE *f = adios_read_open_file(
            (fname + ".dir/" + fname + "." + index).c_str(),
            ADIOS_READ_METHOD_BP, MPI_COMM_SELF);
        ASSERT_NE(f, nullptr);

        // Check the variables exist
        ADIOS_VARINFO *var_i8 = adios_inq_var(f, "i8");
        EXPECT_TRUE(var_i8);
        ASSERT_EQ(var_i8->ndim, 1);
        ASSERT_EQ(var_i8->global, 1);
        ASSERT_EQ(var_i8->nsteps, NSteps);
        ASSERT_EQ(var_i8->dims[0], mpiSize * Nx);
        ADIOS_VARINFO *var_i16 = adios_inq_var(f, "i16");
        EXPECT_TRUE(var_i16);
        ASSERT_EQ(var_i16->ndim, 1);
        ASSERT_EQ(var_i16->global, 1);
        ASSERT_EQ(var_i16->nsteps, NSteps);
        ASSERT_EQ(var_i16->dims[0], mpiSize * Nx);
        ADIOS_VARINFO *var_i32 = adios_inq_var(f, "i32");
        EXPECT_TRUE(var_i32);
        ASSERT_EQ(var_i32->ndim, 1);
        ASSERT_EQ(var_i32->global, 1);
        ASSERT_EQ(var_i32->nsteps, NSteps);
        ASSERT_EQ(var_i32->dims[0], mpiSize * Nx);
        ADIOS_VARINFO *var_i64 = adios_inq_var(f, "i64");
        EXPECT_TRUE(var_i64);
        ASSERT_EQ(var_i64->ndim, 1);
        ASSERT_EQ(var_i64->global, 1);
        ASSERT_EQ(var_i64->nsteps, NSteps);
        ASSERT_EQ(var_i64->dims[0], mpiSize * Nx);
        ADIOS_VARINFO *var_u8 = adios_inq_var(f, "u8");
        EXPECT_TRUE(var_u8);
        ASSERT_EQ(var_u8->ndim, 1);
        ASSERT_EQ(var_u8->global, 1);
        ASSERT_EQ(var_u8->nsteps, NSteps);
        ASSERT_EQ(var_u8->dims[0], mpiSize * Nx);
        ADIOS_VARINFO *var_u16 = adios_inq_var(f, "u16");
        EXPECT_TRUE(var_u16);
        ASSERT_EQ(var_u16->ndim, 1);
        ASSERT_EQ(var_u16->global, 1);
        ASSERT_EQ(var_u16->nsteps, NSteps);
        ASSERT_EQ(var_u16->dims[0], mpiSize * Nx);
        ADIOS_VARINFO *var_u32 = adios_inq_var(f, "u32");
        EXPECT_TRUE(var_u32);
        ASSERT_EQ(var_u32->ndim, 1);
        ASSERT_EQ(var_u32->global, 1);
        ASSERT_EQ(var_u32->nsteps, NSteps);
        ASSERT_EQ(var_u32->dims[0], mpiSize * Nx);
        ADIOS_VARINFO *var_u64 = adios_inq_var(f, "u64");
        EXPECT_TRUE(var_u64);
        ASSERT_EQ(var_u64->ndim, 1);
        ASSERT_EQ(var_u64->global, 1);
        ASSERT_EQ(var_u64->nsteps, NSteps);
        ASSERT_EQ(var_u64->dims[0], mpiSize * Nx);
        ADIOS_VARINFO *var_r32 = adios_inq_var(f, "r32");
        EXPECT_TRUE(var_r32);
        ASSERT_EQ(var_r32->ndim, 1);
        ASSERT_EQ(var_r32->global, 1);
        ASSERT_EQ(var_r32->nsteps, NSteps);
        ASSERT_EQ(var_r32->dims[0], mpiSize * Nx);
        ADIOS_VARINFO *var_r64 = adios_inq_var(f, "r64");
        EXPECT_TRUE(var_r64);
        ASSERT_EQ(var_r64->ndim, 1);
        ASSERT_EQ(var_r64->global, 1);
        ASSERT_EQ(var_r64->nsteps, NSteps);
        ASSERT_EQ(var_r64->dims[0], mpiSize * Nx);

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

        uint64_t start[1] = {mpiRank * Nx};
        uint64_t count[1] = {Nx};
        ADIOS_SELECTION *sel = adios_selection_boundingbox(1, start, count);

        // Read stuff
        for (size_t t = 0; t < NSteps; ++t)
        {
            // Generate test data for each rank uniquely
            SmallTestData currentTestData =
                generateNewSmallTestData(m_TestData, t, mpiRank, mpiSize);
            // Read the current step
            adios_schedule_read_byid(f, sel, var_i8->varid, t, 1, I8.data());
            adios_schedule_read_byid(f, sel, var_i16->varid, t, 1, I16.data());
            adios_schedule_read_byid(f, sel, var_i32->varid, t, 1, I32.data());
            adios_schedule_read_byid(f, sel, var_i64->varid, t, 1, I64.data());
            adios_schedule_read_byid(f, sel, var_u8->varid, t, 1, U8.data());
            adios_schedule_read_byid(f, sel, var_u16->varid, t, 1, U16.data());
            adios_schedule_read_byid(f, sel, var_u32->varid, t, 1, U32.data());
            adios_schedule_read_byid(f, sel, var_u64->varid, t, 1, U64.data());
            adios_schedule_read_byid(f, sel, var_r32->varid, t, 1, R32.data());
            adios_schedule_read_byid(f, sel, var_r64->varid, t, 1, R64.data());
            adios_perform_reads(f, 1);

            // Check if it's correct
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

        adios_selection_delete(sel);

        // Cleanup variable structures
        adios_free_varinfo(var_i8);
        adios_free_varinfo(var_i16);
        adios_free_varinfo(var_i32);
        adios_free_varinfo(var_i64);
        adios_free_varinfo(var_u8);
        adios_free_varinfo(var_u16);
        adios_free_varinfo(var_u32);
        adios_free_varinfo(var_u64);
        adios_free_varinfo(var_r32);
        adios_free_varinfo(var_r64);

        // Cleanup file
        adios_read_close(f);

        adios_read_finalize_method(ADIOS_READ_METHOD_BP);
    }
}

//******************************************************************************
// 2D 2x4 test data
//******************************************************************************

// ADIOS2 BP write, native ADIOS1 read
TEST_F(BPWriteReadTest, ADIOS2BPWriteADIOS1Read2D2x4fstream)
{
    // Each process would write a 2x4 array and all processes would
    // form a 2D 2 * (numberOfProcess*Nx) matrix where Nx is 4 here
    std::string fname = "ADIOS2BPWriteADIOS1Read2D2x4Testfstream.bp";

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const std::size_t Nx = 4;

    // Number of rows
    const std::size_t Ny = 2;

    // Number of steps
    const std::size_t NSteps = 3;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

    // Write test data using ADIOS2
    {
#ifdef ADIOS2_HAVE_MPI
        adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
        adios2::ADIOS adios(true);
#endif
        adios2::IO io = adios.DeclareIO("TestIO");

        // Declare 2D variables (Ny * (NumOfProcesses * Nx))
        // The local process' part (start, count) can be defined now or later
        // before Write().
        {
            adios2::Dims shape{static_cast<unsigned int>(Ny),
                               static_cast<unsigned int>(Nx * mpiSize)};
            adios2::Dims start{static_cast<unsigned int>(0),
                               static_cast<unsigned int>(mpiRank * Nx)};
            adios2::Dims count{static_cast<unsigned int>(Ny),
                               static_cast<unsigned int>(Nx)};
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

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            // Create the BP Engine
            io.SetEngine("BPFile");
        }
        io.AddTransport("file", {{"Library", "fstream"}});

        adios2::Engine engine = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            // Generate test data for each process uniquely
            SmallTestData currentTestData =
                generateNewSmallTestData(m_TestData, step, mpiRank, mpiSize);

            // Retrieve the variables that previously went out of scope
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

            // Make a 2D selection to describe the local dimensions of the
            // variable we write and its offsets in the global spaces
            adios2::Box<adios2::Dims> sel(
                {0, static_cast<unsigned int>(mpiRank * Nx)}, {Ny, Nx});
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
            engine.BeginStep();
            engine.Put(var_i8, currentTestData.I8.data());
            engine.Put(var_i16, currentTestData.I16.data());
            engine.Put(var_i32, currentTestData.I32.data());
            engine.Put(var_i64, currentTestData.I64.data());
            engine.Put(var_u8, currentTestData.U8.data());
            engine.Put(var_u16, currentTestData.U16.data());
            engine.Put(var_u32, currentTestData.U32.data());
            engine.Put(var_u64, currentTestData.U64.data());
            engine.Put(var_r32, currentTestData.R32.data());
            engine.Put(var_r64, currentTestData.R64.data());
            engine.EndStep();
        }

        engine.Close();
    }

    {
        adios_read_init_method(ADIOS_READ_METHOD_BP, MPI_COMM_SELF,
                               "verbose=3");

        // Open the file for reading
        // Note: Since collective metadata generation is not implemented yet,
        // SO for now we read each subfile instead of a single bp file with all
        // metadata.
        // Meanwhile if we open file with MPI_COMM_WORLD, then the selection
        // bounding box should be [0, Nx]
        std::string index = std::to_string(mpiRank);
        ADIOS_FILE *f = adios_read_open_file(
            (fname + ".dir/" + fname + "." + index).c_str(),
            ADIOS_READ_METHOD_BP, MPI_COMM_SELF);
        ASSERT_NE(f, nullptr);

        // Check the variables exist
        ADIOS_VARINFO *var_i8 = adios_inq_var(f, "i8");
        EXPECT_TRUE(var_i8);
        ASSERT_EQ(var_i8->ndim, 2);
        ASSERT_EQ(var_i8->global, 1);
        ASSERT_EQ(var_i8->nsteps, NSteps);
        ASSERT_EQ(var_i8->dims[0], Ny);
        ASSERT_EQ(var_i8->dims[1], mpiSize * Nx);
        ADIOS_VARINFO *var_i16 = adios_inq_var(f, "i16");
        EXPECT_TRUE(var_i16);
        ASSERT_EQ(var_i16->ndim, 2);
        ASSERT_EQ(var_i16->global, 1);
        ASSERT_EQ(var_i16->nsteps, NSteps);
        ASSERT_EQ(var_i16->dims[0], Ny);
        ASSERT_EQ(var_i16->dims[1], mpiSize * Nx);
        ADIOS_VARINFO *var_i32 = adios_inq_var(f, "i32");
        EXPECT_TRUE(var_i32);
        ASSERT_EQ(var_i32->ndim, 2);
        ASSERT_EQ(var_i32->global, 1);
        ASSERT_EQ(var_i32->nsteps, NSteps);
        ASSERT_EQ(var_i32->dims[0], Ny);
        ASSERT_EQ(var_i32->dims[1], mpiSize * Nx);
        ADIOS_VARINFO *var_i64 = adios_inq_var(f, "i64");
        EXPECT_TRUE(var_i64);
        ASSERT_EQ(var_i64->ndim, 2);
        ASSERT_EQ(var_i64->global, 1);
        ASSERT_EQ(var_i64->nsteps, NSteps);
        ASSERT_EQ(var_i64->dims[0], Ny);
        ASSERT_EQ(var_i64->dims[1], mpiSize * Nx);
        ADIOS_VARINFO *var_u8 = adios_inq_var(f, "u8");
        EXPECT_TRUE(var_u8);
        ASSERT_EQ(var_u8->ndim, 2);
        ASSERT_EQ(var_u8->global, 1);
        ASSERT_EQ(var_u8->nsteps, NSteps);
        ASSERT_EQ(var_u8->dims[0], Ny);
        ASSERT_EQ(var_u8->dims[1], mpiSize * Nx);
        ADIOS_VARINFO *var_u16 = adios_inq_var(f, "u16");
        EXPECT_TRUE(var_u16);
        ASSERT_EQ(var_u16->ndim, 2);
        ASSERT_EQ(var_u16->global, 1);
        ASSERT_EQ(var_u16->nsteps, NSteps);
        ASSERT_EQ(var_u16->dims[0], Ny);
        ASSERT_EQ(var_u16->dims[1], mpiSize * Nx);
        ADIOS_VARINFO *var_u32 = adios_inq_var(f, "u32");
        EXPECT_TRUE(var_u32);
        ASSERT_EQ(var_u32->ndim, 2);
        ASSERT_EQ(var_u32->global, 1);
        ASSERT_EQ(var_u32->nsteps, NSteps);
        ASSERT_EQ(var_u32->dims[0], Ny);
        ASSERT_EQ(var_u32->dims[1], mpiSize * Nx);
        ADIOS_VARINFO *var_u64 = adios_inq_var(f, "u64");
        EXPECT_TRUE(var_u64);
        ASSERT_EQ(var_u64->ndim, 2);
        ASSERT_EQ(var_u64->global, 1);
        ASSERT_EQ(var_u64->nsteps, NSteps);
        ASSERT_EQ(var_u64->dims[0], Ny);
        ASSERT_EQ(var_u64->dims[1], mpiSize * Nx);
        ADIOS_VARINFO *var_r32 = adios_inq_var(f, "r32");
        EXPECT_TRUE(var_r32);
        ASSERT_EQ(var_r32->ndim, 2);
        ASSERT_EQ(var_r32->global, 1);
        ASSERT_EQ(var_r32->nsteps, NSteps);
        ASSERT_EQ(var_r32->dims[0], Ny);
        ASSERT_EQ(var_r32->dims[1], mpiSize * Nx);
        ADIOS_VARINFO *var_r64 = adios_inq_var(f, "r64");
        EXPECT_TRUE(var_r64);
        ASSERT_EQ(var_r64->ndim, 2);
        ASSERT_EQ(var_r64->global, 1);
        ASSERT_EQ(var_r64->nsteps, NSteps);
        ASSERT_EQ(var_r64->dims[0], Ny);
        ASSERT_EQ(var_r64->dims[1], mpiSize * Nx);

        // If the size of the array is smaller than the data
        // the result is weird... double and uint64_t would get completely
        // garbage data
        std::array<int8_t, Nx * Ny> I8;
        std::array<int16_t, Nx * Ny> I16;
        std::array<int32_t, Nx * Ny> I32;
        std::array<int64_t, Nx * Ny> I64;
        std::array<uint8_t, Nx * Ny> U8;
        std::array<uint16_t, Nx * Ny> U16;
        std::array<uint32_t, Nx * Ny> U32;
        std::array<uint64_t, Nx * Ny> U64;
        std::array<float, Nx * Ny> R32;
        std::array<double, Nx * Ny> R64;

        uint64_t start[2] = {0, mpiRank * Nx};
        uint64_t count[2] = {Ny, Nx};
        ADIOS_SELECTION *sel = adios_selection_boundingbox(2, start, count);

        // Read stuff
        for (size_t t = 0; t < NSteps; ++t)
        {
            // Generate test data for each rank uniquely
            SmallTestData currentTestData =
                generateNewSmallTestData(m_TestData, t, mpiRank, mpiSize);
            // Read the current step
            adios_schedule_read_byid(f, sel, var_i8->varid, t, 1, I8.data());
            adios_schedule_read_byid(f, sel, var_i16->varid, t, 1, I16.data());
            adios_schedule_read_byid(f, sel, var_i32->varid, t, 1, I32.data());
            adios_schedule_read_byid(f, sel, var_i64->varid, t, 1, I64.data());
            adios_schedule_read_byid(f, sel, var_u8->varid, t, 1, U8.data());
            adios_schedule_read_byid(f, sel, var_u16->varid, t, 1, U16.data());
            adios_schedule_read_byid(f, sel, var_u32->varid, t, 1, U32.data());
            adios_schedule_read_byid(f, sel, var_u64->varid, t, 1, U64.data());
            adios_schedule_read_byid(f, sel, var_r32->varid, t, 1, R32.data());
            adios_schedule_read_byid(f, sel, var_r64->varid, t, 1, R64.data());
            adios_perform_reads(f, 1);

            // Check if it's correct
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

        adios_selection_delete(sel);

        // Cleanup variable structures
        adios_free_varinfo(var_i8);
        adios_free_varinfo(var_i16);
        adios_free_varinfo(var_i32);
        adios_free_varinfo(var_i64);
        adios_free_varinfo(var_u8);
        adios_free_varinfo(var_u16);
        adios_free_varinfo(var_u32);
        adios_free_varinfo(var_u64);
        adios_free_varinfo(var_r32);
        adios_free_varinfo(var_r64);

        // Cleanup file
        adios_read_close(f);

        adios_read_finalize_method(ADIOS_READ_METHOD_BP);
    }
}

// ADIOS2 write, native ADIOS1 read
TEST_F(BPWriteReadTest, ADIOS2BPWriteADIOS1Read2D4x2fstream)
{
    // Each process would write a 4x2 array and all processes would
    // form a 2D 4 * (NumberOfProcess * Nx) matrix where Nx is 2 here
    std::string fname = "ADIOS2BPWriteADIOS1Read2D4x2Testfstream.bp";

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const std::size_t Nx = 2;
    // Number of cols
    const std::size_t Ny = 4;

    // Number of steps
    const std::size_t NSteps = 3;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

    // Write test data using ADIOS2
    {
#ifdef ADIOS2_HAVE_MPI
        adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
        adios2::ADIOS adios(true);
#endif
        adios2::IO io = adios.DeclareIO("TestIO");

        // Declare 2D variables (4 * (NumberOfProcess * Nx))
        // The local process' part (start, count) can be defined now or later
        // before Write().
        {
            adios2::Dims shape{static_cast<unsigned int>(Ny),
                               static_cast<unsigned int>(mpiSize * Nx)};
            adios2::Dims start{static_cast<unsigned int>(0),
                               static_cast<unsigned int>(mpiRank * Nx)};
            adios2::Dims count{static_cast<unsigned int>(Ny),
                               static_cast<unsigned int>(Nx)};
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

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            // Create the BP Engine
            io.SetEngine("BPFile");
        }
        io.AddTransport("file", {{"Library", "fstream"}});

        adios2::Engine engine = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            // Generate test data for each process uniquely
            SmallTestData currentTestData =
                generateNewSmallTestData(m_TestData, step, mpiRank, mpiSize);

            // Retrieve the variables that previously went out of scope
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

            // Make a 2D selection to describe the local dimensions of the
            // variable we write and its offsets in the global spaces
            adios2::Box<adios2::Dims> sel(
                {0, static_cast<unsigned int>(mpiRank * Nx)}, {Ny, Nx});
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
            engine.BeginStep();
            engine.Put(var_i8, currentTestData.I8.data());
            engine.Put(var_i16, currentTestData.I16.data());
            engine.Put(var_i32, currentTestData.I32.data());
            engine.Put(var_i64, currentTestData.I64.data());
            engine.Put(var_u8, currentTestData.U8.data());
            engine.Put(var_u16, currentTestData.U16.data());
            engine.Put(var_u32, currentTestData.U32.data());
            engine.Put(var_u64, currentTestData.U64.data());
            engine.Put(var_r32, currentTestData.R32.data());
            engine.Put(var_r64, currentTestData.R64.data());
            engine.EndStep();
        }

        engine.Close();
    }

    {
        adios_read_init_method(ADIOS_READ_METHOD_BP, MPI_COMM_SELF,
                               "verbose=3");

        // Open the file for reading
        // Note: Since collective metadata generation is not implemented yet,
        // SO for now we read each subfile instead of a single bp file with all
        // metadata.
        // Meanwhile if we open file with MPI_COMM_WORLD, then the selection
        // bounding box should be [0, Nx]
        std::string index = std::to_string(mpiRank);
        ADIOS_FILE *f = adios_read_open_file(
            (fname + ".dir/" + fname + "." + index).c_str(),
            ADIOS_READ_METHOD_BP, MPI_COMM_SELF);
        ASSERT_NE(f, nullptr);

        // Check the variables exist
        ADIOS_VARINFO *var_i8 = adios_inq_var(f, "i8");
        EXPECT_TRUE(var_i8);
        ASSERT_EQ(var_i8->ndim, 2);
        ASSERT_EQ(var_i8->global, 1);
        ASSERT_EQ(var_i8->nsteps, NSteps);
        ASSERT_EQ(var_i8->dims[0], Ny);
        ASSERT_EQ(var_i8->dims[1], mpiSize * Nx);
        ADIOS_VARINFO *var_i16 = adios_inq_var(f, "i16");
        EXPECT_TRUE(var_i16);
        ASSERT_EQ(var_i16->ndim, 2);
        ASSERT_EQ(var_i16->global, 1);
        ASSERT_EQ(var_i16->nsteps, NSteps);
        ASSERT_EQ(var_i16->dims[0], Ny);
        ASSERT_EQ(var_i16->dims[1], mpiSize * Nx);
        ADIOS_VARINFO *var_i32 = adios_inq_var(f, "i32");
        EXPECT_TRUE(var_i32);
        ASSERT_EQ(var_i32->ndim, 2);
        ASSERT_EQ(var_i32->global, 1);
        ASSERT_EQ(var_i32->nsteps, NSteps);
        ASSERT_EQ(var_i32->dims[0], Ny);
        ASSERT_EQ(var_i32->dims[1], mpiSize * Nx);
        ADIOS_VARINFO *var_i64 = adios_inq_var(f, "i64");
        EXPECT_TRUE(var_i64);
        ASSERT_EQ(var_i64->ndim, 2);
        ASSERT_EQ(var_i64->global, 1);
        ASSERT_EQ(var_i64->nsteps, NSteps);
        ASSERT_EQ(var_i64->dims[0], Ny);
        ASSERT_EQ(var_i64->dims[1], mpiSize * Nx);
        ADIOS_VARINFO *var_u8 = adios_inq_var(f, "u8");
        EXPECT_TRUE(var_u8);
        ASSERT_EQ(var_u8->ndim, 2);
        ASSERT_EQ(var_u8->global, 1);
        ASSERT_EQ(var_u8->nsteps, NSteps);
        ASSERT_EQ(var_u8->dims[0], Ny);
        ASSERT_EQ(var_u8->dims[1], mpiSize * Nx);
        ADIOS_VARINFO *var_u16 = adios_inq_var(f, "u16");
        EXPECT_TRUE(var_u16);
        ASSERT_EQ(var_u16->ndim, 2);
        ASSERT_EQ(var_u16->global, 1);
        ASSERT_EQ(var_u16->nsteps, NSteps);
        ASSERT_EQ(var_u16->dims[0], Ny);
        ASSERT_EQ(var_u16->dims[1], mpiSize * Nx);
        ADIOS_VARINFO *var_u32 = adios_inq_var(f, "u32");
        EXPECT_TRUE(var_u32);
        ASSERT_EQ(var_u32->ndim, 2);
        ASSERT_EQ(var_u32->global, 1);
        ASSERT_EQ(var_u32->nsteps, NSteps);
        ASSERT_EQ(var_u32->dims[0], Ny);
        ASSERT_EQ(var_u32->dims[1], mpiSize * Nx);
        ADIOS_VARINFO *var_u64 = adios_inq_var(f, "u64");
        EXPECT_TRUE(var_u64);
        ASSERT_EQ(var_u64->ndim, 2);
        ASSERT_EQ(var_u64->global, 1);
        ASSERT_EQ(var_u64->nsteps, NSteps);
        ASSERT_EQ(var_u64->dims[0], Ny);
        ASSERT_EQ(var_u64->dims[1], mpiSize * Nx);
        ADIOS_VARINFO *var_r32 = adios_inq_var(f, "r32");
        EXPECT_TRUE(var_r32);
        ASSERT_EQ(var_r32->ndim, 2);
        ASSERT_EQ(var_r32->global, 1);
        ASSERT_EQ(var_r32->nsteps, NSteps);
        ASSERT_EQ(var_r32->dims[0], Ny);
        ASSERT_EQ(var_r32->dims[1], mpiSize * Nx);
        ADIOS_VARINFO *var_r64 = adios_inq_var(f, "r64");
        EXPECT_TRUE(var_r64);
        ASSERT_EQ(var_r64->ndim, 2);
        ASSERT_EQ(var_r64->global, 1);
        ASSERT_EQ(var_r64->nsteps, NSteps);
        ASSERT_EQ(var_r64->dims[0], Ny);
        ASSERT_EQ(var_r64->dims[1], mpiSize * Nx);

        // If the size of the array is smaller than the data
        // the result is weird... double and uint64_t would get completely
        // garbage data
        std::array<int8_t, Nx * Ny> I8;
        std::array<int16_t, Nx * Ny> I16;
        std::array<int32_t, Nx * Ny> I32;
        std::array<int64_t, Nx * Ny> I64;
        std::array<uint8_t, Nx * Ny> U8;
        std::array<uint16_t, Nx * Ny> U16;
        std::array<uint32_t, Nx * Ny> U32;
        std::array<uint64_t, Nx * Ny> U64;
        std::array<float, Nx * Ny> R32;
        std::array<double, Nx * Ny> R64;

        uint64_t start[2] = {0, mpiRank * Nx};
        uint64_t count[2] = {Ny, Nx};
        ADIOS_SELECTION *sel = adios_selection_boundingbox(2, start, count);

        // Read stuff
        for (size_t t = 0; t < NSteps; ++t)
        {
            // Generate test data for each rank uniquely
            SmallTestData currentTestData =
                generateNewSmallTestData(m_TestData, t, mpiRank, mpiSize);
            // Read the current step
            adios_schedule_read_byid(f, sel, var_i8->varid, t, 1, I8.data());
            adios_schedule_read_byid(f, sel, var_i16->varid, t, 1, I16.data());
            adios_schedule_read_byid(f, sel, var_i32->varid, t, 1, I32.data());
            adios_schedule_read_byid(f, sel, var_i64->varid, t, 1, I64.data());
            adios_schedule_read_byid(f, sel, var_u8->varid, t, 1, U8.data());
            adios_schedule_read_byid(f, sel, var_u16->varid, t, 1, U16.data());
            adios_schedule_read_byid(f, sel, var_u32->varid, t, 1, U32.data());
            adios_schedule_read_byid(f, sel, var_u64->varid, t, 1, U64.data());
            adios_schedule_read_byid(f, sel, var_r32->varid, t, 1, R32.data());
            adios_schedule_read_byid(f, sel, var_r64->varid, t, 1, R64.data());
            adios_perform_reads(f, 1);

            // Check if it's correct
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

        adios_selection_delete(sel);

        // Cleanup variable structures
        adios_free_varinfo(var_i8);
        adios_free_varinfo(var_i16);
        adios_free_varinfo(var_i32);
        adios_free_varinfo(var_i64);
        adios_free_varinfo(var_u8);
        adios_free_varinfo(var_u16);
        adios_free_varinfo(var_u32);
        adios_free_varinfo(var_u64);
        adios_free_varinfo(var_r32);
        adios_free_varinfo(var_r64);

        // Cleanup file
        adios_read_close(f);

        adios_read_finalize_method(ADIOS_READ_METHOD_BP);
    }
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

    if (argc > 1)
    {
        engineName = std::string(argv[1]);
    }
    result = RUN_ALL_TESTS();

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return result;
}
