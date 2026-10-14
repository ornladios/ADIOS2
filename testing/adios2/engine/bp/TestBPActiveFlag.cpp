/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * A BP4/BP5 writer that is killed never clears the active flag in md.idx.  Readers
 * ignore that flag by default (IgnoreActiveFlag=true) and only wait for more
 * steps when asked to (IgnoreActiveFlag=false, as FileStream does).
 */

#include <chrono>
#include <fstream>
#include <string>
#include <vector>

#include <adios2.h>

#include <gtest/gtest.h>

std::string engineName; // comes from command line

// offset of activeFlag in the md.idx header
static std::streamoff ActiveFlagPosition() { return (engineName == "BP4") ? 38 : 39; }

static void WriteSteps(const std::string &fname, const adios2::Mode mode, const size_t nsteps,
                       const double base, const adios2::Params &params = {})
{
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("Write");
    io.SetEngine(engineName);
    io.SetParameters(params);
    auto var = io.DefineVariable<double>("x");
    adios2::Engine writer = io.Open(fname, mode);
    for (size_t step = 0; step < nsteps; ++step)
    {
        writer.BeginStep();
        writer.Put(var, base + (double)step);
        writer.EndStep();
    }
    writer.Close();
}

static char GetActiveFlag(const std::string &fname)
{
    std::ifstream idx(fname + "/md.idx", std::ios::binary);
    idx.seekg(ActiveFlagPosition());
    char flag = 0;
    idx.read(&flag, 1);
    return flag;
}

// leave the file as a killed writer would: complete steps, still marked active
static void MarkActive(const std::string &fname)
{
    std::fstream idx(fname + "/md.idx", std::ios::in | std::ios::out | std::ios::binary);
    ASSERT_TRUE(idx.is_open());
    idx.seekp(ActiveFlagPosition());
    const char active = 1;
    idx.write(&active, 1);
}

// read with BeginStep(timeout), return values seen and the final status
static std::vector<double> ReadSteps(const std::string &fname, const adios2::Params &params,
                                     const float timeout, adios2::StepStatus &lastStatus)
{
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("Read");
    io.SetEngine(engineName);
    io.SetParameters(params);
    adios2::Engine reader = io.Open(fname, adios2::Mode::Read);
    std::vector<double> values;
    while ((lastStatus = reader.BeginStep(adios2::StepMode::Read, timeout)) ==
           adios2::StepStatus::OK)
    {
        double x = -1.0;
        reader.Get(io.InquireVariable<double>("x"), x, adios2::Mode::Sync);
        values.push_back(x);
        reader.EndStep();
    }
    reader.Close();
    return values;
}

class BPActiveFlag : public ::testing::Test
{
};

TEST_F(BPActiveFlag, DefaultReadsKilledFile)
{
    const std::string fname = "ActiveFlagDefault.bp";
    WriteSteps(fname, adios2::Mode::Write, 3, 0.0);
    MarkActive(fname);
    ASSERT_EQ(GetActiveFlag(fname), 1);

    // default timeout waits forever when the flag is honored; here it must not
    adios2::StepStatus status;
    auto values = ReadSteps(fname, {}, -1.0f, status);
    EXPECT_EQ(status, adios2::StepStatus::EndOfStream);
    EXPECT_EQ(values, std::vector<double>({0.0, 1.0, 2.0}));
}

TEST_F(BPActiveFlag, HonorActiveFlag)
{
    const std::string fname = "ActiveFlagHonor.bp";
    WriteSteps(fname, adios2::Mode::Write, 3, 0.0);
    MarkActive(fname);

    adios2::StepStatus status;
    auto values = ReadSteps(fname, {{"IgnoreActiveFlag", "false"}}, 0.0f, status);
    EXPECT_EQ(status, adios2::StepStatus::NotReady);
    EXPECT_EQ(values, std::vector<double>({0.0, 1.0, 2.0}));
}

TEST_F(BPActiveFlag, MissingFileDoesNotWait)
{
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("Read");
    io.SetEngine(engineName);
    const auto start = std::chrono::steady_clock::now();
    EXPECT_ANY_THROW(io.Open("ActiveFlagMissing.bp", adios2::Mode::Read));
    const auto elapsed = std::chrono::steady_clock::now() - start;
    EXPECT_LT(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count(), 30);
}

TEST_F(BPActiveFlag, AppendToKilledFile)
{
    const std::string fname = "ActiveFlagAppend.bp";
    WriteSteps(fname, adios2::Mode::Write, 3, 0.0);
    MarkActive(fname);
    WriteSteps(fname, adios2::Mode::Append, 2, 10.0);
    EXPECT_EQ(GetActiveFlag(fname), 0);

    adios2::StepStatus status;
    auto values = ReadSteps(fname, {{"IgnoreActiveFlag", "false"}}, 0.0f, status);
    EXPECT_EQ(status, adios2::StepStatus::EndOfStream);
    EXPECT_EQ(values, std::vector<double>({0.0, 1.0, 2.0, 10.0, 11.0}));
}

TEST_F(BPActiveFlag, AppendAfterStepsToKilledFile)
{
    if (engineName == "BP4")
    {
        GTEST_SKIP() << "BP4 has no AppendAfterSteps";
    }
    const std::string fname = "ActiveFlagAppendAfter.bp";
    WriteSteps(fname, adios2::Mode::Write, 3, 0.0);
    MarkActive(fname);

    // the restart pattern: find the steps to keep, then append after them
    adios2::StepStatus status;
    auto existing = ReadSteps(fname, {}, -1.0f, status);
    ASSERT_EQ(existing.size(), 3u);

    WriteSteps(fname, adios2::Mode::Append, 2, 10.0, {{"AppendAfterSteps", "2"}});
    EXPECT_EQ(GetActiveFlag(fname), 0);

    auto values = ReadSteps(fname, {{"IgnoreActiveFlag", "false"}}, 0.0f, status);
    EXPECT_EQ(status, adios2::StepStatus::EndOfStream);
    EXPECT_EQ(values, std::vector<double>({0.0, 1.0, 10.0, 11.0}));
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
