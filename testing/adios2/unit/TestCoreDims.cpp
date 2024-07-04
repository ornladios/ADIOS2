#include <adios2/helper/adiosType.h>
#include <algorithm>
#include <iostream>
#include <vector>

#include <gtest/gtest.h>

namespace adios2
{
namespace helper
{

// Assuming CoreDims and DimsArray definitions are included here
using namespace adios2::helper;

TEST(CoreDims, Basic)
{
    // Using CoreDims with a vector
    std::vector<size_t> vec = {1, 2, 3, 4, 5};
    CoreDims coreDims(vec);
    std::cout << "CoreDims from vector: " << coreDims << std::endl;

    // Using DimsArray with different constructors
    DimsArray dims1(5); // Initialize with no values
    std::cout << "DimsArray with no values: " << dims1 << std::endl;
    ASSERT_EQ(dims1.size(), 5);

    DimsArray dims2(5, 42); // Initialize with a single value
    std::cout << "DimsArray with single value: " << dims2 << std::endl;
    ASSERT_EQ(dims2.size(), 5);
    ASSERT_EQ(dims2[0], 42);
    ASSERT_EQ(dims2[1], 42);
    ASSERT_EQ(dims2[2], 42);
    ASSERT_EQ(dims2[3], 42);
    ASSERT_EQ(dims2[4], 42);

    DimsArray dims3(vec); // Initialize from vector
    std::cout << "DimsArray from vector: " << dims3 << std::endl;
    ASSERT_EQ(dims3.size(), 5);
    ASSERT_EQ(dims3[0], 1);
    ASSERT_EQ(dims3[1], 2);
    ASSERT_EQ(dims3[2], 3);
    ASSERT_EQ(dims3[3], 4);
    ASSERT_EQ(dims3[4], 5);

    size_t arr[] = {10, 20, 30, 40, 50};
    DimsArray dims4(5, arr); // Initialize from array
    std::cout << "DimsArray from array: " << dims4 << std::endl;
    ASSERT_EQ(dims4.size(), 5);
    ASSERT_EQ(dims4[0], 10);
    ASSERT_EQ(dims4[1], 20);
    ASSERT_EQ(dims4[2], 30);
    ASSERT_EQ(dims4[3], 40);
    ASSERT_EQ(dims4[4], 50);

    // Copying from CoreDims
    DimsArray dims5(coreDims);
    std::cout << "DimsArray copied from CoreDims: " << dims5 << std::endl;
    ASSERT_EQ(dims5.size(), 5);
    ASSERT_EQ(dims5[0], 1);
    ASSERT_EQ(dims5[1], 2);
    ASSERT_EQ(dims5[2], 3);
    ASSERT_EQ(dims5[3], 4);
    ASSERT_EQ(dims5[4], 5);

    DimsArray dims6(dims4);
    ASSERT_EQ(dims6.size(), 5);
    ASSERT_EQ(dims6[0], 10);
    ASSERT_EQ(dims6[1], 20);
    ASSERT_EQ(dims6[2], 30);
    ASSERT_EQ(dims6[3], 40);
    ASSERT_EQ(dims6[4], 50);

    std::cout << "DimsArray copied from dims4: " << dims4 << std::endl;

    dims6[1] = 100;
    std::cout << "dims4: " << dims4 << std::endl;
    std::cout << "dims6: " << dims6 << std::endl;
    ASSERT_EQ(dims6.size(), 5);
    ASSERT_EQ(dims6[0], 10);
    ASSERT_EQ(dims6[1], 100);
    ASSERT_EQ(dims6[2], 30);
    ASSERT_EQ(dims6[3], 40);
    ASSERT_EQ(dims6[4], 50);

    // dims4 should be unchanged
    ASSERT_EQ(dims4.size(), 5);
    ASSERT_EQ(dims4[0], 10);
    ASSERT_EQ(dims4[1], 20);
    ASSERT_EQ(dims4[2], 30);
    ASSERT_EQ(dims4[3], 40);
    ASSERT_EQ(dims4[4], 50);
}
}
}

int main(int argc, char **argv)
{

    int result;
    ::testing::InitGoogleTest(&argc, argv);
    result = RUN_ALL_TESTS();

    return result;
}
