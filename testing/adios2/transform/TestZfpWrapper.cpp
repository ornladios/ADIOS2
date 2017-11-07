#include <cmath> //std::abs
#include <iostream>
#include <numeric> //std::iota
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

class ADIOSZfpWrapper : public ::testing::Test
{
public:
    ADIOSZfpWrapper() : adios(true), io(adios.DeclareIO("TestADIOSZfp")) {}

protected:
    adios2::ADIOS adios;
    adios2::IO &io;
};

TEST_F(ADIOSZfpWrapper, Float100)
{
    /** Application variable uints from 0 to 1000 */
    std::vector<float> myFloats(100);
    std::iota(myFloats.begin(), myFloats.end(), 0.f);
    const std::size_t Nx = myFloats.size();
    const std::size_t inputBytes = Nx * sizeof(float);

    // Define ADIOS variable
    auto &var_Floats = io.DefineVariable<float>("myFloats", {}, {}, {Nx},
                                                adios2::ConstantDims);

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(var_Floats),
                                  adios2::Variable<float> &>();

    // Define bzip2 transform
    adios2::Operator &adiosZfp = adios.DefineOperator("ZfpCompressor", "Zfp");

    const unsigned int zfpID =
        var_Floats.AddTransform(adiosZfp, {{"Rate", "8"}});

    const std::size_t estimatedSize =
        adiosZfp.BufferMaxSize(myFloats.data(), var_Floats.m_Count,
                               var_Floats.m_OperatorsInfo[zfpID].Parameters);

    std::vector<char> compressedBuffer(estimatedSize);

    size_t compressedSize = adiosZfp.Compress(
        myFloats.data(), var_Floats.m_Count, var_Floats.m_ElementSize,
        var_Floats.m_Type, compressedBuffer.data(),
        var_Floats.m_OperatorsInfo[zfpID].Parameters);

    compressedBuffer.resize(compressedSize);

    // Allocate original data size
    std::vector<float> decompressedBuffer(Nx);

    size_t decompressedSize = adiosZfp.Decompress(
        compressedBuffer.data(), compressedBuffer.size(),
        decompressedBuffer.data(), var_Floats.m_Count, var_Floats.m_Type,
        var_Floats.m_OperatorsInfo[zfpID].Parameters);

    // testing data recovery for rate = 8
    for (size_t i = 0; i < Nx; ++i)
    {
        ASSERT_LT(std::abs(decompressedBuffer[i] - myFloats[i]), 1E-6);
    }
}

TEST_F(ADIOSZfpWrapper, UnsupportedCall)
{
    /** Application variable uints from 0 to 1000 */
    std::vector<float> myFloats(100);
    std::iota(myFloats.begin(), myFloats.end(), 0.f);
    const std::size_t Nx = myFloats.size();
    const std::size_t inputBytes = Nx * sizeof(float);

    // Define ADIOS variable
    auto &var_Floats = io.DefineVariable<float>("myFloats", {}, {}, {Nx},
                                                adios2::ConstantDims);

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(var_Floats),
                                  adios2::Variable<float> &>();

    // Define bzip2 transform
    adios2::Operator &adiosZfp = adios.DefineOperator("ZfpCompressor", "zfp");

    const unsigned int zfpID =
        var_Floats.AddTransform(adiosZfp, {{"Rate", "8"}});

    // Wrong signature for Zfp
    EXPECT_THROW(const std::size_t estimatedSize =
                     adiosZfp.BufferMaxSize(Nx * var_Floats.m_ElementSize),
                 std::invalid_argument);
}

TEST_F(ADIOSZfpWrapper, MissingMandatoryParameter)
{
    /** Application variable uints from 0 to 1000 */
    std::vector<float> myFloats(100);
    std::iota(myFloats.begin(), myFloats.end(), 0.f);
    const std::size_t Nx = myFloats.size();
    const std::size_t inputBytes = Nx * sizeof(float);

    // Define ADIOS variable
    auto &var_Floats = io.DefineVariable<float>("myFloats", {}, {}, {Nx},
                                                adios2::ConstantDims);

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(var_Floats),
                                  adios2::Variable<float> &>();

    // Define bzip2 transform
    adios2::Operator &adiosZfp = adios.DefineOperator("ZFPCompressor", "zfp");

    const unsigned int zfpID = var_Floats.AddTransform(adiosZfp);

    EXPECT_THROW(const std::size_t estimatedSize = adiosZfp.BufferMaxSize(
                     myFloats.data(), var_Floats.m_Count,
                     var_Floats.m_OperatorsInfo[zfpID].Parameters),
                 std::invalid_argument);
}
