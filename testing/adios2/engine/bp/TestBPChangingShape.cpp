/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TestBPChangingShape.cpp :
 *
 *  Created on: Dec 17, 2018
 *      Author: Norbert Podhorszki, Keichi Takahashi
 */

#include <cstdint>
#include <cstring>

#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#include "../SmallTestData.h"

std::string engineName; // comes from command line

class BPChangingShape : public ::testing::Test
{
public:
    BPChangingShape() = default;

    SmallTestData m_TestData;
};

TEST_F(BPChangingShape, BPWriteReadShape2D)
{
    // Each process would write a 4x2 array and all processes would
    // form a 2D 4 * (NumberOfProcess * Nx) matrix where Nx is 2 here

    const std::string fname("BPChangingShape.bp");
    const int nsteps = 10;
    int rank = 0, nproc = 1;

#if ADIOS2_USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    adios2::ADIOS adios;
#endif
    // Writer
    {
        adios2::IO outIO = adios.DeclareIO("Output");

        if (!engineName.empty())
        {
            outIO.SetEngine(engineName);
        }

        adios2::Engine writer = outIO.Open(fname, adios2::Mode::Write);

        const size_t dim0 = static_cast<size_t>(nproc);
        const size_t off0 = static_cast<size_t>(rank);
        auto var =
            outIO.DefineVariable<double>("v", {dim0, 1}, {off0, 0}, {1, 1});

        std::vector<double> buf(nsteps, 0.0);
        for (size_t i = 0; i < buf.size(); i++)
        {
            buf[i] = rank + static_cast<double>(i) / 10.0;
        }

        if (!rank)
        {
            std::cout << "Writing:" << std::endl;
        }
        for (size_t i = 0; i < nsteps; i++)
        {
            writer.BeginStep();

            var.SetShape({dim0, i + 1});
            var.SetSelection({{off0, 0}, {1, i + 1}});

            if (!rank)
            {
                std::cout << "Step " << i << " shape (" << var.Shape()[0]
                          << ", " << var.Shape()[1] << ")" << std::endl;
            }

            writer.Put(var, buf.data());

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
            std::cout << "Reading as stream with BeginStep/EndStep:"
                      << std::endl;
        }

        int i = 0;
        while (true)
        {
            adios2::StepStatus status =
                reader.BeginStep(adios2::StepMode::Read);

            if (status != adios2::StepStatus::OK)
            {
                break;
            }

            size_t step = reader.CurrentStep();
            size_t expected_shape = step + 1;

            auto var = inIO.InquireVariable<double>("v");
            EXPECT_TRUE(var);

            if (!rank)
            {

                std::cout << "Step " << i << " shape (" << var.Shape()[0]
                          << ", " << var.Shape()[1] << ")" << std::endl;
            }

            EXPECT_EQ(var.Shape()[0], nproc);
            EXPECT_EQ(var.Shape()[1], expected_shape);

            reader.EndStep();
            ++i;
        }

        reader.Close();
    }

    // Reader with file reading
    {
        adios2::IO inIO = adios.DeclareIO("InputFile");

        if (!engineName.empty())
        {
            inIO.SetEngine(engineName);
        }
        adios2::Engine reader =
            inIO.Open(fname, adios2::Mode::ReadRandomAccess);

        if (!rank)
        {
            std::cout << "Reading as file with SetStepSelection:" << std::endl;
        }

        auto var = inIO.InquireVariable<double>("v");
        EXPECT_TRUE(var);

        for (size_t i = 0; i < nsteps; i++)
        {
            var.SetStepSelection({i, 1});
            if (!rank)
            {

                std::cout << "Step " << i << " shape (" << var.Shape()[0]
                          << ", " << var.Shape()[1] << ")" << std::endl;
            }
            size_t expected_shape = i + 1;
            EXPECT_EQ(var.Shape()[0], nproc);
            EXPECT_EQ(var.Shape()[1], expected_shape);
        }

        reader.Close();
    }
}

TEST_F(BPChangingShape, MultiBlock)
{
    // Write multiple blocks and change shape in between
    // At read, the last shape should be used not the first one
    // This test guarantees that one can change the variable shape
    // until EndStep()

    const std::string fname("BPChangingShapeMultiblock.bp");
    const int nsteps = 2;
    const std::vector<int> nblocks = {2, 3};
    EXPECT_EQ(nsteps, nblocks.size());
    int rank = 0, nproc = 1;

#if ADIOS2_USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    adios2::ADIOS adios;
#endif

    // Writer
    {
        adios2::IO outIO = adios.DeclareIO("Output");

        if (!engineName.empty())
        {
            outIO.SetEngine(engineName);
        }

        adios2::Engine writer = outIO.Open(fname, adios2::Mode::Write);

        const size_t dim0 = static_cast<size_t>(nproc);
        const size_t off0 = static_cast<size_t>(rank);
        auto var =
            outIO.DefineVariable<double>("v", {dim0, 1}, {off0, 0}, {1, 1});

        if (!rank)
        {
            std::cout << "Writing:" << std::endl;
        }
        for (size_t i = 0; i < nsteps; i++)
        {
            writer.BeginStep();

            double value =
                static_cast<double>(rank) + static_cast<double>(i + 1) / 10.0;

            for (size_t j = 0; j < static_cast<size_t>(nblocks[i]); j++)
            {
                var.SetShape({dim0, j + 1});
                var.SetSelection({{off0, j}, {1, 1}});

                if (!rank)
                {
                    std::cout << "Step " << i << " block " << j << " shape ("
                              << var.Shape()[0] << ", " << var.Shape()[1] << ")"
                              << " value = " << value << std::endl;
                }

                writer.Put(var, &value, adios2::Mode::Sync);
                value += 0.01;
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
            std::cout << "Reading as stream with BeginStep/EndStep:"
                      << std::endl;
        }

        int step = 0;
        while (true)
        {
            adios2::StepStatus status =
                reader.BeginStep(adios2::StepMode::Read);

            if (status != adios2::StepStatus::OK)
            {
                break;
            }

            size_t expected_shape = nblocks[step];

            auto var = inIO.InquireVariable<double>("v");
            EXPECT_TRUE(var);

            if (!rank)
            {

                std::cout << "Step " << step << " shape (" << var.Shape()[0]
                          << ", " << var.Shape()[1] << ")" << std::endl;
            }

            EXPECT_EQ(var.Shape()[0], nproc);
            EXPECT_EQ(var.Shape()[1], expected_shape);

            var.SetSelection(
                {{0, 0}, {static_cast<size_t>(nproc), expected_shape}});

            // Check data on rank 0
            if (!rank)
            {
                std::vector<double> data(nproc * expected_shape);
                reader.Get(var, data.data());

                reader.PerformGets();

                for (int i = 0; i < nproc; i++)
                {
                    double value = static_cast<double>(i) +
                                   static_cast<double>(step + 1) / 10.0;

                    for (int j = 0; j < nblocks[step]; j++)
                    {
                        EXPECT_EQ(data[i * nblocks[step] + j], value);
                        value += 0.01;
                    }
                }
            }

            reader.EndStep();
            ++step;
        }
        reader.Close();
    }

    // Reader with file reading
    {
        adios2::IO inIO = adios.DeclareIO("InputFile");

        if (!engineName.empty())
        {
            inIO.SetEngine(engineName);
        }
        adios2::Engine reader =
            inIO.Open(fname, adios2::Mode::ReadRandomAccess);

        if (!rank)
        {
            std::cout << "Reading as file with SetStepSelection:" << std::endl;
        }

        auto var = inIO.InquireVariable<double>("v");
        EXPECT_TRUE(var);
        for (int step = 0; step < nsteps; step++)
        {
            var.SetStepSelection({step, 1});
            if (!rank)
            {
                std::cout << "Step " << step << " shape (" << var.Shape()[0]
                          << ", " << var.Shape()[1] << ")" << std::endl;
            }
            size_t expected_shape = nblocks[step];
            EXPECT_EQ(var.Shape()[0], nproc);
            EXPECT_EQ(var.Shape()[1], expected_shape);

            var.SetSelection(
                {{0, 0}, {static_cast<size_t>(nproc), expected_shape}});

            std::vector<double> data(nproc * expected_shape);
            reader.Get(var, data.data());

            EXPECT_THROW(reader.EndStep(), std::logic_error);
        }
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
