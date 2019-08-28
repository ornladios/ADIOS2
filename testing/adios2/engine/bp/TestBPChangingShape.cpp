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

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(true);
#endif
    // Writer
    adios2::IO outIO = adios.DeclareIO("Output");

    if (!engineName.empty())
    {
        outIO.SetEngine(engineName);
    }

    adios2::Engine writer = outIO.Open(fname, adios2::Mode::Write);

    const size_t dim0 = static_cast<size_t>(nproc);
    const size_t off0 = static_cast<size_t>(rank);
    auto var = outIO.DefineVariable<double>("v", {dim0, 1}, {off0, 0}, {1, 1});

    std::vector<double> buf(nsteps, 0.0);
    for (size_t i = 0; i < buf.size(); i++)
    {
        buf[i] = rank + static_cast<double>(i) / 10.0;
    }

    if (!rank)
    {
        std::cout << "Writing:" << std::endl;
    }
    for (size_t i = 1; i <= nsteps; i++)
    {
        writer.BeginStep();

        var.SetShape({dim0, i});
        var.SetSelection({{off0, 0}, {1, i}});

        if (!rank)
        {
            std::cout << "Step " << i << " shape (" << var.Shape()[0] << ", "
                      << var.Shape()[1] << ")" << std::endl;
        }

        writer.Put(var, buf.data());

        writer.EndStep();
    }

    writer.Close();

    // Reader

    adios2::IO inIO = adios.DeclareIO("Input");

    if (!engineName.empty())
    {
        inIO.SetEngine(engineName);
    }
    adios2::Engine reader = inIO.Open(fname, adios2::Mode::Read);

    if (!rank)
    {
        std::cout << "Reading:" << std::endl;
    }

    int i = 0;
    while (true)
    {
        adios2::StepStatus status = reader.BeginStep(adios2::StepMode::Read);

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

            std::cout << "Step " << i << " shape (" << var.Shape()[0] << ", "
                      << var.Shape()[1] << ")" << std::endl;
        }

        EXPECT_EQ(var.Shape()[0], nproc);
        EXPECT_EQ(var.Shape()[1], expected_shape);

        ++i;
    }

    reader.Close();
}

int main(int argc, char **argv)
{
#ifdef ADIOS2_HAVE_MPI
    MPI_Init(nullptr, nullptr);
#endif

    int result;
    ::testing::InitGoogleTest(&argc, argv);

    if (argc > 1)
    {
        engineName = std::string(argv[1]);
    }

    result = RUN_ALL_TESTS();

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return result;
}
