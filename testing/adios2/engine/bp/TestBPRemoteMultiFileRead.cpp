/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Interleaved reads from two files open at once.  Run remotely
 * (DoRemote=1), both readers share one server connection; each read must
 * address the reader's own file regardless of open/read ordering.
 */

#include <cstdint>

#include <numeric>
#include <vector>

#include <adios2.h>

#include <gtest/gtest.h>

#include "../TestHelpers.h"

std::string engineName; // comes from command line

TEST(BPRemoteMultiFileRead, InterleavedReads)
{
    const std::string fnameA = "RemoteMultiFileA.bp";
    const std::string fnameB = "RemoteMultiFileB.bp";
    const size_t Nx = 100;

    std::vector<double> uniqueA(Nx), uniqueB(Nx), sharedA(Nx), sharedB(Nx);
    std::iota(uniqueA.begin(), uniqueA.end(), 0.0);
    std::iota(uniqueB.begin(), uniqueB.end(), 1000.0);
    std::iota(sharedA.begin(), sharedA.end(), 2000.0);
    std::iota(sharedB.begin(), sharedB.end(), 3000.0);

    adios2::ADIOS adios;

    // Write the two files, each with one uniquely named variable and one
    // variable whose name exists in both files but with different content
    {
        adios2::IO io = adios.DeclareIO("WriteA");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        auto varUnique = io.DefineVariable<double>("uniqueA", {Nx}, {0}, {Nx});
        auto varShared = io.DefineVariable<double>("shared", {Nx}, {0}, {Nx});
        adios2::Engine writer = io.Open(fnameA, adios2::Mode::Write);
        writer.BeginStep();
        writer.Put(varUnique, uniqueA.data());
        writer.Put(varShared, sharedA.data());
        writer.EndStep();
        writer.Close();
    }
    {
        adios2::IO io = adios.DeclareIO("WriteB");
        if (!engineName.empty())
        {
            io.SetEngine(engineName);
        }
        auto varUnique = io.DefineVariable<double>("uniqueB", {Nx}, {0}, {Nx});
        auto varShared = io.DefineVariable<double>("shared", {Nx}, {0}, {Nx});
        adios2::Engine writer = io.Open(fnameB, adios2::Mode::Write);
        writer.BeginStep();
        writer.Put(varUnique, uniqueB.data());
        writer.Put(varShared, sharedB.data());
        writer.EndStep();
        writer.Close();
    }

    // Read with both files open at once, switching between them
    {
        adios2::IO ioA = adios.DeclareIO("ReadA");
        adios2::IO ioB = adios.DeclareIO("ReadB");
        if (!engineName.empty())
        {
            ioA.SetEngine(engineName);
            ioB.SetEngine(engineName);
        }
        adios2::Engine readerA = ioA.Open(fnameA, adios2::Mode::ReadRandomAccess);
        adios2::Engine readerB = ioB.Open(fnameB, adios2::Mode::ReadRandomAccess);

        auto varUniqueA = ioA.InquireVariable<double>("uniqueA");
        auto varSharedA = ioA.InquireVariable<double>("shared");
        auto varUniqueB = ioB.InquireVariable<double>("uniqueB");
        auto varSharedB = ioB.InquireVariable<double>("shared");
        ASSERT_TRUE(varUniqueA);
        ASSERT_TRUE(varSharedA);
        ASSERT_TRUE(varUniqueB);
        ASSERT_TRUE(varSharedB);

        std::vector<double> in(Nx);

        readerA.Get(varUniqueA, in.data(), adios2::Mode::Sync);
        ASSERT_EQ(in, uniqueA);

        readerB.Get(varUniqueB, in.data(), adios2::Mode::Sync);
        ASSERT_EQ(in, uniqueB);

        // back to reader A: same-named variable must come from file A
        readerA.Get(varSharedA, in.data(), adios2::Mode::Sync);
        ASSERT_EQ(in, sharedA);

        // and a variable that exists only in file A must still resolve
        readerA.Get(varUniqueA, in.data(), adios2::Mode::Sync);
        ASSERT_EQ(in, uniqueA);

        readerB.Get(varSharedB, in.data(), adios2::Mode::Sync);
        ASSERT_EQ(in, sharedB);

        readerA.Close();
        readerB.Close();
    }

    CleanupTestFiles(fnameA);
    CleanupTestFiles(fnameB);
}

int main(int argc, char **argv)
{
    int result;
    ::testing::InitGoogleTest(&argc, argv);
    if (argc > 1)
    {
        engineName = std::string(argv[1]);
    }
    result = RUN_ALL_TESTS();
    return result;
}
