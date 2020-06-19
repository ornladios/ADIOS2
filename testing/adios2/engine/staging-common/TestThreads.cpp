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

int value_errors = 0;

std::mutex StdOutMtx;

int Read()
{
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("IO");

    {
        std::lock_guard<std::mutex> guard(StdOutMtx);
        std::cout << "Reader: engine = " << engine << std::endl;
    }
    io.SetEngine(engine);
    io.SetParameters(engineParams);

    {
        std::lock_guard<std::mutex> guard(StdOutMtx);
        std::cout << "Reader: call Open" << std::endl;
    }

    try
    {
        adios2::Engine Reader = io.Open("communicate", adios2::Mode::Read);
        {
            std::lock_guard<std::mutex> guard(StdOutMtx);
            std::cout << "Reader: passed Open" << std::endl;
        }
        std::array<dt, 1000> ar;

        auto status = Reader.BeginStep();
        {
            std::lock_guard<std::mutex> guard(StdOutMtx);
            std::cout << "Reader: passed BeginStep";
        }
        if (status == adios2::StepStatus::EndOfStream)
        {
            {
                std::lock_guard<std::mutex> guard(StdOutMtx);
                std::cout << " with EndOfStream " << std::endl;
            }
            return false;
        }
        {
            std::lock_guard<std::mutex> guard(StdOutMtx);
            std::cout << " with success " << std::endl;
        }

        adios2::Variable<dt> var = io.InquireVariable<dt>("data");
        Reader.Get(var, ar.begin());
        Reader.EndStep();
        dt expect = 0;
        for (auto &val : ar)
        {
            if (val != expect)
            {
                value_errors++;
            }
            expect++;
        }

        Reader.Close();
        {
            std::lock_guard<std::mutex> guard(StdOutMtx);
            std::cout << "Reader got " << expect << " values, " << value_errors
                      << " were incorrect" << std::endl;
        }
    }
    catch (std::exception &e)
    {
        {
            std::lock_guard<std::mutex> guard(StdOutMtx);
            std::cout << "Reader: Exception: " << e.what() << std::endl;
        }
        return false;
    }
    return true;
}

bool Write()
{
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("IO");
    io.SetEngine(engine);
    io.SetParameters(engineParams);
    {
        std::lock_guard<std::mutex> guard(StdOutMtx);
        std::cout << "Writer: engine = " << engine << std::endl;
    }
    auto var = io.DefineVariable<dt>("data", adios2::Dims{100, 10},
                                     adios2::Dims{0, 0}, adios2::Dims{100, 10});

    std::array<dt, 1000> ar;
    std::iota(ar.begin(), ar.end(), 0);

    try
    {
        adios2::Engine Writer = io.Open("communicate", adios2::Mode::Write);

        {
            std::lock_guard<std::mutex> guard(StdOutMtx);
            std::cout << "Writer completed Open() " << std::endl;
        }
        Writer.BeginStep();
        Writer.Put<dt>(var, ar.begin());
        Writer.EndStep();
        Writer.Close();
        {
            std::lock_guard<std::mutex> guard(StdOutMtx);
            std::cout << "Writer completed Close() " << std::endl;
        }
    }
    catch (std::exception &e)
    {
        {
            std::lock_guard<std::mutex> guard(StdOutMtx);
            std::cout << "Writer: Exception: " << e.what() << std::endl;
        }
        return false;
    }
    return true;
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
    bool reader_success = read_fut.get();
    bool writer_success = write_fut.get();
    EXPECT_TRUE(reader_success);
    EXPECT_TRUE(writer_success);
    EXPECT_EQ(value_errors, 0)
        << "We got " << value_errors << " erroneous values at the reader";
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    ParseArgs(argc, argv);

    int result;
    result = RUN_ALL_TESTS();
    return result;
}
