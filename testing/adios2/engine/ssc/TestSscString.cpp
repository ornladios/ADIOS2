/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#include "TestSscCommon.h"
#include <adios2.h>
#include <gtest/gtest.h>
#include <mpi.h>
#include <numeric>
#include <thread>

using namespace adios2;
int mpiRank = 0;
int mpiSize = 1;
MPI_Comm mpiComm;

class SscEngineTest : public ::testing::Test
{
public:
    SscEngineTest() = default;
};

void Writer(const Dims &shape, const Dims &start, const Dims &count,
            const size_t steps, const adios2::Params &engineParams,
            const std::string &name)
{
    size_t datasize =
        std::accumulate(count.begin(), count.end(), static_cast<size_t>(1),
                        std::multiplies<size_t>());
    adios2::ADIOS adios(mpiComm);
    adios2::IO dataManIO = adios.DeclareIO("WAN");
    dataManIO.SetEngine("ssc");
    dataManIO.SetParameters(engineParams);
    std::vector<char> myChars(datasize);
    std::vector<unsigned char> myUChars(datasize);
    std::vector<short> myShorts(datasize);
    std::vector<unsigned short> myUShorts(datasize);
    std::vector<int> myInts(datasize);
    std::vector<unsigned int> myUInts(datasize);
    std::vector<float> myFloats(datasize);
    std::vector<double> myDoubles(datasize);
    std::vector<std::complex<float>> myComplexes(datasize);
    std::vector<std::complex<double>> myDComplexes(datasize);
    auto bpChars =
        dataManIO.DefineVariable<char>("bpChars", shape, start, count);
    auto bpUChars = dataManIO.DefineVariable<unsigned char>("bpUChars", shape,
                                                            start, count);
    auto bpShorts =
        dataManIO.DefineVariable<short>("bpShorts", shape, start, count);
    auto bpUShorts = dataManIO.DefineVariable<unsigned short>(
        "bpUShorts", shape, start, count);
    auto bpInts = dataManIO.DefineVariable<int>("bpInts", shape, start, count);
    auto bpUInts =
        dataManIO.DefineVariable<unsigned int>("bpUInts", shape, start, count);
    auto bpFloats =
        dataManIO.DefineVariable<float>("bpFloats", shape, start, count);
    auto bpDoubles =
        dataManIO.DefineVariable<double>("bpDoubles", shape, start, count);
    auto bpComplexes = dataManIO.DefineVariable<std::complex<float>>(
        "bpComplexes", shape, start, count);
    auto bpDComplexes = dataManIO.DefineVariable<std::complex<double>>(
        "bpDComplexes", shape, start, count);
    auto scalarInt = dataManIO.DefineVariable<int>("scalarInt");
    auto stringVar = dataManIO.DefineVariable<std::string>("stringVar");
    dataManIO.DefineAttribute<int>("AttInt", 110);
    adios2::Engine engine = dataManIO.Open(name, adios2::Mode::Write);
    for (int i = 0; i < steps; ++i)
    {
        engine.BeginStep();
        GenData(myChars, i, start, count, shape);
        GenData(myUChars, i, start, count, shape);
        GenData(myShorts, i, start, count, shape);
        GenData(myUShorts, i, start, count, shape);
        GenData(myInts, i, start, count, shape);
        GenData(myUInts, i, start, count, shape);
        GenData(myFloats, i, start, count, shape);
        GenData(myDoubles, i, start, count, shape);
        GenData(myComplexes, i, start, count, shape);
        GenData(myDComplexes, i, start, count, shape);
        engine.Put(bpChars, myChars.data(), adios2::Mode::Sync);
        engine.Put(bpUChars, myUChars.data(), adios2::Mode::Sync);
        engine.Put(bpShorts, myShorts.data(), adios2::Mode::Sync);
        engine.Put(bpUShorts, myUShorts.data(), adios2::Mode::Sync);
        engine.Put(bpInts, myInts.data(), adios2::Mode::Sync);
        engine.Put(bpUInts, myUInts.data(), adios2::Mode::Sync);
        engine.Put(bpFloats, myFloats.data(), adios2::Mode::Sync);
        engine.Put(bpDoubles, myDoubles.data(), adios2::Mode::Sync);
        engine.Put(bpComplexes, myComplexes.data(), adios2::Mode::Sync);
        engine.Put(bpDComplexes, myDComplexes.data(), adios2::Mode::Sync);
        engine.Put(scalarInt, i);
        std::string s = "sample string sample string sample string";
        engine.Put(stringVar, s);
        engine.EndStep();
    }
    engine.Close();
}

void Reader(const Dims &shape, const Dims &start, const Dims &count,
            const size_t steps, const adios2::Params &engineParams,
            const std::string &name)
{
    adios2::ADIOS adios(mpiComm);
    adios2::IO dataManIO = adios.DeclareIO("Test");
    dataManIO.SetEngine("ssc");
    dataManIO.SetParameters(engineParams);
    adios2::Engine engine = dataManIO.Open(name, adios2::Mode::Read);

    size_t datasize =
        std::accumulate(count.begin(), count.end(), static_cast<size_t>(1),
                        std::multiplies<size_t>());
    std::vector<char> myChars(datasize);

    while (true)
    {
        adios2::StepStatus status = engine.BeginStep(StepMode::Read, 5);
        if (status == adios2::StepStatus::OK)
        {
            auto scalarInt = dataManIO.InquireVariable<int>("scalarInt");
            auto blocksInfo =
                engine.BlocksInfo(scalarInt, engine.CurrentStep());

            for (const auto &bi : blocksInfo)
            {
                ASSERT_EQ(bi.IsValue, true);
                ASSERT_EQ(bi.Value, engine.CurrentStep());
                ASSERT_EQ(scalarInt.Min(), engine.CurrentStep());
                ASSERT_EQ(scalarInt.Max(), engine.CurrentStep());
            }

            const auto &vars = dataManIO.AvailableVariables();
            ASSERT_EQ(vars.size(), 12);
            size_t currentStep = engine.CurrentStep();
            adios2::Variable<char> bpChars =
                dataManIO.InquireVariable<char>("bpChars");
            adios2::Variable<std::string> stringVar =
                dataManIO.InquireVariable<std::string>("stringVar");

            std::string s;
            engine.Get(stringVar, s, adios2::Mode::Sync);
            ASSERT_EQ(s, "sample string sample string sample string");
            ASSERT_EQ(stringVar.Min(),
                      "sample string sample string sample string");
            ASSERT_EQ(stringVar.Max(),
                      "sample string sample string sample string");
            bpChars.SetSelection({start, count});

            engine.Get(bpChars, myChars.data(), adios2::Mode::Sync);

            int i;
            engine.Get(scalarInt, &i);
            engine.PerformGets();
            ASSERT_EQ(i, currentStep);

            VerifyData(myChars.data(), currentStep, start, count, shape,
                       mpiRank);
            engine.EndStep();
        }
        else if (status == adios2::StepStatus::EndOfStream)
        {
            std::cout << "[Rank " + std::to_string(mpiRank) +
                             "] SscTest reader end of stream!"
                      << std::endl;
            break;
        }
    }
    auto attInt = dataManIO.InquireAttribute<int>("AttInt");
    std::cout << "[Rank " + std::to_string(mpiRank) + "] Attribute received "
              << attInt.Data()[0] << ", expected 110" << std::endl;
    ASSERT_EQ(110, attInt.Data()[0]);
    ASSERT_NE(111, attInt.Data()[0]);
    engine.Close();
}

TEST_F(SscEngineTest, TestSscString)
{
    std::string filename = "TestSscString";
    adios2::Params engineParams = {};

    int worldRank, worldSize;
    MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    int mpiGroup = worldRank / (worldSize / 2);
    MPI_Comm_split(MPI_COMM_WORLD, mpiGroup, worldRank, &mpiComm);

    MPI_Comm_rank(mpiComm, &mpiRank);
    MPI_Comm_size(mpiComm, &mpiSize);

    Dims shape = {10, (size_t)mpiSize * 2};
    Dims start = {2, (size_t)mpiRank * 2};
    Dims count = {5, 2};
    size_t steps = 100;

    if (mpiGroup == 0)
    {
        Writer(shape, start, count, steps, engineParams, filename);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    if (mpiGroup == 1)
    {
        Reader(shape, start, count, steps, engineParams, filename);
    }

    MPI_Barrier(MPI_COMM_WORLD);
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    int worldRank, worldSize;
    MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();

    MPI_Finalize();
    return result;
}
