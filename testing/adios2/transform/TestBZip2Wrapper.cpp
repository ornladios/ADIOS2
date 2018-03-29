#include <cstdint>
#include <iostream>
#include <numeric> //std::iota
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

class ADIOSBZip2Wrapper : public ::testing::Test
{
public:
    ADIOSBZip2Wrapper() : adios(true), io(adios.DeclareIO("TestADIOSBZip2")) {}

protected:
    adios2::ADIOS adios;
    adios2::IO &io;
};

TEST_F(ADIOSBZip2Wrapper, UInt100)
{
    /** Application variable uints from 0 to 1000 */
    std::vector<uint32_t> myUInts(100);
    std::iota(myUInts.begin(), myUInts.end(), 0.f);
    const std::size_t Nx = myUInts.size();
    const std::size_t inputBytes = Nx * sizeof(uint32_t);

    // Define ADIOS variable
    auto &var_UInt = io.DefineVariable<uint32_t>("myUInts", {}, {}, {Nx},
                                                 adios2::ConstantDims);

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(var_UInt),
                                  adios2::Variable<uint32_t> &>();

    // Define bzip2 transform
    adios2::Operator &adiosBZip2 =
        adios.DefineOperator("BZip2Compressor", "BZip2");

    const unsigned int bzip2ID =
        var_UInt.AddTransform(adiosBZip2, {{"BlockSize100K", "2"}});

    const std::size_t estimatedSize =
        adiosBZip2.BufferMaxSize(Nx * var_UInt.m_ElementSize);
    std::vector<char> compressedBuffer(estimatedSize);
    size_t compressedSize = adiosBZip2.Compress(
        myUInts.data(), var_UInt.m_Count, var_UInt.m_ElementSize,
        var_UInt.m_Type, compressedBuffer.data(),
        var_UInt.m_OperatorsInfo[bzip2ID].Parameters);

    EXPECT_LE(compressedSize, estimatedSize);

    compressedBuffer.resize(compressedSize);

    // Allocate original data size
    std::vector<uint32_t> decompressedBuffer(Nx);
    size_t decompressedSize = adiosBZip2.Decompress(
        compressedBuffer.data(), compressedSize, decompressedBuffer.data(),
        decompressedBuffer.size() * sizeof(uint32_t));
    ASSERT_EQ(decompressedSize, inputBytes);

    // testing data recovery
    for (size_t i = 0; i < Nx; ++i)
    {
        ASSERT_EQ(decompressedBuffer[i], myUInts[i]);
    }
}

TEST_F(ADIOSBZip2Wrapper, WrongParameterValue)
{
    /** Application variable uints from 0 to 1000 */
    std::vector<unsigned int> myUInts(100);
    std::iota(myUInts.begin(), myUInts.end(), 0.f);
    const std::size_t Nx = myUInts.size();

    // Define ADIOS variable
    auto &var_UInt = io.DefineVariable<unsigned int>("myUInts", {}, {}, {Nx},
                                                     adios2::ConstantDims);

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(var_UInt),
                                  adios2::Variable<unsigned int> &>();

    // Define bzip2 transform
    adios2::Operator &adiosBZip2 =
        adios.DefineOperator("BZip2Compressor", "BZip2");

    const unsigned int bzip2ID =
        var_UInt.AddTransform(adiosBZip2, {{"BlockSize100K", "10"}});

    const std::size_t estimatedSize =
        adiosBZip2.BufferMaxSize(Nx * var_UInt.m_ElementSize);
    std::vector<char> compressedBuffer(estimatedSize);

    EXPECT_THROW(size_t compressedSize = adiosBZip2.Compress(
                     myUInts.data(), var_UInt.m_Count, var_UInt.m_ElementSize,
                     var_UInt.m_Type, compressedBuffer.data(),
                     var_UInt.m_OperatorsInfo[bzip2ID].Parameters),
                 std::invalid_argument);
}

TEST_F(ADIOSBZip2Wrapper, WrongBZip2Name)
{
    /** Application variable uints from 0 to 1000 */
    std::vector<unsigned int> myUInts(100);
    std::iota(myUInts.begin(), myUInts.end(), 0.f);
    const std::size_t Nx = myUInts.size();
    const std::size_t inputBytes = Nx * sizeof(unsigned int);

    // Define ADIOS variable
    auto &var_UInt = io.DefineVariable<unsigned int>("myUInts", {}, {}, {Nx},
                                                     adios2::ConstantDims);

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(var_UInt),
                                  adios2::Variable<unsigned int> &>();

    // Check bzip2 lower case and camel case
    EXPECT_NO_THROW(adios2::Operator &adiosBZip2 =
                        adios.DefineOperator("BZip2Compressor1", "bzip2"));
    EXPECT_NO_THROW(adios2::Operator &adiosBZip2 =
                        adios.DefineOperator("BZip2Compressor2", "BZip2"));
    EXPECT_THROW(adios2::Operator &adiosBZip2 =
                     adios.DefineOperator("BZip2Compressor3", "bzip"),
                 std::invalid_argument);
    EXPECT_THROW(adios2::Operator &adiosBZip2 =
                     adios.DefineOperator("BZip2Compressor4", "BZIP2"),
                 std::invalid_argument);
}

int main(int argc, char **argv)
{
#ifdef ADIOS2_HAVE_MPI
    MPI_Init(&argc, &argv);
#endif

    int result = -1;
    try
    {
        ::testing::InitGoogleTest(&argc, argv);
        result = RUN_ALL_TESTS();
    }
    catch (std::exception &e)
    {
        result = 1;
    }

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return result;
}
