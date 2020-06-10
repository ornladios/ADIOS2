/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TestDataMan1D.cpp
 *
 *  Created on: Jul 12, 2018
 *      Author: Jason Wang
 */

#include <iostream>
#include <mutex>
#include <numeric>
#include <thread>

#include <adios2.h>
#include <adios2/common/ADIOSMacros.h>
#include <adios2/helper/adiosFunctions.h>
#include <gtest/gtest.h>

using namespace adios2;

size_t print_lines = 0;
size_t to_print_lines = 10;

std::mutex coutMtx;

template <class T>
void GenData(std::vector<std::complex<T>> &data, const size_t step)
{
    for (size_t i = 0; i < data.size(); ++i)
    {
        data[i] = {static_cast<T>(i + step * 100), static_cast<T>(i)};
    }
}

template <class T>
void GenData(std::vector<T> &data, const size_t step)
{
    for (size_t i = 0; i < data.size(); ++i)
    {
        data[i] = i + step * 100;
    }
}

template <class T>
void PrintData(const T *data, const size_t size, const size_t step)
{
    std::lock_guard<std::mutex> coutLock(coutMtx);

    std::cout << "Step: " << step << " [";
    size_t printsize = 32;
    if (size < printsize)
    {
        printsize = size;
    }
    for (size_t i = 0; i < printsize; ++i)
    {
        std::cout << data[i] << " ";
    }
    std::cout << "]" << std::endl;
}

template <class T>
void VerifyData(const std::complex<T> *data, const size_t size, size_t step)
{
    std::vector<std::complex<T>> tmpdata(size);
    GenData(tmpdata, step);
    for (size_t i = 0; i < size; ++i)
    {
        ASSERT_EQ(data[i], tmpdata[i]);
    }
    if (print_lines < to_print_lines)
    {
        PrintData(data, size, step);
        ++print_lines;
    }
}

template <class T>
void VerifyData(const T *data, const size_t size, size_t step)
{
    std::vector<T> tmpdata(size);
    GenData(tmpdata, step);
    for (size_t i = 0; i < size; ++i)
    {
        ASSERT_EQ(data[i], tmpdata[i]);
    }
    if (print_lines < to_print_lines)
    {
        PrintData(data, size, step);
        ++print_lines;
    }
}

template <class T>
void VerifyData(const std::vector<T> &data, const size_t step)
{
    VerifyData(data.data(), data.size(), step);
}

void DataManWriter(const Dims &shape, const Dims &start, const Dims &count,
                   const size_t steps, const adios2::Params &engineParams)
{
    size_t datasize = std::accumulate(count.begin(), count.end(), 1,
                                      std::multiplies<size_t>());
    adios2::ADIOS adios;
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
    auto bpUInt64s = dataManIO.DefineVariable<uint64_t>("bpUInt64s");
    dataManIO.DefineAttribute<int>("AttInt", 110);
    adios2::Engine dataManWriter =
        dataManIO.Open("stream", adios2::Mode::Write);
    for (uint64_t i = 0; i < steps; ++i)
    {
        dataManWriter.BeginStep();
        GenData(myChars, i);
        GenData(myUChars, i);
        GenData(myShorts, i);
        GenData(myUShorts, i);
        GenData(myInts, i);
        GenData(myUInts, i);
        GenData(myFloats, i);
        GenData(myDoubles, i);
        GenData(myComplexes, i);
        GenData(myDComplexes, i);
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
        dataManWriter.Put(bpUInt64s, i);
        dataManWriter.EndStep();
    }
    dataManWriter.Close();
}

void DataManReader(const Dims &shape, const Dims &start, const Dims &count,
                   const size_t steps, const adios2::Params &engineParams)
{
    size_t datasize = std::accumulate(count.begin(), count.end(), 1,
                                      std::multiplies<size_t>());
    adios2::ADIOS adios;
    adios2::IO dataManIO = adios.DeclareIO("WAN");
    dataManIO.SetEngine("DataMan");
    dataManIO.SetParameters(engineParams);
    adios2::Engine dataManReader = dataManIO.Open("stream", adios2::Mode::Read);

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
        adios2::StepStatus status = dataManReader.BeginStep(StepMode::Read, 5);
        if (status == adios2::StepStatus::OK)
        {
            received_steps = true;
            const auto &vars = dataManIO.AvailableVariables();
            ASSERT_EQ(vars.size(), 11);
            if (print_lines < 10)
            {
                std::lock_guard<std::mutex> countLock(coutMtx);
                std::cout << "All available variables : ";
                for (const auto &var : vars)
                {
                    std::cout << var.first << ", ";
                }
                std::cout << std::endl;
            }
            currentStep = dataManReader.CurrentStep();
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
            adios2::Variable<uint64_t> bpUInt64s =
                dataManIO.InquireVariable<uint64_t>("bpUInt64s");
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
            uint64_t stepValue;
            dataManReader.Get(bpUInt64s, &stepValue, adios2::Mode::Sync);
            ASSERT_EQ(currentStep, stepValue);
            VerifyData(myChars, currentStep);
            VerifyData(myUChars, currentStep);
            VerifyData(myShorts, currentStep);
            VerifyData(myUShorts, currentStep);
            VerifyData(myInts, currentStep);
            VerifyData(myUInts, currentStep);
            VerifyData(myFloats, currentStep);
            VerifyData(myDoubles, currentStep);
            VerifyData(myComplexes, currentStep);
            VerifyData(myDComplexes, currentStep);
            dataManReader.EndStep();
        }
        else if (status == adios2::StepStatus::EndOfStream)
        {
            std::lock_guard<std::mutex> countLock(coutMtx);
            std::cout << "DataManReader end of stream at Step " << currentStep
                      << std::endl;
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
        ASSERT_EQ(110, attInt.Data()[0]);
        ASSERT_NE(111, attInt.Data()[0]);
    }
    else
    {
        std::cout << "no steps received " << std::endl;
    }
    dataManReader.Close();
}

class DataManEngineTest : public ::testing::Test
{
public:
    DataManEngineTest() = default;
};

TEST_F(DataManEngineTest, 1D)
{
    // set parameters
    Dims shape = {10};
    Dims start = {0};
    Dims count = {10};
    size_t steps = 5000;
    adios2::Params engineParams = {{"IPAddress", "127.0.0.1"},
                                   {"Port", "12300"}};

    // run workflow
    auto r =
        std::thread(DataManReader, shape, start, count, steps, engineParams);
    {
        std::lock_guard<std::mutex> coutLock(coutMtx);
        std::cout << "Reader thread started" << std::endl;
    }
    auto w =
        std::thread(DataManWriter, shape, start, count, steps, engineParams);
    {
        std::lock_guard<std::mutex> coutLock(coutMtx);
        std::cout << "Writer thread started" << std::endl;
    }
    w.join();
    {
        std::lock_guard<std::mutex> coutLock(coutMtx);
        std::cout << "Writer thread ended" << std::endl;
    }
    r.join();
    std::cout << "Reader thread ended" << std::endl;
}

int main(int argc, char **argv)
{

    int result;
    ::testing::InitGoogleTest(&argc, argv);
    result = RUN_ALL_TESTS();

    return result;
}
