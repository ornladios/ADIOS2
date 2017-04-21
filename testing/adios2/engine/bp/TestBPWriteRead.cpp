#include <cstdint>

#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

class BPWriteValidationTest : public ::testing::Test
{
protected:
    // virtual void SetUp() { }

    // virtual void TearDown() { }

    adios::ADIOS adios;

    // Test data for each type.  Make sure our values exceed the range of the
    // previous size to make sure we all bytes for each element
    std::vector<char> m_DataInt8 = {0, 1, -2, 3, -4, 5, -6, 7, -8, 9};
    std::vector<int16_t> m_DataInt16 = {512, 513,  -510, 515,  -508,
                                        517, -506, 519,  -504, 521};
    std::vector<int32_t> m_DataInt32 = {131072,  131073, -131070, 131075,
                                        -131068, 131077, -131066, 131079,
                                        -131064, 131081};
    std::vector<int64_t> m_DataInt64 = {
        8589934592, 8589934593,  -8589934590, 8589934595,  -8589934588,
        8589934597, -8589934586, 8589934599,  -8589934584, 8589934601};
    std::vector<unsigned char> m_DataUInt8 = {128, 129, 130, 131, 132,
                                              133, 134, 135, 136, 137};
    std::vector<uint16_t> m_DataUInt16 = {32768, 32769, 32770, 32771, 32772,
                                          32773, 32774, 32775, 32776, 32777};
    std::vector<uint32_t> m_DataUInt32 = {
        2147483648, 2147483649, 2147483650, 2147483651, 2147483652,
        2147483653, 2147483654, 2147483655, 2147483656, 2147483657};
    std::vector<uint64_t> m_DataUInt64 = {
        9223372036854775808UL, 9223372036854775809UL, 9223372036854775810UL,
        9223372036854775811UL, 9223372036854775812UL, 9223372036854775813UL,
        9223372036854775814UL, 9223372036854775815UL, 9223372036854775816UL,
        9223372036854775817UL};
    std::vector<float> m_DataR32 = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::vector<double> m_DataR64 = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
};

TEST_F(BPWriteValidationTest, DefineVars1x10)
{
    // Define ADIOS variables for each type
    auto &var_i8_10 = adios.DefineVariable<char>("i8_10", adios::Dims{10});
    auto &var_i16_10 = adios.DefineVariable<int16_t>("i16_10", adios::Dims{10});
    auto &var_i32_10 = adios.DefineVariable<int32_t>("i32_10", adios::Dims{10});
    auto &var_i64_10 = adios.DefineVariable<int64_t>("i64_10", adios::Dims{10});
    auto &var_u8_10 =
        adios.DefineVariable<unsigned char>("u8_10", adios::Dims{10});
    auto &var_u16_10 =
        adios.DefineVariable<uint16_t>("u16_10", adios::Dims{10});
    auto &var_u32_10 =
        adios.DefineVariable<uint32_t>("u32_10", adios::Dims{10});
    auto &var_u64_10 =
        adios.DefineVariable<uint64_t>("u64_10", adios::Dims{10});
    auto &var_r32_10 = adios.DefineVariable<float>("r32_10", adios::Dims{10});
    auto &var_r64_10 = adios.DefineVariable<double>("r64_10", adios::Dims{10});
}
