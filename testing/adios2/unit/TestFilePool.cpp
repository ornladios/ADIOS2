#include "adios2/common/ADIOSConfig.h"
#include "adios2/core/CoreTypes.h"
#include "adios2/core/IO.h"
#include "adios2/helper/adiosCommDummy.h"
#include "adios2/toolkit/filepool/FilePool.h"
#include "adios2/toolkit/remote/Remote.h"
#include "adios2/toolkit/transportman/TransportMan.h"
#include <adios2.h>
#include <adios2/helper/adiosType.h>
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <random>
#ifndef _MSC_VER
#include <unistd.h>
#else
#include <windows.h>
#define unlink _unlink
#endif

#include <vector>

namespace adios2
{
namespace helper
{

std::string filename(size_t count, std::string prefix)
{
    return "Test_output" + std::to_string(count) + prefix;
}
void create_test_files(size_t count, std::string prefix)
{
    for (size_t i = 0; i < count; i++)
    {
        std::ofstream outputFile(filename(i, prefix), std::ios::binary);
        size_t array[100];
        std::iota(std::begin(array), std::end(array), i * 1000);
        outputFile.write((char *)&array[0], sizeof(array));
        outputFile.close();
    }
}

void remove_test_files(size_t count, std::string prefix)
{
    for (size_t i = 0; i < count; i++)
    {
        unlink(filename(i, prefix).c_str());
    }
}

TEST(FilePool, FileLimit)
{
    core::ADIOS adios("C++");
    core::IO io(adios, "name", false, "C++");
    helper::Comm comm = adios2::helper::CommDummy();

    adios2::transportman::TransportMan factory(io, comm);
    std::string prefix = "FileLimit";

    create_test_files(10, prefix);

    {
        // Check that reuse works for POSIX where ReentrantRead is true
        FilePool pool(&factory, {{"library", "posix"}}, 10);
        auto handle0 = pool.Acquire(filename(0, prefix));
        auto handle1 = pool.Acquire(filename(0, prefix));
        auto handle2 = pool.Acquire(filename(0, prefix));
        auto handle3 = pool.Acquire(filename(1, prefix));
        auto handle4 = pool.Acquire(filename(1, prefix));
        auto handle5 = pool.Acquire(filename(1, prefix));
        EXPECT_EQ(2, pool.GetMax());
    }
    {
        // Mostly not the same file
        FilePool pool(&factory, {{"library", "posix"}}, 10);
        auto handle0 = pool.Acquire(filename(0, prefix));
        auto handle1 = pool.Acquire(filename(1, prefix));
        auto handle2 = pool.Acquire(filename(2, prefix));
        auto handle3 = pool.Acquire(filename(3, prefix));
        auto handle4 = pool.Acquire(filename(4, prefix));
        auto handle5 = pool.Acquire(filename(0, prefix));
        EXPECT_EQ(5, pool.GetMax());
    }
    {
        // Files are considered destroyable when they go out of scope
        FilePool pool(&factory, {{"library", "posix"}}, 3);
        {
            auto handle0 = pool.Acquire(filename(0, prefix));
            auto handle1 = pool.Acquire(filename(1, prefix));
            auto handle2 = pool.Acquire(filename(2, prefix));
        }
        {
            auto handle3 = pool.Acquire(filename(3, prefix));
            auto handle4 = pool.Acquire(filename(4, prefix));
            auto handle5 = pool.Acquire(filename(5, prefix));
        }
        EXPECT_EQ(3, pool.GetMax());
    }
    {
        // We get an exception of we try to open more concurrent files than allowed
        FilePool pool(&factory, {{"library", "posix"}}, 3);
        {
            auto handle0 = pool.Acquire(filename(0, prefix));
            auto handle1 = pool.Acquire(filename(1, prefix));
            auto handle2 = pool.Acquire(filename(2, prefix));
            std::unique_ptr<PoolableFile> handle3 = nullptr;
            EXPECT_THROW(handle3 = pool.Acquire(filename(3, prefix)), std::runtime_error);
            EXPECT_THROW(auto handle4 = pool.Acquire(filename(4, prefix)), std::runtime_error);
        }
    }
    {
        // stdio does not allow reuse, check results
        FilePool pool(&factory, {{"library", "stdio"}}, 10);
        auto handle0 = pool.Acquire(filename(0, prefix));
        auto handle1 = pool.Acquire(filename(0, prefix));
        auto handle2 = pool.Acquire(filename(0, prefix));
        auto handle3 = pool.Acquire(filename(1, prefix));
        auto handle4 = pool.Acquire(filename(1, prefix));
        auto handle5 = pool.Acquire(filename(1, prefix));
        EXPECT_EQ(6, pool.GetMax());
    }
    {
        // stdio does not allow reuse, check results when some go out of scope
        FilePool pool(&factory, {{"library", "stdio"}}, 3);
        {
            auto handle0 = pool.Acquire(filename(0, prefix));
            auto handle1 = pool.Acquire(filename(1, prefix));
            auto handle2 = pool.Acquire(filename(2, prefix));
        }
        {
            auto handle3 = pool.Acquire(filename(3, prefix));
            auto handle4 = pool.Acquire(filename(4, prefix));
            auto handle5 = pool.Acquire(filename(5, prefix));
            EXPECT_THROW(auto handle6 = pool.Acquire(filename(5, prefix)), std::runtime_error);
        }
        EXPECT_EQ(3, pool.GetMax());
    }
    remove_test_files(10, prefix);
}

class FilePoolTest : public ::testing::TestWithParam<std::string>
{
    // You can add SetUp, TearDown, or shared objects here if needed
};

TEST_P(FilePoolTest, SimpleRead)
{
    std::string param = GetParam(); // Access the current parameter, which is the transport to use
    core::ADIOS adios("C++");
    core::IO io(adios, "name", false, "C++");
    helper::Comm comm = adios2::helper::CommDummy();

    adios2::transportman::TransportMan factory(io, comm);
    std::string prefix = "SimpleRead";
    FilePool pool(&factory, {{"library", param}}, 1024);
    const int file_count = 1;

    create_test_files(file_count, prefix);

    auto file0 = pool.Acquire(filename(0, prefix));
    size_t file0_result;
    size_t i;
    for (i = 20; i < 60; i++)
    {
        file0->Read((char *)&file0_result, sizeof(size_t), i * sizeof(size_t));
        EXPECT_EQ(i, file0_result);
    }
    for (i = 20; i < 60; i++)
    {
        auto file1 = pool.Acquire(filename(0, prefix));
        size_t file1_result;
        file1->Read((char *)&file1_result, sizeof(size_t), i * sizeof(size_t));
        EXPECT_EQ(i, file1_result);
    }
    remove_test_files(file_count, prefix);
}

TEST_P(FilePoolTest, ConcurrentRead)
{
    std::string param = GetParam(); // Access the current parameter, which is the transport to use
    core::ADIOS adios("C++");
    core::IO io(adios, "name", false, "C++");
    helper::Comm comm = adios2::helper::CommDummy();

    adios2::transportman::TransportMan factory(io, comm);
    std::string prefix = "ConcurrentRead-" + param;
    FilePool pool(&factory, {{"library", param}}, 1024);
    const int file_count = 5;

    create_test_files(file_count, prefix);

    auto thread_body = [&](int n) {
        auto start = std::chrono::high_resolution_clock::now();
        std::chrono::milliseconds duration;
        std::random_device rd;
        std::mt19937 rng(rd());
        std::uniform_int_distribution<int> uni(0, file_count - 1);
        int file0_num = uni(rng);
        int file1_num = uni(rng);
#ifdef ThreadUnsafeVerbosity
        std::cout << "thread " << n << " running, will read from files "
                  << std::to_string(file0_num) + " and " << std::to_string(file1_num)
                  << " using transport " << param << std::endl;
#endif
        do
        {
            auto file0 = pool.Acquire(filename(file0_num, prefix));
            auto file1 = pool.Acquire(filename(file1_num, prefix));
            auto now = std::chrono::high_resolution_clock::now();
            std::uniform_int_distribution<int> elementuni(0, 99);
            size_t file0_element = elementuni(rng);
            size_t file1_element = elementuni(rng);
            size_t file0_result;
            size_t file1_result;
            file0->Read((char *)&file0_result, sizeof(size_t), file0_element * sizeof(size_t));
            file1->Read((char *)&file1_result, sizeof(size_t), file1_element * sizeof(size_t));
            EXPECT_EQ(file0_element + file0_num * 1000, file0_result);
            EXPECT_EQ(file1_element + file1_num * 1000, file1_result);
            duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        } while (duration < std::chrono::milliseconds(1000));
    };
    std::thread myThreads[4];

    for (int i = 0; i < 4; i++)
    {
        myThreads[i] = std::thread(thread_body, i);
    }
    for (int i = 0; i < 4; i++)
    {
        myThreads[i].join();
    }
    remove_test_files(file_count, prefix);
}

INSTANTIATE_TEST_SUITE_P(FullTransportTest, FilePoolTest, ::testing::Values("posix", "stdio"));

}
}

int main(int argc, char **argv)
{

    int result;
    ::testing::InitGoogleTest(&argc, argv);
    result = RUN_ALL_TESTS();

    return result;
}
