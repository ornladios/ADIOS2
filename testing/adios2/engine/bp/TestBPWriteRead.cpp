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

class BPWriteReadTest : public ::testing::Test
{
public:
    BPWriteReadTest() = default;

    SmallTestData m_TestData;
};

//******************************************************************************
// 1D 1x8 test data
//******************************************************************************

// ADIOS2 write, native ADIOS1 read
TEST_F(BPWriteReadTest, ADIOS2BPWriteADIOS1Read1D8)
{
    std::string fname = "ADIOS2BPWriteADIOS1Read1D8.bp";

    // Write test data using ADIOS2
    {
        adios2::ADIOS adios(true);
        adios2::IO &io = adios.DeclareIO("TestIO");

        // Declare 1D variables
        {
            auto &var_i8 =
                io.DefineVariable<int8_t>("i8", {}, {}, adios2::Dims{8});
            auto &var_i16 =
                io.DefineVariable<int16_t>("i16", {}, {}, adios2::Dims{8});
            auto &var_i32 =
                io.DefineVariable<int32_t>("i32", {}, {}, adios2::Dims{8});
            auto &var_i64 =
                io.DefineVariable<int64_t>("i64", {}, {}, adios2::Dims{8});
            auto &var_u8 =
                io.DefineVariable<uint8_t>("u8", {}, {}, adios2::Dims{8});
            auto &var_u16 =
                io.DefineVariable<uint16_t>("u16", {}, {}, adios2::Dims{8});
            auto &var_u32 =
                io.DefineVariable<uint32_t>("u32", {}, {}, adios2::Dims{8});
            auto &var_u64 =
                io.DefineVariable<uint64_t>("u64", {}, {}, adios2::Dims{8});
            auto &var_r32 =
                io.DefineVariable<float>("r32", {}, {}, adios2::Dims{8});
            auto &var_r64 =
                io.DefineVariable<double>("r64", {}, {}, adios2::Dims{8});
        }

        // Create the BP Engine
        io.SetEngine("BPFileWriter");
        io.AddTransport("File");

        auto engine = io.Open(fname, adios2::Mode::Write);
        ASSERT_NE(engine.get(), nullptr);

        for (size_t step = 0; step < 3; ++step)
        {
            // Retrieve the variables that previously went out of scope
            auto &var_i8 = io.GetVariable<int8_t>("i8");
            auto &var_i16 = io.GetVariable<int16_t>("i16");
            auto &var_i32 = io.GetVariable<int32_t>("i32");
            auto &var_i64 = io.GetVariable<int64_t>("i64");
            auto &var_u8 = io.GetVariable<uint8_t>("u8");
            auto &var_u16 = io.GetVariable<uint16_t>("u16");
            auto &var_u32 = io.GetVariable<uint32_t>("u32");
            auto &var_u64 = io.GetVariable<uint64_t>("u64");
            auto &var_r32 = io.GetVariable<float>("r32");
            auto &var_r64 = io.GetVariable<double>("r64");

            // Write each one
            engine->Write(var_i8, m_TestData.I8.data() + step);
            engine->Write(var_i16, m_TestData.I16.data() + step);
            engine->Write(var_i32, m_TestData.I32.data() + step);
            engine->Write(var_i64, m_TestData.I64.data() + step);
            engine->Write(var_u8, m_TestData.U8.data() + step);
            engine->Write(var_u16, m_TestData.U16.data() + step);
            engine->Write(var_u32, m_TestData.U32.data() + step);
            engine->Write(var_u64, m_TestData.U64.data() + step);
            engine->Write(var_r32, m_TestData.R32.data() + step);
            engine->Write(var_r64, m_TestData.R64.data() + step);

            // Advance to the next time step
            engine->Advance();
        }

        // Close the file
        engine->Close();
    }

// Read test data using ADIOS1
#ifdef ADIOS2_HAVE_MPI
    // Read everything from rank 0
    int rank;
    MPI_Comm_rank();
    if (rank == 0)
#endif
    {
        adios_read_init_method(ADIOS_READ_METHOD_BP, MPI_COMM_WORLD,
                               "verbose=3");

        // Open the file for reading
        ADIOS_FILE *f =
            adios_read_open_file((fname + ".dir/" + fname + ".0").c_str(),
                                 ADIOS_READ_METHOD_BP, MPI_COMM_WORLD);
        ASSERT_NE(f, nullptr);

        // Check the variables exist
        ADIOS_VARINFO *var_i8 = adios_inq_var(f, "i8");
        ASSERT_NE(var_i8, nullptr);
        ASSERT_EQ(var_i8->ndim, 1);
        ASSERT_EQ(var_i8->dims[0], 8);
        ADIOS_VARINFO *var_i16 = adios_inq_var(f, "i16");
        ASSERT_NE(var_i16, nullptr);
        ASSERT_EQ(var_i16->ndim, 1);
        ASSERT_EQ(var_i16->dims[0], 8);
        ADIOS_VARINFO *var_i32 = adios_inq_var(f, "i32");
        ASSERT_NE(var_i32, nullptr);
        ASSERT_EQ(var_i32->ndim, 1);
        ASSERT_EQ(var_i32->dims[0], 8);
        ADIOS_VARINFO *var_i64 = adios_inq_var(f, "i64");
        ASSERT_NE(var_i64, nullptr);
        ASSERT_EQ(var_i64->ndim, 1);
        ASSERT_EQ(var_i64->dims[0], 8);
        ADIOS_VARINFO *var_u8 = adios_inq_var(f, "u8");
        ASSERT_NE(var_u8, nullptr);
        ASSERT_EQ(var_u8->ndim, 1);
        ASSERT_EQ(var_u8->dims[0], 8);
        ADIOS_VARINFO *var_u16 = adios_inq_var(f, "u16");
        ASSERT_NE(var_u16, nullptr);
        ASSERT_EQ(var_u16->ndim, 1);
        ASSERT_EQ(var_u16->dims[0], 8);
        ADIOS_VARINFO *var_u32 = adios_inq_var(f, "u32");
        ASSERT_NE(var_u32, nullptr);
        ASSERT_EQ(var_u32->ndim, 1);
        ASSERT_EQ(var_u32->dims[0], 8);
        ADIOS_VARINFO *var_u64 = adios_inq_var(f, "u64");
        ASSERT_NE(var_u64, nullptr);
        ASSERT_EQ(var_u64->ndim, 1);
        ASSERT_EQ(var_u64->dims[0], 8);
        ADIOS_VARINFO *var_r32 = adios_inq_var(f, "r32");
        ASSERT_NE(var_r32, nullptr);
        ASSERT_EQ(var_r32->ndim, 1);
        ASSERT_EQ(var_r32->dims[0], 8);
        ADIOS_VARINFO *var_r64 = adios_inq_var(f, "r64");
        ASSERT_NE(var_r64, nullptr);
        ASSERT_EQ(var_r64->ndim, 1);
        ASSERT_EQ(var_r64->dims[0], 8);

        std::array<int8_t, 8> I8;
        std::array<int16_t, 8> I16;
        std::array<int32_t, 8> I32;
        std::array<int64_t, 8> I64;
        std::array<uint8_t, 8> U8;
        std::array<uint16_t, 8> U16;
        std::array<uint32_t, 8> U32;
        std::array<uint64_t, 8> U64;
        std::array<float, 8> R32;
        std::array<double, 8> R64;

        uint64_t start[1] = {0};
        uint64_t count[1] = {8};
        ADIOS_SELECTION *sel = adios_selection_boundingbox(1, start, count);

        // Read stuff
        for (size_t t = 0; t < 3; ++t)
        {
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
            for (size_t i = 0; i < 8; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i;
                std::string msg = ss.str();

                EXPECT_EQ(I8[i], m_TestData.I8[i + t]) << msg;
                EXPECT_EQ(I16[i], m_TestData.I16[i + t]) << msg;
                EXPECT_EQ(I32[i], m_TestData.I32[i + t]) << msg;
                EXPECT_EQ(I64[i], m_TestData.I64[i + t]) << msg;
                EXPECT_EQ(U8[i], m_TestData.U8[i + t]) << msg;
                EXPECT_EQ(U16[i], m_TestData.U16[i + t]) << msg;
                EXPECT_EQ(U32[i], m_TestData.U32[i + t]) << msg;
                EXPECT_EQ(U64[i], m_TestData.U64[i + t]) << msg;
                EXPECT_EQ(R32[i], m_TestData.R32[i + t]) << msg;
                EXPECT_EQ(R64[i], m_TestData.R64[i + t]) << msg;
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
    }
}

// ADIOS2 write, ADIOS2 read
TEST_F(BPWriteReadTest, DISABLED_ADIOS2BPWriteADIOS2BPRead1D8)
{
    std::string fname = "ADIOS2BPWriteADIOS2BPRead1D8.bp";

    ASSERT_TRUE(false) << "ADIOS2 read API is not yet implemented";
}

//******************************************************************************
// 2D 2x4 test data
//******************************************************************************

// ADIOS2 write, native ADIOS1 read
TEST_F(BPWriteReadTest, ADIOS2BPWriteADIOS1Read2D2x4)
{
    std::string fname = "ADIOS2BPWriteADIOS1Read2D2x4Test.bp";

    // Write test data using ADIOS2
    {
        adios2::ADIOS adios(true);
        adios2::IO &io = adios.DeclareIO("TestIO");

        // Declare 1D variables
        {
            auto &var_i8 =
                io.DefineVariable<int8_t>("i8", {}, {}, adios2::Dims{2, 4});
            auto &var_i16 =
                io.DefineVariable<int16_t>("i16", {}, {}, adios2::Dims{2, 4});
            auto &var_i32 =
                io.DefineVariable<int32_t>("i32", {}, {}, adios2::Dims{2, 4});
            auto &var_i64 =
                io.DefineVariable<int64_t>("i64", {}, {}, adios2::Dims{2, 4});
            auto &var_u8 =
                io.DefineVariable<uint8_t>("u8", {}, {}, adios2::Dims{2, 4});
            auto &var_u16 =
                io.DefineVariable<uint16_t>("u16", {}, {}, adios2::Dims{2, 4});
            auto &var_u32 =
                io.DefineVariable<uint32_t>("u32", {}, {}, adios2::Dims{2, 4});
            auto &var_u64 =
                io.DefineVariable<uint64_t>("u64", {}, {}, adios2::Dims{2, 4});
            auto &var_r32 =
                io.DefineVariable<float>("r32", {}, {}, adios2::Dims{2, 4});
            auto &var_r64 =
                io.DefineVariable<double>("r64", {}, {}, adios2::Dims{2, 4});
        }

        // Create the BP Engine
        io.SetEngine("BPFileWriter");
        io.AddTransport("file");

        auto engine = io.Open(fname, adios2::Mode::Write);
        ASSERT_NE(engine.get(), nullptr);

        for (size_t step = 0; step < 3; ++step)
        {
            // Retrieve the variables that previously went out of scope
            auto &var_i8 = io.GetVariable<int8_t>("i8");
            auto &var_i16 = io.GetVariable<int16_t>("i16");
            auto &var_i32 = io.GetVariable<int32_t>("i32");
            auto &var_i64 = io.GetVariable<int64_t>("i64");
            auto &var_u8 = io.GetVariable<uint8_t>("u8");
            auto &var_u16 = io.GetVariable<uint16_t>("u16");
            auto &var_u32 = io.GetVariable<uint32_t>("u32");
            auto &var_u64 = io.GetVariable<uint64_t>("u64");
            auto &var_r32 = io.GetVariable<float>("r32");
            auto &var_r64 = io.GetVariable<double>("r64");

            // Write each one
            engine->Write(var_i8, m_TestData.I8.data() + step);
            engine->Write(var_i16, m_TestData.I16.data() + step);
            engine->Write(var_i32, m_TestData.I32.data() + step);
            engine->Write(var_i64, m_TestData.I64.data() + step);
            engine->Write(var_u8, m_TestData.U8.data() + step);
            engine->Write(var_u16, m_TestData.U16.data() + step);
            engine->Write(var_u32, m_TestData.U32.data() + step);
            engine->Write(var_u64, m_TestData.U64.data() + step);
            engine->Write(var_r32, m_TestData.R32.data() + step);
            engine->Write(var_r64, m_TestData.R64.data() + step);

            // Advance to the next time step
            engine->Advance();
        }

        // Close the file
        engine->Close();
    }

// Read test data using ADIOS1
#ifdef ADIOS2_HAVE_MPI
    // Read everything from rank 0
    int rank;
    MPI_Comm_rank();
    if (rank == 0)
#endif
    {
        adios_read_init_method(ADIOS_READ_METHOD_BP, MPI_COMM_WORLD,
                               "verbose=3");

        // Open the file for reading
        ADIOS_FILE *f =
            adios_read_open_file((fname + ".dir/" + fname + ".0").c_str(),
                                 ADIOS_READ_METHOD_BP, MPI_COMM_WORLD);
        ASSERT_NE(f, nullptr);

        // Check the variables exist
        ADIOS_VARINFO *var_i8 = adios_inq_var(f, "i8");
        ASSERT_NE(var_i8, nullptr);
        ASSERT_EQ(var_i8->ndim, 2);
        ASSERT_EQ(var_i8->dims[0], 2);
        ASSERT_EQ(var_i8->dims[1], 4);
        ADIOS_VARINFO *var_i16 = adios_inq_var(f, "i16");
        ASSERT_NE(var_i16, nullptr);
        ASSERT_EQ(var_i16->ndim, 2);
        ASSERT_EQ(var_i16->dims[0], 2);
        ASSERT_EQ(var_i16->dims[1], 4);
        ADIOS_VARINFO *var_i32 = adios_inq_var(f, "i32");
        ASSERT_NE(var_i32, nullptr);
        ASSERT_EQ(var_i32->ndim, 2);
        ASSERT_EQ(var_i32->dims[0], 2);
        ASSERT_EQ(var_i32->dims[1], 4);
        ADIOS_VARINFO *var_i64 = adios_inq_var(f, "i64");
        ASSERT_NE(var_i64, nullptr);
        ASSERT_EQ(var_i64->ndim, 2);
        ASSERT_EQ(var_i64->dims[0], 2);
        ASSERT_EQ(var_i64->dims[1], 4);
        ADIOS_VARINFO *var_u8 = adios_inq_var(f, "u8");
        ASSERT_NE(var_u8, nullptr);
        ASSERT_EQ(var_u8->ndim, 2);
        ASSERT_EQ(var_u8->dims[0], 2);
        ASSERT_EQ(var_u8->dims[1], 4);
        ADIOS_VARINFO *var_u16 = adios_inq_var(f, "u16");
        ASSERT_NE(var_u16, nullptr);
        ASSERT_EQ(var_u16->ndim, 2);
        ASSERT_EQ(var_u16->dims[0], 2);
        ASSERT_EQ(var_u16->dims[1], 4);
        ADIOS_VARINFO *var_u32 = adios_inq_var(f, "u32");
        ASSERT_NE(var_u32, nullptr);
        ASSERT_EQ(var_u32->ndim, 2);
        ASSERT_EQ(var_u32->dims[0], 2);
        ASSERT_EQ(var_u32->dims[1], 4);
        ADIOS_VARINFO *var_u64 = adios_inq_var(f, "u64");
        ASSERT_NE(var_u64, nullptr);
        ASSERT_EQ(var_u64->ndim, 2);
        ASSERT_EQ(var_u64->dims[0], 2);
        ASSERT_EQ(var_u64->dims[1], 4);
        ADIOS_VARINFO *var_r32 = adios_inq_var(f, "r32");
        ASSERT_NE(var_r32, nullptr);
        ASSERT_EQ(var_r32->ndim, 2);
        ASSERT_EQ(var_r32->dims[0], 2);
        ASSERT_EQ(var_r32->dims[1], 4);
        ADIOS_VARINFO *var_r64 = adios_inq_var(f, "r64");
        ASSERT_NE(var_r64, nullptr);
        ASSERT_EQ(var_r64->ndim, 2);
        ASSERT_EQ(var_r64->dims[0], 2);
        ASSERT_EQ(var_r64->dims[1], 4);

        std::array<int8_t, 8> I8;
        std::array<int16_t, 8> I16;
        std::array<int32_t, 8> I32;
        std::array<int64_t, 8> I64;
        std::array<uint8_t, 8> U8;
        std::array<uint16_t, 8> U16;
        std::array<uint32_t, 8> U32;
        std::array<uint64_t, 8> U64;
        std::array<float, 8> R32;
        std::array<double, 8> R64;

        uint64_t start[2] = {0, 0};
        uint64_t count[2] = {2, 4};
        ADIOS_SELECTION *sel = adios_selection_boundingbox(2, start, count);

        // Read stuff
        for (size_t t = 0; t < 3; ++t)
        {
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
            for (size_t i = 0; i < 8; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i;
                std::string msg = ss.str();

                EXPECT_EQ(I8[i], m_TestData.I8[i + t]) << msg;
                EXPECT_EQ(I16[i], m_TestData.I16[i + t]) << msg;
                EXPECT_EQ(I32[i], m_TestData.I32[i + t]) << msg;
                EXPECT_EQ(I64[i], m_TestData.I64[i + t]) << msg;
                EXPECT_EQ(U8[i], m_TestData.U8[i + t]) << msg;
                EXPECT_EQ(U16[i], m_TestData.U16[i + t]) << msg;
                EXPECT_EQ(U32[i], m_TestData.U32[i + t]) << msg;
                EXPECT_EQ(U64[i], m_TestData.U64[i + t]) << msg;
                EXPECT_EQ(R32[i], m_TestData.R32[i + t]) << msg;
                EXPECT_EQ(R64[i], m_TestData.R64[i + t]) << msg;
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
    }
}

// ADIOS2 write, ADIOS2 read
TEST_F(BPWriteReadTest, DISABLED_ADIOS2BPWriteADIOS2BPRead2D2x4)
{
    std::string fname = "ADIOS2BPWriteADIOS2BPRead2D2x4Test.bp";

    ASSERT_TRUE(false) << "ADIOS2 read API is not yet implemented";
}

//******************************************************************************
// 2D 4x2 test data
//******************************************************************************

// ADIOS2 write, native ADIOS1 read
TEST_F(BPWriteReadTest, ADIOS2BPWriteADIOS1Read2D4x2)
{
    std::string fname = "ADIOS2BPWriteADIOS1Read2D4x2Test.bp";

    // Write test data using ADIOS2
    {
        adios2::ADIOS adios(true);
        adios2::IO &io = adios.DeclareIO("TestIO");

        // Declare 1D variables
        {
            auto &var_i8 =
                io.DefineVariable<int8_t>("i8", {}, {}, adios2::Dims{4, 2});
            auto &var_i16 =
                io.DefineVariable<int16_t>("i16", {}, {}, adios2::Dims{4, 2});
            auto &var_i32 =
                io.DefineVariable<int32_t>("i32", {}, {}, adios2::Dims{4, 2});
            auto &var_i64 =
                io.DefineVariable<int64_t>("i64", {}, {}, adios2::Dims{4, 2});
            auto &var_u8 =
                io.DefineVariable<uint8_t>("u8", {}, {}, adios2::Dims{4, 2});
            auto &var_u16 =
                io.DefineVariable<uint16_t>("u16", {}, {}, adios2::Dims{4, 2});
            auto &var_u32 =
                io.DefineVariable<uint32_t>("u32", {}, {}, adios2::Dims{4, 2});
            auto &var_u64 =
                io.DefineVariable<uint64_t>("u64", {}, {}, adios2::Dims{4, 2});
            auto &var_r32 =
                io.DefineVariable<float>("r32", {}, {}, adios2::Dims{4, 2});
            auto &var_r64 =
                io.DefineVariable<double>("r64", {}, {}, adios2::Dims{4, 2});
        }

        // Create the BP Engine
        io.SetEngine("BPFileWriter");
        io.AddTransport("file");

        auto engine = io.Open(fname, adios2::Mode::Write);
        ASSERT_NE(engine.get(), nullptr);

        for (size_t step = 0; step < 3; ++step)
        {
            // Retrieve the variables that previously went out of scope
            auto &var_i8 = io.GetVariable<int8_t>("i8");
            auto &var_i16 = io.GetVariable<int16_t>("i16");
            auto &var_i32 = io.GetVariable<int32_t>("i32");
            auto &var_i64 = io.GetVariable<int64_t>("i64");
            auto &var_u8 = io.GetVariable<uint8_t>("u8");
            auto &var_u16 = io.GetVariable<uint16_t>("u16");
            auto &var_u32 = io.GetVariable<uint32_t>("u32");
            auto &var_u64 = io.GetVariable<uint64_t>("u64");
            auto &var_r32 = io.GetVariable<float>("r32");
            auto &var_r64 = io.GetVariable<double>("r64");

            // Write each one
            engine->Write(var_i8, m_TestData.I8.data() + step);
            engine->Write(var_i16, m_TestData.I16.data() + step);
            engine->Write(var_i32, m_TestData.I32.data() + step);
            engine->Write(var_i64, m_TestData.I64.data() + step);
            engine->Write(var_u8, m_TestData.U8.data() + step);
            engine->Write(var_u16, m_TestData.U16.data() + step);
            engine->Write(var_u32, m_TestData.U32.data() + step);
            engine->Write(var_u64, m_TestData.U64.data() + step);
            engine->Write(var_r32, m_TestData.R32.data() + step);
            engine->Write(var_r64, m_TestData.R64.data() + step);

            // Advance to the next time step
            engine->Advance();
        }

        // Close the file
        engine->Close();
    }

// Read test data using ADIOS1
#ifdef ADIOS2_HAVE_MPI
    // Read everything from rank 0
    int rank;
    MPI_Comm_rank();
    if (rank == 0)
#endif
    {
        adios_read_init_method(ADIOS_READ_METHOD_BP, MPI_COMM_WORLD,
                               "verbose=3");

        // Open the file for reading
        ADIOS_FILE *f =
            adios_read_open_file((fname + ".dir/" + fname + ".0").c_str(),
                                 ADIOS_READ_METHOD_BP, MPI_COMM_WORLD);
        ASSERT_NE(f, nullptr);

        // Check the variables exist
        ADIOS_VARINFO *var_i8 = adios_inq_var(f, "i8");
        ASSERT_NE(var_i8, nullptr);
        ASSERT_EQ(var_i8->ndim, 2);
        ASSERT_EQ(var_i8->dims[0], 4);
        ASSERT_EQ(var_i8->dims[1], 2);
        ADIOS_VARINFO *var_i16 = adios_inq_var(f, "i16");
        ASSERT_NE(var_i16, nullptr);
        ASSERT_EQ(var_i16->ndim, 2);
        ASSERT_EQ(var_i16->dims[0], 4);
        ASSERT_EQ(var_i16->dims[1], 2);
        ADIOS_VARINFO *var_i32 = adios_inq_var(f, "i32");
        ASSERT_NE(var_i32, nullptr);
        ASSERT_EQ(var_i32->ndim, 2);
        ASSERT_EQ(var_i32->dims[0], 4);
        ASSERT_EQ(var_i32->dims[1], 2);
        ADIOS_VARINFO *var_i64 = adios_inq_var(f, "i64");
        ASSERT_NE(var_i64, nullptr);
        ASSERT_EQ(var_i64->ndim, 2);
        ASSERT_EQ(var_i64->dims[0], 4);
        ASSERT_EQ(var_i64->dims[1], 2);
        ADIOS_VARINFO *var_u8 = adios_inq_var(f, "u8");
        ASSERT_NE(var_u8, nullptr);
        ASSERT_EQ(var_u8->ndim, 2);
        ASSERT_EQ(var_u8->dims[0], 4);
        ASSERT_EQ(var_u8->dims[1], 2);
        ADIOS_VARINFO *var_u16 = adios_inq_var(f, "u16");
        ASSERT_NE(var_u16, nullptr);
        ASSERT_EQ(var_u16->ndim, 2);
        ASSERT_EQ(var_u16->dims[0], 4);
        ASSERT_EQ(var_u16->dims[1], 2);
        ADIOS_VARINFO *var_u32 = adios_inq_var(f, "u32");
        ASSERT_NE(var_u32, nullptr);
        ASSERT_EQ(var_u32->ndim, 2);
        ASSERT_EQ(var_u32->dims[0], 4);
        ASSERT_EQ(var_u32->dims[1], 2);
        ADIOS_VARINFO *var_u64 = adios_inq_var(f, "u64");
        ASSERT_NE(var_u64, nullptr);
        ASSERT_EQ(var_u64->ndim, 2);
        ASSERT_EQ(var_u64->dims[0], 4);
        ASSERT_EQ(var_u64->dims[1], 2);
        ADIOS_VARINFO *var_r32 = adios_inq_var(f, "r32");
        ASSERT_NE(var_r32, nullptr);
        ASSERT_EQ(var_r32->ndim, 2);
        ASSERT_EQ(var_r32->dims[0], 4);
        ASSERT_EQ(var_r32->dims[1], 2);
        ADIOS_VARINFO *var_r64 = adios_inq_var(f, "r64");
        ASSERT_NE(var_r64, nullptr);
        ASSERT_EQ(var_r64->ndim, 2);
        ASSERT_EQ(var_r64->dims[0], 4);
        ASSERT_EQ(var_r64->dims[1], 2);

        std::array<int8_t, 8> I8;
        std::array<int16_t, 8> I16;
        std::array<int32_t, 8> I32;
        std::array<int64_t, 8> I64;
        std::array<uint8_t, 8> U8;
        std::array<uint16_t, 8> U16;
        std::array<uint32_t, 8> U32;
        std::array<uint64_t, 8> U64;
        std::array<float, 8> R32;
        std::array<double, 8> R64;

        uint64_t start[2] = {0, 0};
        uint64_t count[2] = {4, 2};
        ADIOS_SELECTION *sel = adios_selection_boundingbox(2, start, count);

        // Read stuff
        for (size_t t = 0; t < 3; ++t)
        {
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
            for (size_t i = 0; i < 8; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i;
                std::string msg = ss.str();

                EXPECT_EQ(I8[i], m_TestData.I8[i + t]) << msg;
                EXPECT_EQ(I16[i], m_TestData.I16[i + t]) << msg;
                EXPECT_EQ(I32[i], m_TestData.I32[i + t]) << msg;
                EXPECT_EQ(I64[i], m_TestData.I64[i + t]) << msg;
                EXPECT_EQ(U8[i], m_TestData.U8[i + t]) << msg;
                EXPECT_EQ(U16[i], m_TestData.U16[i + t]) << msg;
                EXPECT_EQ(U32[i], m_TestData.U32[i + t]) << msg;
                EXPECT_EQ(U64[i], m_TestData.U64[i + t]) << msg;
                EXPECT_EQ(R32[i], m_TestData.R32[i + t]) << msg;
                EXPECT_EQ(R64[i], m_TestData.R64[i + t]) << msg;
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
    }
}

// ADIOS2 write, ADIOS2 read
TEST_F(BPWriteReadTest, DISABLED_ADIOS2BPWriteADIOS2BPRead2D4x2)
{
    std::string fname = "ADIOS2BPWriteADIOS2BPRead2D4x2Test.bp";

    ASSERT_TRUE(false) << "ADIOS2 read API is not yet implemented";
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
