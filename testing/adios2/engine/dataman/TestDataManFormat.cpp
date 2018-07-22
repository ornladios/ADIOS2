/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TestDataMan.cpp
 *
 *  Created on: Jul 12, 2018
 *      Author: Jason Wang
 */

#include <numeric>

#include <adios2.h>
#include <gtest/gtest.h>

#include "adios2/core/Variable.h"
#include "adios2/toolkit/format/dataman/DataManDeserializer.tcc"
#include "adios2/toolkit/format/dataman/DataManSerializer.tcc"

using namespace adios2;

class DataManFormatTest : public ::testing::Test
{
public:
    DataManFormatTest() = default;
};

template <class T>
void GenData(std::vector<T> &data, const size_t step)
{
    for (size_t i = 0; i < data.size(); ++i)
    {
        data[i] = i + step * 10000;
    }
}

template <class T>
void PrintData(std::vector<T> &data, size_t step)
{
    std::cout << "Rank: "
              << " Step: " << step << " [";
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

void DataManSerialize(Dims shape, Dims start, Dims count, size_t steps,
                      std::string workflowMode, std::string transport)
{
}

void DataManDeserialize(Dims shape, Dims start, Dims count, size_t steps,
                        std::string workflowMode, std::string transport)
{
}

TEST_F(DataManFormatTest, WriteRead_1D_Subscribe_ZeroMQ)
{
    Dims shape = {10};
    Dims start = {0};
    Dims count = {10};

    size_t datasize = std::accumulate(count.begin(), count.end(), 1,
                                      std::multiplies<size_t>());

    size_t steps = 200;
    std::string mode = "subscribe";
    std::string transport = "ZMQ";
    DataManSerialize(shape, start, count, steps, mode, transport);
    DataManDeserialize(shape, start, count, steps, mode, transport);
}

int main(int argc, char **argv)
{
    int result;
    ::testing::InitGoogleTest(&argc, argv);
    result = RUN_ALL_TESTS();
    return result;
}
