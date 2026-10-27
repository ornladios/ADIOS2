/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstdint>
#include <cstring>

#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#include "../TestHelpers.h"

std::string engineName; // comes from command line

class BPChangingShape : public ::testing::Test
{
public:
    BPChangingShape() = default;
};

TEST_F(BPChangingShape, BPWriteReadShape2D)
{
    // Each process would write a 4x2 array and all processes would
    // form a 2D 4 * (NumberOfProcess * Nx) matrix where Nx is 2 here

    const int nsteps = 10;
    int rank = 0, nproc = 1;

#if ADIOS2_USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    const std::string fname("BPChangingShape_MPI.bp");
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    const std::string fname("BPChangingShape.bp");
    adios2::ADIOS adios;
#endif
    // Writer
    {
        adios2::IO outIO = adios.DeclareIO("Output");

        if (!engineName.empty())
        {
            outIO.SetEngine(engineName);
        }

        adios2::Engine writer = outIO.Open(fname, adios2::Mode::Write);

        const size_t dim0 = static_cast<size_t>(nproc);
        const size_t off0 = static_cast<size_t>(rank);
        auto var = outIO.DefineVariable<double>("v", {dim0, 1}, {off0, 0}, {1, 1});

        std::vector<double> buf(nsteps, 0.0);
        for (size_t i = 0; i < buf.size(); i++)
        {
            buf[i] = rank + static_cast<double>(i) / 10.0;
        }

        if (!rank)
        {
            std::cout << "Writing:" << std::endl;
        }
        for (size_t i = 0; i < nsteps; i++)
        {
            writer.BeginStep();

            var.SetShape({dim0, i + 1});
            var.SetSelection({{off0, 0}, {1, i + 1}});

            if (!rank)
            {
                std::cout << "Step " << i << " shape (" << var.Shape()[0] << ", " << var.Shape()[1]
                          << ")" << std::endl;
            }

            writer.Put(var, buf.data());

            writer.EndStep();
        }

        writer.Close();
    }

    // Reader with streaming
    {
        adios2::IO inIO = adios.DeclareIO("Input");

        if (!engineName.empty())
        {
            inIO.SetEngine(engineName);
        }
        adios2::Engine reader = inIO.Open(fname, adios2::Mode::Read);

        if (!rank)
        {
            std::cout << "Reading as stream with BeginStep/EndStep:" << std::endl;
        }

        int i = 0;
        while (true)
        {
            adios2::StepStatus status = reader.BeginStep(adios2::StepMode::Read);

            if (status != adios2::StepStatus::OK)
            {
                break;
            }

            size_t step = reader.CurrentStep();
            size_t expected_shape = step + 1;

            auto var = inIO.InquireVariable<double>("v");
            EXPECT_TRUE(var);

            if (!rank)
            {

                std::cout << "Step " << i << " shape (" << var.Shape()[0] << ", " << var.Shape()[1]
                          << ")" << std::endl;
            }

            EXPECT_EQ(var.Shape()[0], nproc);
            EXPECT_EQ(var.Shape()[1], expected_shape);

            reader.EndStep();
            ++i;
        }

        reader.Close();
    }

    // Reader with file reading
    {
        adios2::IO inIO = adios.DeclareIO("InputFile");

        if (!engineName.empty())
        {
            inIO.SetEngine(engineName);
        }
        adios2::Engine reader = inIO.Open(fname, adios2::Mode::ReadRandomAccess);

        if (!rank)
        {
            std::cout << "Reading as file with SetStepSelection:" << std::endl;
        }

        auto var = inIO.InquireVariable<double>("v");
        EXPECT_TRUE(var);

        for (size_t i = 0; i < nsteps; i++)
        {
            var.SetStepSelection({i, 1});
            if (!rank)
            {

                std::cout << "Step " << i << " shape (" << var.Shape()[0] << ", " << var.Shape()[1]
                          << ")" << std::endl;
            }
            size_t expected_shape = i + 1;
            EXPECT_EQ(var.Shape()[0], nproc);
            EXPECT_EQ(var.Shape()[1], expected_shape);
        }

        reader.Close();
    }

    // Cleanup generated files
#if ADIOS2_USE_MPI
    CleanupTestFilesMPI(fname, MPI_COMM_WORLD);
#else
    CleanupTestFiles(fname);
#endif
}

// Reading a changing-shape variable with no spatial selection: the whole
// variable is selected, and that has to resolve at the selected step rather
// than at whichever step the reader-side variable was first built from.  The
// test above only ever checks Shape(), which is why this went unnoticed.
TEST_F(BPChangingShape, BPReadDefaultSelectionPerStep)
{
    const int nsteps = 4;
    int rank = 0, nproc = 1;
#if ADIOS2_USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    const std::string fname("BPChangingShapeDefaultSel_MPI.bp");
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    const std::string fname("BPChangingShapeDefaultSel.bp");
    adios2::ADIOS adios;
#endif
    // step i holds (i+1) * nproc doubles; rank r contributes (i+1) of them
    {
        adios2::IO io = adios.DeclareIO("DefaultSelWrite");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        auto var = io.DefineVariable<double>("v", {1}, {0}, {1});
        for (int i = 0; i < nsteps; ++i)
        {
            const size_t perRank = static_cast<size_t>(i + 1);
            std::vector<double> data(perRank);
            for (size_t k = 0; k < perRank; ++k)
            {
                data[k] = static_cast<double>(i * 1000 + rank * 100 + k);
            }
            writer.BeginStep();
            var.SetShape({perRank * static_cast<size_t>(nproc)});
            var.SetSelection({{perRank * static_cast<size_t>(rank)}, {perRank}});
            writer.Put(var, data.data(), adios2::Mode::Sync);
            writer.EndStep();
        }
        writer.Close();
    }

    adios2::IO io = adios.DeclareIO("DefaultSelRead");
    if (!engineName.empty())
    {
        io.SetEngine(engineName);
    }
    adios2::Engine reader = io.Open(fname, adios2::Mode::ReadRandomAccess);
    auto var = io.InquireVariable<double>("v");
    ASSERT_TRUE(var);

    const double sentinel = -999.0;
    for (int i = 0; i < nsteps; ++i)
    {
        const size_t expected = static_cast<size_t>(i + 1) * static_cast<size_t>(nproc);
        var.SetStepSelection({static_cast<size_t>(i), 1});

        EXPECT_EQ(var.Shape()[0], expected) << "step " << i;
        EXPECT_EQ(var.SelectionSize(), expected) << "step " << i;
        // Count() must resolve at the same step SelectionSize() does, or a
        // caller sizing from one and striding by the other walks off the end
        ASSERT_EQ(var.Count().size(), 1u) << "step " << i;
        EXPECT_EQ(var.Count()[0], expected) << "step " << i;

        std::vector<double> got(expected + 4, sentinel);
        reader.Get(var, got.data(), adios2::Mode::Sync);
        for (size_t k = 0; k < expected; ++k)
        {
            EXPECT_NE(got[k], sentinel) << "step " << i << " element " << k << " not written";
        }
        for (size_t k = expected; k < got.size(); ++k)
        {
            EXPECT_EQ(got[k], sentinel)
                << "step " << i << " wrote past SelectionSize() at element " << k;
        }
    }

    // and with no step selection at all, which resolves to the first step
    {
        adios2::IO io2 = adios.DeclareIO("DefaultSelReadNoStep");
        if (!engineName.empty())
        {
            io2.SetEngine(engineName);
        }
        adios2::Engine r2 = io2.Open(fname, adios2::Mode::ReadRandomAccess);
        auto v2 = io2.InquireVariable<double>("v");
        ASSERT_TRUE(v2);
        const size_t expected = static_cast<size_t>(nproc);
        EXPECT_EQ(v2.SelectionSize(), expected);
        ASSERT_EQ(v2.Count().size(), 1u);
        EXPECT_EQ(v2.Count()[0], expected);
        std::vector<double> got(expected + 4, sentinel);
        r2.Get(v2, got.data(), adios2::Mode::Sync);
        for (size_t k = expected; k < got.size(); ++k)
        {
            EXPECT_EQ(got[k], sentinel) << "no-step-selection read went past SelectionSize()";
        }
        r2.Close();
    }
    reader.Close();

#if ADIOS2_USE_MPI
    CleanupTestFilesMPI(fname, MPI_COMM_WORLD);
#else
    CleanupTestFiles(fname);
#endif
}

// A step selection has no meaning while stepping, and must be refused rather
// than silently honoured or ignored.
TEST_F(BPChangingShape, BPStepSelectionWhileStreamingThrows)
{
    int rank = 0;
#if ADIOS2_USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    const std::string fname("BPStepSelWhileStreaming_MPI.bp");
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    const std::string fname("BPStepSelWhileStreaming.bp");
    adios2::ADIOS adios;
#endif
    (void)rank;
    {
        adios2::IO io = adios.DeclareIO("StepSelWrite");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        auto var = io.DefineVariable<int32_t>("x", {4}, {0}, {4});
        for (int32_t i = 0; i < 3; ++i)
        {
            const std::vector<int32_t> data = {i, i + 1, i + 2, i + 3};
            writer.BeginStep();
            writer.Put(var, data.data(), adios2::Mode::Sync);
            writer.EndStep();
        }
        writer.Close();
    }

    adios2::IO io = adios.DeclareIO("StepSelRead");
    if (!engineName.empty())
    {
        io.SetEngine(engineName);
    }
    adios2::Engine reader = io.Open(fname, adios2::Mode::Read);
    ASSERT_EQ(reader.BeginStep(), adios2::StepStatus::OK);
    auto var = io.InquireVariable<int32_t>("x");
    ASSERT_TRUE(var);
    var.SetStepSelection({0, 2});
    std::vector<int32_t> got(16, -999);
    EXPECT_THROW(reader.Get(var, got.data(), adios2::Mode::Sync), std::invalid_argument);
    reader.EndStep();
    reader.Close();

#if ADIOS2_USE_MPI
    CleanupTestFilesMPI(fname, MPI_COMM_WORLD);
#else
    CleanupTestFiles(fname);
#endif
}

// A Variable handle the application holds must stay usable across a step
// boundary.  BP5 used to destroy its reader-side variables on every BeginStep so
// that InquireVariable would fail for a variable the new step does not contain,
// which left any handle already held pointing at freed memory: ShapeID came back
// Unknown, the name came back empty and the Get silently wrote nothing.
TEST_F(BPChangingShape, BPHeldVariableSurvivesStepBoundary)
{
    const int nsteps = 3;
    int rank = 0, nproc = 1;
#if ADIOS2_USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    const std::string fname("BPHeldVariable_MPI.bp");
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    const std::string fname("BPHeldVariable.bp");
    adios2::ADIOS adios;
#endif
    {
        adios2::IO io = adios.DeclareIO("HeldWrite");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        auto always = io.DefineVariable<int32_t>("always", {static_cast<size_t>(nproc)},
                                                 {static_cast<size_t>(rank)}, {1});
        auto once = io.DefineVariable<int32_t>("once", {static_cast<size_t>(nproc)},
                                               {static_cast<size_t>(rank)}, {1});
        for (int i = 0; i < nsteps; ++i)
        {
            const int32_t v = i * 10 + rank;
            writer.BeginStep();
            writer.Put(always, &v, adios2::Mode::Sync);
            if (i == 0)
            {
                writer.Put(once, &v, adios2::Mode::Sync); // only at step 0
            }
            writer.EndStep();
        }
        writer.Close();
    }

    adios2::IO io = adios.DeclareIO("HeldRead");
    if (!engineName.empty())
    {
        io.SetEngine(engineName);
    }
    adios2::Engine reader = io.Open(fname, adios2::Mode::Read);

    adios2::Variable<int32_t> held; // inquired once, reused every step
    for (int i = 0; i < nsteps; ++i)
    {
        ASSERT_EQ(reader.BeginStep(), adios2::StepStatus::OK) << "step " << i;
        if (i == 0)
        {
            held = io.InquireVariable<int32_t>("always");
            ASSERT_TRUE(held);
        }

        // the handle from step 0 still describes the variable
        EXPECT_EQ(held.Name(), "always") << "step " << i;
        EXPECT_EQ(held.ShapeID(), adios2::ShapeID::GlobalArray) << "step " << i;

        const int32_t sentinel = -999;
        std::vector<int32_t> got(static_cast<size_t>(nproc), sentinel);
        held.SetSelection({{0}, {static_cast<size_t>(nproc)}});
        reader.Get(held, got.data(), adios2::Mode::Sync);
        for (int r = 0; r < nproc; ++r)
        {
            EXPECT_EQ(got[static_cast<size_t>(r)], i * 10 + r) << "step " << i << " rank " << r;
        }

        // and a variable the step does not contain is still hidden, which is
        // what the per-step teardown used to provide
        auto absent = io.InquireVariable<int32_t>("once");
        if (i == 0)
        {
            EXPECT_TRUE(absent) << "step " << i;
        }
        else
        {
            EXPECT_FALSE(absent) << "step " << i << ": variable absent at this step is visible";
        }
        reader.EndStep();
    }
    reader.Close();

#if ADIOS2_USE_MPI
    CleanupTestFilesMPI(fname, MPI_COMM_WORLD);
#else
    CleanupTestFiles(fname);
#endif
}

// A handle kept past the step its variable was written at used to answer from
// whichever step the variable last appeared in -- BP3/BP4 returned that step's
// data, BP5 wrote nothing -- with no indication either way.  InquireVariable
// returns null for an absent variable; a Get through a stale handle says so.
TEST_F(BPChangingShape, BPGetThroughHandleAbsentAtStepThrows)
{
    const int nsteps = 3;
    int rank = 0, nproc = 1;
#if ADIOS2_USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    const std::string fname("BPStaleHandle_MPI.bp");
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    const std::string fname("BPStaleHandle.bp");
    adios2::ADIOS adios;
#endif
    {
        adios2::IO io = adios.DeclareIO("StaleHandleWrite");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        auto always = io.DefineVariable<int32_t>("always", {static_cast<size_t>(nproc)},
                                                 {static_cast<size_t>(rank)}, {1});
        auto once = io.DefineVariable<int32_t>("once", {static_cast<size_t>(nproc)},
                                               {static_cast<size_t>(rank)}, {1});
        for (int i = 0; i < nsteps; ++i)
        {
            const int32_t v = i * 10 + rank;
            writer.BeginStep();
            writer.Put(always, &v, adios2::Mode::Sync);
            if (i == 0)
            {
                writer.Put(once, &v, adios2::Mode::Sync); // step 0 only
            }
            writer.EndStep();
        }
        writer.Close();
    }

    adios2::IO io = adios.DeclareIO("StaleHandleRead");
    if (!engineName.empty())
    {
        io.SetEngine(engineName);
    }
    adios2::Engine reader = io.Open(fname, adios2::Mode::Read);

    adios2::Variable<int32_t> stale;
    for (int i = 0; i < nsteps; ++i)
    {
        ASSERT_EQ(reader.BeginStep(), adios2::StepStatus::OK) << "step " << i;
        std::vector<int32_t> got(static_cast<size_t>(nproc), -999);
        if (i == 0)
        {
            stale = io.InquireVariable<int32_t>("once");
            ASSERT_TRUE(stale);
            stale.SetSelection({{0}, {static_cast<size_t>(nproc)}});
            reader.Get(stale, got.data(), adios2::Mode::Sync);
            EXPECT_EQ(got[0], 0) << "step 0 reads normally";
        }
        else
        {
            // "once" is not in this step: InquireVariable says so, and a Get
            // through the step-0 handle is refused rather than answered stale
            EXPECT_FALSE(io.InquireVariable<int32_t>("once")) << "step " << i;
            EXPECT_THROW(reader.Get(stale, got.data(), adios2::Mode::Sync), std::invalid_argument)
                << "step " << i;
        }
        reader.EndStep();
    }
    reader.Close();

#if ADIOS2_USE_MPI
    CleanupTestFilesMPI(fname, MPI_COMM_WORLD);
#else
    CleanupTestFiles(fname);
#endif
}

int main(int argc, char **argv)
{
#if ADIOS2_USE_MPI
    int provided;

    // MPI_THREAD_MULTIPLE is only required if you enable the SST MPI_DP
    MPI_Init_thread(nullptr, nullptr, MPI_THREAD_MULTIPLE, &provided);
#endif

    int result;
    ::testing::InitGoogleTest(&argc, argv);

    if (argc > 1)
    {
        engineName = std::string(argv[1]);
    }

    result = RUN_ALL_TESTS();

#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif

    return result;
}
