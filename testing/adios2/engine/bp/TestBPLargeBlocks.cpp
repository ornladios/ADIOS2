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

#include <adios2.h>

#include <gtest/gtest.h>

std::string engineName; // comes from command line

class LargeBlocks : public ::testing::Test
{
public:
    LargeBlocks() = default;
};

constexpr size_t DefaultMaxFileBatchSize = 2147381248;

TEST_F(LargeBlocks, MultiBlock)
{
    // Write a small variable
    // Write a large (>2147381248 bytes) so that it's offset is >0 in the file.
    // Read the large variable back and confirm that the data on the
    // chunk boundary of 2147381248 bytes is correct.
    // Testing if the POSIX reader transport is reading in chunks of 2147381248
    // bytes correctly (from the correct offsets)

    int rank = 0, nproc = 1;

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
