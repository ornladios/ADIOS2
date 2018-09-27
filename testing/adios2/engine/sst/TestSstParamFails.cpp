/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include <cstdint>
#include <cstring>

#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#include "TestData.h"

class SstParamFailTest : public ::testing::Test
{
public:
    SstParamFailTest() = default;
};

adios2::Params engineParams = {}; // parsed from command line
int TimeGapExpected = 0;
std::string fname = "ADIOS2Sst";

static std::string Trim(std::string &str)
{
    size_t first = str.find_first_not_of(' ');
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

/*
 * Engine parameters spec is a poor-man's JSON.  name:value pairs are separated
 * by commas.  White space is trimmed off front and back.  No quotes or anything
 * fancy allowed.
 */
static adios2::Params ParseEngineParams(std::string Input)
{
    std::istringstream ss(Input);
    std::string Param;
    adios2::Params Ret = {};

    while (std::getline(ss, Param, ','))
    {
        std::istringstream ss2(Param);
        std::string ParamName;
        std::string ParamValue;
        std::getline(ss2, ParamName, ':');
        if (!std::getline(ss2, ParamValue, ':'))
        {
            throw std::invalid_argument("Engine parameter \"" + Param +
                                        "\" missing value");
        }
        Ret[Trim(ParamName)] = Trim(ParamValue);
    }
    return Ret;
}

// ADIOS2 Sst param fail
TEST_F(SstParamFailTest, ParamNoValue)
{
    EXPECT_THROW(engineParams = ParseEngineParams("MarshalMethod:"),
                 std::invalid_argument);
}

// ADIOS2 Sst param fail
TEST_F(SstParamFailTest, ParamNoValue2)
{
    EXPECT_THROW(engineParams = ParseEngineParams("MarshalMethod"),
                 std::invalid_argument);
}

// ADIOS2 Sst param fail
TEST_F(SstParamFailTest, MarshalMethodUnknown)
{
    engineParams = ParseEngineParams("MarshalMethod:unknown");

#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(true);
#endif

    adios2::IO io = adios.DeclareIO("TestIO");

    // Create the Engine
    io.SetEngine("Sst");
    io.SetParameters(engineParams);

    adios2::Engine engine;
    EXPECT_THROW(engine = io.Open(fname, adios2::Mode::Write),
                 std::invalid_argument);
    // Close the file
    EXPECT_FALSE(engine);
}

// ADIOS2 Sst param fail
TEST_F(SstParamFailTest, CompressionMethodUnknown)
{
    engineParams = ParseEngineParams("CompressionMethod:unknown");

#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(true);
#endif

    adios2::IO io = adios.DeclareIO("TestIO");

    // Create the Engine
    io.SetEngine("Sst");
    io.SetParameters(engineParams);

    adios2::Engine engine;
    EXPECT_THROW(engine = io.Open(fname, adios2::Mode::Write),
                 std::invalid_argument);
    // Close the file
    EXPECT_FALSE(engine);
}

// ADIOS2 Sst param fail
TEST_F(SstParamFailTest, QueueFullPolicyUnknown)
{
    engineParams = ParseEngineParams("QueueFullPolicy:unknown");

#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(true);
#endif

    adios2::IO io = adios.DeclareIO("TestIO");

    // Create the Engine
    io.SetEngine("Sst");
    io.SetParameters(engineParams);

    adios2::Engine engine;
    EXPECT_THROW(engine = io.Open(fname, adios2::Mode::Write),
                 std::invalid_argument);
    // Close the file
    EXPECT_FALSE(engine);
}

// ADIOS2 Sst param fail
TEST_F(SstParamFailTest, RendezvousReaderCountBad)
{
    engineParams = ParseEngineParams("RendezvousReaderCount:unknown");

#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(true);
#endif

    adios2::IO io = adios.DeclareIO("TestIO");

    // Create the Engine
    io.SetEngine("Sst");
    io.SetParameters(engineParams);

    adios2::Engine engine;
    EXPECT_THROW(engine = io.Open(fname, adios2::Mode::Write),
                 std::invalid_argument);
    // Close the file
    EXPECT_FALSE(engine);
}

// ADIOS2 Sst param fail
TEST_F(SstParamFailTest, QueueLimitBad)
{
    engineParams = ParseEngineParams("QueueLimit:unknown");

#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(true);
#endif

    adios2::IO io = adios.DeclareIO("TestIO");

    // Create the Engine
    io.SetEngine("Sst");
    io.SetParameters(engineParams);

    adios2::Engine engine;
    EXPECT_THROW(engine = io.Open(fname, adios2::Mode::Write),
                 std::invalid_argument);
    // Close the file
    EXPECT_FALSE(engine);
}

//******************************************************************************
// main
//******************************************************************************

int main(int argc, char **argv)
{
#ifdef ADIOS2_HAVE_MPI
    MPI_Init(nullptr, nullptr);
#endif

    int result;
    ::testing::InitGoogleTest(&argc, argv);

    result = RUN_ALL_TESTS();

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return result;
}
