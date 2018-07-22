/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TestDataMan.cpp
 *
 *  Created on: Jul 12, 2018
 *      Author: Jason Wang
 */

#include <future>
#include <numeric>

#include <adios2.h>
#include <gtest/gtest.h>

#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#endif

using namespace adios2;

int mpiRank = 0;
int mpiSize = 1;
std::string ip = "127.0.0.1";
std::string port = "12306";

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
void PrintData(std::vector<T> &data, size_t step)
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
}

void DataManWriter(Dims shape, Dims start, Dims count, size_t steps,
                   std::string workflowMode, std::string transport)
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
    dataManIO.AddTransport(
        "WAN", {{"Library", transport}, {"IPAddress", ip}, {"Port", port}});
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

void DataManReader(Dims shape, Dims start, Dims count, size_t steps,
                   std::string workflowMode, std::string transport)
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
    dataManIO.AddTransport(
        "WAN", {{"Library", transport}, {"IPAddress", ip}, {"Port", port}});
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
            dataManReader.EndStep();
        }
        else if (status == adios2::StepStatus::NotReady)
        {
        }
        else if (status == adios2::StepStatus::EndOfStream)
        {
            break;
        }
    }
    dataManReader.Close();
}

#ifdef ADIOS2_HAVE_ZEROMQ
TEST_F(DataManEngineTest, WriteRead_1D_Subscribe_ZeroMQ)
{
    Dims shape = {10};
    Dims start = {0};
    Dims count = {10};
    size_t steps = 200;
    std::string mode = "subscribe";
    std::string transport = "ZMQ";
    auto r = std::async(std::launch::async, DataManReader, shape, start, count,
                        steps, mode, transport);
    DataManWriter(shape, start, count, steps, mode, transport);
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
