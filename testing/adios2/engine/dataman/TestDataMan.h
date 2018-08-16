/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TestDataMan.h
 *
 *  Created on: Jul 12, 2018
 *      Author: Jason Wang
 */

#include <numeric>
#include <thread>

#include <adios2.h>
#include <adios2/ADIOSMacros.h>
#include <adios2/helper/adiosFunctions.h>
#include <gtest/gtest.h>

using namespace adios2;

int mpiRank = 0;
int mpiSize = 1;

size_t print_step = 0;

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
    size_t printsize = 32;
    if (data.size() < printsize)
    {
        printsize = data.size();
    }
    for (size_t i = 0; i < printsize; ++i)
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
    if (print_step < 10)
    {
        PrintData(data, step);
        ++print_step;
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

    if (step < 500)
    {
        PrintData(data, step);
    }
}

void UserCallBack(void *data, const std::string &doid, const std::string var,
                  const std::string &dtype,
                  const std::vector<std::size_t> varshape)
{
    std::cout << "Object : " << doid << ", ";
    std::cout << "Variable :" << var << ", ";
    std::cout << "Type : " << dtype << ", ";
    std::cout << "Shape : [";
    for (size_t i = 0; i < varshape.size(); ++i)
    {
        std::cout << varshape[i];
        if (i != varshape.size() - 1)
        {
            std::cout << ", ";
        }
    }
    std::cout << "]"
              << ". ";

    size_t varsize = std::accumulate(varshape.begin(), varshape.end(), 1,
                                     std::multiplies<std::size_t>());

    size_t dumpsize = 128;
    if (varsize < dumpsize)
    {
        dumpsize = varsize;
    }

    std::cout << "Printing data for the first " << dumpsize << " elements: ";

#define declare_type(T)                                                        \
    if (dtype == adios2::helper::GetType<T>())                                 \
    {                                                                          \
        for (size_t i = 0; i < dumpsize; ++i)                                  \
        {                                                                      \
            std::cout << (reinterpret_cast<T *>(data))[i] << " ";              \
        }                                                                      \
        std::cout << std::endl;                                                \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
}

void DataManWriter(const Dims &shape, const Dims &start, const Dims &count,
                   const size_t steps, const std::string &workflowMode,
                   const std::vector<adios2::Params> &transParams)
{
    size_t datasize = std::accumulate(count.begin(), count.end(), 1,
                                      std::multiplies<size_t>());
    adios2::ADIOS adios;
    adios2::IO dataManIO = adios.DeclareIO("WAN");
    dataManIO.SetEngine("DataMan");
    dataManIO.SetParameters({{"WorkflowMode", workflowMode}});
    for (const auto &params : transParams)
    {
        dataManIO.AddTransport("WAN", params);
    }
    std::vector<char> myChars(datasize);
    std::vector<unsigned char> myUChars(datasize);
    std::vector<short> myShorts(datasize);
    std::vector<unsigned short> myUShorts(datasize);
    std::vector<int> myInts(datasize);
    std::vector<unsigned int> myUInts(datasize);
    std::vector<float> myFloats(datasize);
    std::vector<double> myDoubles(datasize);
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
    adios2::Engine dataManWriter =
        dataManIO.Open("stream", adios2::Mode::Write);
    for (int i = 0; i < steps; ++i)
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
        dataManWriter.Put(bpChars, myChars.data(), adios2::Mode::Sync);
        dataManWriter.Put(bpUChars, myUChars.data(), adios2::Mode::Sync);
        dataManWriter.Put(bpShorts, myShorts.data(), adios2::Mode::Sync);
        dataManWriter.Put(bpUShorts, myUShorts.data(), adios2::Mode::Sync);
        dataManWriter.Put(bpInts, myInts.data(), adios2::Mode::Sync);
        dataManWriter.Put(bpUInts, myUInts.data(), adios2::Mode::Sync);
        dataManWriter.Put(bpFloats, myFloats.data(), adios2::Mode::Sync);
        dataManWriter.Put(bpDoubles, myDoubles.data(), adios2::Mode::Sync);
        dataManWriter.EndStep();
    }
    dataManWriter.Close();
}

void DataManReaderP2P(const Dims &shape, const Dims &start, const Dims &count,
                      const size_t steps, const std::string &workflowMode,
                      const std::vector<adios2::Params> &transParams)
{
    size_t datasize = std::accumulate(count.begin(), count.end(), 1,
                                      std::multiplies<size_t>());
    std::vector<float> myFloats(datasize);
    adios2::ADIOS adios(adios2::DebugON);
    adios2::IO dataManIO = adios.DeclareIO("WAN");
    dataManIO.SetEngine("DataMan");
    dataManIO.SetParameters({{"WorkflowMode", workflowMode}});
    for (const auto &params : transParams)
    {
        dataManIO.AddTransport("WAN", params);
    }
    adios2::Engine dataManReader = dataManIO.Open("stream", adios2::Mode::Read);
    adios2::Variable<float> bpFloats;

    size_t i;
    for (i = 0; i < steps; ++i)
    {
        adios2::StepStatus status =
            dataManReader.BeginStep(StepMode::NextAvailable, 5);
        if (status == adios2::StepStatus::OK)
        {
            const auto &vars = dataManIO.AvailableVariables();
            ASSERT_EQ(vars.size(), 8);
            if (print_step < 10)
            {
                std::cout << "All available variables : ";
                for (const auto &var : vars)
                {
                    std::cout << var.first << ", ";
                }
                std::cout << std::endl;
            }
            bpFloats = dataManIO.InquireVariable<float>("bpFloats");
            bpFloats.SetSelection({start, count});
            dataManReader.Get<float>(bpFloats, myFloats.data(),
                                     adios2::Mode::Sync);
            size_t currentStep = dataManReader.CurrentStep();
            ASSERT_EQ(i, currentStep);
            VerifyData(myFloats, currentStep);
            dataManReader.EndStep();
        }
        else
        {
            std::cout << "DataManReader end of stream at Step " << i
                      << std::endl;
            break;
        }
    }
    ASSERT_EQ(i, steps);
    dataManReader.Close();
    print_step = 0;
}

void DataManReaderCallback(const Dims &shape, const Dims &start,
                           const Dims &count, const size_t steps,
                           const std::string &workflowMode,
                           const std::vector<adios2::Params> &transParams,
                           const size_t timeout)
{
    adios2::ADIOS adios(adios2::DebugON);
    adios2::Operator callbackFloat = adios.DefineOperator(
        "Print float Variable callback",
        std::function<void(void *, const std::string &, const std::string &,
                           const std::string &, const adios2::Dims &)>(
            UserCallBack));

    adios2::IO dataManIO = adios.DeclareIO("WAN");
    dataManIO.SetEngine("DataMan");
    dataManIO.SetParameters({
        {"WorkflowMode", "subscribe"},
    });
    for (const auto &params : transParams)
    {
        dataManIO.AddTransport("WAN", params);
    }
    dataManIO.AddOperation(callbackFloat);

    adios2::Engine dataManReader = dataManIO.Open("stream", adios2::Mode::Read);

    std::this_thread::sleep_for(std::chrono::seconds(timeout));

    dataManReader.Close();
}

void DataManReaderSubscribe(const Dims &shape, const Dims &start,
                            const Dims &count, const size_t steps,
                            const std::string &workflowMode,
                            const std::vector<adios2::Params> &transParams,
                            const size_t timeout)
{
    size_t datasize = std::accumulate(count.begin(), count.end(), 1,
                                      std::multiplies<size_t>());
    std::vector<float> myFloats(datasize);
    adios2::ADIOS adios(adios2::DebugON);
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
        if (duration.count() > timeout)
        {
            std::cout << "DataMan Timeout" << std::endl;
            return;
        }
        adios2::StepStatus status = dataManReader.BeginStep();
        if (status == adios2::StepStatus::OK)
        {
            bpFloats = dataManIO.InquireVariable<float>("bpFloats");
            if (bpFloats)
            {
                bpFloats.SetSelection({start, count});
                dataManReader.Get<float>(bpFloats, myFloats.data(),
                                         adios2::Mode::Sync);
                i = dataManReader.CurrentStep();
                VerifyData(myFloats, i);
            }
        }
        else if (status == adios2::StepStatus::EndOfStream)
        {
            break;
        }
        dataManReader.EndStep();
    }
    dataManReader.Close();
}
