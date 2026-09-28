/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstdint>
#include <cstring>

#include <algorithm>
#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#include "../TestHelpers.h"

std::string engineName; // comes from command line

namespace
{
constexpr size_t MaxEncodedRanks = 1u << 20;
constexpr size_t MaxEncodedBlocks = 4;
constexpr size_t MaxEncodedRows = 16;
constexpr size_t Ncols = 4;

struct ExpectedBlock
{
    size_t Start;
    size_t Rows;
    int Rank;
    size_t Block;
};

size_t BlocksForRank(int rank, const std::vector<int> &nblocksPerProcess)
{
    return rank < static_cast<int>(nblocksPerProcess.size())
               ? static_cast<size_t>(nblocksPerProcess[rank])
               : 1;
}

size_t RowsForBlock(int step, int rank, size_t block)
{
    return 5 + static_cast<size_t>((step * 3 + rank * 5 + block * 2) % 6);
}

uint64_t EncodeElement(size_t step, int rank, size_t block, size_t row, size_t col)
{
    // Keep each tuple distinct while remaining exactly representable as a double.
    return (((((static_cast<uint64_t>(step) * MaxEncodedRanks +
                static_cast<uint64_t>(rank)) *
                   MaxEncodedBlocks +
               block) *
                  MaxEncodedRows +
              row) *
                 Ncols) +
            col);
}
} // namespace

class BPJoinedArray : public ::testing::Test
{
public:
    BPJoinedArray() = default;

    SmallTestData m_TestData;
};

TEST_F(BPJoinedArray, MultiBlock)
{
    // Write multiple blocks per process
    // Change number of rows per block and per process
    // Change total number of rows in each step
    // Write two variables to ensure both will end up with the same order of
    // rows in reading

    const int nsteps = 3;
    const size_t ncols = Ncols;
    const std::vector<int> nblocksPerProcess = {2, 3, 2, 1, 3, 2};

    int rank = 0, nproc = 1;

#if ADIOS2_USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    adios2::ADIOS adios(MPI_COMM_WORLD);
    const int nblocks =
        (rank < static_cast<int>(nblocksPerProcess.size()) ? nblocksPerProcess[rank] : 1);
#else
    adios2::ADIOS adios;
    const int nblocks = nblocksPerProcess[0];
#endif

    const std::string fname = "BPJoinedArrayMultiblock_nproc_" + std::to_string(nproc) + ".bp";
    int nMyTotalRows[nsteps];
    int nTotalRows[nsteps];

    // Writer
    {
        adios2::IO outIO = adios.DeclareIO("Output");

        if (!engineName.empty())
        {
            outIO.SetEngine(engineName);
        }

        adios2::Engine writer = outIO.Open(fname, adios2::Mode::Write);
        auto var = outIO.DefineVariable<double>(
            "table", {static_cast<size_t>(adios2::JoinedDim), ncols}, {}, {1, ncols});

        if (!rank)
        {
            std::cout << "Writing to " << fname << std::endl;
        }

        for (int step = 0; step < nsteps; step++)
        {
            // Use deterministic, varying block sizes so the reader can derive
            // the expected layout independently on every rank.
            std::vector<size_t> Nrows;
            nMyTotalRows[step] = 0;
            for (int i = 0; i < nblocks; ++i)
            {
                const size_t rows = RowsForBlock(step, rank, static_cast<size_t>(i));
                Nrows.push_back(rows);
                nMyTotalRows[step] += static_cast<int>(rows);
            }

            nTotalRows[step] = nMyTotalRows[step];
#if ADIOS2_USE_MPI
            MPI_Allreduce(&(nMyTotalRows[step]), &(nTotalRows[step]), 1, MPI_INT, MPI_SUM,
                          MPI_COMM_WORLD);
#endif

            if (!rank)
            {
                std::cout << "Writing " << nTotalRows[step] << " rows in step " << step
                          << std::endl;
            }

            writer.BeginStep();
            for (int block = 0; block < nblocks; ++block)
            {
                std::vector<double> mytable(Nrows[block] * ncols);
                for (size_t row = 0; row < Nrows[block]; row++)
                {
                    for (size_t col = 0; col < ncols; col++)
                    {
                        mytable[row * ncols + col] = static_cast<double>(EncodeElement(
                            static_cast<size_t>(step), rank, static_cast<size_t>(block), row, col));
                    }
                }

                var.SetSelection({{}, {Nrows[block], ncols}});

                std::cout << "Step " << step << " rank " << rank << " block " << block << " count ("
                          << var.Count()[0] << ", " << var.Count()[1] << ")" << std::endl;

                writer.Put(var, mytable.data(), adios2::Mode::Sync);
            }
            writer.EndStep();
        }
        writer.Close();
    }

    // Reader with streaming
    {
        adios2::IO inIO = adios.DeclareIO("Input");

        if (!engineName.empty())
        {
            inIO.SetEngine(engineName);
        }
        adios2::Engine reader = inIO.Open(fname, adios2::Mode::Read);

        if (!rank)
        {
            std::cout << "Reading as stream with BeginStep/EndStep:" << std::endl;
        }

        int step = 0;
        while (true)
        {
            adios2::StepStatus status = reader.BeginStep(adios2::StepMode::Read);

            if (status != adios2::StepStatus::OK)
            {
                break;
            }

            auto var = inIO.InquireVariable<double>("table");
            EXPECT_TRUE(var);

            if (!rank)
            {
                std::cout << "Step " << step << " table shape (" << var.Shape()[0] << ", "
                          << var.Shape()[1] << ")" << std::endl;
            }

            size_t Nrows = static_cast<size_t>(nTotalRows[step]);
            EXPECT_EQ(var.Shape()[0], Nrows);
            EXPECT_EQ(var.Shape()[1], Ncols);

            std::vector<ExpectedBlock> expectedBlocks;
            size_t expectedStart = 0;
            for (int writerRank = 0; writerRank < nproc; ++writerRank)
            {
                for (size_t block = 0; block < BlocksForRank(writerRank, nblocksPerProcess); ++block)
                {
                    const size_t rows = RowsForBlock(step, writerRank, block);
                    expectedBlocks.push_back({expectedStart, rows, writerRank, block});
                    expectedStart += rows;
                }
            }

            EXPECT_EQ(expectedStart, Nrows);

            const auto blockInfos = reader.BlocksInfo(var, static_cast<size_t>(step));
            EXPECT_EQ(blockInfos.size(), expectedBlocks.size());
            for (size_t blockIndex = 0;
                 blockIndex < std::min(blockInfos.size(), expectedBlocks.size()); ++blockIndex)
            {
                const auto &info = blockInfos[blockIndex];
                const auto &expected = expectedBlocks[blockIndex];
                // JoinedArray metadata may report an aggregator rank. The encoded
                // rank in each element validates its writer independently.
                EXPECT_EQ(info.BlockID, blockIndex);
                if (info.Start.size() == 2 && info.Count.size() == 2)
                {
                    EXPECT_EQ(info.Start[0], expected.Start);
                    EXPECT_EQ(info.Start[1], 0u);
                    EXPECT_EQ(info.Count[0], expected.Rows);
                    EXPECT_EQ(info.Count[1], ncols);
                }
                else
                {
                    ADD_FAILURE() << "Unexpected JoinedArray block dimensionality";
                }
            }

            // Each rank reads a disjoint row interval. Block metadata establishes
            // the expected global position of every encoded row.
            const size_t readStart = Nrows * static_cast<size_t>(rank) / nproc;
            const size_t readEnd = Nrows * static_cast<size_t>(rank + 1) / nproc;
            const size_t readRows = readEnd - readStart;
            var.SetSelection({{readStart, 0}, {readRows, ncols}});

            std::vector<double> data(readRows * ncols);
            reader.Get(var, data.data());
            reader.PerformGets();

            size_t blockIndex = 0;
            for (size_t localRow = 0; localRow < readRows; ++localRow)
            {
                const size_t globalRow = readStart + localRow;
                while (blockIndex + 1 < expectedBlocks.size() &&
                       globalRow >= expectedBlocks[blockIndex].Start +
                                        expectedBlocks[blockIndex].Rows)
                {
                    ++blockIndex;
                }

                const auto &expected = expectedBlocks[blockIndex];
                EXPECT_GE(globalRow, expected.Start);
                EXPECT_LT(globalRow, expected.Start + expected.Rows);
                const size_t blockRow = globalRow - expected.Start;
                for (size_t col = 0; col < ncols; ++col)
                {
                    const double expectedValue = static_cast<double>(EncodeElement(
                        static_cast<size_t>(step), expected.Rank, expected.Block, blockRow, col));
                    EXPECT_EQ(data[localRow * ncols + col], expectedValue)
                        << "step=" << step << " global row=" << globalRow << " column=" << col;
                }
            }

            reader.EndStep();
            ++step;
        }
        reader.Close();
        EXPECT_EQ(step, nsteps);
    }

    // Cleanup generated files
#if ADIOS2_USE_MPI
    CleanupTestFilesMPI(fname, MPI_COMM_WORLD);
#else
    CleanupTestFiles(fname);
#endif
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
