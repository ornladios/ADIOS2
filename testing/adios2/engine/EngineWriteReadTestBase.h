#include <cstdint>

#include <array>
#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

class EngineWriteReadTestBase : public ::testing::Test
{
public:
    EngineWriteReadTestBase();

protected:
    adios::ADIOS m_adios;

    // To be created by child class SetUp()
    std::shared_ptr<adios::Engine> m_Engine;

    // Shared test functions
    void Declare1D_8();
    void Declare2D_2x4();
    void Declare2D_4x2();

    virtual void OpenWrite(std::string fname) = 0;

    void WriteStep(size_t step);

    void OpenWriteClose(std::string fname);

    //*************************************************************************
    // Test Data
    //*************************************************************************

    // Test data for each type.  Make sure our values exceed the range of the
    // previous size to make sure we all bytes for each element
    std::array<char, 10> m_DataI8 = {0, 1, -2, 3, -4, 5, -6, 7, -8, 9};
    std::array<int16_t, 10> m_DataI16 = {512, 513,  -510, 515,  -508,
                                         517, -506, 519,  -504, 521};
    std::array<int32_t, 10> m_DataI32 = {131072,  131073, -131070, 131075,
                                         -131068, 131077, -131066, 131079,
                                         -131064, 131081};
    std::array<int64_t, 10> m_DataI64 = {
        8589934592, 8589934593,  -8589934590, 8589934595,  -8589934588,
        8589934597, -8589934586, 8589934599,  -8589934584, 8589934601};
    std::array<unsigned char, 10> m_DataU8 = {128, 129, 130, 131, 132,
                                              133, 134, 135, 136, 137};
    std::array<uint16_t, 10> m_DataU16 = {32768, 32769, 32770, 32771, 32772,
                                          32773, 32774, 32775, 32776, 32777};
    std::array<uint32_t, 10> m_DataU32 = {
        2147483648, 2147483649, 2147483650, 2147483651, 2147483652,
        2147483653, 2147483654, 2147483655, 2147483656, 2147483657};
    std::array<uint64_t, 10> m_DataU64 = {
        9223372036854775808UL, 9223372036854775809UL, 9223372036854775810UL,
        9223372036854775811UL, 9223372036854775812UL, 9223372036854775813UL,
        9223372036854775814UL, 9223372036854775815UL, 9223372036854775816UL,
        9223372036854775817UL};
    std::array<float, 10> m_DataR32 = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::array<double, 10> m_DataR64 = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
};
