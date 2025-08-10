/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TestLargeBlocks.cpp :
 *
 *  Created on: Aug 5, 2025
 *      Author: Norbert Podhorszki
 */

#include <cstdint>
#include <cstring>

#include <iostream>
#include <stdexcept>
#include <sys/statvfs.h>

#include <adios2.h>
#include <adios2sys/SystemTools.hxx>

#include <gtest/gtest.h>

std::string engineName; // comes from command line

constexpr size_t DefaultMaxFileBatchSize = 4294762496;
constexpr double AdditionalSpace = 0.2;
constexpr size_t RequiredDiskSpace =
    static_cast<size_t>(DefaultMaxFileBatchSize * (1.0 + AdditionalSpace));

class LargeBlocks : public ::testing::Test
{
public:
    LargeBlocks() = default;
    bool CanDiskAllocateBytes(size_t size)
    {
        struct statvfs stat;
        if (statvfs(".", &stat) != 0)
        {
            return false;
        }
        // # of block avail * block_size
        const size_t available_size = static_cast<size_t>(stat.f_bavail) * stat.f_frsize;
        return available_size >= size;
    }
};

TEST_F(LargeBlocks, MultiBlock)
{
    if (!this->CanDiskAllocateBytes(RequiredDiskSpace))
    {
        std::cerr << "SKIP: insufficient space in disk, bytes needed: " << RequiredDiskSpace
                  << std::endl;
        return;
    }

    // Write a small variable
    // Write a large (>2147381248 bytes) so that it's offset is >0 in the file.
    // Read the large variable back and confirm that the data on the
    // chunk boundary of 2147381248 bytes is correct.
    // Testing if the POSIX reader transport is reading in chunks of 2147381248
    // bytes correctly (from the correct offsets)

    adios2::ADIOS adios;

    const std::string fname = "LargeBlocks.bp";
    size_t NX = DefaultMaxFileBatchSize / 4 +
                1024; // data must be > 2147381248 bytes to test chunked writing/reading

    std::vector<uint32_t> LargeData(NX);
    uint32_t myvalue = 0;
    for (size_t n = 0; n < NX; ++n)
    {
        LargeData[n] = myvalue++;
    }

    // Writer
    {
        adios2::IO outIO = adios.DeclareIO("Output");
        if (!engineName.empty())
        {
            outIO.SetEngine(engineName);
        }
        adios2::Engine writer = outIO.Open(fname, adios2::Mode::Write);
        auto varSmall = outIO.DefineVariable<uint32_t>("small", {2000}, {0}, {2000});
        auto varLarge = outIO.DefineVariable<uint32_t>("large", {NX}, {0}, {NX});

        std::cout << "Writing to " << fname << std::endl;
        writer.BeginStep();
        writer.Put(varSmall, LargeData.data(), adios2::Mode::Deferred);
        writer.Put(varLarge, LargeData.data(), adios2::Mode::Deferred);
        writer.EndStep();
        writer.Close();
    }

    // Reader with streaming
    {
        adios2::IO inIO = adios.DeclareIO("Input");
        if (!engineName.empty())
        {
            inIO.SetEngine(engineName);
        }
        adios2::Engine reader = inIO.Open(fname, adios2::Mode::ReadRandomAccess);
        std::cout << "Reading as stream with BeginStep/EndStep:" << std::endl;

        auto varLarge = inIO.InquireVariable<uint32_t>("large");
        EXPECT_TRUE(varLarge);
        std::cout << " large data shape " << varLarge.Shape()[0] << std::endl;

        varLarge.SetSelection({{0}, {NX}});
        std::vector<uint32_t> data(NX);
        reader.Get(varLarge, data.data());
        reader.PerformGets();

        size_t TestOffset = DefaultMaxFileBatchSize / 4;
        std::cout << "large data from " << TestOffset - 10 << ":" << std::endl;
        std::cout << "  written: " << std::endl;
        for (size_t i = TestOffset - 10; i < TestOffset + 10; ++i)
        {
            std::cout << static_cast<int>(LargeData[i]) << " ";
        }
        std::cout << std::endl;
        std::cout << "  read: " << std::endl;
        for (size_t i = TestOffset - 10; i < TestOffset + 10; ++i)
        {
            std::cout << static_cast<int>(data[i]) << " ";
        }
        std::cout << std::endl;
        for (size_t i = TestOffset - 10; i < TestOffset + 10; ++i)
        {
            EXPECT_EQ(data[i], LargeData[i]);
        }
        reader.Close();
    }
    adios2sys::SystemTools::RemoveADirectory(fname);
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
