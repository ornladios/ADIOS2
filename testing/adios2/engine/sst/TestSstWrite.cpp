/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include <cstdint>
#include <cstring>
#include <ctime>

#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#include "TestData.h"

class SstWriteTest : public ::testing::Test
{
public:
    SstWriteTest() = default;
};

int CompressSz = 0;
int CompressZfp = 0;

adios2::Params engineParams = {}; // parsed from command line

static std::string Trim(std::string &str)
{
    size_t first = str.find_first_not_of(' ');
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

/*
 * Engine parameters spec is a poor-man's JSON.  name:value pairs are separated
 * by commas.  White space is trimmed off front and back.  No quotes or anything
 * fancy allowed.
 */
static adios2::Params ParseEngineParams(std::string Input)
{
    std::istringstream ss(Input);
    std::string Param;
    adios2::Params Ret = {};

    while (std::getline(ss, Param, ','))
    {
        std::istringstream ss2(Param);
        std::string ParamName;
        std::string ParamValue;
        std::getline(ss2, ParamName, ':');
        if (!std::getline(ss2, ParamValue, ':'))
        {
            throw std::invalid_argument("Engine parameter \"" + Param +
                                        "\" missing value");
        }
        Ret[Trim(ParamName)] = Trim(ParamValue);
    }
    return Ret;
}

// ADIOS2 SST write
TEST_F(SstWriteTest, ADIOS2SstWrite)
{
    // form a mpiSize * Nx 1D array
    const std::string fname = "ADIOS2Sst";

    int mpiRank = 0, mpiSize = 1;

    // Number of steps
    const std::size_t NSteps = 10;

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

    // Declare 1D variables (NumOfProcesses * Nx)
    // The local process' part (start, count) can be defined now or later
    // before Write().
    {
        adios2::Dims shape{static_cast<unsigned int>(Nx * mpiSize)};
        adios2::Dims start{static_cast<unsigned int>(Nx * mpiRank)};
        adios2::Dims count{static_cast<unsigned int>(Nx)};
        adios2::Dims shape2{static_cast<unsigned int>(Nx * mpiSize), 2};
        adios2::Dims start2{static_cast<unsigned int>(Nx * mpiRank), 0};
        adios2::Dims count2{static_cast<unsigned int>(Nx), 2};
        adios2::Dims shape3{2, static_cast<unsigned int>(Nx * mpiSize)};
        adios2::Dims start3{0, static_cast<unsigned int>(Nx * mpiRank)};
        adios2::Dims count3{2, static_cast<unsigned int>(Nx)};
        adios2::Dims shape4{static_cast<unsigned int>(Nx * 10 * mpiSize)};
        adios2::Dims start4{static_cast<unsigned int>(Nx * 10 * mpiRank)};
        adios2::Dims count4{static_cast<unsigned int>(Nx * 10)};
        adios2::Dims time_shape{static_cast<unsigned int>(mpiSize)};
        adios2::Dims time_start{static_cast<unsigned int>(mpiRank)};
        adios2::Dims time_count{1};
        auto var_i8 = io.DefineVariable<int8_t>("i8", shape, start, count);
        auto var_i16 = io.DefineVariable<int16_t>("i16", shape, start, count);
        auto var_i32 = io.DefineVariable<int32_t>("i32", shape, start, count);
        auto var_i64 = io.DefineVariable<int64_t>("i64", shape, start, count);
        auto var_r32 = io.DefineVariable<float>("r32", shape, start, count);
        auto var_r32_large =
            io.DefineVariable<float>("r32_large", shape4, start4, count4);
        auto var_r64 = io.DefineVariable<double>("r64", shape, start, count);
        auto var_r64_2d =
            io.DefineVariable<double>("r64_2d", shape2, start2, count2);
        auto var_r64_2d_rev =
            io.DefineVariable<double>("r64_2d_rev", shape3, start3, count3);
        auto var_time = io.DefineVariable<int64_t>("time", time_shape,
                                                   time_start, time_count);
        if (CompressSz)
        {
            adios2::Operator SzOp = adios.DefineOperator("szCompressor", "sz");
            var_r32_large.AddOperation(SzOp, {{"accuracy", "0.001"}});
        }
        if (CompressZfp)
        {
            adios2::Operator ZfpOp =
                adios.DefineOperator("zfpCompressor", "zfp");
            var_r32.AddOperation(ZfpOp, {{"rate", "20"}});
            var_r64.AddOperation(ZfpOp, {{"rate", "20"}});
            var_r64_2d.AddOperation(ZfpOp, {{"rate", "20"}});
            var_r64_2d_rev.AddOperation(ZfpOp, {{"rate", "20"}});
        }
    }

    // Create the Engine
    io.SetEngine("Sst");
    io.SetParameters(engineParams);

    adios2::Engine engine = io.Open(fname, adios2::Mode::Write);

    for (size_t step = 0; step < NSteps; ++step)
    {
        // Generate test data for each process uniquely
        generateSstTestData(step, mpiRank, mpiSize);

        engine.BeginStep();
        // Retrieve the variables that previously went out of scope
        auto var_i8 = io.InquireVariable<int8_t>("i8");
        auto var_i16 = io.InquireVariable<int16_t>("i16");
        auto var_i32 = io.InquireVariable<int32_t>("i32");
        auto var_i64 = io.InquireVariable<int64_t>("i64");
        auto var_u8 = io.InquireVariable<uint8_t>("u8");
        auto var_r32 = io.InquireVariable<float>("r32");
        auto var_r32_large = io.InquireVariable<float>("r32_large");
        auto var_r64 = io.InquireVariable<double>("r64");
        auto var_r64_2d = io.InquireVariable<double>("r64_2d");
        auto var_r64_2d_rev = io.InquireVariable<double>("r64_2d_rev");
        auto var_time = io.InquireVariable<int64_t>("time");

        // Make a 1D selection to describe the local dimensions of the
        // variable we write and its offsets in the global spaces
        adios2::Box<adios2::Dims> sel({mpiRank * Nx}, {Nx});
        adios2::Box<adios2::Dims> sel2({mpiRank * Nx, 0}, {Nx, 2});
        adios2::Box<adios2::Dims> sel3({0, mpiRank * Nx}, {2, Nx});
        adios2::Box<adios2::Dims> sel_time(
            {static_cast<unsigned long>(mpiRank)}, {1});
        adios2::Box<adios2::Dims> sel4({mpiRank * Nx * 10}, {Nx * 10});
        var_i8.SetSelection(sel);
        var_i16.SetSelection(sel);
        var_i32.SetSelection(sel);
        var_i64.SetSelection(sel);
        var_r32.SetSelection(sel);
        var_r32_large.SetSelection(sel4);
        var_r64.SetSelection(sel);
        var_r64_2d.SetSelection(sel2);
        var_r64_2d_rev.SetSelection(sel3);
        var_time.SetSelection(sel_time);

        // Write each one
        // fill in the variable with values from starting index to
        // starting index + count
        const adios2::Mode sync = adios2::Mode::Sync;

        engine.Put(var_i8, data_I8.data(), sync);
        engine.Put(var_i16, data_I16.data(), sync);
        engine.Put(var_i32, data_I32.data(), sync);
        engine.Put(var_i64, data_I64.data(), sync);
        engine.Put(var_r32, data_R32.data(), sync);
        engine.Put(var_r32_large, data_R32_large.data(), sync);
        engine.Put(var_r64, data_R64.data(), sync);
        engine.Put(var_r64_2d, &data_R64_2d[0][0], sync);
        engine.Put(var_r64_2d_rev, &data_R64_2d_rev[0][0], sync);
        // Advance to the next time step
        std::time_t localtime = std::time(NULL);
        engine.Put(var_time, (int64_t *)&localtime);
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

    int result;
    ::testing::InitGoogleTest(&argc, argv);

    while ((argc > 1) && (argv[1][0] == '-'))
    {
        if (std::string(argv[1]) == "--expect_time_gap")
        {
            //  TimeGapExpected++;   Nothing on write side
        }
        else if (std::string(argv[1]) == "--compress_sz")
        {
            CompressSz++;
        }
        else if (std::string(argv[1]) == "--compress_zfp")
        {
            CompressZfp++;
        }
        else
        {
            throw std::invalid_argument("Unknown argument \"" +
                                        std::string(argv[1]) + "\"");
        }
        argv++;
        argc--;
    }
    if (argc > 1)
    {
        engineParams = ParseEngineParams(argv[1]);
    }

    result = RUN_ALL_TESTS();

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return result;
}
