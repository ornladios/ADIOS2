/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include <adios2.h>
#include <gtest/gtest.h>

// Install this file in testing/adios2/engine/bp/operations/ in ADIOS2.
#include "../../TestHelpers.h"

namespace
{
std::string engineName = "BP5";
constexpr double accuracy = 0.001;

size_t Elements(const adios2::Dims &count)
{
    size_t result = 1;
    for (const auto extent : count)
    {
        result *= extent;
    }
    return result;
}

template <class T>
std::vector<T> MakeData(const adios2::Dims &count, const int rank, const size_t step)
{
    std::vector<T> values(Elements(count));
    const size_t width = count.back();
    const size_t height = count[count.size() - 2];
    const size_t time = count[count.size() - 3];
    const size_t sliceSize = count.size() == 5 ? values.size() / count[0] : values.size();
    for (size_t i = 0; i < values.size(); ++i)
    {
        const size_t x = i % width;
        const size_t y = (i / width) % height;
        const size_t t = (i / (width * height)) % time;
        const size_t variable = i / sliceSize;
        // Rank, step, and variable offsets expose swapped blocks, stale steps,
        // and mistakes in the operator's 5D variable slicing.
        values[i] = static_cast<T>(2.0 + 3.0 * rank + 0.75 * step + 5.0 * variable +
                                   0.6 * std::sin(0.025 * x) + 0.3 * std::cos(0.031 * y) +
                                   0.2 * std::sin(0.3 * t));
    }
    return values;
}

template <class T>
void CheckData(const std::vector<T> &expected, const std::vector<T> &restored,
               const adios2::Dims &count, const bool bypass, const int readRank, const size_t step)
{
    ASSERT_EQ(restored.size(), expected.size());
    const size_t slices = count.size() == 5 ? count[0] : 1;
    const size_t sliceSize = expected.size() / slices;
    for (size_t slice = 0; slice < slices; ++slice)
    {
        SCOPED_TRACE(::testing::Message()
                     << "read rank=" << readRank << " step=" << step << " variable=" << slice);
        double minimum = std::numeric_limits<double>::infinity();
        double maximum = -minimum;
        double squaredError = 0;
        bool finite = true;
        bool exact = true;
        for (size_t i = slice * sliceSize; i < (slice + 1) * sliceSize; ++i)
        {
            const double reference = static_cast<double>(expected[i]);
            const double actual = static_cast<double>(restored[i]);
            minimum = std::min(minimum, reference);
            maximum = std::max(maximum, reference);
            finite = finite && std::isfinite(actual);
            exact = exact && expected[i] == restored[i];
            const double difference = actual - reference;
            squaredError += difference * difference;
        }
        EXPECT_TRUE(finite) << "Reconstruction contains NaN or infinity";
        if (bypass)
        {
            EXPECT_TRUE(exact) << "Threshold bypass must preserve the original values exactly";
        }
        else
        {
            const double range = maximum - minimum;
            ASSERT_GT(range, 0.0);
            const double nrmse = std::sqrt(squaredError / sliceSize) / range;
            // CAESAR internally converts double input to float32. Allow only
            // a small float32 rounding margin around the requested NRMSE.
            EXPECT_LE(nrmse, accuracy + 8 * std::numeric_limits<float>::epsilon());
        }
    }
}

template <class T>
void WriteRead(const std::string &name, const adios2::Dims &count, const size_t steps,
               const bool bypass = false)
{
    int rank = 0;
    int size = 1;
#if ADIOS2_USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    adios2::ADIOS adios(MPI_COMM_WORLD);
    const std::string filename = "BPWRCAESAR_" + name + "_MPI.bp";
#else
    adios2::ADIOS adios;
    const std::string filename = "BPWRCAESAR_" + name + "_Serial.bp";
#endif

    auto shape = count;
    shape[0] *= static_cast<size_t>(size);
    adios2::Dims start(count.size(), 0);
    start[0] = static_cast<size_t>(rank) * count[0];

    {
        auto io = adios.DeclareIO("Write_" + name);
        io.SetEngine(engineName);
        auto variable = io.DefineVariable<T>("field", shape, start, count, adios2::ConstantDims);
        auto operation = adios.DefineOperator("caesar_" + name, "caesar");
        // Zero forces a compression attempt for the non-bypass cases. The
        // operator may still store raw data if its compressed payload will not fit.
        const adios2::Params parameters{
            {"accuracy", "0.001"}, {"method", "gae"}, {"threshold", bypass ? "1000000" : "0"}};
        variable.AddOperation(operation, parameters);
        auto writer = io.Open(filename, adios2::Mode::Write);
        for (size_t step = 0; step < steps; ++step)
        {
            auto values = MakeData<T>(count, rank, step);
            writer.BeginStep();
            writer.Put(variable, values.data(), adios2::Mode::Sync);
            writer.EndStep();
        }
        writer.Close();
    }

#if ADIOS2_USE_MPI
    MPI_Barrier(MPI_COMM_WORLD);
#endif
    {
        auto io = adios.DeclareIO("Read_" + name);
        io.SetEngine(engineName);
        auto reader = io.Open(filename, adios2::Mode::Read);
        const int readRank = (rank + 1) % size;
        auto readStart = start;
        readStart[0] = static_cast<size_t>(readRank) * count[0];
        size_t step = 0;
        while (reader.BeginStep() == adios2::StepStatus::OK)
        {
            auto variable = io.InquireVariable<T>("field");
            if (!variable)
            {
                throw std::runtime_error("CAESAR field is missing or has the wrong type");
            }
            EXPECT_EQ(variable.ShapeID(), adios2::ShapeID::GlobalArray);
            EXPECT_EQ(variable.Shape(), shape);
            variable.SetSelection({readStart, count});
            std::vector<T> restored;
            reader.Get(variable, restored, adios2::Mode::Sync);
            reader.EndStep();
            CheckData(MakeData<T>(count, readRank, step), restored, count, bypass, readRank, step);
            ++step;
        }
        reader.Close();
        EXPECT_EQ(step, steps);
    }

#if ADIOS2_USE_MPI
    CleanupTestFilesMPI(filename, MPI_COMM_WORLD);
#else
    CleanupTestFiles(filename);
#endif
}
} // namespace

TEST(BPWriteReadCAESAR, Float3DTwoSteps) { WriteRead<float>("Float3DTwoSteps", {8, 256, 256}, 2); }

TEST(BPWriteReadCAESAR, Float4D) { WriteRead<float>("Float4D", {1, 8, 256, 256}, 1); }

TEST(BPWriteReadCAESAR, Double5DTwoVariables)
{
    WriteRead<double>("Double5DTwoVariables", {2, 1, 8, 256, 256}, 1);
}

TEST(BPWriteReadCAESAR, SmallDataThresholdBypass)
{
    WriteRead<double>("SmallDataThresholdBypass", {2, 4, 8}, 2, true);
}

int main(int argc, char **argv)
{
#if ADIOS2_USE_MPI
    int provided = MPI_THREAD_SINGLE;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided < MPI_THREAD_MULTIPLE)
    {
        std::cerr << "The BP5 MPI test requires MPI_THREAD_MULTIPLE\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
#endif
    int result = 1;
    try
    {
        ::testing::InitGoogleTest(&argc, argv);
        // Let the outer handler abort all MPI ranks on an I/O exception,
        // instead of leaving peers waiting in collective engine operations.
        GTEST_FLAG_SET(catch_exceptions, false);
        if (argc > 1)
        {
            engineName = argv[1];
        }
        result = RUN_ALL_TESTS();
    }
    catch (const std::exception &error)
    {
        std::cerr << "CAESAR BP write/read test failed: " << error.what() << '\n';
#if ADIOS2_USE_MPI
        MPI_Abort(MPI_COMM_WORLD, 1);
#endif
        return 1;
    }
#if ADIOS2_USE_MPI
    int globalResult = 1;
    MPI_Allreduce(&result, &globalResult, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    MPI_Finalize();
    result = globalResult;
#endif
    return result;
}
