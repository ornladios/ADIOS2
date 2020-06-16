#include <adios2.h>
#include <array>
#include <condition_variable>
#include <future>
#include <iostream>
#include <numeric>
#include <thread>

#include <gtest/gtest.h>

#include "ParseArgs.h"

using dt = long long;

int failure = 0;

void Read()
{
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("IO");
    io.SetEngine("sst");

    io.SetEngine(engine);
    io.SetParameters(engineParams);

    adios2::Engine Reader = io.Open("communicate", adios2::Mode::Read);

    std::array<dt, 100000> ar;

    auto status = Reader.BeginStep();
    if (status == adios2::StepStatus::EndOfStream)
    {
        return;
    }

    adios2::Variable<dt> var = io.InquireVariable<dt>("data");
    Reader.Get(var, ar.begin());
    Reader.EndStep();
    dt expect = 0;
    for (auto &val : ar)
    {
        if (val != expect)
        {
            failure++;
            break;
        }
        expect++;
    }

    Reader.Close();
}

void Write()
{
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("IO");
    io.SetEngine(engine);
    io.SetParameters(engineParams);
    io.SetEngine("sst");
    adios2::Engine Writer = io.Open("communicate", adios2::Mode::Write);

    auto var =
        io.DefineVariable<dt>("data", adios2::Dims{10000, 10},
                              adios2::Dims{0, 0}, adios2::Dims{10000, 10});

    std::array<dt, 100000> ar;

    std::iota(ar.begin(), ar.end(), 0);

    Writer.BeginStep();
    Writer.Put<dt>(var, ar.begin());
    Writer.EndStep();
    Writer.Close();
}

class TestThreads : public ::testing::Test
{
public:
    TestThreads() = default;
};

TEST_F(TestThreads, Basic)
{
    auto read_fut = std::async(std::launch::async, Read);
    auto write_fut = std::async(std::launch::async, Write);
    read_fut.wait();
    write_fut.wait();
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    ParseArgs(argc, argv);

    int result;
    result = RUN_ALL_TESTS();
    return result;
}
