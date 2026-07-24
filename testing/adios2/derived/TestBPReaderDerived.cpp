/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

#include <adios2.h>

#include <gtest/gtest.h>

// Reader-side derived variables: an expression defined on a READ IO over
// variables that were never marked derived at write time. The reader computes
// the result on Get. See IO::DefineReaderDerivedVariable.

namespace
{
const std::string expr = "x=vx \n y=vy \n z=vz \n magnitude(x,y,z)";

double component(size_t step, size_t i, int c)
{
    return 1.0 + static_cast<double>(i) + 100.0 * static_cast<double>(step) + 10.0 * c;
}

double expectedSpeed(size_t step, size_t i)
{
    const double x = component(step, i, 0);
    const double y = component(step, i, 1);
    const double z = component(step, i, 2);
    return std::sqrt(x * x + y * y + z * z);
}

// Write vx, vy, vz as ordinary (non-derived) global arrays.
void WriteInputs(adios2::ADIOS &adios, const std::string &fname, size_t N, size_t steps)
{
    adios2::IO io = adios.DeclareIO("w_" + fname);
    io.SetEngine("BP5");
    auto vx = io.DefineVariable<double>("vx", {N}, {0}, {N});
    auto vy = io.DefineVariable<double>("vy", {N}, {0}, {N});
    auto vz = io.DefineVariable<double>("vz", {N}, {0}, {N});
    auto w = io.Open(fname, adios2::Mode::Write);
    for (size_t s = 0; s < steps; s++)
    {
        std::vector<double> dx(N), dy(N), dz(N);
        for (size_t i = 0; i < N; i++)
        {
            dx[i] = component(s, i, 0);
            dy[i] = component(s, i, 1);
            dz[i] = component(s, i, 2);
        }
        w.BeginStep();
        w.Put(vx, dx.data());
        w.Put(vy, dy.data());
        w.Put(vz, dz.data());
        w.EndStep();
    }
    w.Close();
}
} // namespace

TEST(ReaderDerived, StreamingBasic)
{
    const size_t N = 16;
    adios2::ADIOS adios;
    WriteInputs(adios, "ReaderDerivedStreaming.bp", N, 1);

    adios2::IO io = adios.DeclareIO("r");
    io.SetEngine("BP5");
    // Defined BEFORE Open: resolved lazily against the file's variables.
    io.DefineReaderDerivedVariable("speed", expr);
    auto r = io.Open("ReaderDerivedStreaming.bp", adios2::Mode::Read);

    ASSERT_EQ(r.BeginStep(), adios2::StepStatus::OK);
    auto speed = io.InquireVariable<double>("speed");
    ASSERT_TRUE(speed);
    EXPECT_EQ(speed.Shape().size(), 1u);
    EXPECT_EQ(speed.Shape()[0], N);

    std::vector<double> out;
    r.Get(speed, out);
    r.EndStep();
    r.Close();

    ASSERT_EQ(out.size(), N);
    for (size_t i = 0; i < N; i++)
        EXPECT_NEAR(out[i], expectedSpeed(0, i), 1e-9);
}

TEST(ReaderDerived, RandomAccessMultiStep)
{
    const size_t N = 20;
    const size_t steps = 3;
    adios2::ADIOS adios;
    WriteInputs(adios, "ReaderDerivedRA.bp", N, steps);

    adios2::IO io = adios.DeclareIO("r");
    io.SetEngine("BP5");
    io.DefineReaderDerivedVariable("speed", expr);
    auto r = io.Open("ReaderDerivedRA.bp", adios2::Mode::ReadRandomAccess);

    auto speed = io.InquireVariable<double>("speed");
    ASSERT_TRUE(speed);
    EXPECT_EQ(speed.Steps(), steps);

    // Read a single, non-zero step.
    speed.SetStepSelection({2, 1});
    std::vector<double> out;
    r.Get(speed, out);
    r.Close();

    ASSERT_EQ(out.size(), N);
    for (size_t i = 0; i < N; i++)
        EXPECT_NEAR(out[i], expectedSpeed(2, i), 1e-9);
}

TEST(ReaderDerived, StreamingPartialSelection)
{
    const size_t N = 20;
    const size_t steps = 3;
    adios2::ADIOS adios;
    WriteInputs(adios, "ReaderDerivedPartial.bp", N, steps);

    adios2::IO io = adios.DeclareIO("r");
    io.SetEngine("BP5");
    io.DefineReaderDerivedVariable("speed", expr);
    auto r = io.Open("ReaderDerivedPartial.bp", adios2::Mode::Read);

    size_t s = 0;
    while (r.BeginStep() == adios2::StepStatus::OK)
    {
        auto speed = io.InquireVariable<double>("speed");
        ASSERT_TRUE(speed);
        speed.SetSelection({{5}, {3}}); // start=5, count=3
        std::vector<double> out;
        r.Get(speed, out);
        r.EndStep();

        ASSERT_EQ(out.size(), 3u);
        for (size_t k = 0; k < 3; k++)
            EXPECT_NEAR(out[k], expectedSpeed(s, 5 + k), 1e-9);
        s++;
    }
    r.Close();
    EXPECT_EQ(s, steps);
}

TEST(ReaderDerived, ReopenSameIO)
{
    const size_t N = 8;
    adios2::ADIOS adios;
    WriteInputs(adios, "ReaderDerivedReopen.bp", N, 1);

    adios2::IO io = adios.DeclareIO("r");
    io.SetEngine("BP5");
    io.DefineReaderDerivedVariable("speed", expr);

    // Open, read, close, then re-open the SAME IO: the definition persists and
    // must re-resolve against the freshly opened file.
    for (int pass = 0; pass < 2; pass++)
    {
        auto r = io.Open("ReaderDerivedReopen.bp", adios2::Mode::Read);
        ASSERT_EQ(r.BeginStep(), adios2::StepStatus::OK);
        auto speed = io.InquireVariable<double>("speed");
        ASSERT_TRUE(speed) << "pass " << pass;
        std::vector<double> out;
        r.Get(speed, out);
        r.EndStep();
        r.Close();
        ASSERT_EQ(out.size(), N);
        for (size_t i = 0; i < N; i++)
            EXPECT_NEAR(out[i], expectedSpeed(0, i), 1e-9) << "pass " << pass;
    }
}

TEST(ReaderDerived, SyntaxErrorAtDefine)
{
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("r");
    // Parse happens at define time, so a bad expression throws immediately,
    // before any Open.
    EXPECT_THROW(io.DefineReaderDerivedVariable("bad", "x=vx \n nofunction(x)"),
                 std::invalid_argument);
}

TEST(ReaderDerived, DuplicateNameAtDefine)
{
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("r");
    io.DefineReaderDerivedVariable("speed", expr);
    // Same name a second time is rejected at define.
    EXPECT_THROW(io.DefineReaderDerivedVariable("speed", expr), std::invalid_argument);
}

TEST(ReaderDerived, NoPrecomputedMinMax)
{
    const size_t N = 16;
    adios2::ADIOS adios;
    WriteInputs(adios, "ReaderDerivedMinMax.bp", N, 1);

    adios2::IO io = adios.DeclareIO("r");
    io.SetEngine("BP5");
    io.DefineReaderDerivedVariable("speed", expr);
    auto r = io.Open("ReaderDerivedMinMax.bp", adios2::Mode::ReadRandomAccess);

    auto speed = io.InquireVariable<double>("speed");
    ASSERT_TRUE(speed);
    // Reader-derived variables have no precomputed stats: min/max report an
    // explicitly invalid range (min > max), which no real data can produce, not
    // an input variable's min/max.
    EXPECT_EQ(speed.Min(), std::numeric_limits<double>::max());
    EXPECT_EQ(speed.Max(), std::numeric_limits<double>::lowest());
    EXPECT_GT(speed.Min(), speed.Max());
    r.Close();
}

TEST(ReaderDerived, NoInputVariablesThrow)
{
    const size_t N = 8;
    adios2::ADIOS adios;
    WriteInputs(adios, "ReaderDerivedNoInputs.bp", N, 1);

    adios2::IO io = adios.DeclareIO("r");
    io.SetEngine("BP5");
    // An expression over no file variables has nothing to read; rejected at
    // resolve (it parses fine, so the define itself succeeds).
    io.DefineReaderDerivedVariable("constant", "3 + 4");
    auto r = io.Open("ReaderDerivedNoInputs.bp", adios2::Mode::Read);
    EXPECT_THROW(r.BeginStep(), std::exception);
    r.Close();
}

TEST(ReaderDerived, IncongruentInputsThrow)
{
    const size_t N = 12, M = 8;
    adios2::ADIOS adios;
    {
        adios2::IO io = adios.DeclareIO("w");
        io.SetEngine("BP5");
        auto vx = io.DefineVariable<double>("vx", {N}, {0}, {N});
        auto vy = io.DefineVariable<double>("vy", {N}, {0}, {N});
        auto vz = io.DefineVariable<double>("vz", {M}, {0}, {M}); // different shape
        std::vector<double> dx(N, 1.0), dy(N, 2.0), dz(M, 3.0);
        auto w = io.Open("ReaderDerivedIncongruent.bp", adios2::Mode::Write);
        w.BeginStep();
        w.Put(vx, dx.data());
        w.Put(vy, dy.data());
        w.Put(vz, dz.data());
        w.EndStep();
        w.Close();
    }

    adios2::IO io = adios.DeclareIO("r");
    io.SetEngine("BP5");
    io.DefineReaderDerivedVariable("speed", expr);
    // Resolution happens on Open/BeginStep; mismatched input shapes must throw.
    auto r = io.Open("ReaderDerivedIncongruent.bp", adios2::Mode::Read);
    EXPECT_THROW(r.BeginStep(), std::exception);
    r.Close();
}

TEST(ReaderDerived, NameCollidesWithFileVariable)
{
    const size_t N = 8;
    adios2::ADIOS adios;
    {
        adios2::IO io = adios.DeclareIO("w");
        io.SetEngine("BP5");
        auto vx = io.DefineVariable<double>("vx", {N}, {0}, {N});
        auto vy = io.DefineVariable<double>("vy", {N}, {0}, {N});
        auto vz = io.DefineVariable<double>("vz", {N}, {0}, {N});
        auto speed = io.DefineVariable<double>("speed", {N}, {0}, {N}); // occupies the name
        std::vector<double> d(N, 1.0);
        auto w = io.Open("ReaderDerivedCollision.bp", adios2::Mode::Write);
        w.BeginStep();
        w.Put(vx, d.data());
        w.Put(vy, d.data());
        w.Put(vz, d.data());
        w.Put(speed, d.data());
        w.EndStep();
        w.Close();
    }

    adios2::IO io = adios.DeclareIO("r");
    io.SetEngine("BP5");
    // Cannot be seen at define time (file not open), so it surfaces at resolve.
    io.DefineReaderDerivedVariable("speed", expr);
    auto r = io.Open("ReaderDerivedCollision.bp", adios2::Mode::Read);
    EXPECT_THROW(r.BeginStep(), std::exception);
    r.Close();
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
