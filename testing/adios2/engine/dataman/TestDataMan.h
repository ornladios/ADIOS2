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

size_t print_lines = 0;

template <class T>
void GenData(std::vector<std::complex<T>> &data, const size_t step)
{
    for (size_t i = 0; i < data.size(); ++i)
    {
        data[i] = {static_cast<T>(i + mpiRank * 10000 + step * 100),
                   static_cast<T>(i + mpiRank * 10000)};
    }
}

template <class T>
void GenData(std::vector<T> &data, const size_t step)
{
    for (size_t i = 0; i < data.size(); ++i)
    {
        data[i] = i + mpiRank * 10000 + step * 100;
    }
}

template <class T>
void PrintData(const T *data, const size_t size, const size_t step)
{
    std::cout << "Rank: " << mpiRank << " Step: " << step << " [";
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
void VerifyData(const std::complex<T> *data, const size_t size, size_t step,
                const std::vector<Params> &transParams)
{
    std::vector<std::complex<T>> tmpdata(size);
    GenData(tmpdata, step);
    for (size_t i = 0; i < size; ++i)
    {
        ASSERT_EQ(data[i], tmpdata[i]);
    }
    if (print_lines < 100)
    {
        PrintData(data, size, step);
        ++print_lines;
    }
}

template <class T>
void VerifyData(const T *data, const size_t size, size_t step,
                const std::vector<Params> &transParams)
{
    bool compressed = false;
    for (const auto &i : transParams)
    {
        auto j = i.find("CompressionMethod");
        if (j != i.end())
        {
            compressed = true;
        }
    }
    std::vector<T> tmpdata(size);
    GenData(tmpdata, step);
    for (size_t i = 0; i < size; ++i)
    {
        if (!compressed)
        {
            ASSERT_EQ(data[i], tmpdata[i]);
        }
    }
    if (print_lines < 100)
    {
        PrintData(data, size, step);
        ++print_lines;
    }
}

template <class T>
void VerifyData(const std::vector<T> &data, const size_t step,
                const std::vector<Params> &transParams)
{
    VerifyData(data.data(), data.size(), step, transParams);
}

void UserCallBack2(float *data, const std::string &doid, const std::string &var,
                   const std::string &dtype, const size_t step,
                   const adios2::Dims &varshape, const adios2::Dims &start,
                   const adios2::Dims &count)
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

    for (size_t i = 0; i < dumpsize; ++i)
    {
        std::cout << data[i] << " ";
    }
    std::cout << std::endl;
}

void UserCallBack1(void *data, const std::string &doid, const std::string &var,
                   const std::string &dtype, const size_t step,
                   const adios2::Dims &varshape, const adios2::Dims &start,
                   const adios2::Dims &count)
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
                   const size_t steps, const adios2::Params &engineParams,
                   const std::vector<adios2::Params> &transParams)
{
    size_t datasize = std::accumulate(count.begin(), count.end(), 1,
                                      std::multiplies<size_t>());
#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_SELF, adios2::DebugON);
#else
    adios2::ADIOS adios;
#endif
    adios2::IO dataManIO = adios.DeclareIO("WAN");
    dataManIO.SetEngine("DataMan");
    dataManIO.SetParameters(engineParams);
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
        dataManWriter.EndStep();
    }
    dataManWriter.Close();
}

void DataManReaderP2P(const Dims &shape, const Dims &start, const Dims &count,
                      const size_t steps, const adios2::Params &engineParams,
                      const std::vector<adios2::Params> &transParams)
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
    for (const auto &params : transParams)
    {
        dataManIO.AddTransport("WAN", params);
    }
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
    size_t i;
    for (i = 0; i < steps; ++i)
    {
        adios2::StepStatus status =
            dataManReader.BeginStep(StepMode::NextAvailable, 5);
        if (status == adios2::StepStatus::OK)
        {
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
            size_t currentStep = dataManReader.CurrentStep();
            ASSERT_EQ(i, currentStep);
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
            VerifyData(myChars, currentStep, transParams);
            VerifyData(myUChars, currentStep, transParams);
            VerifyData(myShorts, currentStep, transParams);
            VerifyData(myUShorts, currentStep, transParams);
            VerifyData(myInts, currentStep, transParams);
            VerifyData(myUInts, currentStep, transParams);
            VerifyData(myFloats, currentStep, transParams);
            VerifyData(myDoubles, currentStep, transParams);
            VerifyData(myComplexes, currentStep, transParams);
            VerifyData(myDComplexes, currentStep, transParams);
            dataManReader.EndStep();
        }
        else
        {
            std::cout << "DataManReader end of stream at Step " << i
                      << std::endl;
            break;
        }
    }
    auto attInt = dataManIO.InquireAttribute<int>("AttInt");
    ASSERT_EQ(110, attInt.Data()[0]);
    ASSERT_NE(111, attInt.Data()[0]);
    ASSERT_EQ(i, steps);
    dataManReader.Close();
    print_lines = 0;
}

void DataManReaderCallback(const Dims &shape, const Dims &start,
                           const Dims &count, const size_t steps,
                           const adios2::Params &engineParams,
                           const std::vector<adios2::Params> &transParams,
                           const size_t timeout)
{
#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_SELF, adios2::DebugON);
#else
    adios2::ADIOS adios(adios2::DebugON);
#endif
    adios2::Operator callback = adios.DefineOperator(
        "Print all variables callback void",
        std::function<void(void *, const std::string &, const std::string &,
                           const std::string &, const size_t,
                           const adios2::Dims &, const adios2::Dims &,
                           const adios2::Dims &)>(UserCallBack1));
    adios2::IO dataManIO = adios.DeclareIO("WAN");
    dataManIO.SetEngine("DataMan");
    dataManIO.SetParameters(engineParams);
    for (const auto &params : transParams)
    {
        dataManIO.AddTransport("WAN", params);
    }
    dataManIO.AddOperation(callback);
    adios2::Engine dataManReader = dataManIO.Open("stream", adios2::Mode::Read);
    std::this_thread::sleep_for(std::chrono::seconds(timeout));
    dataManReader.Close();
}

void DataManReaderSubscribe(const Dims &shape, const Dims &start,
                            const Dims &count, const size_t steps,
                            const adios2::Params &engineParams,
                            const std::vector<adios2::Params> &transParams,
                            const size_t timeout)
{
    size_t datasize = std::accumulate(count.begin(), count.end(), 1,
                                      std::multiplies<size_t>());
    std::vector<float> myFloats(datasize);
#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_SELF, adios2::DebugON);
#else
    adios2::ADIOS adios(adios2::DebugON);
#endif
    adios2::IO dataManIO = adios.DeclareIO("WAN");
    dataManIO.SetEngine("DataMan");
    dataManIO.SetParameters(engineParams);
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
            std::cout << "DataMan Timeout. Last step received: " << i
                      << std::endl;
            ASSERT_GT(i, 0);
            break;
        }
        adios2::StepStatus status = dataManReader.BeginStep();
        if (status == adios2::StepStatus::OK)
        {
            while (not bpFloats)
            {
                bpFloats = dataManIO.InquireVariable<float>("bpFloats");
            }
            bpFloats.SetSelection({start, count});
            dataManReader.Get<float>(bpFloats, myFloats.data(),
                                     adios2::Mode::Sync);
            i = dataManReader.CurrentStep();
            VerifyData(myFloats, i, transParams);
        }
        else if (status == adios2::StepStatus::EndOfStream)
        {
            break;
        }
        dataManReader.EndStep();
    }
    dataManReader.Close();
}
