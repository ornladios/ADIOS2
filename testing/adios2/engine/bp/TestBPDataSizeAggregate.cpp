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
uint64_t sumFirstN(const std::vector<uint64_t> &vec, uint64_t n)
{
    uint64_t sum = 0;
    for (int i = 0; i < n; ++i)
    {
        sum += vec[i];
    }
    return sum;
}

void swap(std::vector<uint64_t> &vec, uint64_t idx1, uint64_t idx2)
{
    uint64_t tmp = vec[idx1];
    vec[idx1] = vec[idx2];
    vec[idx2] = tmp;
}

std::string aggregationType;  // comes from command line
std::string numberOfSubFiles; // comes from command line
}

class DSATest : public ::testing::Test
{
public:
    DSATest() = default;
};

TEST_F(DSATest, TestWriteUnbalancedData)
{
    adios2::ADIOS adios(MPI_COMM_WORLD);

    std::vector<uint64_t> rankDataSizes(worldSize);
    std::iota(rankDataSizes.begin(), rankDataSizes.end(), 1);
    // swap(rankDataSizes, 1, 2);

    std::cout << "Rank data sizes (in number of columns)" << std::endl;
    for (int i = 0; i < rankDataSizes.size(); ++i)
    {
        std::cout << rankDataSizes[i] << " ";
    }
    std::cout << std::endl;

    uint64_t globalNx = worldSize;
    uint64_t globalNy = sumFirstN(rankDataSizes, rankDataSizes.size());

    {
        // Define local data, size varies by rank
        uint64_t localNy = rankDataSizes[worldRank];
        uint64_t localOffset = sumFirstN(rankDataSizes, worldRank);
        std::cout << "Rank " << worldRank << " local offset: " << localOffset << std::endl;
        std::vector<double> rankData;
        rankData.resize(globalNx * localNy);
        uint64_t localElementCount = rankData.size();

        adios2::IO bpIO = adios.DeclareIO("WriteIO");
        bpIO.SetEngine("BPFile");

        std::cout << "Setting AggregationType to " << aggregationType << std::endl;
        bpIO.SetParameter("AggregationType", aggregationType);
        std::cout << "Setting NumSubFiles to " << numberOfSubFiles << std::endl;
        bpIO.SetParameter("NumSubFiles", numberOfSubFiles);

        adios2::Variable<double> varGlobalArray =
            bpIO.DefineVariable<double>("GlobalArray", {globalNx, globalNy});

        adios2::Engine bpWriter = bpIO.Open("unbalanced_output.bp", adios2::Mode::Write);

        bpWriter.BeginStep();

        uint64_t index = 0;
        for (uint64_t x = 0; x < globalNx; ++x)
        {
            uint64_t value = (x * globalNy) + localOffset;
            for (uint64_t y = 0; y < localNy; ++y)
            {
                rankData[index++] = value++;
            }
        }

        varGlobalArray.SetSelection(adios2::Box<adios2::Dims>(
            {0, static_cast<size_t>(localOffset)}, {globalNx, static_cast<size_t>(localNy)}));
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

        for (size_t i = 0; i < array.size(); ++i)
        {
            ASSERT_EQ(array[i], static_cast<double>(i));
        }

        bpReader.Close();
    }
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    ::testing::InitGoogleTest(&argc, argv);

    if (argc > 1)
    {
        aggregationType = std::string(argv[1]);
    }
    if (argc > 2)
    {
        numberOfSubFiles = std::string(argv[2]);
    }

    int result = RUN_ALL_TESTS();

    MPI_Finalize();
    return result;
}
