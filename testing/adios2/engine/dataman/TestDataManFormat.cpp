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
    std::cout << std::endl;
}

TEST_F(DataManFormatTest, 1D_MultiStepRowMajorLittleEndian)
{
    Dims shape = {10};
    Dims start = {0};
    Dims count = {10};
    Params params;

    size_t datasize = std::accumulate(count.begin(), count.end(), 1,
                                      std::multiplies<size_t>());

    format::DataManSerializer s(true, true);

    for (int i = 0; i < 100; ++i)
    {
        std::vector<float> in_float(datasize);
        GenData(in_float, i);
        s.Put(in_float.data(), "float", shape, start, count, "Test", i, 0,
              params);
    }

    auto buffer = s.Get();

    format::DataManDeserializer d(true, true);

    d.Put(buffer);

    for (int i = 0; i < 100; ++i)
    {
        std::vector<float> out_float(datasize);
        d.Get(out_float.data(), "float", start, count, i);
        VerifyData(out_float, i);
    }
}

int main(int argc, char **argv)
{
    int result;
    ::testing::InitGoogleTest(&argc, argv);
    result = RUN_ALL_TESTS();
    return result;
}
