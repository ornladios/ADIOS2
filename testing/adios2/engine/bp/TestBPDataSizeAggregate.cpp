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

int worldRank, worldSize;
uint64_t nSteps = 1;
std::string aggregationType = "DataSizeBased"; // comes from command line
std::string numberOfSubFiles = "2";            // comes from command line
std::string numberOfSteps = "1";               // comes from command line
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
    swap(rankDataSizes, 4, 5);

    if (worldRank == 0)
    {
        std::cout << "Number of timesteps: " << nSteps << std::endl;
        std::cout << "Aggregation type: " << aggregationType << std::endl;
        std::cout << "Number of subfiles: " << numberOfSubFiles << std::endl;
        std::cout << "Rank data sizes (in number of columns)" << std::endl;
        for (int i = 0; i < rankDataSizes.size(); ++i)
        {
            std::cout << rankDataSizes[i] << " ";
        }
        std::cout << std::endl;
    }

    uint64_t globalNx = worldSize;
    uint64_t globalNy = sumFirstN(rankDataSizes, rankDataSizes.size());

    {
        // Define local data, size varies by rank
        uint64_t localNy = rankDataSizes[worldRank];
        uint64_t localOffset = sumFirstN(rankDataSizes, worldRank);
        std::vector<uint64_t> rankData;
        rankData.resize(globalNx * localNy);
        uint64_t localElementCount = rankData.size();

        adios2::IO bpIO = adios.DeclareIO("WriteIO");
        bpIO.SetEngine("BPFile");
        bpIO.SetParameter("AggregationType", aggregationType);
        bpIO.SetParameter("NumSubFiles", numberOfSubFiles);

        adios2::Variable<uint64_t> varGlobalArray =
            bpIO.DefineVariable<uint64_t>("GlobalArray", {globalNx, globalNy});

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
        bpWriter.Put<uint64_t>(varGlobalArray, rankData.data());
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

        auto var_array = io.InquireVariable<uint64_t>("GlobalArray");
        EXPECT_TRUE(var_array);
        ASSERT_EQ(var_array.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_array.Steps(), 1);
        ASSERT_EQ(var_array.Shape().size(), 2);

        std::vector<uint64_t> array;

        bpReader.Get(var_array, array, adios2::Mode::Sync);
        ASSERT_EQ(array.size(), var_array.Shape()[0] * var_array.Shape()[1]);

        for (size_t i = 0; i < array.size(); ++i)
        {
            ASSERT_EQ(array[i], static_cast<uint64_t>(i));
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
    if (argc > 3)
    {
        numberOfSteps = std::string(argv[3]);
    }

    try
    {
        nSteps = std::stoul(numberOfSteps);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Unable to convert " << numberOfSteps << " to a number due to: " << e.what()
                  << ". Defaulting to " << nSteps << std::endl;
    }

    int result = RUN_ALL_TESTS();

    MPI_Finalize();
    return result;
}
