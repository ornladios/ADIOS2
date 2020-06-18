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

bool reader_failure = false;
bool writer_failure = false;
int value_errors = 0;

void Read()
{
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("IO");

    std::cout << "Reader: engine = " << engine << std::endl;
    io.SetEngine(engine);
    io.SetParameters(engineParams);

    std::cout << "Reader: call Open" << std::endl;

    try
    {
        adios2::Engine Reader = io.Open("communicate", adios2::Mode::Read);
        std::cout << "Reader: passed Open" << std::endl;
        std::array<dt, 100000> ar;

        auto status = Reader.BeginStep();
        std::cout << "Reader: passed BeginStep";
        if (status == adios2::StepStatus::EndOfStream)
        {
            std::cout << " with EndOfStream " << std::endl;
            reader_failure = true;
            return;
        }
        std::cout << " with success " << std::endl;

        adios2::Variable<dt> var = io.InquireVariable<dt>("data");
        Reader.Get(var, ar.begin());
        Reader.EndStep();
        dt expect = 0;
        for (auto &val : ar)
        {
            if (val != expect)
            {
                value_errors++;
                reader_failure = true;
            }
            expect++;
        }

        Reader.Close();
        std::cout << " Reader got " << expect << " values, " << reader_failure
                  << " were incorrect" << std::endl;
    }
    catch (std::exception &e)
    {
        std::cout << "Reader: Exception: " << e.what() << std::endl;
        reader_failure = true;
        return;
    }
}

void Write()
{
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("IO");
    io.SetEngine(engine);
    io.SetParameters(engineParams);
    std::cout << "Writer: engine = " << engine << std::endl;
    auto var =
        io.DefineVariable<dt>("data", adios2::Dims{10000, 10},
                              adios2::Dims{0, 0}, adios2::Dims{10000, 10});

    std::array<dt, 100000> ar;
    std::iota(ar.begin(), ar.end(), 0);

    try
    {
        adios2::Engine Writer = io.Open("communicate", adios2::Mode::Write);

        Writer.BeginStep();
        Writer.Put<dt>(var, ar.begin());
        Writer.EndStep();
        Writer.Close();
    }
    catch (std::exception &e)
    {
        std::cout << "Writer: Exception: " << e.what() << std::endl;
        writer_failure = true;
        return;
    }
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
    if (reader_failure)
        std::cout << "Reader FAIL" << std::endl;
    if (writer_failure)
        std::cout << "Writer FAIL" << std::endl;
    if (value_errors > 0)
        std::cout << "Number of values not matching after reading = "
                  << value_errors << std::endl;
    EXPECT_FALSE(reader_failure);
    EXPECT_FALSE(writer_failure);
    EXPECT_TRUE(value_errors == 0);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    ParseArgs(argc, argv);

    int result;
    result = RUN_ALL_TESTS();
    return result;
}
