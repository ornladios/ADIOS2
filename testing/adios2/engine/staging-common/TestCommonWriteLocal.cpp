/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include <cstdint>
#include <cstring>
#include <ctime>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

#include <adios2.h>

#include <gtest/gtest.h>

#include "TestData.h"

#include "ParseArgs.h"

class CommonWriteTest : public ::testing::Test
{
public:
    CommonWriteTest() = default;
};

#if ADIOS2_USE_MPI
MPI_Comm testComm;
#endif

// ADIOS2 COMMON write
TEST_F(CommonWriteTest, ADIOS2CommonWrite)
{
    // form a mpiSize * Nx 1D array
    int mpiRank = 0, mpiSize = 1;

#if ADIOS2_USE_MPI
    MPI_Comm_rank(testComm, &mpiRank);
    MPI_Comm_size(testComm, &mpiSize);
#endif

    // Write test data using ADIOS2

#if ADIOS2_USE_MPI
    adios2::ADIOS adios(testComm);
#else
    adios2::ADIOS adios;
#endif
    adios2::IO io = adios.DeclareIO("TestIO");

    // Declare 1D variables (NumOfProcesses * Nx)
    // The local process' part (start, count) can be defined now or later
    // before Write().

    /* we'll be dropping the size for some vars */
    if (VaryingDataSize)
    {
        assert(Nx > NSteps + mpiSize);
    }

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
        adios2::Dims time_shape{static_cast<unsigned int>(mpiSize)};
        adios2::Dims time_start{static_cast<unsigned int>(mpiRank)};
        adios2::Dims time_count{1};

        (void)io.DefineVariable<double>("scalar_r64");
        (void)io.DefineVariable<int8_t>("i8", {}, {}, count);
        (void)io.DefineVariable<int16_t>("i16", {}, {}, count);
        (void)io.DefineVariable<int32_t>("i32", {}, {}, count);
        (void)io.DefineVariable<int64_t>("i64", {}, {}, count);
        auto local_var_r32 = io.DefineVariable<float>("r32", {}, {}, count);
        auto local_var_r64 = io.DefineVariable<double>("r64", {}, {}, count);
        (void)io.DefineVariable<std::complex<float>>("c32", {}, {}, count);
        (void)io.DefineVariable<std::complex<double>>("c64", {}, {}, count);
        auto local_var_r64_2d =
            io.DefineVariable<double>("r64_2d", {}, {}, count2);
        auto local_var_r64_2d_rev =
            io.DefineVariable<double>("r64_2d_rev", {}, {}, count3);
        (void)io.DefineVariable<int64_t>("time", time_shape, time_start,
                                         time_count);
        if (CompressZfp)
        {
            adios2::Operator ZfpOp =
                adios.DefineOperator("zfpCompressor", "zfp");
            local_var_r32.AddOperation(ZfpOp, {{"rate", "20"}});
            local_var_r64.AddOperation(ZfpOp, {{"rate", "20"}});
            local_var_r64_2d.AddOperation(ZfpOp, {{"rate", "20"}});
            local_var_r64_2d_rev.AddOperation(ZfpOp, {{"rate", "20"}});
        }
    }

    // Create the Engine
    io.SetEngine(engine);
    io.SetParameters(engineParams);

    adios2::Engine engine = io.Open(fname, adios2::Mode::Write);

    for (size_t step = 0; step < NSteps; ++step)
    {
        // Generate test data for each process uniquely
        generateCommonTestData((int)step, mpiRank, mpiSize, (int)Nx, (int)Nx);

        engine.BeginStep();
        // Retrieve the variables that previously went out of scope
        auto scalar_r64 = io.InquireVariable<double>("scalar_r64");
        auto var_i8 = io.InquireVariable<int8_t>("i8");
        auto var_i16 = io.InquireVariable<int16_t>("i16");
        auto var_i32 = io.InquireVariable<int32_t>("i32");
        auto var_i64 = io.InquireVariable<int64_t>("i64");
        auto var_r32 = io.InquireVariable<float>("r32");
        auto var_r64 = io.InquireVariable<double>("r64");
        auto var_c32 = io.InquireVariable<std::complex<float>>("c32");
        auto var_c64 = io.InquireVariable<std::complex<double>>("c64");
        auto var_r64_2d = io.InquireVariable<double>("r64_2d");
        auto var_r64_2d_rev = io.InquireVariable<double>("r64_2d_rev");
        auto var_time = io.InquireVariable<int64_t>("time");

        // // Make a 1D selection to describe the local dimensions of the
        // // variable we write
        adios2::Box<adios2::Dims> sel_shrinking({}, {Nx - step - mpiRank});
        // adios2::Box<adios2::Dims> sel({mpiRank * Nx}, {Nx});
        // adios2::Box<adios2::Dims> sel2({mpiRank * Nx, 0}, {Nx, 2});
        // adios2::Box<adios2::Dims> sel3({0, mpiRank * Nx}, {2, Nx});
        // adios2::Box<adios2::Dims> sel_time(
        //     {static_cast<unsigned long>(mpiRank)}, {1});
        if (VaryingDataSize)
        {
            var_i8.SetSelection(sel_shrinking);
        }
        // var_i16.SetSelection(sel);
        // var_i32.SetSelection(sel);
        // var_i64.SetSelection(sel);
        // var_r32.SetSelection(sel);
        // var_r64.SetSelection(sel);
        // var_c32.SetSelection(sel);
        // var_c64.SetSelection(sel);
        // var_r64_2d.SetSelection(sel2);
        // var_r64_2d_rev.SetSelection(sel3);
        // var_time.SetSelection(sel_time);

        // Write each one
        // fill in the variable with values from starting index to
        // starting index + count
        const adios2::Mode sync = adios2::Mode::Deferred;

        if (mpiRank == 0)
            engine.Put(scalar_r64, data_scalar_R64);
        engine.Put(var_i8, data_I8.data(), sync);
        engine.Put(var_i16, data_I16.data(), sync);
        engine.Put(var_i32, data_I32.data(), sync);
        engine.Put(var_i64, data_I64.data(), sync);
        engine.Put(var_r32, data_R32.data(), sync);
        for (int index = 1; index < LocalCount; index++)
        {
            for (int i = 0; i < data_R32.size(); i++)
            {
                data_R32[i] += 1000.0;
            }
            engine.Put(var_r32, data_R32.data(), sync);
        }
        engine.Put(var_r64, data_R64.data(), sync);
        engine.Put(var_c32, data_C32.data(), sync);
        engine.Put(var_c64, data_C64.data(), sync);
        engine.Put(var_r64_2d, &data_R64_2d[0], sync);
        engine.Put(var_r64_2d_rev, &data_R64_2d_rev[0], sync);
        // Advance to the next time step
        std::time_t localtime = std::time(NULL);
        engine.Put(var_time, (int64_t *)&localtime);
        engine.EndStep();
        std::this_thread::sleep_for(std::chrono::milliseconds(
            DelayMS)); /* sleep for DelayMS milliseconds */
    }

    // Close the file
    engine.Close();
}

int main(int argc, char **argv)
{
#if ADIOS2_USE_MPI
    MPI_Init(nullptr, nullptr);

    int key;
    MPI_Comm_rank(MPI_COMM_WORLD, &key);

    const unsigned int color = 1;
    MPI_Comm_split(MPI_COMM_WORLD, color, key, &testComm);
#endif

    int result;
    ::testing::InitGoogleTest(&argc, argv);

    DelayMS = 0; // zero default for common writer

    ParseArgs(argc, argv);

    result = RUN_ALL_TESTS();

#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif

    return result;
}
