/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstdint>
#include <cstring>

#include <iostream>
#include <numeric>
#include <stdexcept>
#include <vector>

#include <adios2.h>

#include <gtest/gtest.h>

std::string engineName;
std::string engineParameters;

class BPSelectionGetTest : public ::testing::Test
{
public:
    BPSelectionGetTest() = default;
};

// Write a 2D global array (Ny x Nx) for NSteps steps, then read it back
// using Selection::All, Selection::BoundingBox, and Selection::Block
TEST_F(BPSelectionGetTest, GlobalArray2D)
{
    const size_t Ny = 4;
    const size_t Nx = 5;
    const size_t NSteps = 3;
    const std::string fname("BPSelectionGet_GlobalArray2D.bp");

    adios2::ADIOS adios;
    // Write
    {
        adios2::IO io = adios.DeclareIO("WriteIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        if (!engineParameters.empty())
        {
            io.SetParameters(engineParameters);
        }

        auto var = io.DefineVariable<double>("data", {Ny, Nx}, {0, 0}, {Ny, Nx});

        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        for (size_t step = 0; step < NSteps; ++step)
        {
            std::vector<double> data(Ny * Nx);
            for (size_t i = 0; i < Ny * Nx; ++i)
            {
                data[i] = static_cast<double>(step * 100 + i);
            }
            writer.BeginStep();
            writer.Put(var, data.data());
            writer.EndStep();
        }
        writer.Close();
    }

    // Read with Selection::All
    {
        adios2::IO io = adios.DeclareIO("ReadAllIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        adios2::Engine reader = io.Open(fname, adios2::Mode::ReadRandomAccess);

        auto var = io.InquireVariable<double>("data");
        ASSERT_TRUE(var);
        ASSERT_EQ(var.Steps(), NSteps);

        auto sel = adios2::Selection::All().WithSteps(0, 1);
        std::vector<double> data(Ny * Nx);
        reader.Get(var, data.data(), sel, adios2::Mode::Sync);

        // Verify step 0 data
        for (size_t i = 0; i < Ny * Nx; ++i)
        {
            EXPECT_DOUBLE_EQ(data[i], static_cast<double>(i))
                << "Mismatch at index " << i << " for Selection::All step 0";
        }

        // Read step 2
        auto sel2 = adios2::Selection::All().WithSteps(2, 1);
        reader.Get(var, data.data(), sel2, adios2::Mode::Sync);
        for (size_t i = 0; i < Ny * Nx; ++i)
        {
            EXPECT_DOUBLE_EQ(data[i], static_cast<double>(200 + i))
                << "Mismatch at index " << i << " for Selection::All step 2";
        }

        reader.Close();
    }

    // Read with Selection::BoundingBox (subset)
    {
        adios2::IO io = adios.DeclareIO("ReadBBoxIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        adios2::Engine reader = io.Open(fname, adios2::Mode::ReadRandomAccess);

        auto var = io.InquireVariable<double>("data");
        ASSERT_TRUE(var);

        // Read a 2x3 sub-region starting at (1, 1) from step 1
        const size_t subNy = 2, subNx = 3;
        auto sel = adios2::Selection::BoundingBox({1, 1}, {subNy, subNx}).WithSteps(1, 1);
        std::vector<double> subdata(subNy * subNx);
        reader.Get(var, subdata.data(), sel, adios2::Mode::Sync);

        // Verify: step 1 data[y][x] = 100 + y*Nx + x
        for (size_t y = 0; y < subNy; ++y)
        {
            for (size_t x = 0; x < subNx; ++x)
            {
                double expected = 100.0 + (y + 1) * Nx + (x + 1);
                EXPECT_DOUBLE_EQ(subdata[y * subNx + x], expected)
                    << "Mismatch at (" << y << "," << x << ") for BoundingBox";
            }
        }

        reader.Close();
    }

    // Read with Selection::BoundingBox using std::vector (auto-resize)
    {
        adios2::IO io = adios.DeclareIO("ReadBBoxVectorIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        adios2::Engine reader = io.Open(fname, adios2::Mode::ReadRandomAccess);

        auto var = io.InquireVariable<double>("data");
        ASSERT_TRUE(var);

        auto sel = adios2::Selection::BoundingBox({0, 0}, {Ny, Nx}).WithSteps(0, 1);
        std::vector<double> data;
        reader.Get(var, data, sel, adios2::Mode::Sync);

        ASSERT_EQ(data.size(), Ny * Nx);
        for (size_t i = 0; i < Ny * Nx; ++i)
        {
            EXPECT_DOUBLE_EQ(data[i], static_cast<double>(i));
        }

        reader.Close();
    }
}

// Write local arrays (one block per rank), read back with Selection::Block
TEST_F(BPSelectionGetTest, LocalArrayBlock)
{
    const size_t Nx = 10;
    const size_t NSteps = 2;
    const std::string fname("BPSelectionGet_LocalArrayBlock.bp");

    adios2::ADIOS adios;
    // Write a local array
    {
        adios2::IO io = adios.DeclareIO("WriteLocalIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        if (!engineParameters.empty())
        {
            io.SetParameters(engineParameters);
        }

        auto var = io.DefineVariable<int32_t>("localdata", {}, {}, {Nx});

        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        for (size_t step = 0; step < NSteps; ++step)
        {
            std::vector<int32_t> data(Nx);
            for (size_t i = 0; i < Nx; ++i)
            {
                data[i] = static_cast<int32_t>(step * 1000 + i);
            }
            writer.BeginStep();
            writer.Put(var, data.data());
            writer.EndStep();
        }
        writer.Close();
    }

    // Read with Selection::Block
    {
        adios2::IO io = adios.DeclareIO("ReadBlockIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        adios2::Engine reader = io.Open(fname, adios2::Mode::ReadRandomAccess);

        auto var = io.InquireVariable<int32_t>("localdata");
        ASSERT_TRUE(var);

        // Read block 0 from step 0
        auto sel = adios2::Selection::Block(0).WithSteps(0, 1);
        std::vector<int32_t> data(Nx);
        reader.Get(var, data.data(), sel, adios2::Mode::Sync);

        for (size_t i = 0; i < Nx; ++i)
        {
            EXPECT_EQ(data[i], static_cast<int32_t>(i))
                << "Mismatch at index " << i << " for Block step 0";
        }

        // Read block 0 from step 1
        auto sel1 = adios2::Selection::Block(0).WithSteps(1, 1);
        reader.Get(var, data.data(), sel1, adios2::Mode::Sync);

        for (size_t i = 0; i < Nx; ++i)
        {
            EXPECT_EQ(data[i], static_cast<int32_t>(1000 + i))
                << "Mismatch at index " << i << " for Block step 1";
        }

        reader.Close();
    }
}

// Test Selection with deferred mode
TEST_F(BPSelectionGetTest, DeferredGet)
{
    const size_t Nx = 8;
    const std::string fname("BPSelectionGet_Deferred.bp");

    adios2::ADIOS adios;
    // Write
    {
        adios2::IO io = adios.DeclareIO("WriteDeferredIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        if (!engineParameters.empty())
        {
            io.SetParameters(engineParameters);
        }

        auto var = io.DefineVariable<float>("fdata", {Nx}, {0}, {Nx});

        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        std::vector<float> data(Nx);
        for (size_t i = 0; i < Nx; ++i)
        {
            data[i] = static_cast<float>(i) * 1.5f;
        }
        writer.BeginStep();
        writer.Put(var, data.data());
        writer.EndStep();
        writer.Close();
    }

    // Read deferred with Selection
    {
        adios2::IO io = adios.DeclareIO("ReadDeferredIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        adios2::Engine reader = io.Open(fname, adios2::Mode::ReadRandomAccess);

        auto var = io.InquireVariable<float>("fdata");
        ASSERT_TRUE(var);

        // Read first 4 elements with BoundingBox, deferred
        auto sel = adios2::Selection::BoundingBox({0}, {4}).WithSteps(0, 1);
        std::vector<float> data(4);
        reader.Get(var, data.data(), sel, adios2::Mode::Deferred);
        reader.PerformGets();

        for (size_t i = 0; i < 4; ++i)
        {
            EXPECT_FLOAT_EQ(data[i], static_cast<float>(i) * 1.5f)
                << "Mismatch at index " << i << " for deferred BoundingBox";
        }

        // Read last 4 elements
        auto sel2 = adios2::Selection::BoundingBox({4}, {4}).WithSteps(0, 1);
        std::vector<float> data2(4);
        reader.Get(var, data2.data(), sel2, adios2::Mode::Deferred);
        reader.PerformGets();

        for (size_t i = 0; i < 4; ++i)
        {
            EXPECT_FLOAT_EQ(data2[i], static_cast<float>(i + 4) * 1.5f)
                << "Mismatch at index " << i << " for deferred BoundingBox (second half)";
        }

        reader.Close();
    }
}

// Test Selection::All with streaming mode (BeginStep/EndStep)
TEST_F(BPSelectionGetTest, StreamingAll)
{
    const size_t Nx = 6;
    const size_t NSteps = 3;
    const std::string fname("BPSelectionGet_StreamingAll.bp");

    adios2::ADIOS adios;
    // Write
    {
        adios2::IO io = adios.DeclareIO("WriteStreamIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        if (!engineParameters.empty())
        {
            io.SetParameters(engineParameters);
        }

        auto var = io.DefineVariable<int64_t>("sdata", {Nx}, {0}, {Nx});

        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        for (size_t step = 0; step < NSteps; ++step)
        {
            std::vector<int64_t> data(Nx);
            for (size_t i = 0; i < Nx; ++i)
            {
                data[i] = static_cast<int64_t>(step * 10 + i);
            }
            writer.BeginStep();
            writer.Put(var, data.data());
            writer.EndStep();
        }
        writer.Close();
    }

    // Read in streaming mode with Selection::All
    {
        adios2::IO io = adios.DeclareIO("ReadStreamIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        adios2::Engine reader = io.Open(fname, adios2::Mode::Read);

        size_t step = 0;
        while (reader.BeginStep() == adios2::StepStatus::OK)
        {
            auto var = io.InquireVariable<int64_t>("sdata");
            ASSERT_TRUE(var);

            auto sel = adios2::Selection::All();
            std::vector<int64_t> data(Nx);
            reader.Get(var, data.data(), sel, adios2::Mode::Sync);

            for (size_t i = 0; i < Nx; ++i)
            {
                EXPECT_EQ(data[i], static_cast<int64_t>(step * 10 + i))
                    << "Mismatch at step " << step << " index " << i;
            }

            reader.EndStep();
            ++step;
        }
        EXPECT_EQ(step, NSteps);

        reader.Close();
    }
}

// Test SelectionSize() with all selection types
TEST_F(BPSelectionGetTest, SelectionSize)
{
    const size_t Ny = 4;
    const size_t Nx = 5;
    const size_t NSteps = 2;
    const size_t LocalNx = 10;

    adios2::ADIOS adios;

    // Write a global array and a local array
    const std::string fname("BPSelectionGet_SelectionSize.bp");
    {
        adios2::IO io = adios.DeclareIO("WriteSizeIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        if (!engineParameters.empty())
        {
            io.SetParameters(engineParameters);
        }

        auto gvar = io.DefineVariable<double>("global", {Ny, Nx}, {0, 0}, {Ny, Nx});
        auto lvar = io.DefineVariable<int32_t>("local", {}, {}, {LocalNx});

        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        for (size_t step = 0; step < NSteps; ++step)
        {
            std::vector<double> gdata(Ny * Nx, 1.0);
            std::vector<int32_t> ldata(LocalNx, 1);
            writer.BeginStep();
            writer.Put(gvar, gdata.data());
            writer.Put(lvar, ldata.data());
            writer.EndStep();
        }
        writer.Close();
    }

    // Test SelectionSize in random access mode
    {
        adios2::IO io = adios.DeclareIO("ReadSizeIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        adios2::Engine reader = io.Open(fname, adios2::Mode::ReadRandomAccess);

        auto gvar = io.InquireVariable<double>("global");
        ASSERT_TRUE(gvar);
        auto lvar = io.InquireVariable<int32_t>("local");
        ASSERT_TRUE(lvar);

        // All — should return full global array size
        auto selAll = adios2::Selection::All().WithSteps(0, 1);
        EXPECT_EQ(gvar.SelectionSize(selAll), Ny * Nx);

        // All with multiple steps
        auto selAllMulti = adios2::Selection::All().WithSteps(0, 2);
        EXPECT_EQ(gvar.SelectionSize(selAllMulti), Ny * Nx * 2);

        // All — still correct after SetSelection narrows m_Count
        gvar.SetSelection({{0, 0}, {2, 3}});
        EXPECT_EQ(gvar.SelectionSize(selAll), Ny * Nx)
            << "SelectionSize(All) should use shape, not stale m_Count";

        // BoundingBox
        auto selBBox = adios2::Selection::BoundingBox({0, 0}, {2, 3}).WithSteps(0, 1);
        EXPECT_EQ(gvar.SelectionSize(selBBox), 2u * 3u);

        // Block for local array
        auto selBlock = adios2::Selection::Block(0).WithSteps(0, 1);
        EXPECT_EQ(lvar.SelectionSize(selBlock), LocalNx);

        reader.Close();
    }
}

// Step 1 of a 2-element box out of a 6-element array used to land at offset 6,
// outside a SelectionSize() allocation.
TEST_F(BPSelectionGetTest, MultiStepPlacementStaysWithinSelectionSize)
{
    const std::string fname("BPSelectionGet_MultiStepPlacement.bp");
    const int32_t sentinel = -999;

    adios2::ADIOS adios;
    {
        adios2::IO io = adios.DeclareIO("WriteStrideIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        if (!engineParameters.empty())
        {
            io.SetParameters(engineParameters);
        }
        auto var = io.DefineVariable<int32_t>("x", {6}, {0}, {6});
        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        const std::vector<int32_t> step0 = {1, 2, 3, 4, 5, 6};
        const std::vector<int32_t> step1 = {11, 12, 13, 14, 15, 16};
        writer.BeginStep();
        writer.Put(var, step0.data(), adios2::Mode::Sync);
        writer.EndStep();
        writer.BeginStep();
        writer.Put(var, step1.data(), adios2::Mode::Sync);
        writer.EndStep();
        writer.Close();
    }

    {
        adios2::IO io = adios.DeclareIO("ReadStrideIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        adios2::Engine reader = io.Open(fname, adios2::Mode::ReadRandomAccess);
        auto var = io.InquireVariable<int32_t>("x");
        ASSERT_TRUE(var);

        const auto selection = adios2::Selection::BoundingBox({1}, {2}).WithSteps(0, 2);
        ASSERT_EQ(var.SelectionSize(selection), 4u);

        // Oversized so a misplaced write is observable rather than fatal.
        std::vector<int32_t> got(12, sentinel);
        reader.Get(var, got.data(), selection, adios2::Mode::Sync);

        const std::vector<int32_t> expected = {2, 3, 12, 13};
        for (size_t i = 0; i < expected.size(); ++i)
        {
            EXPECT_EQ(got[i], expected[i]) << "at destination element " << i;
        }
        for (size_t i = expected.size(); i < got.size(); ++i)
        {
            EXPECT_EQ(got[i], sentinel)
                << "element " << i << " is past SelectionSize() and must not be written";
        }
        reader.Close();
    }
}

// Block(n) keeps the All spatial type; SelectionSize used to report the whole
// global array.
TEST_F(BPSelectionGetTest, BlockSelectionSizeMatchesSelectedBlock)
{
    const std::string fname("BPSelectionGet_BlockSize.bp");

    adios2::ADIOS adios;
    {
        adios2::IO io = adios.DeclareIO("WriteBlockSizeIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        if (!engineParameters.empty())
        {
            io.SetParameters(engineParameters);
        }
        // Two differently sized blocks over a 6-element global array.
        auto var = io.DefineVariable<int32_t>("x", {6}, {0}, {2});
        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        const std::vector<int32_t> block0 = {1, 2};
        const std::vector<int32_t> block1 = {3, 4, 5, 6};
        writer.BeginStep();
        writer.Put(var, block0.data(), adios2::Mode::Sync);
        var.SetSelection({{2}, {4}});
        writer.Put(var, block1.data(), adios2::Mode::Sync);
        writer.EndStep();
        writer.Close();
    }

    {
        adios2::IO io = adios.DeclareIO("ReadBlockSizeIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        adios2::Engine reader = io.Open(fname, adios2::Mode::ReadRandomAccess);
        auto var = io.InquireVariable<int32_t>("x");
        ASSERT_TRUE(var);

        EXPECT_EQ(var.SelectionSize(adios2::Selection::Block(0)), 2u);
        EXPECT_EQ(var.SelectionSize(adios2::Selection::Block(1)), 4u);

        // The vector overload sizes its destination from SelectionSize.
        std::vector<int32_t> got;
        reader.Get(var, got, adios2::Selection::Block(1), adios2::Mode::Sync);
        EXPECT_EQ(got, std::vector<int32_t>({3, 4, 5, 6}));
        reader.Close();
    }
}

// All must use the shape at the selected step, not the inquired shape.
TEST_F(BPSelectionGetTest, AllUsesPerStepShape)
{
    const std::string fname("BPSelectionGet_AllPerStepShape.bp");
    const int32_t sentinel = -999;

    adios2::ADIOS adios;
    {
        adios2::IO io = adios.DeclareIO("WriteGrowIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        if (!engineParameters.empty())
        {
            io.SetParameters(engineParameters);
        }
        auto var = io.DefineVariable<int32_t>("x", {2}, {0}, {2});
        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        const std::vector<int32_t> step0 = {1, 2};
        const std::vector<int32_t> step1 = {11, 12, 13, 14};
        writer.BeginStep();
        writer.Put(var, step0.data(), adios2::Mode::Sync);
        writer.EndStep();
        var.SetShape({4});
        var.SetSelection({{0}, {4}});
        writer.BeginStep();
        writer.Put(var, step1.data(), adios2::Mode::Sync);
        writer.EndStep();
        writer.Close();
    }

    {
        adios2::IO io = adios.DeclareIO("ReadGrowIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        adios2::Engine reader = io.Open(fname, adios2::Mode::ReadRandomAccess);
        auto var = io.InquireVariable<int32_t>("x");
        ASSERT_TRUE(var);

        const auto selection = adios2::Selection::All().WithSteps(1, 1);
        ASSERT_EQ(var.SelectionSize(selection), 4u);

        std::vector<int32_t> got(8, sentinel);
        reader.Get(var, got.data(), selection, adios2::Mode::Sync);

        const std::vector<int32_t> expected = {11, 12, 13, 14};
        for (size_t i = 0; i < expected.size(); ++i)
        {
            EXPECT_EQ(got[i], expected[i]) << "at destination element " << i;
        }
        for (size_t i = expected.size(); i < got.size(); ++i)
        {
            EXPECT_EQ(got[i], sentinel) << "element " << i << " is past SelectionSize()";
        }
        reader.Close();
    }
}

// A multi-step Get returns one rectangle, so a range spanning a shape change
// has no answer and must be refused.  Reading the steps one at a time is the
// documented way to handle a changing shape, and must give each step's shape.
TEST_F(BPSelectionGetTest, MultiStepAllAcrossChangingShapeThrows)
{
    const std::string fname("BPSelectionGet_AllChangingShape.bp");

    adios2::ADIOS adios;
    {
        adios2::IO io = adios.DeclareIO("WriteRaggedIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        if (!engineParameters.empty())
        {
            io.SetParameters(engineParameters);
        }
        auto var = io.DefineVariable<int32_t>("x", {2}, {0}, {2});
        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        for (int32_t step = 0; step < 3; ++step)
        {
            // shape grows 2, 4, 6
            const size_t n = 2 + 2 * static_cast<size_t>(step);
            std::vector<int32_t> data(n);
            for (size_t i = 0; i < n; ++i)
            {
                data[i] = step * 100 + static_cast<int32_t>(i);
            }
            writer.BeginStep();
            var.SetShape({n});
            var.SetSelection({{0}, {n}});
            writer.Put(var, data.data(), adios2::Mode::Sync);
            writer.EndStep();
        }
        writer.Close();
    }

    adios2::IO io = adios.DeclareIO("ReadRaggedIO");
    if (!engineName.empty())
    {
        io.SetEngine(engineName);
    }
    adios2::Engine reader = io.Open(fname, adios2::Mode::ReadRandomAccess);
    auto var = io.InquireVariable<int32_t>("x");
    ASSERT_TRUE(var);

    // the ragged range is refused
    std::vector<int32_t> got(32, -999);
    EXPECT_THROW(
        reader.Get(var, got.data(), adios2::Selection::All().WithSteps(0, 3), adios2::Mode::Sync),
        std::invalid_argument);

    // one step at a time resolves that step's shape
    for (size_t step = 0; step < 3; ++step)
    {
        const auto sel = adios2::Selection::All().WithSteps(step, 1);
        const size_t n = 2 + 2 * step;
        EXPECT_EQ(var.SelectionSize(sel), n) << "step " << step;
        std::vector<int32_t> d(n + 4, -999);
        reader.Get(var, d.data(), sel, adios2::Mode::Sync);
        for (size_t i = 0; i < n; ++i)
        {
            EXPECT_EQ(d[i], static_cast<int32_t>(step * 100 + i)) << "step " << step << " el " << i;
        }
        for (size_t i = n; i < d.size(); ++i)
        {
            EXPECT_EQ(d[i], -999) << "step " << step << " wrote past SelectionSize()";
        }
    }

    // a fixed box across the same range is uniform, so it still reads
    const auto boxSel = adios2::Selection::BoundingBox({0}, {2}).WithSteps(0, 3);
    ASSERT_EQ(var.SelectionSize(boxSel), 6u);
    std::vector<int32_t> box(6, -999);
    reader.Get(var, box.data(), boxSel, adios2::Mode::Sync);
    EXPECT_EQ(box, std::vector<int32_t>({0, 1, 100, 101, 200, 201}));
    reader.Close();
}

// A box on top of a block is block-relative and must be bounded by that block.
// BP3/BP4 reject an out-of-bounds box; BP5 used to read past the block and
// return garbage.
TEST_F(BPSelectionGetTest, BlockRelativeBoxOutOfBoundsThrows)
{
    const std::string fname("BPSelectionGet_BlockBoxBounds.bp");

    adios2::ADIOS adios;
    {
        adios2::IO io = adios.DeclareIO("WriteBoundsIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        if (!engineParameters.empty())
        {
            io.SetParameters(engineParameters);
        }
        auto var = io.DefineVariable<int32_t>("larr", {}, {}, {2});
        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        const std::vector<int32_t> block0 = {0, 1};
        const std::vector<int32_t> block1 = {10, 11};
        writer.BeginStep();
        var.SetSelection({{}, {2}});
        writer.Put(var, block0.data(), adios2::Mode::Sync);
        var.SetSelection({{}, {2}});
        writer.Put(var, block1.data(), adios2::Mode::Sync);
        writer.EndStep();
        writer.Close();
    }

    {
        adios2::IO io = adios.DeclareIO("ReadBoundsIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        adios2::Engine reader = io.Open(fname, adios2::Mode::ReadRandomAccess);
        auto var = io.InquireVariable<int32_t>("larr");
        ASSERT_TRUE(var);

        // block 1 holds 2 elements, so [1,3) runs off the end of it
        std::vector<int32_t> got(4, -999);
        var.SetBlockSelection(1);
        var.SetSelection({{1}, {2}});
        EXPECT_THROW(reader.Get(var, got.data(), adios2::Mode::Sync), std::invalid_argument);
        reader.Close();
    }

    {
        // the in-bounds box still reads
        adios2::IO io = adios.DeclareIO("ReadBoundsOkIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        adios2::Engine reader = io.Open(fname, adios2::Mode::ReadRandomAccess);
        auto var = io.InquireVariable<int32_t>("larr");
        ASSERT_TRUE(var);
        std::vector<int32_t> got(4, -999);
        var.SetBlockSelection(1);
        var.SetSelection({{1}, {1}});
        reader.Get(var, got.data(), adios2::Mode::Sync);
        EXPECT_EQ(got[0], 11);
        EXPECT_EQ(got[1], -999);
        reader.Close();
    }
}

// Test ToString() method
TEST_F(BPSelectionGetTest, SelectionToString)
{
    auto all = adios2::Selection::All();
    EXPECT_NE(all.ToString().find("All"), std::string::npos);

    auto bbox = adios2::Selection::BoundingBox({1, 2}, {10, 20});
    std::string s = bbox.ToString();
    EXPECT_NE(s.find("BoundingBox"), std::string::npos);
    EXPECT_NE(s.find("10"), std::string::npos);
    EXPECT_NE(s.find("20"), std::string::npos);

    auto block = adios2::Selection::Block(5);
    EXPECT_NE(block.ToString().find("All"), std::string::npos);
    EXPECT_NE(block.ToString().find("block"), std::string::npos);
    EXPECT_NE(block.ToString().find("5"), std::string::npos);

    auto withSteps = adios2::Selection::All().WithSteps(3, 7);
    std::string ws = withSteps.ToString();
    EXPECT_NE(ws.find("All"), std::string::npos);
    EXPECT_NE(ws.find("3"), std::string::npos);
    EXPECT_NE(ws.find("7"), std::string::npos);
}

// A block and a bounding box compose only on a local array, where the box is
// relative to the block.  On a global array they select alternative regions,
// so the pair is refused rather than quietly dropping the block.
TEST_F(BPSelectionGetTest, BlockWithBoxOnGlobalArrayThrows)
{
    const std::string fname("BPSelBlockBoxGlobal.bp");
    const size_t Nx = 8;

    adios2::ADIOS adios;
    {
        adios2::IO io = adios.DeclareIO("WriteBlockBoxIO");
        auto varG = io.DefineVariable<int32_t>("g", {Nx}, {0}, {4});
        auto varL = io.DefineVariable<int32_t>("l", {}, {}, {4});
        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        const std::vector<int32_t> b0 = {0, 1, 2, 3};
        const std::vector<int32_t> b1 = {10, 11, 12, 13};
        writer.BeginStep();
        varG.SetSelection({{0}, {4}});
        writer.Put(varG, b0.data(), adios2::Mode::Sync);
        varG.SetSelection({{4}, {4}});
        writer.Put(varG, b1.data(), adios2::Mode::Sync);
        writer.Put(varL, b0.data(), adios2::Mode::Sync);
        writer.Put(varL, b1.data(), adios2::Mode::Sync);
        writer.EndStep();
        writer.Close();
    }

    adios2::IO io = adios.DeclareIO("ReadBlockBoxIO");
    adios2::Engine reader = io.Open(fname, adios2::Mode::ReadRandomAccess);
    const auto sel = adios2::Selection::BoundingBox({0}, {2}).WithBlock(1).WithSteps(0, 1);

    auto varG = io.InquireVariable<int32_t>("g");
    ASSERT_TRUE(varG);
    std::vector<int32_t> data(2, -999);
    EXPECT_THROW(reader.Get(varG, data.data(), sel, adios2::Mode::Sync), std::invalid_argument);
    // SelectionSize must agree with Get about what is legal
    EXPECT_THROW(varG.SelectionSize(sel), std::invalid_argument);

    // The same pair on a local array is the supported ghost-cell case
    auto varL = io.InquireVariable<int32_t>("l");
    ASSERT_TRUE(varL);
    EXPECT_EQ(varL.SelectionSize(sel), 2u);
    reader.Get(varL, data.data(), sel, adios2::Mode::Sync);
    EXPECT_EQ(data, std::vector<int32_t>({10, 11}));
    reader.Close();
}

int main(int argc, char **argv)
{
#if ADIOS2_USE_MPI
    int provided;
    MPI_Init_thread(nullptr, nullptr, MPI_THREAD_MULTIPLE, &provided);
#endif

    int result;
    ::testing::InitGoogleTest(&argc, argv);

    if (argc > 1)
    {
        engineName = std::string(argv[1]);
    }
    if (argc > 2)
    {
        engineParameters = std::string(argv[2]);
    }
    result = RUN_ALL_TESTS();

#if ADIOS2_USE_MPI
    MPI_Finalize();
#endif

    return result;
}
