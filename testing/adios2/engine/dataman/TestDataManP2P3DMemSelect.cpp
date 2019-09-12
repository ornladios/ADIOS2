/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TestDataManP2P3SDMemSelect.cpp
 *
 *  Created on: Nov 24, 2018
 *      Author: Jason Wang
 */

#include <adios2.h>
#include <gtest/gtest.h>
#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#endif
#include <numeric>
#include <thread>

using namespace adios2;
int mpiRank = 0;
int mpiSize = 1;
size_t print_lines = 0;

Dims shape = {4, 4, 4};
std::vector<int> global_data = {0,   1,   2,   3,   10,  11,  12,  13,
                                20,  21,  22,  23,  30,  31,  32,  33,

                                100, 101, 102, 103, 110, 111, 112, 113,
                                120, 121, 122, 123, 130, 131, 132, 133,

                                200, 201, 202, 203, 210, 211, 212, 213,
                                220, 221, 222, 223, 230, 231, 232, 233,

                                300, 301, 302, 303, 310, 311, 312, 313,
                                320, 321, 322, 323, 330, 331, 332, 333};

Dims start = {1, 2, 1};
Dims count = {2, 1, 2};
std::vector<int> writer_data = {121, 122, 221, 222};

Dims memstart = {0, 1, 1};
Dims memcount = {3, 3, 3};
std::vector<int> reader_data = {11,  12,  13,  21,  22,  23,  31,  32,  33,

                                111, 112, 113, 121, 122, 123, 131, 132, 133,

                                211, 212, 213, 221, 222, 223, 231, 232, 233};

class DataManEngineTest : public ::testing::Test
{
public:
    DataManEngineTest() = default;
};

template <class T>
void PrintData(const T *data, const size_t step, const Dims &start,
               const Dims &count)
{
    size_t size = std::accumulate(count.begin(), count.end(), 1,
                                  std::multiplies<size_t>());
    std::cout << "Rank: " << mpiRank << " Step: " << step << " Size:" << size
              << "\n";
    size_t printsize = 128;

    if (size < printsize)
    {
        printsize = size;
    }
    int s = 0;
    for (size_t i = 0; i < printsize; ++i)
    {
        ++s;
        std::cout << data[i] << " ";
        if (s == count[1])
        {
            std::cout << std::endl;
            s = 0;
        }
    }

    std::cout << "]" << std::endl;
}

void VerifyData(const int *data, size_t step, const Dims &start,
                const Dims &count, const Dims &shape)
{
    size_t size = std::accumulate(count.begin(), count.end(), 1,
                                  std::multiplies<size_t>());
    bool compressed = false;
    if (print_lines < 100)
    {
        PrintData(data, step, start, count);
        ++print_lines;
    }
    for (size_t i = 0; i < size; ++i)
    {
        if (!compressed)
        {
            ASSERT_EQ(data[i], reader_data[i]);
        }
    }
}

void DataManWriterP2PMemSelect(const Dims &shape, const Dims &start,
                               const Dims &count, const size_t steps,
                               const adios2::Params &engineParams)
{
#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_SELF, adios2::DebugON);
#else
    adios2::ADIOS adios(adios2::DebugON);
#endif
    adios2::IO dataManIO = adios.DeclareIO("WAN");
    dataManIO.SetEngine("DataMan");
    dataManIO.SetParameters(engineParams);
    auto bpInts = dataManIO.DefineVariable<int>("bpInts", shape, start, count);
    adios2::Engine dataManWriter =
        dataManIO.Open("stream", adios2::Mode::Write);
    for (int i = 0; i < steps; ++i)
    {
        dataManWriter.BeginStep();
        dataManWriter.Put(bpInts, writer_data.data(), adios2::Mode::Sync);
        dataManWriter.EndStep();
    }
    dataManWriter.Close();
}

void DataManReaderP2PMemSelect(const Dims &shape, const Dims &start,
                               const Dims &count, const Dims &memStart,
                               const Dims &memCount, const size_t steps,
                               const adios2::Params &engineParams)
{
#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_SELF, adios2::DebugON);
#else
    adios2::ADIOS adios(adios2::DebugON);
#endif
    adios2::IO dataManIO = adios.DeclareIO("WAN");
    dataManIO.SetEngine("DataMan");
    dataManIO.SetParameters(engineParams);
    adios2::Engine dataManReader = dataManIO.Open("stream", adios2::Mode::Read);
    std::vector<int> myInts = reader_data;
    size_t i;
    while (true)
    {
        adios2::StepStatus status = dataManReader.BeginStep(StepMode::Read, 5);
        if (status == adios2::StepStatus::OK)
        {
            const auto &vars = dataManIO.AvailableVariables();
            ASSERT_EQ(vars.size(), 1);
            if (print_lines == 0)
            {
                std::cout << "All available variables : ";
                for (const auto &var : vars)
                {
                    std::cout << var.first << ", ";
                }
                std::cout << std::endl;
            }
            size_t currentStep = dataManReader.CurrentStep();
            adios2::Variable<int> bpInts =
                dataManIO.InquireVariable<int>("bpInts");
            bpInts.SetSelection({start, count});
            bpInts.SetMemorySelection({memStart, memCount});
            dataManReader.Get(bpInts, myInts.data(), adios2::Mode::Sync);
            VerifyData(myInts.data(), currentStep, memStart, memCount, shape);
            dataManReader.EndStep();
        }
        else if (status == adios2::StepStatus::EndOfStream)
        {
            std::cout << "DataManReader end of stream at Step " << i
                      << std::endl;
            break;
        }
        else if (status == adios2::StepStatus::NotReady)
        {
            continue;
        }
    }
    dataManReader.Close();
    print_lines = 0;
}

#ifdef ADIOS2_HAVE_ZEROMQ
TEST_F(DataManEngineTest, WriteRead_3D_MemSelect)
{

    size_t steps = 10000;
    adios2::Params engineParams = {{"IPAddress", "127.0.0.1"},
                                   {"Port", "12310"}};
    // run workflow

    auto r = std::thread(DataManReaderP2PMemSelect, shape, start, count,
                         memstart, memcount, steps, engineParams);
    std::cout << "Reader thread started" << std::endl;

    auto w = std::thread(DataManWriterP2PMemSelect, shape, start, count, steps,
                         engineParams);
    std::cout << "Writer thread started" << std::endl;

    w.join();
    std::cout << "Writer thread ended" << std::endl;

    r.join();
    std::cout << "Reader thread ended" << std::endl;
}

#endif // ZEROMQ

int main(int argc, char **argv)
{
#ifdef ADIOS2_HAVE_MPI
    int mpi_provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &mpi_provided);
    std::cout << "MPI_Init_thread required Mode " << MPI_THREAD_MULTIPLE
              << " and provided Mode " << mpi_provided << std::endl;
    if (mpi_provided != MPI_THREAD_MULTIPLE)
    {
        MPI_Finalize();
        return 0;
    }
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
