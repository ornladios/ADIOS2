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
    for (size_t i = 1; i <= n; ++i)
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

    {
        // Define local data, size varies by rank
        uint64_t localNy = worldRank + 1;
        std::vector<double> rankData;
        rankData.resize(globalNx * localNy);
        uint64_t localElementCount = rankData.size();

        adios2::IO bpIO = adios.DeclareIO("WriteIO");
        bpIO.SetEngine("BPFile");
        bpIO.SetParameter("AggregationType", "DataSizeBased");
        bpIO.SetParameter("NumAggregators", "4");

        adios2::Variable<double> varGlobalArray =
            bpIO.DefineVariable<double>("GlobalArray", {globalNx, globalNy});

        adios2::Engine bpWriter = bpIO.Open("unbalanced_output.bp", adios2::Mode::Write);

        bpWriter.BeginStep();

        for (uint64_t i = 0; i < localElementCount; ++i)
        {
            rankData[i] =
                globalNx * worldSize * 1.0 + worldRank * localNy * 1.0 + static_cast<double>(i);
            ;
        }

        varGlobalArray.SetSelection(
            adios2::Box<adios2::Dims>({0, static_cast<size_t>(sum0ToN(worldRank))},
                                      {globalNx, static_cast<size_t>(localNy)}));
        bpWriter.Put<double>(varGlobalArray, rankData.data());
        bpWriter.EndStep();

        bpWriter.Close();
    }

    if (worldRank == 0)
    {
        std::cout << "Finished writing unbalanced data, reading it back" << std::endl;
    }

    {
        adios2::IO io = adios.DeclareIO("ReadIO");

        io.SetEngine("BPFile");
        adios2::Engine bpReader = io.Open("unbalanced_output.bp", adios2::Mode::ReadRandomAccess);

        auto var_array = io.InquireVariable<double>("GlobalArray");
        EXPECT_TRUE(var_array);
        ASSERT_EQ(var_array.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_array.Steps(), 1);
        ASSERT_EQ(var_array.Shape().size(), 2);

        std::vector<double> array;

        bpReader.Get(var_array, array, adios2::Mode::Sync);
        ASSERT_EQ(array.size(), var_array.Shape()[0] * var_array.Shape()[1]);

        bpReader.Close();
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
