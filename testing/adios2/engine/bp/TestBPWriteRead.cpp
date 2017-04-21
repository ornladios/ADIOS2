#include <cstdint>

#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

class BPWriteReadTest : public ::testing::Test
{
public:
    BPWriteReadTest() : adios(adios::Verbose::WARN, true) {}

protected:
    virtual void SetUp() {}

    virtual void TearDown() {}

    adios::ADIOS adios;

    // Test data for each type.  Make sure our values exceed the range of the
    // previous size to make sure we all bytes for each element
    std::vector<char> m_DataI8 = {0, 1, -2, 3, -4, 5, -6, 7, -8, 9};
    std::vector<int16_t> m_DataI16 = {512, 513,  -510, 515,  -508,
                                      517, -506, 519,  -504, 521};
    std::vector<int32_t> m_DataI32 = {131072,  131073, -131070, 131075,
                                      -131068, 131077, -131066, 131079,
                                      -131064, 131081};
    std::vector<int64_t> m_DataI64 = {
        8589934592, 8589934593,  -8589934590, 8589934595,  -8589934588,
        8589934597, -8589934586, 8589934599,  -8589934584, 8589934601};
    std::vector<unsigned char> m_DataU8 = {128, 129, 130, 131, 132,
                                           133, 134, 135, 136, 137};
    std::vector<uint16_t> m_DataU16 = {32768, 32769, 32770, 32771, 32772,
                                       32773, 32774, 32775, 32776, 32777};
    std::vector<uint32_t> m_DataU32 = {
        2147483648, 2147483649, 2147483650, 2147483651, 2147483652,
        2147483653, 2147483654, 2147483655, 2147483656, 2147483657};
    std::vector<uint64_t> m_DataU64 = {
        9223372036854775808UL, 9223372036854775809UL, 9223372036854775810UL,
        9223372036854775811UL, 9223372036854775812UL, 9223372036854775813UL,
        9223372036854775814UL, 9223372036854775815UL, 9223372036854775816UL,
        9223372036854775817UL};
    std::vector<float> m_DataR32 = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::vector<double> m_DataR64 = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
};

TEST_F(BPWriteReadTest, Write1D)
{
    auto &var_i8 = adios.DefineVariable<char>("i8", adios::Dims{8});
    auto &var_i16 = adios.DefineVariable<short>("i16", adios::Dims{8});
    auto &var_i32 = adios.DefineVariable<int>("i32", adios::Dims{8});
    auto &var_i64 = adios.DefineVariable<long>("i64", adios::Dims{8});
    auto &var_u8 = adios.DefineVariable<unsigned char>("u8", adios::Dims{8});
    auto &var_u16 = adios.DefineVariable<unsigned short>("u16", adios::Dims{8});
    auto &var_u32 = adios.DefineVariable<unsigned int>("u32", adios::Dims{8});
    auto &var_u64 = adios.DefineVariable<unsigned long>("u64", adios::Dims{8});
    auto &var_r32 = adios.DefineVariable<float>("r32", adios::Dims{8});
    auto &var_r64 = adios.DefineVariable<double>("r64", adios::Dims{8});

    auto &bpWriterSettings = adios.DeclareMethod("SingleFile");
    bpWriterSettings.SetParameters("profile_units=mus");
    bpWriterSettings.AddTransport("File", "profile_units=mus",
                                  "have_metadata_file=no");

    auto bpWriter = adios.Open("BPWriteReadTest1D.bp", "w", bpWriterSettings);
    ASSERT_NE(bpWriter, nullptr);

    for (size_t t = 0; t < 3; ++t)
    {
        bpWriter->Write(var_i8, m_DataI8.data() + t);
        bpWriter->Write(var_i16, m_DataI16.data() + t);
        bpWriter->Write(var_i32, m_DataI32.data() + t);
        bpWriter->Write(var_i64, m_DataI64.data() + t);
        bpWriter->Write(var_u8, m_DataU8.data() + t);
        bpWriter->Write(var_u16, m_DataU16.data() + t);
        bpWriter->Write(var_u32, m_DataU32.data() + t);
        bpWriter->Write(var_u64, m_DataU64.data() + t);
        bpWriter->Write(var_r32, m_DataR32.data() + t);
        bpWriter->Write(var_r64, m_DataR64.data() + t);
        bpWriter->Advance();
    }

    bpWriter->Close();
}

TEST_F(BPWriteReadTest, Write2D_2x4)
{
    auto &var_i8 = adios.DefineVariable<char>("i8", adios::Dims{2, 4});
    auto &var_i16 = adios.DefineVariable<short>("i16", adios::Dims{2, 4});
    auto &var_i32 = adios.DefineVariable<int>("i32", adios::Dims{2, 4});
    auto &var_i64 = adios.DefineVariable<long>("i64", adios::Dims{2, 4});
    auto &var_u8 = adios.DefineVariable<unsigned char>("u8", adios::Dims{2, 4});
    auto &var_u16 =
        adios.DefineVariable<unsigned short>("u16", adios::Dims{2, 4});
    auto &var_u32 =
        adios.DefineVariable<unsigned int>("u32", adios::Dims{2, 4});
    auto &var_u64 =
        adios.DefineVariable<unsigned long>("u64", adios::Dims{2, 4});
    auto &var_r32 = adios.DefineVariable<float>("r32", adios::Dims{2, 4});
    auto &var_r64 = adios.DefineVariable<double>("r64", adios::Dims{2, 4});

    auto &bpWriterSettings = adios.DeclareMethod("SingleFile");
    bpWriterSettings.SetParameters("profile_units=mus");
    bpWriterSettings.AddTransport("File", "profile_units=mus",
                                  "have_metadata_file=no");

    auto bpWriter = adios.Open("BPWriteReadTest1D.bp", "w", bpWriterSettings);
    ASSERT_NE(bpWriter, nullptr);

    for (size_t t = 0; t < 3; ++t)
    {
        bpWriter->Write(var_i8, m_DataI8.data() + t);
        bpWriter->Write(var_i16, m_DataI16.data() + t);
        bpWriter->Write(var_i32, m_DataI32.data() + t);
        bpWriter->Write(var_i64, m_DataI64.data() + t);
        bpWriter->Write(var_u8, m_DataU8.data() + t);
        bpWriter->Write(var_u16, m_DataU16.data() + t);
        bpWriter->Write(var_u32, m_DataU32.data() + t);
        bpWriter->Write(var_u64, m_DataU64.data() + t);
        bpWriter->Write(var_r32, m_DataR32.data() + t);
        bpWriter->Write(var_r64, m_DataR64.data() + t);
        bpWriter->Advance();
    }

    bpWriter->Close();
}

TEST_F(BPWriteReadTest, Write2D_4x2)
{
    auto &var_i8 = adios.DefineVariable<char>("i8", adios::Dims{4, 2});
    auto &var_i16 = adios.DefineVariable<short>("i16", adios::Dims{4, 2});
    auto &var_i32 = adios.DefineVariable<int>("i32", adios::Dims{4, 2});
    auto &var_i64 = adios.DefineVariable<long>("i64", adios::Dims{4, 2});
    auto &var_u8 = adios.DefineVariable<unsigned char>("u8", adios::Dims{4, 2});
    auto &var_u16 =
        adios.DefineVariable<unsigned short>("u16", adios::Dims{4, 2});
    auto &var_u32 =
        adios.DefineVariable<unsigned int>("u32", adios::Dims{4, 2});
    auto &var_u64 =
        adios.DefineVariable<unsigned long>("u64", adios::Dims{4, 2});
    auto &var_r32 = adios.DefineVariable<float>("r32", adios::Dims{4, 2});
    auto &var_r64 = adios.DefineVariable<double>("r64", adios::Dims{4, 2});

    auto &bpWriterSettings = adios.DeclareMethod("SingleFile");
    bpWriterSettings.SetParameters("profile_units=mus");
    bpWriterSettings.AddTransport("File", "profile_units=mus",
                                  "have_metadata_file=no");

    auto bpWriter = adios.Open("BPWriteReadTest1D.bp", "w", bpWriterSettings);
    ASSERT_NE(bpWriter, nullptr);

    for (size_t t = 0; t < 3; ++t)
    {
        bpWriter->Write(var_i8, m_DataI8.data() + t);
        bpWriter->Write(var_i16, m_DataI16.data() + t);
        bpWriter->Write(var_i32, m_DataI32.data() + t);
        bpWriter->Write(var_i64, m_DataI64.data() + t);
        bpWriter->Write(var_u8, m_DataU8.data() + t);
        bpWriter->Write(var_u16, m_DataU16.data() + t);
        bpWriter->Write(var_u32, m_DataU32.data() + t);
        bpWriter->Write(var_u64, m_DataU64.data() + t);
        bpWriter->Write(var_r32, m_DataR32.data() + t);
        bpWriter->Write(var_r64, m_DataR64.data() + t);
        bpWriter->Advance();
    }

    bpWriter->Close();
}
