/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TestBPWriteAggregateReadLocal.cpp
 *
 *  Created on: Jan 11, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include <adios2_c.h>

#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#endif

#include <gtest/gtest.h>

#include <numeric>

#include "SmallTestData_c.h"

void LocalAggregate1D(const std::string substreams)
{
    // Each process would write a 1x8 array and all processes would
    // form a mpiSize * Nx 1D array
    const std::string fname("LocalAggregate1D_" + substreams + ".bp");

    int mpiRank = 0, mpiSize = 1;
    // Number of steps
    const size_t NSteps = 2;

    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    // Write test data using BP
    // data
    std::vector<int> inumbers(5);
    std::iota(inumbers.begin(), inumbers.end(), mpiRank);

    std::vector<float> fnumbers(5);
    const float randomStart = rand() % mpiSize;
    std::iota(fnumbers.begin(), fnumbers.end(), randomStart);

    // adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
    adios2_adios *adiosH = adios2_init(MPI_COMM_WORLD, adios2_debug_mode_on);
    {
        adios2_io *ioH = adios2_declare_io(adiosH, "TestIO");

        size_t countiNumbers[1];
        countiNumbers[0] = inumbers.size();
        adios2_variable *variNumbers =
            adios2_define_variable(ioH, "ints", adios2_type_int, 1, NULL, NULL,
                                   countiNumbers, adios2_constant_dims_true);

        size_t countfNumbers[1];
        countfNumbers[0] = fnumbers.size();
        adios2_variable *varfNumbers = adios2_define_variable(
            ioH, "floats", adios2_type_float, 1, NULL, NULL, countfNumbers,
            adios2_constant_dims_true);

        // adios2_set_parameter(ioH, "CollectiveMetadata", "Off");
        adios2_set_parameter(ioH, "Profile", "Off");

        if (mpiSize > 1)
        {
            adios2_set_parameter(ioH, "substreams", substreams.c_str());
        }

        adios2_engine *bpWriter =
            adios2_open(ioH, fname.c_str(), adios2_mode_write);

        adios2_step_status step_status;
        for (size_t i = 0; i < NSteps; ++i)
        {
            adios2_begin_step(bpWriter, adios2_step_mode_next_available, 0,
                              &step_status);
            if (mpiRank % 3 == 1)
            {
                adios2_put(bpWriter, variNumbers, inumbers.data(),
                           adios2_mode_sync);
            }

            std::this_thread::sleep_for(std::chrono::seconds(rand() % 2));

            if (mpiRank % 3 == 1)
            {
                //                adios2_put(bpWriter, varfNumbers,
                //                fnumbers.data(),
                //                           adios2_mode_sync);
            }
            adios2_end_step(bpWriter);
        }

        adios2_close(bpWriter);
        bpWriter = NULL;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    // Reader TODO, might need to generate metadata file
    if (false)
    {
        adios2_io *ioH = adios2_declare_io(adiosH, "Reader");
        adios2_engine *bpReader =
            adios2_open(ioH, fname.c_str(), adios2_mode_read);

        adios2_step_status step_status;
        while (true)
        {
            adios2_begin_step(bpReader, adios2_step_mode_next_available, 0,
                              &step_status);

            if (step_status == adios2_step_status_end_of_stream)
            {
                break;
            }

            adios2_variable *varInts = adios2_inquire_variable(ioH, "ints");
            adios2_variable *varFloats = adios2_inquire_variable(ioH, "floats");

            EXPECT_NE(varInts, nullptr);
            EXPECT_NE(varFloats, nullptr);

            adios2_end_step(bpReader);
        }

        adios2_close(bpReader);
        bpReader = NULL;
    }

    adios2_finalize(adiosH);
}

class BPWriteAggregateReadLocalTest
    : public ::testing::TestWithParam<std::string>
{
public:
    BPWriteAggregateReadLocalTest() = default;

    SmallTestData m_TestData;

    virtual void SetUp() {}
    virtual void TearDown() {}
};

TEST_P(BPWriteAggregateReadLocalTest, Aggregate1D)
{
    LocalAggregate1D(GetParam());
}

INSTANTIATE_TEST_CASE_P(Substreams, BPWriteAggregateReadLocalTest,
                        ::testing::Values("1", "2", "4"));

int main(int argc, char **argv)
{
#ifdef ADIOS2_HAVE_MPI
    MPI_Init(nullptr, nullptr);
#endif

    int result;
    ::testing::InitGoogleTest(&argc, argv);
    result = RUN_ALL_TESTS();

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return result;
}
