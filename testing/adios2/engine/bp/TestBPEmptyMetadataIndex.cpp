/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <string>
#include <vector>

#include <adios2.h>

#include <gtest/gtest.h>

std::string engineName;       // comes from command line
std::string engineParameters; // comes from command line

namespace
{

const std::size_t Nx = 4;

adios2::IO DeclareIO(adios2::ADIOS &adios, const std::string &name)
{
    adios2::IO io = adios.DeclareIO(name);
    if (!engineName.empty())
    {
        io.SetEngine(engineName);
    }
    if (!engineParameters.empty())
    {
        io.SetParameters(engineParameters);
    }
    return io;
}

void ExpectFiles(const std::string &fname)
{
    for (const char *file : {"data.0", "md.0", "md.idx", "mmd.0"})
    {
        ASSERT_TRUE(std::filesystem::exists(std::filesystem::path(fname) / file))
            << fname << "/" << file;
    }
}

// A BP5 writer writes the metadata index header together with the record of
// its first step. A writer that is killed before it completes its first step
// leaves all files of the output empty.
void WriteKilledBeforeFirstStep(const std::string &fname)
{
    std::filesystem::remove_all(fname);
    std::filesystem::create_directory(fname);
    for (const char *file : {"data.0", "md.0", "md.idx", "mmd.0"})
    {
        std::ofstream(std::filesystem::path(fname) / file);
    }
    ExpectFiles(fname);
}

// A writer that is destroyed without Close() before it completes its first
// step, e.g. while an exception unwinds, only marks the output as no longer
// written: the metadata index is shorter than its header.
void WriteDestroyedBeforeFirstStep(const std::string &fname)
{
    std::filesystem::remove_all(fname);
    {
        adios2::ADIOS adios;
        adios2::IO io = DeclareIO(adios, "WriteIO");
        auto var = io.DefineVariable<double>("x", {Nx}, {0}, {Nx});
        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        writer.BeginStep();
        std::vector<double> data(Nx, 1.0);
        writer.Put(var, data.data(), adios2::Mode::Sync);
    }
    ExpectFiles(fname);
}

// The metadata index lists a step whose metadata the metadata file does not
// hold, e.g. if the file system lost the end of md.0 when the writer died.
void WriteStepWithoutMetadata(const std::string &fname)
{
    std::filesystem::remove_all(fname);
    {
        adios2::ADIOS adios;
        adios2::IO io = DeclareIO(adios, "WriteIO");
        auto var = io.DefineVariable<double>("x", {Nx}, {0}, {Nx});
        adios2::Engine writer = io.Open(fname, adios2::Mode::Write);
        writer.BeginStep();
        std::vector<double> data(Nx, 1.0);
        writer.Put(var, data.data(), adios2::Mode::Sync);
        writer.EndStep();
        writer.Close();
    }
    std::filesystem::resize_file(std::filesystem::path(fname) / "md.0", 0);
    ExpectFiles(fname);
}

// Reading from an output without metadata must throw, not crash. The variable
// is defined by the reader itself, since the output holds none.
void ExpectNoData(adios2::IO &io, adios2::Engine &reader)
{
    auto var = io.DefineVariable<double>("notInFile", {Nx}, {0}, {Nx});
    std::vector<double> data(Nx);
    EXPECT_THROW(reader.Get(var, data.data(), adios2::Mode::Sync), std::runtime_error);
    EXPECT_THROW(reader.Get(var, data.data(), adios2::Mode::Deferred), std::runtime_error);
    EXPECT_THROW(reader.BlocksInfo(var, 0), std::runtime_error);
    EXPECT_NO_THROW(reader.PerformGets());
}

void ReadInRandomAccess(const std::string &fname)
{
    adios2::ADIOS adios;
    adios2::IO io = DeclareIO(adios, "ReadIO");
    adios2::Engine reader;
    try
    {
        reader = io.Open(fname, adios2::Mode::ReadRandomAccess);
    }
    catch (std::ios_base::failure const &e)
    {
        // a subclass of std::runtime_error, so catch it first
        FAIL() << "unexpected I/O error: " << e.what();
    }
    catch (std::runtime_error const &)
    {
        // the output is final and has no readable step
        return;
    }
    EXPECT_EQ(reader.Steps(), 0u);
    ExpectNoData(io, reader);
    EXPECT_NO_THROW(reader.Close());
}

void ReadInStreamingMode(const std::string &fname)
{
    adios2::ADIOS adios;
    adios2::IO io = DeclareIO(adios, "ReadIO");
    adios2::Engine reader = io.Open(fname, adios2::Mode::Read);
    ExpectNoData(io, reader);
    EXPECT_NE(reader.BeginStep(adios2::StepMode::Read, 0.0f), adios2::StepStatus::OK);
    EXPECT_NO_THROW(reader.Close());
}

} // namespace

TEST(BPEmptyMetadataIndex, KilledBeforeFirstStep)
{
    const std::string fname = "BPEmptyMetadataIndexKilled.bp";
    WriteKilledBeforeFirstStep(fname);
    ReadInRandomAccess(fname);
    ReadInStreamingMode(fname);
}

TEST(BPEmptyMetadataIndex, DestroyedBeforeFirstStep)
{
    const std::string fname = "BPEmptyMetadataIndexDestroyed.bp";
    WriteDestroyedBeforeFirstStep(fname);
    ReadInRandomAccess(fname);
    ReadInStreamingMode(fname);
}

TEST(BPEmptyMetadataIndex, StepWithoutMetadata)
{
    const std::string fname = "BPEmptyMetadataIndexNoMetadata.bp";
    WriteStepWithoutMetadata(fname);
    ReadInRandomAccess(fname);
    ReadInStreamingMode(fname);
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
        engineParameters = std::string(argv[2]);
    }
    result = RUN_ALL_TESTS();
    return result;
}
