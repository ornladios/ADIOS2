/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include <cstdint>
#include <cstring>

#include <iostream>
#include <numeric> //std::iota
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#include "../SmallTestData.h"

std::string engineName;       // comes from command line
std::string engineParameters; // comes from command line

class BPOpenWithMetadata : public ::testing::Test
{
public:
    BPOpenWithMetadata() = default;

    SmallTestData m_TestData;
};

//******************************************************************************
// Create an output
// Open normally
// Get metadata
// Open again with metadata
//******************************************************************************

// ADIOS2 BP write and read 1D arrays
TEST_F(BPOpenWithMetadata, ADIOS2BPOpenWithMetadata)
{
    const std::string fname("ADIOS2BPOpenWithMetadata.bp");
    const size_t Nx = 6;
    const size_t NSteps = 3;

    adios2::ADIOS adios;
    {
        adios2::IO io = adios.DeclareIO("TestIO");
        const adios2::Dims shape{Nx};
        const adios2::Dims start{0};
        const adios2::Dims count{Nx};
        auto v = io.DefineVariable<double>("r64", shape, start, count);

        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            // Create the BP Engine
            io.SetEngine("BPFile");
        }
        if (!engineParameters.empty())
        {
            io.SetParameters(engineParameters);
        }

        adios2::Engine bpWriter = io.Open(fname, adios2::Mode::Write);
        EXPECT_EQ(bpWriter.OpenMode(), adios2::Mode::Write);
        for (size_t step = 0; step < NSteps; ++step)
        {
            // Generate test data for each process uniquely
            SmallTestData currentTestData =
                generateNewSmallTestData(m_TestData, static_cast<int>(step), 0, 1);

            bpWriter.BeginStep();
            bpWriter.Put(v, currentTestData.R64.data());
            bpWriter.EndStep();
        }
        bpWriter.Close();
    }

    char *md = nullptr;
    size_t mdsize = 0;

    {
        adios2::IO io = adios.DeclareIO("ReadIO");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        if (!engineParameters.empty())
        {
            io.SetParameters(engineParameters);
        }

        adios2::Engine bpReader = io.Open(fname, adios2::Mode::ReadRandomAccess);

        bpReader.GetMetadata(&md, &mdsize);

        auto var_r64 = io.InquireVariable<double>("r64");
        EXPECT_TRUE(var_r64);
        ASSERT_EQ(var_r64.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r64.Steps(), NSteps);
        ASSERT_EQ(var_r64.Shape()[0], Nx);

        SmallTestData testData;
        std::array<double, Nx> R64;

        const adios2::Dims start{0};
        const adios2::Dims count{Nx};
        const adios2::Box<adios2::Dims> sel(start, count);

        var_r64.SetSelection(sel);

        for (size_t t = 0; t < NSteps; ++t)
        {
            var_r64.SetStepSelection({t, 1});

            // Generate test data for each rank uniquely
            SmallTestData currentTestData =
                generateNewSmallTestData(m_TestData, static_cast<int>(t), 0, 1);

            bpReader.Get(var_r64, R64.data(), adios2::Mode::Sync);
            for (size_t i = 0; i < Nx; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i;
                std::string msg = ss.str();
                EXPECT_EQ(R64[i], currentTestData.R64[i]) << msg;
            }
        }
        bpReader.Close();
    }

    /* Open again with metadata */
    {
        adios2::IO io = adios.DeclareIO("ReadIOMD");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        if (!engineParameters.empty())
        {
            io.SetParameters(engineParameters);
        }

        adios2::Engine bpReader = io.Open(fname, md, mdsize);

        auto var_r64 = io.InquireVariable<double>("r64");
        EXPECT_TRUE(var_r64);
        ASSERT_EQ(var_r64.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r64.Steps(), NSteps);
        ASSERT_EQ(var_r64.Shape()[0], Nx);

        SmallTestData testData;
        std::array<double, Nx> R64;

        const adios2::Dims start{0};
        const adios2::Dims count{Nx};
        const adios2::Box<adios2::Dims> sel(start, count);

        var_r64.SetSelection(sel);

        for (size_t t = 0; t < NSteps; ++t)
        {
            var_r64.SetStepSelection({t, 1});

            // Generate test data for each rank uniquely
            SmallTestData currentTestData =
                generateNewSmallTestData(m_TestData, static_cast<int>(t), 0, 1);

            bpReader.Get(var_r64, R64.data(), adios2::Mode::Sync);
            for (size_t i = 0; i < Nx; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i;
                std::string msg = ss.str();
                EXPECT_EQ(R64[i], currentTestData.R64[i]) << msg;
            }
        }
        bpReader.Close();
    }

    if (md)
    {
        free(md);
    }
}

//******************************************************************************
// main
//******************************************************************************

int main(int argc, char **argv)
{
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
    return result;
}
