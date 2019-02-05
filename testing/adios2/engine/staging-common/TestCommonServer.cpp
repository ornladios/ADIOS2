/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <iostream>
#include <signal.h>
#include <sstream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#include "TestData.h"

class CommonServerTest : public ::testing::Test
{
public:
    CommonServerTest() = default;
};

adios2::Params engineParams = {};         // parsed from command line
int DurationSeconds = 60 * 60 * 24 * 365; // one year default
int DelayMS = 1000;                       // one step per sec default
static int MyCloseNow = 0;
static int GlobalCloseNow = 0;
std::string fname = "ADIOS2CommonServer";
std::string engine = "SST";

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

std::string shutdown_name = "DieTest";

inline bool file_exists(const std::string &name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

// ADIOS2 COMMON write
TEST_F(CommonServerTest, ADIOS2CommonServer)
{
    int mpiRank = 0, mpiSize = 1;

    // Number of steps

    std::remove(shutdown_name.c_str());
#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

// Server test data using ADIOS2

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
        adios2::Dims time_shape{static_cast<unsigned int>(mpiSize)};
        adios2::Dims time_start{static_cast<unsigned int>(mpiRank)};
        adios2::Dims time_count{1};
        io.DefineVariable<double>("scalar_r64");
        io.DefineVariable<int8_t>("i8", shape, start, count);
        io.DefineVariable<int16_t>("i16", shape, start, count);
        io.DefineVariable<int32_t>("i32", shape, start, count);
        io.DefineVariable<int64_t>("i64", shape, start, count);
        io.DefineVariable<float>("r32", shape, start, count);
        io.DefineVariable<double>("r64", shape, start, count);
        io.DefineVariable<std::complex<float>>("c32", shape, start, count);
        io.DefineVariable<std::complex<double>>("c64", shape, start, count);
        io.DefineVariable<double>("r64_2d", shape2, start2, count2);
        io.DefineVariable<double>("r64_2d_rev", shape3, start3, count3);
        io.DefineVariable<int64_t>("time", time_shape, time_start, time_count);
    }

    // Create the Engine
    io.SetEngine(engine);
    io.SetParameters(engineParams);

    adios2::Engine engine = io.Open(fname, adios2::Mode::Write);

    // Advance to the next time step
    std::time_t EndTime = std::time(NULL) + DurationSeconds;
    size_t step = 0;

    while ((std::time(NULL) < EndTime) && !GlobalCloseNow)
    {
        // Generate test data for each process uniquely
        generateCommonTestData(step, mpiRank, mpiSize);

        engine.BeginStep();
        // Retrieve the variables that previously went out of scope
        auto scalar_r64 = io.InquireVariable<double>("scalar_r64");
        auto var_i8 = io.InquireVariable<int8_t>("i8");
        auto var_i16 = io.InquireVariable<int16_t>("i16");
        auto var_i32 = io.InquireVariable<int32_t>("i32");
        auto var_i64 = io.InquireVariable<int64_t>("i64");
        auto var_u8 = io.InquireVariable<uint8_t>("u8");
        auto var_r32 = io.InquireVariable<float>("r32");
        auto var_r64 = io.InquireVariable<double>("r64");
        auto var_c32 = io.InquireVariable<std::complex<float>>("c32");
        auto var_c64 = io.InquireVariable<std::complex<double>>("c64");
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
        var_i8.SetSelection(sel);
        var_i16.SetSelection(sel);
        var_i32.SetSelection(sel);
        var_i64.SetSelection(sel);
        var_r32.SetSelection(sel);
        var_r64.SetSelection(sel);
        var_c32.SetSelection(sel);
        var_c64.SetSelection(sel);
        var_r64_2d.SetSelection(sel2);
        var_r64_2d_rev.SetSelection(sel3);
        var_time.SetSelection(sel_time);

        // Write each one
        // fill in the variable with values from starting index to
        // starting index + count
        const adios2::Mode sync = adios2::Mode::Sync;

        engine.Put(scalar_r64, data_scalar_R64, sync);
        engine.Put(var_i8, data_I8.data(), sync);
        engine.Put(var_i16, data_I16.data(), sync);
        engine.Put(var_i32, data_I32.data(), sync);
        engine.Put(var_i64, data_I64.data(), sync);
        engine.Put(var_r32, data_R32.data(), sync);
        engine.Put(var_r64, data_R64.data(), sync);
        engine.Put(var_c32, data_C32.data(), sync);
        engine.Put(var_c64, data_C64.data(), sync);
        engine.Put(var_r64_2d, &data_R64_2d[0][0], sync);
        engine.Put(var_r64_2d_rev, &data_R64_2d_rev[0][0], sync);
        // Advance to the next time step
        std::time_t localtime = std::time(NULL);
        engine.Put(var_time, (int64_t *)&localtime);
        engine.EndStep();
        usleep(1000 * DelayMS); /* sleep for DelayMS milliseconds */
        step++;
#ifdef ADIOS2_HAVE_MPI
        MPI_Allreduce(&MyCloseNow, &GlobalCloseNow, 1, MPI_INT, MPI_LOR,
                      MPI_COMM_WORLD);
        if (file_exists(shutdown_name))
        {
            MyCloseNow = GlobalCloseNow = 1;
        }
#else
        GlobalCloseNow = MyCloseNow;
        if (file_exists(shutdown_name))
        {
            MyCloseNow = GlobalCloseNow = 1;
        }
#endif
    }
    // Close the file
    engine.Close();
}

int main(int argc, char **argv)
{
#ifdef ADIOS2_HAVE_MPI
    MPI_Init(nullptr, nullptr);
#endif

    int result, bare_args = 0;
    ::testing::InitGoogleTest(&argc, argv);

    while (argc > 1)
    {
        if (std::string(argv[1]) == "--duration")
        {
            std::istringstream ss(argv[2]);
            if (!(ss >> DurationSeconds))
                std::cerr << "Invalid number for duration " << argv[1] << '\n';
            argv++;
            argc--;
        }
        else if (std::string(argv[1]) == "--shutdown_filename")
        {
            shutdown_name = std::string(argv[2]);
            argv++;
            argc--;
        }
        else if (std::string(argv[1]) == "--ms_delay")
        {
            std::istringstream ss(argv[2]);
            if (!(ss >> DelayMS))
                std::cerr << "Invalid number for ms_delay " << argv[1] << '\n';
            argv++;
            argc--;
        }
        else if (std::string(argv[1]) == "--filename")
        {
            fname = std::string(argv[2]);
            argv++;
            argc--;
        }
        else if (std::string(argv[1]) == "--engine")
        {
            engine = std::string(argv[2]);
            argv++;
            argc--;
        }
        else if (std::string(argv[1]) == "--engine_params")
        {
            std::cout << "PArsing engineparams in -- " << argv[2] << std::endl;

            engineParams = ParseEngineParams(argv[2]);
            argv++;
            argc--;
        }
        else
        {
            if (bare_args == 0)
            {
                /* first arg without -- is engine */
                engine = std::string(argv[1]);
            }
            if (bare_args == 1)
            {
                /* second arg without -- is filename */
                fname = std::string(argv[1]);
            }
            if (bare_args == 2)
            {
                /* third arg without -- is engine params */
                engineParams = ParseEngineParams(argv[1]);
            }
            if (bare_args > 2)
            {
                throw std::invalid_argument("Unknown argument \"" +
                                            std::string(argv[1]) + "\"");
            }
            bare_args++;
        }
        argv++;
        argc--;
    }

    result = RUN_ALL_TESTS();

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return result;
}
