/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstdint>
#include <string>
#include <vector>

#include <adios2.h>

#include <gtest/gtest.h>

std::string engineName;       // comes from command line
std::string engineParameters; // comes from command line

namespace
{

// Write `steps` timesteps of a small 1D variable to `fname`.
void WriteFile(const std::string &fname, size_t steps, adios2::Mode openMode)
{
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("WriteIO");
    if (!engineName.empty())
    {
        io.SetEngine(engineName);
    }

    const std::size_t Nx = 10;
    auto var = io.InquireVariable<int32_t>("data");
    if (!var)
    {
        var = io.DefineVariable<int32_t>("data", {Nx}, {0}, {Nx});
    }

    adios2::Engine writer = io.Open(fname, openMode);
    for (size_t s = 0; s < steps; ++s)
    {
        std::vector<int32_t> data(Nx, static_cast<int32_t>(s));
        writer.BeginStep();
        writer.Put(var, data.data());
        writer.EndStep();
    }
    writer.Close();
}

uint32_t ReadFileUUID(const std::string &fname)
{
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("ReadIO");
    if (!engineName.empty())
    {
        io.SetEngine(engineName);
    }
    adios2::Engine reader = io.Open(fname, adios2::Mode::ReadRandomAccess);
    const uint32_t id = reader.FileUUID();
    reader.Close();
    return id;
}

} // namespace

// The writer stamps a non-zero per-file id, and appending preserves it (the
// header is written once at creation, never on append).
TEST(BPFileUUID, StampedAndPreservedAcrossAppend)
{
    const std::string fname = "BPFileUUID.bp";
    WriteFile(fname, 3, adios2::Mode::Write);

    const uint32_t created = ReadFileUUID(fname);
    EXPECT_NE(created, 0u) << "writer should stamp a non-zero per-file id";

    WriteFile(fname, 2, adios2::Mode::Append);
    EXPECT_EQ(ReadFileUUID(fname), created) << "append must preserve the per-file id";
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
