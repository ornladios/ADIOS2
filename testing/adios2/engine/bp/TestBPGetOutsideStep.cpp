/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * In Mode::Read, Get() is only valid between BeginStep() and EndStep().
 */

#include <string>
#include <vector>

#include <adios2.h>

#include <gtest/gtest.h>

std::string engineName; // comes from command line

TEST(BPGetOutsideStep, Read)
{
    const std::string fname = "GetOutsideStep.bp";
    const size_t Nx = 4;
    {
        adios2::ADIOS adios;
        adios2::IO io = adios.DeclareIO("Write");
        io.SetEngine(engineName);
        auto var = io.DefineVariable<double>("x", {Nx}, {0}, {Nx});
        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        for (int step = 0; step < 2; ++step)
        {
            std::vector<double> data(Nx, step);
            writer.BeginStep();
            writer.Put(var, data.data(), adios2::Mode::Sync);
            writer.EndStep();
        }
        writer.Close();
    }

    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("Read");
    io.SetEngine(engineName);
    adios2::Engine reader = io.Open(fname, adios2::Mode::Read);
    std::vector<double> data(Nx);

    // before the first BeginStep
    auto mine = io.DefineVariable<double>("mine", {Nx}, {0}, {Nx});
    EXPECT_THROW(reader.Get(mine, data.data(), adios2::Mode::Sync), std::logic_error);
    EXPECT_THROW(reader.Get(mine, data.data(), adios2::Mode::Deferred), std::logic_error);

    ASSERT_EQ(reader.BeginStep(), adios2::StepStatus::OK);
    auto var = io.InquireVariable<double>("x");
    ASSERT_TRUE(var);
    reader.Get(var, data.data(), adios2::Mode::Sync);
    EXPECT_EQ(data, std::vector<double>(Nx, 0.0));
    reader.EndStep();

    // between steps
    EXPECT_THROW(reader.Get(var, data.data(), adios2::Mode::Sync), std::logic_error);
    EXPECT_THROW(reader.Get(var, data.data(), adios2::Mode::Deferred), std::logic_error);

    // BeginStep replaces the variables of the previous step
    ASSERT_EQ(reader.BeginStep(), adios2::StepStatus::OK);
    var = io.InquireVariable<double>("x");
    ASSERT_TRUE(var);
    reader.Get(var, data.data(), adios2::Mode::Sync);
    EXPECT_EQ(data, std::vector<double>(Nx, 1.0));
    reader.EndStep();
    reader.Close();
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    if (argc > 1)
    {
        engineName = std::string(argv[1]);
    }
    return RUN_ALL_TESTS();
}
