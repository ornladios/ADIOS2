/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include <cstdint>
#include <cstring>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

#include <adios2.h>

#include <gtest/gtest.h>

#include "TestData.h"

class SstReadTest : public ::testing::Test
{
public:
    SstReadTest() = default;
};

adios2::Params engineParams = {}; // parsed from command line
int TimeGapExpected = 0;
int ExpectWriterFailure = 0;
int WriterFailed = 0;
int IgnoreTimeGap = 1;
int IncreasingDelay = 0;
int DelayWhileHoldingStep = 0;
int FirstTimestepMustBeZero = 0;
int NonBlockingBeginStep = 0;
int Latest = 0;
int Discard = 0;
int BeginStepFailedPolls = 0;
int SkippedSteps = 0;
int DelayMS = 500;
int LongFirstDelay = 0;
// Number of steps
size_t NSteps = 10;
std::string fname = "ADIOS2SstServer";
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

// ADIOS2 Sst read
TEST_F(SstReadTest, ADIOS2SstRead)
{
    // Each process would write a 1x8 array and all processes would
    // form a mpiSize * Nx 1D array
    int mpiRank = 0, mpiSize = 1;

    int TimeGapDetected = 0;
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

    // Create the Engine
    io.SetEngine(engine);
    io.SetParameters(engineParams);
    if (Latest)
    {
        io.SetParameters({{"AlwaysProvideLatestTimestep", "true"}});
    }

    adios2::Engine engine = io.Open(fname, adios2::Mode::Read);

    size_t ExpectedStep = adios2::MaxSizeT;

    std::vector<std::time_t> write_times;

    bool Continue = true;
    while (Continue)
    {

        adios2::StepStatus Status;

        ASSERT_FALSE(SstReadTest::HasNonfatalFailure()); // exit if we've failed
                                                         // something
        if (NonBlockingBeginStep)
        {
            Status = engine.BeginStep(adios2::StepMode::Read, 0.0);

            while (Status == adios2::StepStatus::NotReady)
            {
                BeginStepFailedPolls++;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                Status = engine.BeginStep(adios2::StepMode::Read, 0.0);
            }
        }
        else if (Latest)
        {
            if (LongFirstDelay)
            {
                LongFirstDelay = 0;
                std::this_thread::sleep_for(std::chrono::seconds(3));
            }
            /* would like to do blocking, but API is inconvenient, so specify an
             * hour timeout */
            Status = engine.BeginStep(adios2::StepMode::Read, 60 * 60.0);
        }
        else
        {
            Status = engine.BeginStep();
        }

        if (Status != adios2::StepStatus::OK)
        {
            if (Status == adios2::StepStatus::OtherError)
            {
                WriterFailed = 1;
                std::cout << "Noticed Writer failure" << std::endl;
            }
            Continue = false;
            break;
        }

        const size_t currentStep = engine.CurrentStep();

        if (FirstTimestepMustBeZero)
        {
            if (ExpectedStep == adios2::MaxSizeT)
            {
                EXPECT_EQ(currentStep, 0);
                std::cout << "Got my expected first timestep Zero!"
                          << std::endl;
                ExpectedStep = 0;
            }
            else if (ExpectedStep == 1)
            {
                /* we got that timestep 0 we expected, ExpectedStep got
                 * incremented to 1, but now be happy with what is next */
                ExpectedStep = currentStep; // starting out
            }
        }
        if ((ExpectedStep == adios2::MaxSizeT) || Latest || Discard)
        {
            if ((ExpectedStep != adios2::MaxSizeT) &&
                (ExpectedStep != currentStep))
            {
                SkippedSteps++;
            }
            ExpectedStep = currentStep; // starting out
        }

        EXPECT_EQ(currentStep, ExpectedStep);

        size_t writerSize;

        auto scalar_r64 = io.InquireVariable<double>("scalar_r64");
        EXPECT_TRUE(scalar_r64);

        auto var_i8 = io.InquireVariable<int8_t>("i8");
        EXPECT_TRUE(var_i8);
        ASSERT_EQ(var_i8.ShapeID(), adios2::ShapeID::GlobalArray);
        /* must be a multiple of Nx */
        ASSERT_EQ(var_i8.Shape()[0] % Nx, 0);

        /* take the first size as something that gives us writer size */
        writerSize = var_i8.Shape()[0] / 10;

        auto var_i16 = io.InquireVariable<int16_t>("i16");
        EXPECT_TRUE(var_i16);
        ASSERT_EQ(var_i16.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i16.Shape()[0], writerSize * Nx);

        auto var_i32 = io.InquireVariable<int32_t>("i32");
        EXPECT_TRUE(var_i32);
        ASSERT_EQ(var_i32.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i32.Shape()[0], writerSize * Nx);

        auto var_i64 = io.InquireVariable<int64_t>("i64");
        EXPECT_TRUE(var_i64);
        ASSERT_EQ(var_i64.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_i64.Shape()[0], writerSize * Nx);

        auto var_r32 = io.InquireVariable<float>("r32");
        EXPECT_TRUE(var_r32);
        ASSERT_EQ(var_r32.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r32.Shape()[0], writerSize * Nx);

        auto var_r64 = io.InquireVariable<double>("r64");
        EXPECT_TRUE(var_r64);
        ASSERT_EQ(var_r64.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r64.Shape()[0], writerSize * Nx);

        auto var_c32 = io.InquireVariable<std::complex<float>>("c32");
        EXPECT_TRUE(var_c32);
        ASSERT_EQ(var_c32.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_c32.Shape()[0], writerSize * Nx);

        auto var_c64 = io.InquireVariable<std::complex<double>>("c64");
        EXPECT_TRUE(var_c64);
        ASSERT_EQ(var_c64.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_c64.Shape()[0], writerSize * Nx);

        auto var_r64_2d = io.InquireVariable<double>("r64_2d");
        EXPECT_TRUE(var_r64_2d);
        ASSERT_EQ(var_r64_2d.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r64_2d.Shape()[0], writerSize * Nx);
        ASSERT_EQ(var_r64_2d.Shape()[1], 2);

        auto var_r64_2d_rev = io.InquireVariable<double>("r64_2d_rev");
        EXPECT_TRUE(var_r64_2d_rev);
        ASSERT_EQ(var_r64_2d_rev.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r64_2d_rev.Shape()[0], 2);
        ASSERT_EQ(var_r64_2d_rev.Shape()[1], writerSize * Nx);

        auto var_time = io.InquireVariable<int64_t>("time");
        EXPECT_TRUE(var_time);
        ASSERT_EQ(var_time.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_time.Shape()[0], writerSize);

        long unsigned int myStart =
            (long unsigned int)(writerSize * Nx / mpiSize) * mpiRank;
        long unsigned int myLength =
            (long unsigned int)((writerSize * Nx + mpiSize - 1) / mpiSize);

        if (myStart + myLength > writerSize * Nx)
        {
            myLength = (long unsigned int)writerSize * Nx - myStart;
        }
        const adios2::Dims start{myStart};
        const adios2::Dims count{myLength};
        const adios2::Dims start2{myStart, 0};
        const adios2::Dims count2{myLength, 2};
        const adios2::Dims start3{0, myStart};
        const adios2::Dims count3{2, myLength};
        const adios2::Dims start_time{0};
        const adios2::Dims count_time{1};

        const adios2::Box<adios2::Dims> sel(start, count);
        const adios2::Box<adios2::Dims> sel2(start2, count2);
        const adios2::Box<adios2::Dims> sel3(start3, count3);
        const adios2::Box<adios2::Dims> sel_time(start_time, count_time);

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

        in_I8.reserve(myLength);
        in_I16.reserve(myLength);
        in_I32.reserve(myLength);
        in_I64.reserve(myLength);
        in_R32.reserve(myLength);
        in_R64.reserve(myLength);
        in_C32.reserve(myLength);
        in_C64.reserve(myLength);
        in_R64_2d.reserve(myLength * 2);
        in_R64_2d_rev.reserve(myLength * 2);

        engine.Get(scalar_r64, in_scalar_R64);

        engine.Get(var_i8, in_I8.data());
        engine.Get(var_i16, in_I16.data());
        engine.Get(var_i32, in_I32.data());
        engine.Get(var_i64, in_I64.data());

        engine.Get(var_r32, in_R32.data());
        engine.Get(var_r64, in_R64.data());
        engine.Get(var_c32, in_C32.data());
        engine.Get(var_c64, in_C64.data());
        engine.Get(var_r64_2d, in_R64_2d.data());
        engine.Get(var_r64_2d_rev, in_R64_2d_rev.data());
        std::time_t write_time;
        engine.Get(var_time, (int64_t *)&write_time);
        if (IncreasingDelay && DelayWhileHoldingStep)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(
                DelayMS)); /* sleep for DelayMS milliseconds */
            DelayMS += 200;
        }
        try
        {
            engine.EndStep();

            EXPECT_EQ(validateCommonTestData(myStart, myLength, currentStep, 0),
                      0);
            write_times.push_back(write_time);
        }
        catch (...)
        {
            std::cout << "Exception in EndStep, client failed";
        }

        ++ExpectedStep;
        if (NSteps != adios2::MaxSizeT)
        {
            NSteps--;
            if (NSteps == 0)
            {
                break;
            }
        }
        if (IncreasingDelay && !DelayWhileHoldingStep)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(
                DelayMS)); /* sleep for DelayMS milliseconds */
            DelayMS += 200;
        }
    }

    if ((write_times.back() - write_times.front()) > 1)
    {
        TimeGapDetected++;
    }

    if (!IgnoreTimeGap)
    {
        if (TimeGapExpected)
        {
            EXPECT_TRUE(TimeGapDetected);
        }
        else
        {
            EXPECT_FALSE(TimeGapDetected);
        }
    }
    if (ExpectWriterFailure)
    {
        EXPECT_TRUE(WriterFailed);
    }
    if (WriterFailed)
    {
        EXPECT_TRUE(ExpectWriterFailure);
    }

    if (Latest || Discard)
    {
        std::cout << "Total Skipped Timesteps is " << SkippedSteps << std::endl;
        EXPECT_TRUE(SkippedSteps);
    }
    if (NonBlockingBeginStep)
    {
        std::cout << "Number of BeginSteps where we failed timeout is "
                  << BeginStepFailedPolls << std::endl;
        EXPECT_TRUE(BeginStepFailedPolls);
    }

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

    int result, bare_args = 0;
    ::testing::InitGoogleTest(&argc, argv);

    while (argc > 1)
    {
        if (std::string(argv[1]) == "--num_steps")
        {
            std::istringstream ss(argv[2]);
            if (!(ss >> NSteps))
                std::cerr << "Invalid number for num_steps " << argv[1] << '\n';
            argv++;
            argc--;
        }
        else if (std::string(argv[1]) == "--expect_time_gap")
        {
            TimeGapExpected++;
            IgnoreTimeGap = 0;
        }
        else if (std::string(argv[1]) == "--expect_writer_failure")
        {
            IncreasingDelay = 1;
            ExpectWriterFailure++;
        }
        else if (std::string(argv[1]) == "--expect_contiguous_time")
        {
            TimeGapExpected = 0;
            IgnoreTimeGap = 0;
        }
        else if (std::string(argv[1]) == "--non_blocking")
        {
            IncreasingDelay = 1;
            NonBlockingBeginStep = 1;
        }
        else if (std::string(argv[1]) == "--latest")
        {
            IncreasingDelay = 1;
            Latest = 1;
        }
        else if (std::string(argv[1]) == "--long_first_delay")
        {
            LongFirstDelay = 1;
        }
        else if (std::string(argv[1]) == "--discard")
        {
            IncreasingDelay = 1;
            Discard = 1;
        }
        else if (std::string(argv[1]) == "--delay_while_holding")
        {
            DelayWhileHoldingStep = 1;
        }
        else if (std::string(argv[1]) == "--precious_first")
        {
            FirstTimestepMustBeZero = 1;
        }
        else if (std::string(argv[1]) == "--ignore_time_gap")
        {
            IgnoreTimeGap++;
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
