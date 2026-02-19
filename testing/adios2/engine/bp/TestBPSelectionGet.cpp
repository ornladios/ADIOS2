/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TestBPSelectionGet.cpp : Tests for Selection-based Get() API
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
