/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TestDataManEngine.cpp
 *
 *  Created on: Jul 12, 2018
 *      Author: Jason Wang
 */

#include <future>
#include <numeric>
#include <thread>

#include <adios2.h>
#include <gtest/gtest.h>

#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#endif

using namespace adios2;

int mpiRank = 0;
int mpiSize = 1;

class DataManEngineTest : public ::testing::Test
{
public:
    DataManEngineTest() = default;
};

template <class T>
void GenData(std::vector<T> &data, const size_t step)
{
    for (size_t i = 0; i < data.size(); ++i)
    {
        data[i] = i + mpiRank * 10000 + step * 100;
    }
}

template <class T>
void PrintData(const std::vector<T> &data, size_t step)
{
    std::cout << "Rank: " << mpiRank << " Step: " << step << " [";
    for (size_t i = 0; i < data.size(); ++i)
    {
        std::cout << data[i] << " ";
    }
    std::cout << "]" << std::endl;
}

template <class T>
void VerifyData(const std::vector<T> &data, size_t step)
{
    std::vector<T> tmpdata(data.size());
    GenData(tmpdata, step);
    for (size_t i = 0; i < data.size(); ++i)
    {
        ASSERT_EQ(data[i], tmpdata[i]);
    }
    if (data.size() < 32)
    {
        PrintData(data, step);
    }
}

void DataManWriter(const Dims &shape, const Dims &start, const Dims &count,
                   const size_t steps, const std::string &workflowMode,
                   const std::vector<adios2::Params> &transParams)
{
    size_t datasize = std::accumulate(count.begin(), count.end(), 1,
                                      std::multiplies<size_t>());
    std::vector<float> myFloats(datasize);
#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(adios2::DebugON);
#endif
    adios2::IO dataManIO = adios.DeclareIO("WAN");
    dataManIO.SetEngine("DataMan");
    dataManIO.SetParameters({{"WorkflowMode", workflowMode}});
    for (const auto &params : transParams)
    {
        dataManIO.AddTransport("WAN", params);
    }
    auto bpFloats =
        dataManIO.DefineVariable<float>("bpFloats", shape, start, count);
    adios2::Engine dataManWriter =
        dataManIO.Open("stream", adios2::Mode::Write);
    for (int i = 0; i < steps; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        dataManWriter.BeginStep();
        GenData(myFloats, i);
        dataManWriter.Put<float>(bpFloats, myFloats.data(), adios2::Mode::Sync);
        dataManWriter.EndStep();
    }
    dataManWriter.Close();
}

void DataManReaderStrict(const Dims &shape, const Dims &start,
                         const Dims &count, const size_t steps,
                         const std::string &workflowMode,
                         const std::vector<adios2::Params> &transParams)
{
    size_t datasize = std::accumulate(count.begin(), count.end(), 1,
                                      std::multiplies<size_t>());
    std::vector<float> myFloats(datasize);
#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(adios2::DebugON);
#endif
    adios2::IO dataManIO = adios.DeclareIO("WAN");
    dataManIO.SetEngine("DataMan");
    dataManIO.SetParameters({{"WorkflowMode", workflowMode}});
    for (const auto &params : transParams)
    {
        dataManIO.AddTransport("WAN", params);
    }
    adios2::Engine dataManReader = dataManIO.Open("stream", adios2::Mode::Read);
    adios2::Variable<float> bpFloats;
    size_t i = 0;

    for (size_t i = 0; i < steps; ++i)
    {
        adios2::StepStatus status = dataManReader.BeginStep();
        if (status == adios2::StepStatus::OK)
        {
            bpFloats = dataManIO.InquireVariable<float>("bpFloats");
            bpFloats.SetSelection({start, count});
            dataManReader.Get<float>(bpFloats, myFloats.data(),
                                     adios2::Mode::Sync);
            i = dataManReader.CurrentStep();
            VerifyData(myFloats, i);
            dataManReader.EndStep();
        }
        else
        {
            i--;
            continue;
        }
    }
    dataManReader.Close();
}

void DataManReaderLoose(const Dims &shape, const Dims &start, const Dims &count,
                        const size_t steps, const std::string &workflowMode,
                        const std::vector<adios2::Params> &transParams)
{
    size_t datasize = std::accumulate(count.begin(), count.end(), 1,
                                      std::multiplies<size_t>());
    std::vector<float> myFloats(datasize);
#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(adios2::DebugON);
#endif
    adios2::IO dataManIO = adios.DeclareIO("WAN");
    dataManIO.SetEngine("DataMan");
    dataManIO.SetParameters({{"WorkflowMode", workflowMode}});
    for (const auto &params : transParams)
    {
        dataManIO.AddTransport("WAN", params);
    }
    adios2::Engine dataManReader = dataManIO.Open("stream", adios2::Mode::Read);
    adios2::Variable<float> bpFloats;
    size_t i = 0;
    auto start_time = std::chrono::system_clock::now();
    while (i < steps - 1)
    {
        auto now_time = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
            now_time - start_time);
        if (duration.count() > 10)
        {
            std::cout << "DataMan Timeout" << std::endl;
            return;
        }
        adios2::StepStatus status = dataManReader.BeginStep();
        if (status == adios2::StepStatus::OK)
        {
            bpFloats = dataManIO.InquireVariable<float>("bpFloats");
            bpFloats.SetSelection({start, count});
            dataManReader.Get<float>(bpFloats, myFloats.data(),
                                     adios2::Mode::Sync);
            i = dataManReader.CurrentStep();
            VerifyData(myFloats, i);
            dataManReader.EndStep();
        }
        else if (status == adios2::StepStatus::EndOfStream)
        {
            break;
        }
    }
    dataManReader.Close();
}

#ifdef ADIOS2_HAVE_ZEROMQ
TEST_F(DataManEngineTest, WriteRead_1D_Subscribe)
{
    Dims shape = {10};
    Dims start = {0};
    Dims count = {10};
    size_t steps = 200;
    std::vector<adios2::Params> transportParams = {
        {{"Library", "ZMQ"}, {"IPAddress", "127.0.0.1"}, {"Port", "12307"}}};
    std::string workflowMode = "subscribe";
    auto r = std::thread(DataManReaderLoose, shape, start, count, steps,
                         workflowMode, transportParams);
    DataManWriter(shape, start, count, steps, workflowMode, transportParams);
    std::cout << "Writer ended" << std::endl;
    r.join();
    std::cout << "Reader ended" << std::endl;
}

TEST_F(DataManEngineTest, WriteRead_2D_Subscribe_Zfp)
{
    Dims shape = {5, 4};
    Dims start = {0, 0};
    Dims count = {5, 4};
    size_t steps = 200;
    std::vector<adios2::Params> transportParams = {
        {{"Library", "ZMQ"},
         {"IPAddress", "127.0.0.1"},
         {"Port", "12307"},
         {"CompressionMethod", "zfp"},
         {"zfp:rate", "4"}}};
    std::string workflowMode = "subscribe";
    auto r = std::thread(DataManReaderLoose, shape, start, count, steps,
                         workflowMode, transportParams);
    DataManWriter(shape, start, count, steps, workflowMode, transportParams);
    std::cout << "Writer ended" << std::endl;
    r.join();
    std::cout << "Reader ended" << std::endl;
}

TEST_F(DataManEngineTest, WriteRead_2D_Subscribe_SZ)
{
    Dims shape = {5, 4};
    Dims start = {0, 0};
    Dims count = {5, 4};
    size_t steps = 200;
    std::vector<adios2::Params> transportParams = {{
        {"Library", "ZMQ"},
        {"IPAddress", "127.0.0.1"},
        {"Port", "12307"},
        {"CompressionMethod", "sz"},
    }};
    std::string workflowMode = "subscribe";
    auto r = std::thread(DataManReaderLoose, shape, start, count, steps,
                         workflowMode, transportParams);
    DataManWriter(shape, start, count, steps, workflowMode, transportParams);
    std::cout << "Writer ended" << std::endl;
    r.join();
    std::cout << "Reader ended" << std::endl;
}

TEST_F(DataManEngineTest, WriteRead_1D_Subscribe_BZip2)
{
    Dims shape = {10};
    Dims start = {0};
    Dims count = {10};
    size_t steps = 200;
    std::vector<adios2::Params> transportParams = {{
        {"Library", "ZMQ"},
        {"IPAddress", "127.0.0.1"},
        {"Port", "12307"},
        {"CompressionMethod", "bzip2"},
    }};
    std::string workflowMode = "subscribe";
    auto r = std::thread(DataManReaderStrict, shape, start, count, steps,
                         workflowMode, transportParams);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    DataManWriter(shape, start, count, steps, workflowMode, transportParams);
    std::cout << "Writer ended" << std::endl;
    r.join();
    std::cout << "Reader ended" << std::endl;
}

#endif

int main(int argc, char **argv)
{
#ifdef ADIOS2_HAVE_MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

    int result;
    ::testing::InitGoogleTest(&argc, argv);
    result = RUN_ALL_TESTS();

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return result;
}
