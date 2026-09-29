/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstdint>
#include <cstring>

#include <algorithm>
#include <iostream>
#include <set>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#include "../TestHelpers.h"

std::string engineName; // comes from command line

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
    const size_t Ncols = 4;
    const std::vector<int> nblocksPerProcess = {2, 3, 2, 1, 3, 2};
    // Values encode (step, rank, block, row, col) as an exact integer
    const int64_t MaxBlocks = *std::max_element(nblocksPerProcess.begin(), nblocksPerProcess.end());
    const int64_t MaxRows = 10; // blocks have 5..10 rows

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
            "table", {static_cast<size_t>(adios2::JoinedDim), Ncols}, {}, {1, Ncols});

        if (!rank)
        {
            std::cout << "Writing to " << fname << std::endl;
        }

        for (int step = 0; step < nsteps; step++)
        {
            // Application variables for output random size per process, 5..10
            // each
            std::vector<size_t> Nrows;
            nMyTotalRows[step] = 0;
            for (int i = 0; i < nblocks; ++i)
            {
                int n = rand() % 6 + 5;
                Nrows.push_back(static_cast<size_t>(n));
                nMyTotalRows[step] += n;
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
                std::vector<double> mytable(Nrows[block] * Ncols);
                for (size_t row = 0; row < Nrows[block]; row++)
                {
                    for (size_t col = 0; col < Ncols; col++)
                    {
                        mytable[row * Ncols + col] = static_cast<double>(
                            (((step * nproc + rank) * MaxBlocks + block) * MaxRows + row) * Ncols +
                            col);
                    }
                }

                var.SetSelection({{}, {Nrows[block], Ncols}});

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

            var.SetSelection({{0, 0}, {Nrows, Ncols}});

            // Check data on rank 0
            if (!rank)
            {
                std::vector<double> data(Nrows * Ncols);
                reader.Get(var, data.data());
                reader.PerformGets();
                // Row order across writers is not guaranteed, so decode each
                // row and require every written row exactly once
                std::set<int64_t> seen;
                for (size_t i = 0; i < Nrows; ++i)
                {
                    const int64_t id = static_cast<int64_t>(data[i * Ncols]) / Ncols;
                    for (size_t j = 0; j < Ncols; ++j)
                    {
                        EXPECT_EQ(data[i * Ncols + j], static_cast<double>(id * Ncols + j));
                    }
                    const int64_t row = id % MaxRows;
                    const int64_t block = id / MaxRows % MaxBlocks;
                    const int64_t wrank = id / MaxRows / MaxBlocks % nproc;
                    const int64_t wstep = id / MaxRows / MaxBlocks / nproc;
                    const int wblocks = (wrank < static_cast<int64_t>(nblocksPerProcess.size())
                                             ? nblocksPerProcess[wrank]
                                             : 1);
                    EXPECT_EQ(wstep, step);
                    EXPECT_LT(block, wblocks);
                    EXPECT_LT(row, MaxRows);
                    EXPECT_TRUE(seen.insert(id).second) << "duplicate row " << i;
                }
                EXPECT_EQ(seen.size(), Nrows);
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
