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

// ADIOS2  write
TEST_F(CommonWriteTest, ADIOS2CommonWrite)
{
    adios2::ADIOS adios;

    adios2::IO io = adios.DeclareIO("TestIO");

    adios2::Dims big_shape{static_cast<unsigned int>(DataSize)};
    adios2::Dims big_start{static_cast<unsigned int>(0)};
    adios2::Dims big_count{static_cast<unsigned int>(DataSize)};

    adios2::Dims small_shape{static_cast<unsigned int>(100)};
    adios2::Dims small_start{static_cast<unsigned int>(0)};
    adios2::Dims small_count{static_cast<unsigned int>(100)};

    std::vector<adios2::Variable<double>> vars;
    vars.push_back(
        io.DefineVariable<double>("big1", big_shape, big_start, big_count));
    vars.push_back(io.DefineVariable<double>("small1", small_shape, small_start,
                                             small_count));
    vars.push_back(
        io.DefineVariable<double>("big2", big_shape, big_start, big_count));
    vars.push_back(io.DefineVariable<double>("small2", small_shape, small_start,
                                             small_count));
    vars.push_back(
        io.DefineVariable<double>("big3", big_shape, big_start, big_count));

    std::vector<std::vector<double>> data;
    for (int i = 0; i < 5; i++)
    {
        int size = DataSize;
        if ((i == 1) || (i == 3))
        {
            size = 100;
        }
        std::vector<double> tmp(size);
        data.push_back(tmp);
    }

    // Create the Engine
    io.SetEngine(engine);
    io.SetParameters(engineParams);

    adios2::Engine engine = io.Open(fname, adios2::Mode::Write);

    /*
     *   write possibilities:
     *		Don't write
     *          Sync   - always destroy data afterwards
     *		Deferred
     *		Deferred with immediate PerformPuts()  -  Destroy all prior data
     *
     */
    for (int step = 0; step < 4 * 4 * 4 * 4 * 4; ++step)
    {
        int mask = step;
        engine.BeginStep();

        std::cout << "Begin Step " << step << std::endl;
        for (int j = 0; j < 5; j++)
        {
            std::fill(data[j].begin(), data[j].end(), (double)j + 1.0);
        }
        for (int j = 0; j < 5; j++)
        {
            adios2::Mode write_mode;
            int this_var_mask = (mask & 0x3);
            bool do_perform_puts = false;
            mask >>= 2;
            switch (this_var_mask)
            {
            case 0:
                continue;
            case 1:
                write_mode = adios2::Mode::Sync;
                break;
            case 2:
            case 3:
                write_mode = adios2::Mode::Deferred;
                break;
            }
            engine.Put(vars[j], data[j].data(), write_mode);
            if (this_var_mask == 1)
            {
                std::fill(data[j].begin(), data[j].end(), -100.0);
            }
            else if (this_var_mask == 3)
            {
                engine.PerformPuts();
                for (int k = 0; k <= j; k++)
                    std::fill(data[k].begin(), data[j].end(), -100.0);
            }
        }
        engine.EndStep();
    }

    // Close the file
    engine.Close();
}

// ADIOS2  write
TEST_F(CommonWriteTest, ADIOS2CommonRead)
{
    adios2::ADIOS adios;

    adios2::IO io = adios.DeclareIO("TestIO");

    std::vector<adios2::Variable<double>> vars;

    std::vector<std::vector<double>> data;
    for (int i = 0; i < 5; i++)
    {
        int size = DataSize;
        if ((i == 1) || (i == 3))
        {
            size = 100;
        }
        std::vector<double> tmp(size);
        data.push_back(tmp);
    }

    // Create the Engine
    io.SetEngine(engine);
    io.SetParameters(engineParams);

    adios2::Engine engine = io.Open(fname, adios2::Mode::Read);

    /*
     *   write possibilities:
     *		Don't write
     *          Sync   - always destroy data afterwards
     *		Deferred
     *		Deferred with immediate PerformPuts()  -  Destroy all prior data
     *
     */
    for (int step = 0; step < 4 * 4 * 4 * 4 * 4; ++step)
    {
        int mask = step;
        engine.BeginStep();

        vars.push_back(io.InquireVariable<double>("big1"));
        vars.push_back(io.InquireVariable<double>("small1"));
        vars.push_back(io.InquireVariable<double>("big2"));
        vars.push_back(io.InquireVariable<double>("small2"));
        vars.push_back(io.InquireVariable<double>("big3"));

        for (int j = 0; j < 5; j++)
        {
            if (vars[j])
            {
                std::cout << "Variable " << j << " Written in TS " << step
                          << std::endl;
                engine.Get(vars[j], data[j].data());
            }
        }
        engine.EndStep();
    }

    // Close the file
    engine.Close();
}

int main(int argc, char **argv)
{

    int result;
    ::testing::InitGoogleTest(&argc, argv);

    DelayMS = 0; // zero for common writer

    ParseArgs(argc, argv);

    result = RUN_ALL_TESTS();

#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif

    return result;
}
