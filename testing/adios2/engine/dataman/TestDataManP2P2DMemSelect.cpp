/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TestDataManP2PSz2D.cpp
 *
 *  Created on: Aug 16, 2018
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

template <class T>
void GenData(std::vector<T> &data, const size_t step, const Dims &start,
             const Dims &count, const Dims &shape)
{
    if (start.size() == 2)
    {
        for (size_t i = 0; i < count[0]; ++i)
        {
            for (size_t j = 0; j < count[1]; ++j)
            {
                data[i * count[1] + j] =
                    (i + start[1]) * shape[1] + j + start[0];
            }
        }
    }
}

template <class T>
void VerifyData(const std::complex<T> *data, size_t step, const Dims &start,
                const Dims &count, const Dims &shape)
{
    size_t size = std::accumulate(count.begin(), count.end(), 1,
                                  std::multiplies<size_t>());
    std::vector<std::complex<T>> tmpdata(size);
    GenData(tmpdata, step, start, count, shape);
    for (size_t i = 0; i < size; ++i)
    {
        ASSERT_EQ(data[i], tmpdata[i]);
    }
    if (print_lines < 100)
    {
        PrintData(data, step, start, count);
        ++print_lines;
    }
}

template <class T>
void VerifyData(const T *data, size_t step, const Dims &start,
                const Dims &count, const Dims &shape)
{
    size_t size = std::accumulate(count.begin(), count.end(), 1,
                                  std::multiplies<size_t>());
    bool compressed = false;
    std::vector<T> tmpdata(size);
    if (print_lines < 100)
    {
        PrintData(data, step, start, count);
        ++print_lines;
    }
    GenData(tmpdata, step, start, count, shape);
    for (size_t i = 0; i < size; ++i)
    {
        if (!compressed)
        {
            ASSERT_EQ(data[i], tmpdata[i]);
        }
    }
}

void DataManWriterP2PMemSelect(const Dims &shape, const Dims &start,
                               const Dims &count, const size_t steps,
                               const adios2::Params &engineParams)
{
    size_t datasize = std::accumulate(count.begin(), count.end(), 1,
                                      std::multiplies<size_t>());
#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_SELF, adios2::DebugON);
#else
    adios2::ADIOS adios(adios2::DebugON);
#endif
    adios2::IO dataManIO = adios.DeclareIO("WAN");
    dataManIO.SetEngine("DataMan");
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
    dataManIO.DefineAttribute<int>("AttInt", 110);
    adios2::Engine dataManWriter =
        dataManIO.Open("stream", adios2::Mode::Write);
    for (int i = 0; i < steps; ++i)
    {
        dataManWriter.BeginStep();
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
        dataManWriter.Put(bpChars, myChars.data(), adios2::Mode::Sync);
        dataManWriter.Put(bpUChars, myUChars.data(), adios2::Mode::Sync);
        dataManWriter.Put(bpShorts, myShorts.data(), adios2::Mode::Sync);
        dataManWriter.Put(bpUShorts, myUShorts.data(), adios2::Mode::Sync);
        dataManWriter.Put(bpInts, myInts.data(), adios2::Mode::Sync);
        dataManWriter.Put(bpUInts, myUInts.data(), adios2::Mode::Sync);
        dataManWriter.Put(bpFloats, myFloats.data(), adios2::Mode::Sync);
        dataManWriter.Put(bpDoubles, myDoubles.data(), adios2::Mode::Sync);
        dataManWriter.Put(bpComplexes, myComplexes.data(), adios2::Mode::Sync);
        dataManWriter.Put(bpDComplexes, myDComplexes.data(),
                          adios2::Mode::Sync);
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

    size_t datasize = std::accumulate(memCount.begin(), memCount.end(), 1,
                                      std::multiplies<size_t>());
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
    bool received_steps = false;
    size_t currentStep;
    while (true)
    {
        adios2::StepStatus status = dataManReader.BeginStep();
        if (status == adios2::StepStatus::OK)
        {
            received_steps = true;
            const auto &vars = dataManIO.AvailableVariables();
            ASSERT_EQ(vars.size(), 10);
            if (print_lines == 0)
            {
                std::cout << "All available variables : ";
                for (const auto &var : vars)
                {
                    std::cout << var.first << ", ";
                }
                std::cout << std::endl;
            }
            currentStep = dataManReader.CurrentStep();
            GenData(myChars, currentStep, memStart, memCount, shape);
            GenData(myUChars, currentStep, memStart, memCount, shape);
            GenData(myShorts, currentStep, memStart, memCount, shape);
            GenData(myUShorts, currentStep, memStart, memCount, shape);
            GenData(myInts, currentStep, memStart, memCount, shape);
            GenData(myUInts, currentStep, memStart, memCount, shape);
            GenData(myFloats, currentStep, memStart, memCount, shape);
            GenData(myDoubles, currentStep, memStart, memCount, shape);
            GenData(myComplexes, currentStep, memStart, memCount, shape);
            GenData(myDComplexes, currentStep, memStart, memCount, shape);
            adios2::Variable<char> bpChars =
                dataManIO.InquireVariable<char>("bpChars");
            adios2::Variable<unsigned char> bpUChars =
                dataManIO.InquireVariable<unsigned char>("bpUChars");
            adios2::Variable<short> bpShorts =
                dataManIO.InquireVariable<short>("bpShorts");
            adios2::Variable<unsigned short> bpUShorts =
                dataManIO.InquireVariable<unsigned short>("bpUShorts");
            adios2::Variable<int> bpInts =
                dataManIO.InquireVariable<int>("bpInts");
            adios2::Variable<unsigned int> bpUInts =
                dataManIO.InquireVariable<unsigned int>("bpUInts");
            adios2::Variable<float> bpFloats =
                dataManIO.InquireVariable<float>("bpFloats");
            adios2::Variable<double> bpDoubles =
                dataManIO.InquireVariable<double>("bpDoubles");
            adios2::Variable<std::complex<float>> bpComplexes =
                dataManIO.InquireVariable<std::complex<float>>("bpComplexes");
            adios2::Variable<std::complex<double>> bpDComplexes =
                dataManIO.InquireVariable<std::complex<double>>("bpDComplexes");
            auto charsBlocksInfo = dataManReader.AllStepsBlocksInfo(bpChars);

            bpChars.SetSelection({start, count});
            bpUChars.SetSelection({start, count});
            bpShorts.SetSelection({start, count});
            bpUShorts.SetSelection({start, count});
            bpInts.SetSelection({start, count});
            bpUInts.SetSelection({start, count});
            bpFloats.SetSelection({start, count});
            bpDoubles.SetSelection({start, count});
            bpComplexes.SetSelection({start, count});
            bpDComplexes.SetSelection({start, count});

            bpChars.SetMemorySelection({memStart, memCount});
            bpUChars.SetMemorySelection({memStart, memCount});
            bpShorts.SetMemorySelection({memStart, memCount});
            bpUShorts.SetMemorySelection({memStart, memCount});
            bpInts.SetMemorySelection({memStart, memCount});
            bpUInts.SetMemorySelection({memStart, memCount});
            bpFloats.SetMemorySelection({memStart, memCount});
            bpDoubles.SetMemorySelection({memStart, memCount});
            bpComplexes.SetMemorySelection({memStart, memCount});
            bpDComplexes.SetMemorySelection({memStart, memCount});

            dataManReader.Get(bpChars, myChars.data(), adios2::Mode::Sync);
            dataManReader.Get(bpUChars, myUChars.data(), adios2::Mode::Sync);
            dataManReader.Get(bpShorts, myShorts.data(), adios2::Mode::Sync);
            dataManReader.Get(bpUShorts, myUShorts.data(), adios2::Mode::Sync);
            dataManReader.Get(bpInts, myInts.data(), adios2::Mode::Sync);
            dataManReader.Get(bpUInts, myUInts.data(), adios2::Mode::Sync);
            dataManReader.Get(bpFloats, myFloats.data(), adios2::Mode::Sync);
            dataManReader.Get(bpDoubles, myDoubles.data(), adios2::Mode::Sync);
            dataManReader.Get(bpComplexes, myComplexes.data(),
                              adios2::Mode::Sync);
            dataManReader.Get(bpDComplexes, myDComplexes.data(),
                              adios2::Mode::Sync);
            VerifyData(myChars.data(), currentStep, memStart, memCount, shape);
            VerifyData(myUChars.data(), currentStep, memStart, memCount, shape);
            VerifyData(myShorts.data(), currentStep, memStart, memCount, shape);
            VerifyData(myUShorts.data(), currentStep, memStart, memCount,
                       shape);
            VerifyData(myInts.data(), currentStep, memStart, memCount, shape);
            VerifyData(myUInts.data(), currentStep, memStart, memCount, shape);
            VerifyData(myFloats.data(), currentStep, memStart, memCount, shape);
            VerifyData(myDoubles.data(), currentStep, memStart, memCount,
                       shape);
            VerifyData(myComplexes.data(), currentStep, memStart, memCount,
                       shape);
            VerifyData(myDComplexes.data(), currentStep, memStart, memCount,
                       shape);
            dataManReader.EndStep();
        }
        else if (status == adios2::StepStatus::EndOfStream)
        {
            break;
        }
        else if (status == adios2::StepStatus::NotReady)
        {
            continue;
        }
    }
    if (received_steps)
    {
        auto attInt = dataManIO.InquireAttribute<int>("AttInt");
        std::cout << "Attribute received " << attInt.Data()[0]
                  << ", expected 110" << std::endl;
        ASSERT_EQ(110, attInt.Data()[0]);
        ASSERT_NE(111, attInt.Data()[0]);
        ASSERT_EQ(currentStep + 1, steps);
    }
    dataManReader.Close();
    print_lines = 0;
}

#ifdef ADIOS2_HAVE_ZEROMQ
TEST_F(DataManEngineTest, WriteRead_2D_MemSelect)
{
    // set parameters
    Dims shape = {10, 10};
    Dims start = {2, 2};
    Dims count = {5, 5};
    Dims memstart = start;
    Dims memcount = count;
    memstart = {1, 1};
    memcount = {7, 9};

    size_t steps = 10000;
    adios2::Params engineParams = {
        {"IPAddress", "127.0.0.1"}, {"Port", "12308"}, {"Verbose", "0"}};

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
