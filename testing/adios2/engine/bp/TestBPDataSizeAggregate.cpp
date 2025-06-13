/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#include <adios2.h>
#include <gtest/gtest.h>
#include <mpi.h>
#include <numeric>
#include <thread>

using namespace adios2;

int worldRank, worldSize;

namespace
{
uint64_t sum0ToN(uint64_t n)
{
    uint64_t sum = 0;
    for (int i = 1; i <= n; ++i)
    {
        sum += i;
    }
    return sum;
}
}

class DSATest : public ::testing::Test
{
public:
    DSATest() = default;
};

TEST_F(DSATest, TestWriteUnbalancedData)
{
    adios2::ADIOS adios(MPI_COMM_WORLD);

    uint64_t globalNx = worldSize;
    uint64_t globalNy = sum0ToN(globalNx);
    uint64_t globalNumElements = globalNx * globalNy;

    // Define local data, size varies by rank
    uint64_t localNy = worldRank + 1;
    std::vector<double> rankData(globalNx, localNy);
    uint64_t localElementCount = rankData.size();

    adios2::IO bpIO = adios.DeclareIO("WriteIO");
    bpIO.SetEngine("BPFile");
    bpIO.SetParameter("AggregationType", "DataSizeBased");

    adios2::Variable<double> varGlobalArray =
        bpIO.DefineVariable<double>("GlobalArray", {globalNx, globalNy});

    adios2::Engine bpWriter = bpIO.Open("unbalanced_output.bp", adios2::Mode::Write);

    bpWriter.BeginStep();

    for (uint64_t i = 0; i < localElementCount; ++i)
    {
        rankData[i] = globalNx * worldSize * 1.0 + worldRank * localNy * 1.0 + static_cast<double>(i);;
    }

    varGlobalArray.SetSelection(adios2::Box<adios2::Dims>({0, static_cast<size_t>(sum0ToN(worldRank))},
                                                          {globalNx, static_cast<size_t>(localNy)}));
    bpWriter.Put<double>(varGlobalArray, rankData.data());
    bpWriter.EndStep();

    bpWriter.Close();

    if (worldRank == 0)
    {
        std::cout << "Finished writing unbalanced data" << std::endl;
    }
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();

    MPI_Finalize();
    return result;
}
