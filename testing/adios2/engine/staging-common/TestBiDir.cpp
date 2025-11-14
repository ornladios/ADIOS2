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

int writer_first = 0;

#define TEST_SPECIFIC_ARGS                                                                         \
    else if (std::string(argv[1]) == "--writer_first")                                             \
    {                                                                                              \
        writer_first = 1;                                                                          \
    }

#include "ParseArgs.h"

class BiDir : public ::testing::Test
{
public:
    BiDir() = default;
};

#if ADIOS2_USE_MPI
MPI_Comm testComm;
#endif

// ADIOS2 COMMON write
TEST_F(BiDir, ADIOS2BiDir)
{
    // form a mpiSize * Nx 1D array
    int mpiRank = 0, mpiSize = 1;
    std::string writer_fname, reader_fname;
    int ReaderSteps = 0;

    if (writer_first)
    {
        writer_fname = "ServerSource";
        reader_fname = "ClientSource";
    }
    else
    {
        reader_fname = "ServerSource";
        writer_fname = "ClientSource";
    }

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
    adios2::IO reader_io = adios.DeclareIO("ReaderTestIO");
    adios2::IO writer_io = adios.DeclareIO("WriterTestIO");

    std::size_t r64_Nx = Nx;
    std::cout << "Nx is set to " << r64_Nx << " on Rank " << mpiRank << std::endl;

    // Declare 1D variables (NumOfProcesses * Nx)
    // The local process' part (start, count) can be defined now or later
    // before Write().
    {
        adios2::Dims shape{static_cast<unsigned int>(Nx * mpiSize)};
        adios2::Dims start{static_cast<unsigned int>(Nx * mpiRank)};
        adios2::Dims count_r64{static_cast<unsigned int>(r64_Nx)};
        adios2::Dims start_r64{static_cast<unsigned int>(Nx * mpiRank)};
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

        (void)writer_io.DefineVariable<double>("scalar_r64");
        (void)writer_io.DefineVariable<int8_t>("i8", shape, start, count);
        (void)writer_io.DefineVariable<int16_t>("i16", shape, start, count);
        (void)writer_io.DefineVariable<int32_t>("i32(testparen)", shape, start, count);
        (void)writer_io.DefineVariable<int64_t>("i64", shape, start, count);
        (void)writer_io.DefineVariable<float>("r32", shape, start, count);
        (void)writer_io.DefineVariable<double>("r64", shape, start_r64, count_r64);
        (void)writer_io.DefineVariable<std::complex<float>>("c32", shape, start, count);
        (void)writer_io.DefineVariable<std::complex<double>>("c64", shape, start, count);
        (void)writer_io.DefineVariable<double>("r64_2d", shape2, start2, count2);
        (void)writer_io.DefineVariable<double>("r64_2d_rev", shape3, start3, count3);
        (void)writer_io.DefineVariable<int64_t>("time", time_shape, time_start, time_count);
    }

    // Create the Engine
    writer_io.SetEngine(engine);
    writer_io.SetParameters(engineParams);
    reader_io.SetEngine(engine);
    reader_io.SetParameters(engineParams);
    adios2::Engine writer;
    adios2::Engine reader;
    if (writer_first)
    {
        writer = writer_io.Open(writer_fname, adios2::Mode::Write);
        reader = reader_io.Open(reader_fname, adios2::Mode::Read);
    }
    else
    {
        reader = reader_io.Open(reader_fname, adios2::Mode::Read);
        writer = writer_io.Open(writer_fname, adios2::Mode::Write);
    }
    for (int step = 0; step < NSteps; ++step)
    {
        // Generate test data for each process uniquely
        generateCommonTestData((int)step, mpiRank, mpiSize, (int)Nx, (int)r64_Nx);

        writer.BeginStep();
        // Retrieve the variables that previously went out of scope
        auto scalar_r64 = writer_io.InquireVariable<double>("scalar_r64");
        auto var_i8 = writer_io.InquireVariable<int8_t>("i8");
        auto var_i16 = writer_io.InquireVariable<int16_t>("i16");
        auto var_i32 = writer_io.InquireVariable<int32_t>("i32(testparen)");
        auto var_i64 = writer_io.InquireVariable<int64_t>("i64");
        auto var_r32 = writer_io.InquireVariable<float>("r32");
        auto var_r64 = writer_io.InquireVariable<double>("r64");
        auto var_c32 = writer_io.InquireVariable<std::complex<float>>("c32");
        auto var_c64 = writer_io.InquireVariable<std::complex<double>>("c64");
        auto var_r64_2d = writer_io.InquireVariable<double>("r64_2d");
        auto var_r64_2d_rev = writer_io.InquireVariable<double>("r64_2d_rev");
        auto var_time = writer_io.InquireVariable<int64_t>("time");

        // Make a 1D selection to describe the local dimensions of the
        // variable we write and its offsets in the global spaces
        adios2::Box<adios2::Dims> sel({mpiRank * Nx}, {Nx});
        adios2::Box<adios2::Dims> sel_r64({mpiRank * Nx}, {r64_Nx});
        adios2::Box<adios2::Dims> sel2({mpiRank * Nx, 0}, {Nx, 2});
        adios2::Box<adios2::Dims> sel3({0, mpiRank * Nx}, {2, Nx});
        adios2::Box<adios2::Dims> sel_time({static_cast<unsigned long>(mpiRank)}, {1});

        var_i8.SetSelection(sel);
        var_i16.SetSelection(sel);
        var_i32.SetSelection(sel);
        var_i64.SetSelection(sel);
        var_r32.SetSelection(sel);
        var_r64.SetSelection(sel_r64);
        var_c32.SetSelection(sel);
        var_c64.SetSelection(sel);
        var_r64_2d.SetSelection(sel2);
        var_r64_2d_rev.SetSelection(sel3);
        var_time.SetSelection(sel_time);

        // Write each one
        // fill in the variable with values from starting index to
        // starting index + count
        const adios2::Mode sync = adios2::Mode::Deferred;
        std::time_t localtime;
        if (mpiRank == 0)
            writer.Put(scalar_r64, data_scalar_R64);
        writer.Put(var_i8, data_I8.data(), sync);
        writer.Put(var_i16, data_I16.data(), sync);
        writer.Put(var_i32, data_I32.data(), sync);
        writer.Put(var_i64, data_I64.data(), sync);
        writer.Put(var_r32, data_R32.data(), sync);
        writer.Put(var_r64, data_R64.data(), sync);
        writer.Put(var_c32, data_C32.data(), sync);
        writer.Put(var_c64, data_C64.data(), sync);
        writer.Put(var_r64_2d, &data_R64_2d[0], sync);
        writer.Put(var_r64_2d_rev, &data_R64_2d_rev[0], sync);
        // Advance to the next time step
        localtime = std::time(NULL);
        writer.Put(var_time, (int64_t *)&localtime);
        writer.EndStep();
        auto result = reader.BeginStep();
        if (result == adios2::StepStatus::EndOfStream)
            break;
        auto reader_var_time = reader_io.InquireVariable<int64_t>("time");
        auto reader_var_r64 = reader_io.InquireVariable<double>("r64");
        auto writerSize = reader_var_time.Shape()[0];
        long unsigned int myStart = (long unsigned int)(writerSize * Nx / mpiSize) * mpiRank;
        long unsigned int myLength = (long unsigned int)((writerSize * Nx + mpiSize - 1) / mpiSize);
        const adios2::Dims start{myStart};
        const adios2::Dims count{myLength};
        const adios2::Box<adios2::Dims> reader_sel(start, count);
        reader_var_r64.SetSelection(reader_sel);
        in_R64.resize(myLength);
        reader.Get(reader_var_r64, in_R64.data(), GlobalReadMode);
        reader.EndStep();
        ReaderSteps++;
    }

    writer.Close();
    reader.Close();
    std::cout << "Reader Completed " << ReaderSteps << " Steps." << std::endl;
}

int main(int argc, char **argv)
{
    int result;
    ::testing::InitGoogleTest(&argc, argv);

    DelayMS = 0; // zero for common writer

    ParseArgs(argc, argv);

#if ADIOS2_USE_MPI
    int provided;
    int thread_support_level =
        (engine == "SST" || engine == "sst") ? MPI_THREAD_MULTIPLE : MPI_THREAD_SINGLE;

    // MPI_THREAD_MULTIPLE is only required if you enable the SST MPI_DP
    MPI_Init_thread(nullptr, nullptr, thread_support_level, &provided);

    int key;
    MPI_Comm_rank(MPI_COMM_WORLD, &key);

    const unsigned int color = writer_first;
    MPI_Comm_split(MPI_COMM_WORLD, color, key, &testComm);
#endif

    result = RUN_ALL_TESTS();

#if ADIOS2_USE_MPI
#ifdef CRAY_MPICH_VERSION
    MPI_Barrier(MPI_COMM_WORLD);
#else
    MPI_Finalize();
#endif
#endif

    return result;
}
