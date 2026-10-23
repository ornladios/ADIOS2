/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstdint>

#include <stdexcept>
#include <string>
#include <vector>

#include <adios2.h>
#if ADIOS2_USE_MPI
#include <mpi.h>
#endif

#include <gtest/gtest.h>

std::string engineName;
std::string engineParameters;

static int MpiSize()
{
#if ADIOS2_USE_MPI
    int size = 1;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    return size;
#else
    return 1;
#endif
}

static int MpiRank()
{
#if ADIOS2_USE_MPI
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    return rank;
#else
    return 0;
#endif
}

#if ADIOS2_USE_MPI
#define MAKE_ADIOS adios2::ADIOS adios(MPI_COMM_WORLD)
#else
#define MAKE_ADIOS adios2::ADIOS adios
#endif

// A block selection combined with a multi-step selection, where the selected
// block is a different size at each step.  A multi-step Get returns one
// rectangle of stepCount x the per-step extent, so a ragged range has no
// answer and every engine refuses it.  Reading the steps one at a time is the
// way to read such a block.
class BPBlockStepsVaryingTest : public ::testing::Test
{
public:
    BPBlockStepsVaryingTest() = default;
};

static void WriteVaryingBlocks(adios2::ADIOS &adios, const std::string &fname)
{
    adios2::IO io = adios.DeclareIO("WriteVaryingIO");
    if (!engineName.empty())
    {
        io.SetEngine(engineName);
    }
    if (!engineParameters.empty())
    {
        io.SetParameters(engineParameters);
    }
    auto gvar = io.DefineVariable<int32_t>("g", {10}, {0}, {2});
    auto lvar = io.DefineVariable<int32_t>("l", {}, {}, {2});

    adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
    for (int32_t step = 0; step < 3; ++step)
    {
        // block 0 is always 2 elements, block 1 grows 2, 4, 6
        const size_t n = 2 + 2 * static_cast<size_t>(step);
        const std::vector<int32_t> block0 = {step * 100, step * 100 + 1};
        std::vector<int32_t> block1(n);
        for (size_t i = 0; i < n; ++i)
        {
            block1[i] = step * 100 + 10 + static_cast<int32_t>(i);
        }
        writer.BeginStep();
        gvar.SetSelection({{0}, {2}});
        writer.Put(gvar, block0.data(), adios2::Mode::Sync);
        gvar.SetSelection({{2}, {n}});
        writer.Put(gvar, block1.data(), adios2::Mode::Sync);
        lvar.SetSelection({{}, {2}});
        writer.Put(lvar, block0.data(), adios2::Mode::Sync);
        lvar.SetSelection({{}, {n}});
        writer.Put(lvar, block1.data(), adios2::Mode::Sync);
        writer.EndStep();
    }
    writer.Close();
}

TEST_F(BPBlockStepsVaryingTest, BlockOverStepsWithVaryingExtent)
{
    if (MpiSize() > 1)
    {
        GTEST_SKIP() << "per-step extent variation is covered serially; see "
                        "BlockOverStepsAcrossWriterRanks for the multi-rank case";
    }
    const std::string fname("BPBlockStepsVarying_" + engineName + ".bp");

    adios2::ADIOS adios;
    WriteVaryingBlocks(adios, fname);

    for (const std::string &name : {std::string("g"), std::string("l")})
    {
        adios2::IO io = adios.DeclareIO("ReadVaryingIO_" + name);
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        adios2::Engine reader = io.Open(fname, adios2::Mode::ReadRandomAccess);
        auto var = io.InquireVariable<int32_t>(name);
        ASSERT_TRUE(var) << "variable " << name;

        var.SetBlockSelection(1);
        var.SetStepSelection({0, 3});

        std::vector<int32_t> got(32, -999);
        EXPECT_THROW(reader.Get(var, got.data(), adios2::Mode::Sync), std::invalid_argument)
            << "variable " << name;
        reader.Close();

        // one step at a time is how such a block is read
        adios2::IO io2 = adios.DeclareIO("ReadVaryingStepwise_" + name);
        if (!engineName.empty())
        {
            io2.SetEngine(engineName);
        }
        adios2::Engine r2 = io2.Open(fname, adios2::Mode::ReadRandomAccess);
        auto v2 = io2.InquireVariable<int32_t>(name);
        ASSERT_TRUE(v2);
        for (size_t step = 0; step < 3; ++step)
        {
            v2.SetBlockSelection(1);
            v2.SetStepSelection({step, 1});
            const size_t n = 2 + 2 * step;
            EXPECT_EQ(v2.SelectionSize(), n) << "variable " << name << " step " << step;
            std::vector<int32_t> d(n + 4, -999);
            r2.Get(v2, d.data(), adios2::Mode::Sync);
            for (size_t i = 0; i < n; ++i)
            {
                EXPECT_EQ(d[i], static_cast<int32_t>(step * 100 + 10 + i))
                    << "variable " << name << " step " << step << " element " << i;
            }
        }
        r2.Close();
    }
}

// The uniform case must keep working on every engine.
TEST_F(BPBlockStepsVaryingTest, BlockOverStepsWithUniformExtent)
{
    if (MpiSize() > 1)
    {
        GTEST_SKIP() << "serial case; multi-rank is BlockOverStepsAcrossWriterRanks";
    }
    const std::string fname("BPBlockStepsUniform_" + engineName + ".bp");

    adios2::ADIOS adios;
    {
        adios2::IO io = adios.DeclareIO("WriteUniformIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        if (!engineParameters.empty())
        {
            io.SetParameters(engineParameters);
        }
        auto var = io.DefineVariable<int32_t>("g", {4}, {0}, {2});
        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        for (int32_t step = 0; step < 3; ++step)
        {
            const std::vector<int32_t> block0 = {step * 100, step * 100 + 1};
            const std::vector<int32_t> block1 = {step * 100 + 10, step * 100 + 11};
            writer.BeginStep();
            var.SetSelection({{0}, {2}});
            writer.Put(var, block0.data(), adios2::Mode::Sync);
            var.SetSelection({{2}, {2}});
            writer.Put(var, block1.data(), adios2::Mode::Sync);
            writer.EndStep();
        }
        writer.Close();
    }

    adios2::IO io = adios.DeclareIO("ReadUniformIO");
    if (!engineName.empty())
    {
        io.SetEngine(engineName);
    }
    adios2::Engine reader = io.Open(fname, adios2::Mode::ReadRandomAccess);
    auto var = io.InquireVariable<int32_t>("g");
    ASSERT_TRUE(var);
    var.SetBlockSelection(1);
    var.SetStepSelection({0, 3});
    ASSERT_EQ(var.SelectionSize(), 6u);
    std::vector<int32_t> got(6, -999);
    reader.Get(var, got.data(), adios2::Mode::Sync);
    EXPECT_EQ(got, std::vector<int32_t>({10, 11, 110, 111, 210, 211}));
    reader.Close();
}

// Global block ids span writer ranks, so resolving one means walking the ranks.
// A single-rank test never executes that walk.
TEST_F(BPBlockStepsVaryingTest, BlockOverStepsAcrossWriterRanks)
{
    const int size = MpiSize();
    const int rank = MpiRank();
    const int nsteps = 3;
    const std::string fname("BPBlockStepsRanks_" + engineName + ".bp");

    MAKE_ADIOS;
    {
        adios2::IO io = adios.DeclareIO("WriteRanksIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        if (!engineParameters.empty())
        {
            io.SetParameters(engineParameters);
        }
        // two blocks of 2 elements per rank per step
        auto gvar = io.DefineVariable<int32_t>("g", {(size_t)(4 * size)}, {0}, {2});
        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        for (int32_t step = 0; step < nsteps; ++step)
        {
            writer.BeginStep();
            for (int sub = 0; sub < 2; ++sub)
            {
                const int gblk = rank * 2 + sub;
                const std::vector<int32_t> data = {step * 1000 + gblk * 10,
                                                   step * 1000 + gblk * 10 + 1};
                gvar.SetSelection({{(size_t)(gblk * 2)}, {2}});
                writer.Put(gvar, data.data(), adios2::Mode::Sync);
            }
            writer.EndStep();
        }
        writer.Close();
    }
#if ADIOS2_USE_MPI
    MPI_Barrier(MPI_COMM_WORLD);
#endif
    if (rank != 0)
    {
#if ADIOS2_USE_MPI
        MPI_Barrier(MPI_COMM_WORLD);
#endif
        return;
    }

    // Read on rank 0 alone, so the reader needs its own self-scoped ADIOS:
    // Open is collective, and reading through the world communicator here
    // would block waiting for ranks that have already left the test.
#if ADIOS2_USE_MPI
    adios2::ADIOS readAdios(MPI_COMM_SELF);
#else
    adios2::ADIOS readAdios;
#endif
    // the last block lives on the last rank, so it needs the full walk
    const int blockID = 2 * size - 1;
    adios2::IO io = readAdios.DeclareIO("ReadRanksIO");
    if (!engineName.empty())
    {
        io.SetEngine(engineName);
    }
    adios2::Engine reader = io.Open(fname, adios2::Mode::ReadRandomAccess);
    auto var = io.InquireVariable<int32_t>("g");
    ASSERT_TRUE(var);
    var.SetBlockSelection(blockID);
    var.SetStepSelection({0, (size_t)nsteps});

    std::vector<int32_t> expected;
    for (int32_t step = 0; step < nsteps; ++step)
    {
        expected.push_back(step * 1000 + blockID * 10);
        expected.push_back(step * 1000 + blockID * 10 + 1);
    }
    const size_t n = var.SelectionSize();
    EXPECT_EQ(n, expected.size());

    std::vector<int32_t> got(n + 6, -999);
    reader.Get(var, got.data(), adios2::Mode::Sync);
    for (size_t i = 0; i < expected.size() && i < n; ++i)
    {
        EXPECT_EQ(got[i], expected[i]) << "element " << i;
    }
    for (size_t i = n; i < got.size(); ++i)
    {
        EXPECT_EQ(got[i], -999) << "element " << i << " is past SelectionSize()";
    }
    reader.Close();
#if ADIOS2_USE_MPI
    MPI_Barrier(MPI_COMM_WORLD);
#endif
}

// A block id present at some steps and absent at others contributes nothing for
// the steps where it is missing, and the size must agree with that.
TEST_F(BPBlockStepsVaryingTest, BlockAbsentAtSomeSteps)
{
    if (MpiSize() > 1)
    {
        GTEST_SKIP() << "serial case";
    }
    const std::string fname("BPBlockAbsent_" + engineName + ".bp");

    adios2::ADIOS adios;
    {
        adios2::IO io = adios.DeclareIO("WriteAbsentIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        if (!engineParameters.empty())
        {
            io.SetParameters(engineParameters);
        }
        auto var = io.DefineVariable<int32_t>("g", {6}, {0}, {2});
        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        for (int32_t step = 0; step < 3; ++step)
        {
            // step 1 writes only two blocks, so block 2 is absent there
            const int nblocks = (step == 1) ? 2 : 3;
            writer.BeginStep();
            for (int b = 0; b < nblocks; ++b)
            {
                const std::vector<int32_t> data = {step * 100 + b * 10, step * 100 + b * 10 + 1};
                var.SetSelection({{(size_t)(b * 2)}, {2}});
                writer.Put(var, data.data(), adios2::Mode::Sync);
            }
            writer.EndStep();
        }
        writer.Close();
    }

    adios2::IO io = adios.DeclareIO("ReadAbsentIO");
    if (!engineName.empty())
    {
        io.SetEngine(engineName);
    }
    adios2::Engine reader = io.Open(fname, adios2::Mode::ReadRandomAccess);
    auto var = io.InquireVariable<int32_t>("g");
    ASSERT_TRUE(var);
    // block 2 is missing at step 1, so the range is not uniform and is refused
    var.SetBlockSelection(2);
    var.SetStepSelection({0, 3});
    std::vector<int32_t> got(16, -999);
    EXPECT_THROW(reader.Get(var, got.data(), adios2::Mode::Sync), std::invalid_argument);
    reader.Close();
}

int main(int argc, char **argv)
{
#if ADIOS2_USE_MPI
    int provided;
    MPI_Init_thread(nullptr, nullptr, MPI_THREAD_MULTIPLE, &provided);
#endif
    ::testing::InitGoogleTest(&argc, argv);
    if (argc > 1)
    {
        engineName = std::string(argv[1]);
    }
    if (argc > 2)
    {
        engineParameters = std::string(argv[2]);
    }
    const int result = RUN_ALL_TESTS();
#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif
    return result;
}
