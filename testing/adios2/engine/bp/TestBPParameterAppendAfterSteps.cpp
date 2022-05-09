/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Test "AppendAfterSteps" parameter for appending new steps to an existing BP
 * file
 *
 *  Created on: May 9, 2022
 *      Author: Norbert Podhorszki
 */

#include <cstdint>
#include <cstring>

#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#include "../SmallTestData.h"

std::string engineName; // comes from command line
const std::size_t Nx = 10;
using DataArray = std::array<int32_t, Nx>;

class BPParameterAppendAfterSteps : public ::testing::Test
{
public:
    BPParameterAppendAfterSteps() = default;

    DataArray GenerateData(int step, int rank, int size)
    {
        DataArray d;
        int j = rank + 1 + step * size;
        for (size_t i = 0; i < d.size(); ++i)
        {
            d[i] = j;
        }
        return d;
    }

    std::string ArrayToString(int32_t *data, size_t nelems)
    {
        std::stringstream ss;
        ss << "[";
        for (size_t i = 0; i < nelems; ++i)
        {
            ss << data[i];
            if (i < nelems - 1)
            {
                ss << " ";
            }
        }
        ss << "]";
        return ss.str();
    }
};

class BPParameterAppendAfterStepsP
: public BPParameterAppendAfterSteps,
  public ::testing::WithParamInterface<std::tuple<int, int, int>>
{
protected:
    int GetNsteps1() { return std::get<0>(GetParam()); };
    int GetAppendAfterSteps() { return std::get<1>(GetParam()); };
    int GetNsteps2() { return std::get<2>(GetParam()); };
};

TEST_P(BPParameterAppendAfterStepsP, Read)
{
    int mpiRank = 0, mpiSize = 1;
    int nSteps1 = GetNsteps1();
    int nAppendAfterSteps = GetAppendAfterSteps();
    int nSteps2 = GetNsteps2();
#if ADIOS2_USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

#if ADIOS2_USE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD);
#else
    adios2::ADIOS adios;
#endif

    std::string filename =
        "ParameterAppendAfterSteps" + std::to_string(mpiSize) + "_" +
        std::to_string(nSteps1) + "_" + std::to_string(nAppendAfterSteps) +
        "_" + std::to_string(nSteps2) + ".bp";

    const std::size_t Nx = 10;
    adios2::Dims shape{static_cast<unsigned int>(mpiSize * Nx)};
    adios2::Dims start{static_cast<unsigned int>(mpiRank * Nx)};
    adios2::Dims count{static_cast<unsigned int>(Nx)};

    adios2::IO ioWrite = adios.DeclareIO("TestIOWrite");
    ioWrite.SetEngine(engineName);
    auto var0 = ioWrite.DefineVariable<int32_t>("var", shape, start, count);

    /*
     * Create file with nSteps1
     */

    adios2::Engine engine = ioWrite.Open(filename, adios2::Mode::Write);
    for (int step = 0; step < nSteps1; ++step)
    {
        auto d = GenerateData(step, mpiRank, mpiSize);
        engine.BeginStep();
        engine.Put(var0, d.data());
        engine.EndStep();
    }
    engine.Close();

    /*
     * Append nSteps2 after nAppendAfterSteps
     */

    int nStartStep = nAppendAfterSteps;
    if (nAppendAfterSteps < 0)
    {
        nStartStep = nSteps1 + nAppendAfterSteps + 1;
        if (nStartStep < 0)
        {
            nStartStep = 0;
        }
    }
    int nTotalSteps = nStartStep + nSteps2;

    ioWrite.SetParameter("AppendAfterSteps", std::to_string(nAppendAfterSteps));
    engine = ioWrite.Open(filename, adios2::Mode::Append);
    for (int step = nStartStep; step < nStartStep + nSteps2; ++step)
    {
        auto d = GenerateData(step, mpiRank, mpiSize);
        engine.BeginStep();
        engine.Put(var0, d.data());
        engine.EndStep();
    }
    engine.Close();

    /*
     * Read back every step
     */

    adios2::IO ioRead = adios.DeclareIO("TestIORead");
    ioRead.SetEngine(engineName);
    adios2::Engine engine_s =
        ioRead.Open(filename, adios2::Mode::ReadRandomAccess);
    EXPECT_TRUE(engine_s);

    const size_t nsteps = engine_s.Steps();
    EXPECT_EQ(nsteps, nTotalSteps);

    adios2::Variable<int> var = ioRead.InquireVariable<int32_t>("var");

    for (size_t step = 0; step < nsteps; step++)
    {
        var.SetStepSelection(adios2::Box<size_t>(step, 1));
        std::vector<int> res;
        var.SetSelection({{Nx * mpiRank}, {Nx}});
        engine_s.Get<int>(var, res, adios2::Mode::Sync);
        int s = static_cast<int>(step);
        auto d = GenerateData(s, mpiRank, mpiSize);
        EXPECT_EQ(res[0], d[0]);
    }

    engine_s.Close();
#if ADIOS2_USE_MPI
    MPI_Barrier(MPI_COMM_WORLD);
#endif
}

INSTANTIATE_TEST_SUITE_P(
    BPParameterAppendAfterSteps, BPParameterAppendAfterStepsP,
    ::testing::Values(std::make_tuple(2, 1, 2), std::make_tuple(3, 0, 2),
                      std::make_tuple(2, 2, 2), std::make_tuple(2, -1, 2),
                      std::make_tuple(2, -2, 2), std::make_tuple(2, -99, 2)));

int main(int argc, char **argv)
{
#if ADIOS2_USE_MPI
    MPI_Init(nullptr, nullptr);
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
