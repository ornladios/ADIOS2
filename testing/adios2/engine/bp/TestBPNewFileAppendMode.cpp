/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Test using Append mode for writing a new BP file with NumAggregators = 0
 *
 *  Created on: Jul 1, 2025
 *      Author: Caitlin Ross
 */

#include <adios2.h>

#include <gtest/gtest.h>

std::string engineName;                               // comes from command line
std::string aggregationType = "EveryoneWritesSerial"; // comes from command line

class BPNewFileAppendMode : public ::testing::Test
{
public:
    BPNewFileAppendMode() = default;

    // No specific setup needed for this test
};

TEST_F(BPNewFileAppendMode, ADIOS2BPNewFileAppendMode)
{
    std::string fname("ADIOS2BPNewFileAppendMode");
    if (aggregationType == "EveryoneWritesSerial")
    {
        fname += "-EWS.bp";
    }
    else if (aggregationType == "TwoLevelShm")
    {
        fname += "-TLS.bp";
    }
    else if (aggregationType == "DataSizeBased")
    {
        fname += "-DSB.bp";
    }
    else if (aggregationType == "EveryoneWrites")
    {
        fname += "-EW.bp";
    }

    const size_t Nx = 6;

    adios2::ADIOS adios;
    {
        adios2::IO io = adios.DeclareIO("TestIO");
        const adios2::Dims shape{Nx};
        const adios2::Dims start{0};
        const adios2::Dims count{Nx};
        io.DefineVariable<double>("r64", shape, start, count);
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        else
        {
            // Create the BP Engine
            io.SetEngine("BPFile");
        }
        io.SetParameter("AggregationType", aggregationType);
        io.SetParameter("NumAggregators", "0");

        adios2::Engine engine = io.Open(fname, adios2::Mode::Append);

        engine.Close();
    }
}

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
        aggregationType = std::string(argv[2]);
    }

    result = RUN_ALL_TESTS();
    return result;
}