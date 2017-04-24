#include <cstdint>

#include <array>
#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

class EngineWriteReadTestBase : public ::testing::Test
{
public:
    EngineWriteReadTestBase(std::string engineName, std::string fileName);

protected:
    std::unique_ptr<adios::ADIOS> m_adios;
    std::string m_EngineName;
    std::string m_FileName;

    // To be created by child class SetUp()
    adios::Method *m_Method;
    std::shared_ptr<adios::Engine> m_Engine;

    // Handle MPI things if necessary
    static void SetUpTestCase();
    static void TearDownTestCase();

    // Create and destroy the manager class
    virtual void SetUp();
    virtual void TearDown();

    virtual void SetUpWrite();
    virtual void TearDownWrite();

    // Shared test functions
    virtual void Declare() = 0;
    void WriteStep(size_t step);
    void Write3Steps();
    void WriteRead();

    //*************************************************************************
    // Test Data
    //*************************************************************************

    // Test data for each type.  Make sure our values exceed the range of the
    // previous size to make sure we all bytes for each element
    std::array<char, 10> m_DataI8 = {{0, 1, -2, 3, -4, 5, -6, 7, -8, 9}};
    std::array<int16_t, 10> m_DataI16 = {
        {512, 513, -510, 515, -508, 517, -506, 519, -504, 521}};
    std::array<int32_t, 10> m_DataI32 = {{131072, 131073, -131070, 131075,
                                          -131068, 131077, -131066, 131079,
                                          -131064, 131081}};
    std::array<int64_t, 10> m_DataI64 = {
        {8589934592, 8589934593, -8589934590, 8589934595, -8589934588,
         8589934597, -8589934586, 8589934599, -8589934584, 8589934601}};
    std::array<unsigned char, 10> m_DataU8 = {
        {128, 129, 130, 131, 132, 133, 134, 135, 136, 137}};
    std::array<uint16_t, 10> m_DataU16 = {
        {32768, 32769, 32770, 32771, 32772, 32773, 32774, 32775, 32776, 32777}};
    std::array<uint32_t, 10> m_DataU32 = {
        {2147483648, 2147483649, 2147483650, 2147483651, 2147483652, 2147483653,
         2147483654, 2147483655, 2147483656, 2147483657}};
    std::array<uint64_t, 10> m_DataU64 = {
        {9223372036854775808UL, 9223372036854775809UL, 9223372036854775810UL,
         9223372036854775811UL, 9223372036854775812UL, 9223372036854775813UL,
         9223372036854775814UL, 9223372036854775815UL, 9223372036854775816UL,
         9223372036854775817UL}};
    std::array<float, 10> m_DataR32 = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}};
    std::array<double, 10> m_DataR64 = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}};
};

class EngineWriteRead1DTest : public EngineWriteReadTestBase
{
public:
    EngineWriteRead1DTest(std::string engineName, std::string fileName)
    : EngineWriteReadTestBase(engineName, fileName)
    {
    }

protected:
    virtual void Declare();
};

class EngineWriteRead2D2x4Test : public EngineWriteReadTestBase
{
public:
    EngineWriteRead2D2x4Test(std::string engineName, std::string fileName)
    : EngineWriteReadTestBase(engineName, fileName)
    {
    }

protected:
    virtual void Declare();
};

class EngineWriteRead2D4x2Test : public EngineWriteReadTestBase
{
public:
    EngineWriteRead2D4x2Test(std::string engineName, std::string fileName)
    : EngineWriteReadTestBase(engineName, fileName)
    {
    }

protected:
    virtual void Declare();
};
