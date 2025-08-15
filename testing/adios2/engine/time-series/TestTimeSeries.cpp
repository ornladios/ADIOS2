/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TestTimeSeries.cpp :
 *
 *  Created on: Apr 30, 2025
 *      Author: Norbert Podhorszki
 */

#include <cstdint>
#include <cstring>

#include <algorithm> // minmax_element
#include <fstream>
#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

std::string engineName; // comes from command line

class TimeSeries : public ::testing::Test
{
public:
    TimeSeries() = default;
};

TEST_F(TimeSeries, WriteReadShape2D)
{
    const int nsteps = 10;
    const int stepsPerFile = 4;
    int rank = 0, nproc = 1;
    const size_t fixDim = 5;

#if ADIOS2_USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    const std::string fname("TimeSeries");
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    const std::string fname("TimeSeries");
    adios2::ADIOS adios;
#endif

    const size_t dim0 = static_cast<size_t>(nproc);
    const size_t off0 = static_cast<size_t>(rank);

    std::vector<double> bufOut(nsteps, 0.0);
    for (size_t i = 0; i < bufOut.size(); i++)
    {
        bufOut[i] = rank + static_cast<double>(i) / 10.0;
    }

    std::vector<char> textOut = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j'};

    // Writer
    {
        adios2::IO outIO = adios.DeclareIO("Output");
        adios2::Engine writer;

        if (!engineName.empty())
        {
            outIO.SetEngine(engineName);
        }

        auto var = outIO.DefineVariable<double>("v", {dim0, 1}, {off0, 0}, {1, 1});
        auto varchar = outIO.DefineVariable<char>("vchar", {dim0, 1}, {off0, 0}, {1, 1});
        auto varfix =
            outIO.DefineVariable<double>("vfixdim", {dim0, fixDim}, {off0, 0}, {1, fixDim}, true);

        if (!rank)
        {
            std::cout << "Writing:" << std::endl;
        }
        size_t fileIdx = 0;
        std::ofstream atsFile(fname + ".ats", std::ios::out | std::ios::trunc);
        for (size_t i = 0; i < nsteps; i++)
        {
            if (i % stepsPerFile == 0)
            {
                if (writer)
                {
                    writer.Close();
                }
                std::string fileName;
                if (engineName == "HDF5")
                {
                    fileName = fname + "_" + std::to_string(fileIdx) + ".h5";
                }
                else
                {
                    fileName = fname + "_" + std::to_string(fileIdx) + ".bp";
                }

                writer = outIO.Open(fileName, adios2::Mode::Write);
                atsFile << "- " << fileName << std::endl;
                ++fileIdx;
            }
            writer.BeginStep();

            var.SetShape({dim0, i + 1});
            var.SetSelection({{off0, 0}, {1, i + 1}});

            varchar.SetShape({dim0, i + 1});
            varchar.SetSelection({{off0, 0}, {1, i + 1}});

            if (!rank)
            {
                std::cout << "Step " << i << " shape (" << var.Shape()[0] << ", " << var.Shape()[1]
                          << ")" << std::endl;
            }

            writer.Put(var, bufOut.data());
            writer.Put(varchar, textOut.data());
            writer.Put(varfix, bufOut.data());
            writer.EndStep();
        }

        writer.Close();
        atsFile << "- end" << std::endl;
        atsFile.close();
    }

    // Reader with streaming
    {
        adios2::IO inIO = adios.DeclareIO("Input");
        adios2::Engine reader = inIO.Open(fname + ".ats", adios2::Mode::Read);

        if (!rank)
        {
            std::cout << "Reading as stream with BeginStep/EndStep:" << std::endl;
        }

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

            auto varfix = inIO.InquireVariable<double>("vfixdim");
            EXPECT_TRUE(varfix);

            auto shape = var.Shape();
            auto shapefix = varfix.Shape();
            auto mm = var.MinMax();

            if (!rank)
            {

                std::cout << "Step " << step << " shape (" << shape[0] << ", " << shape[1] << ")"
                          << "  min/max = " << mm.first << " / " << mm.second << std::endl;
            }

            EXPECT_EQ(shape[0], nproc);
            EXPECT_EQ(shape[1], expected_shape);

            std::vector<double> bufIn;
            var.SetSelection({{off0, 0}, {1, shape[1]}});
            reader.Get(var, bufIn, adios2::Mode::Sync);
            for (size_t i = 0; i < shape[1]; ++i)
            {
                EXPECT_EQ(bufOut[i], bufIn[i]);
            }

            varfix.SetSelection({{off0, 0}, {1, shapefix[1]}});
            reader.Get(varfix, bufIn, adios2::Mode::Sync);
            for (size_t i = 0; i < shapefix[1]; ++i)
            {
                EXPECT_EQ(bufOut[i], bufIn[i]);
            }
            reader.EndStep();
        }

        reader.Close();
    }

    // Reader with file reading
    {
        adios2::IO inIO = adios.DeclareIO("InputFile");
        adios2::Engine reader = inIO.Open(fname + ".ats", adios2::Mode::ReadRandomAccess);

        if (!rank)
        {
            std::cout << "Reading as file with SetStepSelection:" << std::endl;
        }

        auto var = inIO.InquireVariable<double>("v");
        EXPECT_TRUE(var);

        auto varfix = inIO.InquireVariable<double>("vfixdim");
        EXPECT_TRUE(varfix);

        for (size_t i = 0; i < nsteps; i++)
        {
            var.SetStepSelection({i, 1});
            varfix.SetStepSelection({i, 1});
            auto shape = var.Shape();
            auto shapefix = varfix.Shape();
            auto mm = var.MinMax(i);
            if (!rank)
            {

                std::cout << "Step " << i << " shape (" << shape[0] << ", " << shape[1] << ")"
                          << "  min/max = " << mm.first << " / " << mm.second << std::endl;
            }
            size_t expected_shape = i + 1;
            EXPECT_EQ(shape[0], nproc);
            EXPECT_EQ(shape[1], expected_shape);

            std::vector<double> bufIn;
            var.SetSelection({{off0, 0}, {1, shape[1]}});
            reader.Get(var, bufIn, adios2::Mode::Sync);
            for (size_t i = 0; i < shape[1]; ++i)
            {
                EXPECT_EQ(bufOut[i], bufIn[i]);
            }

            varfix.SetSelection({{off0, 0}, {1, shapefix[1]}});
            reader.Get(varfix, bufIn, adios2::Mode::Sync);
            for (size_t i = 0; i < shapefix[1]; ++i)
            {
                EXPECT_EQ(bufOut[i], bufIn[i]);
            }
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
