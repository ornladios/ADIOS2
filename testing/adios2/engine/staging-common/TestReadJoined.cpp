/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstdint>
#include <cstring>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

#include <adios2.h>

#include <gtest/gtest.h>

#include "TestData.h"

#include "ParseArgs.h"

class CommonReadTest : public ::testing::Test
{
public:
    CommonReadTest() = default;
};

typedef std::chrono::duration<double> Seconds;

#if ADIOS2_USE_MPI
MPI_Comm testComm;
#endif

const int nsteps = 3;
const size_t Ncols = 4;
const std::vector<int> nblocksPerProcess = {2, 3, 2, 1, 3, 2};
int nMyTotalRows[nsteps];
int nTotalRows[nsteps];

namespace
{
constexpr size_t MaxEncodedRanks = 6;
constexpr size_t MaxEncodedBlocks = 3;
constexpr size_t MaxEncodedRows = 11;

struct ExpectedBlock
{
    size_t Start;
    size_t Rows;
    int Rank;
    size_t Block;
};

size_t RowsForBlock(int step, int rank, size_t block)
{
    return 5 + static_cast<size_t>((step * 3 + rank * 5 + block * 2) % 6);
}

uint64_t EncodeElement(size_t step, int rank, size_t block, size_t row, size_t col)
{
    return (((((static_cast<uint64_t>(step) * MaxEncodedRanks + static_cast<uint64_t>(rank)) *
               MaxEncodedBlocks +
               block) *
                  MaxEncodedRows +
              row) *
                 Ncols) +
            col);
}
} // namespace

// ADIOS2 Common read
TEST_F(CommonReadTest, ADIOS2CommonRead1D8)
{
    int mpiRank = 0, mpiSize = 1;

#if ADIOS2_USE_MPI
    MPI_Comm_rank(testComm, &mpiRank);
    MPI_Comm_size(testComm, &mpiSize);
#endif

#if ADIOS2_USE_MPI
    adios2::ADIOS adios(testComm);
#else
    adios2::ADIOS adios;
#endif
    adios2::IO inIO = adios.DeclareIO("Input");

    inIO.SetEngine(engine);
    inIO.SetParameters(engineParams);

    adios2::Engine reader = inIO.Open(fname, adios2::Mode::Read);

    int step = 0;
    while (true)
    {
        adios2::StepStatus status = reader.BeginStep(adios2::StepMode::Read);

        if (status != adios2::StepStatus::OK)
        {
            break;
        }

        auto rows_var = inIO.InquireVariable<int>("totalrows");
        auto bpp_var = inIO.InquireVariable<int>("blocksperprocess");
        auto writers_var = inIO.InquireVariable<int>("numwriters");
        auto var = inIO.InquireVariable<double>("table");
        EXPECT_TRUE(rows_var);
        EXPECT_TRUE(bpp_var);
        EXPECT_TRUE(writers_var);
        EXPECT_TRUE(var);

        int totalRows = 0;
        int numWriters = 0;
        std::vector<int> actualBlocksPerProcess(nblocksPerProcess.size());
        bpp_var.SetSelection({{0}, {actualBlocksPerProcess.size()}});
        reader.Get(rows_var, totalRows);
        reader.Get(writers_var, numWriters);
        reader.Get(bpp_var, actualBlocksPerProcess.data());
        reader.PerformGets();

        EXPECT_GE(numWriters, 1);
        EXPECT_LE(numWriters, static_cast<int>(nblocksPerProcess.size()));
        for (size_t rank = 0; rank < nblocksPerProcess.size(); ++rank)
        {
            EXPECT_EQ(actualBlocksPerProcess[rank], nblocksPerProcess[rank]);
        }

        const size_t writerCount = static_cast<size_t>(
            std::max(0, std::min(numWriters, static_cast<int>(nblocksPerProcess.size()))));
        std::vector<ExpectedBlock> expectedBlocks;
        size_t expectedRows = 0;
        for (size_t writerRank = 0; writerRank < writerCount; ++writerRank)
        {
            const size_t blockCount = static_cast<size_t>(nblocksPerProcess[writerRank]);
            for (size_t block = 0; block < blockCount; ++block)
            {
                const size_t rows = RowsForBlock(step, static_cast<int>(writerRank), block);
                expectedBlocks.push_back({expectedRows, rows, static_cast<int>(writerRank), block});
                expectedRows += rows;
            }
        }

        EXPECT_EQ(totalRows, static_cast<int>(expectedRows));
        EXPECT_EQ(var.Shape().size(), 2u);
        const size_t shapeRows = var.Shape().empty() ? 0 : var.Shape()[0];
        EXPECT_EQ(shapeRows, expectedRows);
        if (var.Shape().size() == 2)
        {
            EXPECT_EQ(var.Shape()[1], Ncols);
        }

        const auto blockInfos = reader.BlocksInfo(var, static_cast<size_t>(step));
        EXPECT_EQ(blockInfos.size(), expectedBlocks.size());
        for (size_t blockIndex = 0;
             blockIndex < std::min(blockInfos.size(), expectedBlocks.size()); ++blockIndex)
        {
            const auto &info = blockInfos[blockIndex];
            const auto &expected = expectedBlocks[blockIndex];
            EXPECT_EQ(info.BlockID, blockIndex);
            if (info.Start.size() == 2 && info.Count.size() == 2)
            {
                EXPECT_EQ(info.Start[0], expected.Start);
                EXPECT_EQ(info.Start[1], 0u);
                EXPECT_EQ(info.Count[0], expected.Rows);
                EXPECT_EQ(info.Count[1], Ncols);
            }
            else
            {
                ADD_FAILURE() << "Unexpected JoinedArray block dimensionality";
            }
        }

        // Divide the joined rows among reader ranks; each element is checked
        // against its expected writer, block, row, and column.
        const size_t readStart = shapeRows * static_cast<size_t>(mpiRank) / mpiSize;
        const size_t readEnd = shapeRows * static_cast<size_t>(mpiRank + 1) / mpiSize;
        const size_t readRows = readEnd - readStart;
        var.SetSelection({{readStart, 0}, {readRows, Ncols}});
        std::vector<double> data(readRows * Ncols);
        reader.Get(var, data.data());
        reader.PerformGets();

        size_t blockIndex = 0;
        for (size_t localRow = 0; localRow < readRows && !expectedBlocks.empty(); ++localRow)
        {
            const size_t globalRow = readStart + localRow;
            while (blockIndex + 1 < expectedBlocks.size() &&
                   globalRow >= expectedBlocks[blockIndex].Start + expectedBlocks[blockIndex].Rows)
            {
                ++blockIndex;
            }

            const auto &expected = expectedBlocks[blockIndex];
            EXPECT_GE(globalRow, expected.Start);
            EXPECT_LT(globalRow, expected.Start + expected.Rows);
            const size_t blockRow = globalRow - expected.Start;
            for (size_t col = 0; col < Ncols; ++col)
            {
                const double expectedValue = static_cast<double>(EncodeElement(
                    static_cast<size_t>(step), expected.Rank, expected.Block, blockRow, col));
                EXPECT_EQ(data[localRow * Ncols + col], expectedValue)
                    << "step=" << step << " global row=" << globalRow << " column=" << col;
            }
        }

        reader.EndStep();
        ++step;
    }
    reader.Close();
    EXPECT_EQ(step, nsteps);
}

//******************************************************************************
// main
//******************************************************************************

int main(int argc, char **argv)
{
    int result;
    ::testing::InitGoogleTest(&argc, argv);
    ParseArgs(argc, argv);

#if ADIOS2_USE_MPI
    int provided;
    int thread_support_level =
        (engine == "SST" || engine == "sst") ? MPI_THREAD_MULTIPLE : MPI_THREAD_SINGLE;

    // MPI_THREAD_MULTIPLE is only required if you enable the SST MPI_DP
    MPI_Init_thread(nullptr, nullptr, thread_support_level, &provided);

    int key;
    MPI_Comm_rank(MPI_COMM_WORLD, &key);

    const unsigned int color = 2;
    MPI_Comm_split(MPI_COMM_WORLD, color, key, &testComm);
#endif

    result = RUN_ALL_TESTS();

#if ADIOS2_USE_MPI
#ifdef CRAY_MPICH_VERSION
    MPI_Barrier(MPI_COMM_WORLD);
#else
    MPI_Finalize();
#endif
#endif

    return result;
}
