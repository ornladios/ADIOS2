/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TestBPWriteProfilingJSON.cpp
 *
 *  Created on: Jul 18, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include <cstdint>
#include <cstring>

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>
#include <json.hpp> //This fails to be included

#include "../SmallTestData.h"

using json = nlohmann::json;

class BPWriteProfilingJSONTest : public ::testing::Test
{
public:
    BPWriteProfilingJSONTest() = default;

    SmallTestData m_TestData;
};

//******************************************************************************
// 1D 1x8 test data
//******************************************************************************

// ADIOS2 write, native ADIOS1 read
TEST_F(BPWriteProfilingJSONTest, ADIOS2BPWriteProfilingJSON)
{
    std::string fname = "ADIOS2BPWriteProfilingJSON.bp";

    // Write test data and profiling.json using ADIOS2
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
        io.SetParameters({{"Threads", "2"}});
        io.AddTransport("File", {{"Library", "POSIX"}});

        auto engine = io.Open(fname, adios2::OpenMode::Write);
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

    // open json file, parse it to a json structure, and verify a few things
    {
        std::ifstream profilingJSONFile(fname + ".dir/profiling.json");
        std::stringstream buffer;
        buffer << profilingJSONFile.rdbuf();

        const json profilingJSON = json::parse(buffer);

        // check rank is zero
        const int rank = profilingJSON[0].value("rank", -1);
        ASSERT_EQ(rank, 0);

        // check threads
        const int threads = profilingJSON[0].value("threads", 0);
        ASSERT_EQ(threads, 2);

        // check bytes
        const unsigned long int bytes = profilingJSON[0].value("bytes", 0UL);
        ASSERT_EQ(bytes, 6536);

        const auto transportType =
            profilingJSON[0]["transport_0"].value("type", "0");
        ASSERT_EQ(transportType, "File_POSIX");
    }
}
