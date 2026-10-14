/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <adios2.h>

#include <gtest/gtest.h>

std::string engineName;       // comes from command line
std::string engineParameters; // comes from command line

namespace
{

// A BP5 writer writes the metadata index header together with the record of
// its first step. A writer that is killed before it completes its first step
// leaves all files of the output empty.
void CreateOutputOfKilledWriter(const std::string &fname)
{
    std::filesystem::remove_all(fname);
    std::filesystem::create_directory(fname);
    for (const char *file : {"data.0", "md.0", "md.idx", "mmd.0"})
    {
        std::ofstream(std::filesystem::path(fname) / file);
    }
}

adios2::IO DeclareReadIO(adios2::ADIOS &adios)
{
    adios2::IO io = adios.DeclareIO("ReadIO");
    if (!engineName.empty())
    {
        io.SetEngine(engineName);
    }
    return io;
}

} // namespace

// In random access, the output is final and holds no step: Open() reports that.
TEST(BPEmptyMetadataIndex, ReadRandomAccessThrows)
{
    const std::string fname = "BPEmptyMetadataIndexRandomAccess.bp";
    CreateOutputOfKilledWriter(fname);

    adios2::ADIOS adios;
    adios2::IO io = DeclareReadIO(adios);
    EXPECT_THROW(io.Open(fname, adios2::Mode::ReadRandomAccess), std::runtime_error);
}

// In streaming mode, the reader opens the output and finds no step yet.
// Nothing may dereference the deserializer, which is only created once the
// metadata index has content.
TEST(BPEmptyMetadataIndex, ReadWithoutStep)
{
    const std::string fname = "BPEmptyMetadataIndexRead.bp";
    CreateOutputOfKilledWriter(fname);

    adios2::ADIOS adios;
    adios2::IO io = DeclareReadIO(adios);
    adios2::Engine reader = io.Open(fname, adios2::Mode::Read);

    EXPECT_NO_THROW(reader.PerformGets());

    // a variable that the reader defines itself, since the output holds none
    auto var = io.DefineVariable<double>("x", {4}, {0}, {4});
    std::vector<double> data(4);
    EXPECT_THROW(reader.Get(var, data.data(), adios2::Mode::Sync), std::logic_error);
    EXPECT_THROW(reader.Get(var, data.data(), adios2::Mode::Deferred), std::logic_error);

    EXPECT_EQ(reader.BeginStep(adios2::StepMode::Read, 0.0f), adios2::StepStatus::NotReady);

    EXPECT_NO_THROW(reader.Close());
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
