#include <cstdint>

#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#include "../engine/SmallTestData.h"

class ADIOSDefineAttributeTest : public ::testing::Test
{
public:
    ADIOSDefineAttributeTest() : adios(true), io(adios.DeclareIO("TestIO")) {}

    SmallTestData m_TestData;

protected:
    adios2::ADIOS adios;
    adios2::IO &io;
};

TEST_F(ADIOSDefineAttributeTest, DefineAttributeNameException)
{
    int mpiRank = 0;

#ifdef ADIOS2_HAVE_MPI
    int mpiSize = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif
    std::string name = std::string("attributeString") + std::to_string(mpiRank);

    // Attribute should be unique per process
    io.DefineAttribute<std::string>(name, "-1");

    EXPECT_THROW(io.DefineAttribute<std::string>(name, "0"),
                 std::invalid_argument);

    auto *attributeString1 =
        io.InquireAttribute<std::string>("NonExistingAttribute");
    EXPECT_EQ(attributeString1, nullptr);

    auto *attributeString2 = io.InquireAttribute<std::string>(name);
    EXPECT_NE(attributeString2, nullptr);
}

TEST_F(ADIOSDefineAttributeTest, DefineAttributeTypeByValue)
{
    int mpiRank = 0;
    int mpiSize = 1;
#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

    // Define unique data for each process
    SmallTestData currentTestData =
        generateNewSmallTestData(m_TestData, 0, mpiRank, mpiSize);

    std::string mpiRankString = std::to_string(mpiRank);
    std::string s1_Single = std::string("s1_Single_") + mpiRankString;
    std::string i8_Single = std::string("i8_Single_") + mpiRankString;
    std::string i16_Single = std::string("i16_Single_") + mpiRankString;
    std::string i32_Single = std::string("i32_Single_") + mpiRankString;
    std::string i64_Single = std::string("i64_Single_") + mpiRankString;
    std::string u8_Single = std::string("u8_Single_") + mpiRankString;
    std::string u16_Single = std::string("u16_Single_") + mpiRankString;
    std::string u32_Single = std::string("u32_Single_") + mpiRankString;
    std::string u64_Single = std::string("u64_Single_") + mpiRankString;
    std::string float_Single = std::string("float_Single_") + mpiRankString;
    std::string double_Single = std::string("double_Single_") + mpiRankString;

    // Define ADIOS global value
    auto &attributeS1 =
        io.DefineAttribute<std::string>(s1_Single, currentTestData.S1);
    auto &attributeI8 =
        io.DefineAttribute<int8_t>(i8_Single, currentTestData.I8.front());
    auto &attributeI16 =
        io.DefineAttribute<int16_t>(i16_Single, currentTestData.I16.front());
    auto &attributeI32 =
        io.DefineAttribute<int32_t>(i32_Single, currentTestData.I32.front());
    auto &attributeI64 =
        io.DefineAttribute<int64_t>(i64_Single, currentTestData.I64.front());

    auto &attributeU8 =
        io.DefineAttribute<uint8_t>(u8_Single, currentTestData.U8.front());
    auto &attributeU16 =
        io.DefineAttribute<uint16_t>(u16_Single, currentTestData.U16.front());
    auto &attributeU32 =
        io.DefineAttribute<uint32_t>(u32_Single, currentTestData.U32.front());
    auto &attributeU64 =
        io.DefineAttribute<uint64_t>(u64_Single, currentTestData.U64.front());

    auto &attributeFloat =
        io.DefineAttribute<float>(float_Single, currentTestData.R32.front());
    auto &attributeDouble =
        io.DefineAttribute<double>(double_Single, currentTestData.R64.front());

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(attributeS1),
                                  adios2::Attribute<std::string> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeI8),
                                  adios2::Attribute<int8_t> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeI16),
                                  adios2::Attribute<int16_t> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeI32),
                                  adios2::Attribute<int32_t> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeI64),
                                  adios2::Attribute<int64_t> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeU8),
                                  adios2::Attribute<uint8_t> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeU16),
                                  adios2::Attribute<uint16_t> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeU32),
                                  adios2::Attribute<uint32_t> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeU64),
                                  adios2::Attribute<uint64_t> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeFloat),
                                  adios2::Attribute<float> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeDouble),
                                  adios2::Attribute<double> &>();

    // Verify the members are correct
    ASSERT_EQ(attributeS1.m_IsSingleValue, true);
    ASSERT_EQ(attributeS1.m_DataArray.empty(), true);
    EXPECT_EQ(attributeS1.m_Name, s1_Single);
    EXPECT_EQ(attributeS1.m_DataSingleValue, currentTestData.S1);
    EXPECT_EQ(attributeS1.m_Elements, 1);
    EXPECT_EQ(attributeS1.m_Type, "string");

    ASSERT_EQ(attributeI8.m_IsSingleValue, true);
    ASSERT_EQ(attributeI8.m_DataArray.empty(), true);
    EXPECT_EQ(attributeI8.m_Name, i8_Single);
    EXPECT_EQ(attributeI8.m_DataSingleValue, currentTestData.I8.front());
    EXPECT_EQ(attributeI8.m_Elements, 1);
    EXPECT_EQ(attributeI8.m_Type, "signed char");

    ASSERT_EQ(attributeI16.m_IsSingleValue, true);
    ASSERT_EQ(attributeI16.m_DataArray.empty(), true);
    EXPECT_EQ(attributeI16.m_Name, i16_Single);
    EXPECT_EQ(attributeI16.m_DataSingleValue, currentTestData.I16.front());
    EXPECT_EQ(attributeI16.m_Elements, 1);
    EXPECT_EQ(attributeI16.m_Type, "short");

    ASSERT_EQ(attributeI32.m_IsSingleValue, true);
    ASSERT_EQ(attributeI32.m_DataArray.empty(), true);
    EXPECT_EQ(attributeI32.m_Name, i32_Single);
    EXPECT_EQ(attributeI32.m_DataSingleValue, currentTestData.I32.front());
    EXPECT_EQ(attributeI32.m_Elements, 1);
    EXPECT_EQ(attributeI32.m_Type, "int");

    ASSERT_EQ(attributeI64.m_IsSingleValue, true);
    ASSERT_EQ(attributeI64.m_DataArray.empty(), true);
    EXPECT_EQ(attributeI64.m_Name, i64_Single);
    EXPECT_EQ(attributeI64.m_DataSingleValue, currentTestData.I64.front());
    EXPECT_EQ(attributeI64.m_Elements, 1);
    EXPECT_EQ(sizeof(attributeI64.m_DataSingleValue), 8);

    ASSERT_EQ(attributeU8.m_IsSingleValue, true);
    ASSERT_EQ(attributeU8.m_DataArray.empty(), true);
    EXPECT_EQ(attributeU8.m_Name, u8_Single);
    EXPECT_EQ(attributeU8.m_DataSingleValue, currentTestData.U8.front());
    EXPECT_EQ(attributeU8.m_Elements, 1);
    EXPECT_EQ(attributeU8.m_Type, "unsigned char");

    ASSERT_EQ(attributeU16.m_IsSingleValue, true);
    ASSERT_EQ(attributeU16.m_DataArray.empty(), true);
    EXPECT_EQ(attributeU16.m_Name, u16_Single);
    EXPECT_EQ(attributeU16.m_DataSingleValue, currentTestData.U16.front());
    EXPECT_EQ(attributeU16.m_Elements, 1);
    EXPECT_EQ(attributeU16.m_Type, "unsigned short");

    ASSERT_EQ(attributeU32.m_IsSingleValue, true);
    ASSERT_EQ(attributeU32.m_DataArray.empty(), true);
    EXPECT_EQ(attributeU32.m_Name, u32_Single);
    EXPECT_EQ(attributeU32.m_DataSingleValue, currentTestData.U32.front());
    EXPECT_EQ(attributeU32.m_Elements, 1);
    EXPECT_EQ(attributeU32.m_Type, "unsigned int");

    ASSERT_EQ(attributeU64.m_IsSingleValue, true);
    ASSERT_EQ(attributeU64.m_DataArray.empty(), true);
    EXPECT_EQ(attributeU64.m_Name, u64_Single);
    EXPECT_EQ(attributeU64.m_DataSingleValue, currentTestData.U64.front());
    EXPECT_EQ(attributeU64.m_Elements, 1);
    EXPECT_EQ(sizeof(attributeU64.m_DataSingleValue), 8);

    ASSERT_EQ(attributeFloat.m_IsSingleValue, true);
    ASSERT_EQ(attributeFloat.m_DataArray.empty(), true);
    EXPECT_EQ(attributeFloat.m_Name, float_Single);
    EXPECT_EQ(attributeFloat.m_DataSingleValue, currentTestData.R32.front());
    EXPECT_EQ(attributeFloat.m_Elements, 1);
    EXPECT_EQ(attributeFloat.m_Type, "float");

    ASSERT_EQ(attributeDouble.m_IsSingleValue, true);
    ASSERT_EQ(attributeDouble.m_DataArray.empty(), true);
    EXPECT_EQ(attributeDouble.m_Name, double_Single);
    EXPECT_EQ(attributeDouble.m_DataSingleValue, currentTestData.R64.front());
    EXPECT_EQ(attributeDouble.m_Elements, 1);
    EXPECT_EQ(attributeDouble.m_Type, "double");
}

TEST_F(ADIOSDefineAttributeTest, DefineAttributeTypeByReference)
{
    int mpiRank = 0, mpiSize = 1;
    size_t numberOfElements = 10;
#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

    // Define unique data for each process
    SmallTestData currentTestData =
        generateNewSmallTestData(m_TestData, 0, mpiRank, mpiSize);

    std::string mpiRankString = std::to_string(mpiRank);
    std::string s3_Single = std::string("s3_Single_") + mpiRankString;
    std::string i8_Single = std::string("i8_Single_") + mpiRankString;
    std::string i16_Single = std::string("i16_Single_") + mpiRankString;
    std::string i32_Single = std::string("i32_Single_") + mpiRankString;
    std::string i64_Single = std::string("i64_Single_") + mpiRankString;
    std::string u8_Single = std::string("u8_Single_") + mpiRankString;
    std::string u16_Single = std::string("u16_Single_") + mpiRankString;
    std::string u32_Single = std::string("u32_Single_") + mpiRankString;
    std::string u64_Single = std::string("u64_Single_") + mpiRankString;
    std::string float_Single = std::string("float_Single_") + mpiRankString;
    std::string double_Single = std::string("double_Single_") + mpiRankString;

    // Define ADIOS global value
    auto &attributeS3 = io.DefineAttribute<std::string>(
        s3_Single, currentTestData.S3.data(), 3);
    auto &attributeI8 = io.DefineAttribute<int8_t>(
        i8_Single, currentTestData.I8.data(), numberOfElements);
    auto &attributeI16 = io.DefineAttribute<int16_t>(
        i16_Single, currentTestData.I16.data(), numberOfElements);
    auto &attributeI32 = io.DefineAttribute<int32_t>(
        i32_Single, currentTestData.I32.data(), numberOfElements);
    auto &attributeI64 = io.DefineAttribute<int64_t>(
        i64_Single, currentTestData.I64.data(), numberOfElements);

    auto &attributeU8 = io.DefineAttribute<uint8_t>(
        u8_Single, currentTestData.U8.data(), numberOfElements);
    auto &attributeU16 = io.DefineAttribute<uint16_t>(
        u16_Single, currentTestData.U16.data(), numberOfElements);
    auto &attributeU32 = io.DefineAttribute<uint32_t>(
        u32_Single, currentTestData.U32.data(), numberOfElements);
    auto &attributeU64 = io.DefineAttribute<uint64_t>(
        u64_Single, currentTestData.U64.data(), numberOfElements);

    auto &attributeFloat = io.DefineAttribute<float>(
        float_Single, currentTestData.R32.data(), numberOfElements);
    auto &attributeDouble = io.DefineAttribute<double>(
        double_Single, currentTestData.R64.data(), numberOfElements);

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(attributeS3),
                                  adios2::Attribute<std::string> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeI8),
                                  adios2::Attribute<int8_t> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeI16),
                                  adios2::Attribute<int16_t> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeI32),
                                  adios2::Attribute<int32_t> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeI64),
                                  adios2::Attribute<int64_t> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeU8),
                                  adios2::Attribute<uint8_t> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeU16),
                                  adios2::Attribute<uint16_t> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeU32),
                                  adios2::Attribute<uint32_t> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeU64),
                                  adios2::Attribute<uint64_t> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeFloat),
                                  adios2::Attribute<float> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeDouble),
                                  adios2::Attribute<double> &>();

    // Verify the members are correct
    ASSERT_EQ(attributeS3.m_IsSingleValue, false);
    ASSERT_EQ(attributeS3.m_DataArray.empty(), false);
    EXPECT_EQ(attributeS3.m_Name, s3_Single);
    EXPECT_EQ(attributeS3.m_Elements, 3);
    EXPECT_EQ(attributeS3.m_Type, "string");

    ASSERT_EQ(attributeI8.m_IsSingleValue, false);
    ASSERT_EQ(attributeI8.m_DataArray.empty(), false);
    EXPECT_EQ(attributeI8.m_Name, i8_Single);
    EXPECT_EQ(attributeI8.m_Elements, numberOfElements);
    EXPECT_EQ(attributeI8.m_Type, "signed char");

    ASSERT_EQ(attributeI16.m_IsSingleValue, false);
    ASSERT_EQ(attributeI16.m_DataArray.empty(), false);
    EXPECT_EQ(attributeI16.m_Name, i16_Single);
    EXPECT_EQ(attributeI16.m_Elements, numberOfElements);
    EXPECT_EQ(attributeI16.m_Type, "short");

    ASSERT_EQ(attributeI32.m_IsSingleValue, false);
    ASSERT_EQ(attributeI32.m_DataArray.empty(), false);
    EXPECT_EQ(attributeI32.m_Name, i32_Single);
    EXPECT_EQ(attributeI32.m_Elements, numberOfElements);
    EXPECT_EQ(attributeI32.m_Type, "int");

    ASSERT_EQ(attributeI64.m_IsSingleValue, false);
    ASSERT_EQ(attributeI64.m_DataArray.empty(), false);
    EXPECT_EQ(attributeI64.m_Name, i64_Single);
    EXPECT_EQ(attributeI64.m_Elements, numberOfElements);
    EXPECT_EQ(sizeof(attributeI64.m_DataSingleValue), 8);

    ASSERT_EQ(attributeU8.m_IsSingleValue, false);
    ASSERT_EQ(attributeU8.m_DataArray.empty(), false);
    EXPECT_EQ(attributeU8.m_Name, u8_Single);
    EXPECT_EQ(attributeU8.m_Elements, numberOfElements);
    EXPECT_EQ(attributeU8.m_Type, "unsigned char");

    ASSERT_EQ(attributeU16.m_IsSingleValue, false);
    ASSERT_EQ(attributeU16.m_DataArray.empty(), false);
    EXPECT_EQ(attributeU16.m_Name, u16_Single);
    EXPECT_EQ(attributeU16.m_Elements, numberOfElements);
    EXPECT_EQ(attributeU16.m_Type, "unsigned short");

    ASSERT_EQ(attributeU32.m_IsSingleValue, false);
    ASSERT_EQ(attributeU32.m_DataArray.empty(), false);
    EXPECT_EQ(attributeU32.m_Name, u32_Single);
    EXPECT_EQ(attributeU32.m_Elements, numberOfElements);
    EXPECT_EQ(attributeU32.m_Type, "unsigned int");

    ASSERT_EQ(attributeU64.m_IsSingleValue, false);
    ASSERT_EQ(attributeU64.m_DataArray.empty(), false);
    EXPECT_EQ(attributeU64.m_Name, u64_Single);
    EXPECT_EQ(attributeU64.m_Elements, numberOfElements);
    EXPECT_EQ(sizeof(attributeU64.m_DataSingleValue), 8);

    ASSERT_EQ(attributeFloat.m_IsSingleValue, false);
    ASSERT_EQ(attributeFloat.m_DataArray.empty(), false);
    EXPECT_EQ(attributeFloat.m_Name, float_Single);
    EXPECT_EQ(attributeFloat.m_Elements, numberOfElements);
    EXPECT_EQ(attributeFloat.m_Type, "float");

    ASSERT_EQ(attributeDouble.m_IsSingleValue, false);
    ASSERT_EQ(attributeDouble.m_DataArray.empty(), false);
    EXPECT_EQ(attributeDouble.m_Name, double_Single);
    EXPECT_EQ(attributeDouble.m_Elements, numberOfElements);
    EXPECT_EQ(attributeDouble.m_Type, "double");

    // Verify data
    for (size_t index = 0; index < numberOfElements; index++)
    {
        EXPECT_EQ(attributeI8.m_DataArray[index], currentTestData.I8.at(index));
        EXPECT_EQ(attributeI16.m_DataArray[index],
                  currentTestData.I16.at(index));
        EXPECT_EQ(attributeI32.m_DataArray[index],
                  currentTestData.I32.at(index));
        EXPECT_EQ(attributeU8.m_DataArray[index], currentTestData.U8.at(index));
        EXPECT_EQ(attributeU16.m_DataArray[index],
                  currentTestData.U16.at(index));
        EXPECT_EQ(attributeU32.m_DataArray[index],
                  currentTestData.U32.at(index));
        EXPECT_EQ(attributeFloat.m_DataArray[index],
                  currentTestData.R32.at(index));
        EXPECT_EQ(attributeDouble.m_DataArray[index],
                  currentTestData.R64.at(index));
    }
}

TEST_F(ADIOSDefineAttributeTest, GetAttribute)
{
    int mpiRank = 0, mpiSize = 1;
    size_t numberOfElements = 10;
#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

    // Define unique data for each process
    SmallTestData currentTestData =
        generateNewSmallTestData(m_TestData, 0, mpiRank, mpiSize);

    std::string mpiRankString = std::to_string(mpiRank);
    std::string s3_Single = std::string("s3_Array_") + mpiRankString;
    std::string i8_Single = std::string("i8_Array_") + mpiRankString;
    std::string i16_Single = std::string("i16_Array_") + mpiRankString;
    std::string i32_Single = std::string("i32_Array_") + mpiRankString;
    std::string i64_Single = std::string("i64_Array_") + mpiRankString;
    std::string u8_Single = std::string("u8_Array_") + mpiRankString;
    std::string u16_Single = std::string("u16_Array_") + mpiRankString;
    std::string u32_Single = std::string("u32_Array_") + mpiRankString;
    std::string u64_Single = std::string("u64_Array_") + mpiRankString;
    std::string float_Single = std::string("float_Array_") + mpiRankString;
    std::string double_Single = std::string("double_Array_") + mpiRankString;

    // Define ADIOS global value
    {
        io.DefineAttribute<std::string>(s3_Single, currentTestData.S3.data(),
                                        3);
        io.DefineAttribute<int8_t>(i8_Single, currentTestData.I8.data(),
                                   numberOfElements);
        io.DefineAttribute<int16_t>(i16_Single, currentTestData.I16.data(),
                                    numberOfElements);
        io.DefineAttribute<int32_t>(i32_Single, currentTestData.I32.data(),
                                    numberOfElements);
        io.DefineAttribute<int64_t>(i64_Single, currentTestData.I64.data(),
                                    numberOfElements);
        io.DefineAttribute<uint8_t>(u8_Single, currentTestData.U8.data(),
                                    numberOfElements);
        io.DefineAttribute<uint16_t>(u16_Single, currentTestData.U16.data(),
                                     numberOfElements);
        io.DefineAttribute<uint32_t>(u32_Single, currentTestData.U32.data(),
                                     numberOfElements);
        io.DefineAttribute<uint64_t>(u64_Single, currentTestData.U64.data(),
                                     numberOfElements);
        io.DefineAttribute<float>(float_Single, currentTestData.R32.data(),
                                  numberOfElements);
        io.DefineAttribute<double>(double_Single, currentTestData.R64.data(),
                                   numberOfElements);
    }

    auto &attributeS3 = *io.InquireAttribute<std::string>(s3_Single);
    auto &attributeI8 = *io.InquireAttribute<int8_t>(i8_Single);
    auto &attributeI16 = *io.InquireAttribute<int16_t>(i16_Single);
    auto &attributeI32 = *io.InquireAttribute<int32_t>(i32_Single);
    auto &attributeI64 = *io.InquireAttribute<int64_t>(i64_Single);
    auto &attributeU8 = *io.InquireAttribute<uint8_t>(u8_Single);
    auto &attributeU16 = *io.InquireAttribute<uint16_t>(u16_Single);
    auto &attributeU32 = *io.InquireAttribute<uint32_t>(u32_Single);
    auto &attributeU64 = *io.InquireAttribute<uint64_t>(u64_Single);
    auto &attributeFloat = *io.InquireAttribute<float>(float_Single);
    auto &attributeDouble = *io.InquireAttribute<double>(double_Single);

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(attributeS3),
                                  adios2::Attribute<std::string> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeI8),
                                  adios2::Attribute<int8_t> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeI16),
                                  adios2::Attribute<int16_t> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeI32),
                                  adios2::Attribute<int32_t> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeI64),
                                  adios2::Attribute<int64_t> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeU8),
                                  adios2::Attribute<uint8_t> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeU16),
                                  adios2::Attribute<uint16_t> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeU32),
                                  adios2::Attribute<uint32_t> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeU64),
                                  adios2::Attribute<uint64_t> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeFloat),
                                  adios2::Attribute<float> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeDouble),
                                  adios2::Attribute<double> &>();

    // Verify the members are correct
    ASSERT_EQ(attributeS3.m_IsSingleValue, false);
    ASSERT_EQ(attributeS3.m_DataArray.empty(), false);
    EXPECT_EQ(attributeS3.m_Name, s3_Single);
    EXPECT_EQ(attributeS3.m_Elements, 3);
    EXPECT_EQ(attributeS3.m_Type, "string");

    ASSERT_EQ(attributeI8.m_IsSingleValue, false);
    ASSERT_EQ(attributeI8.m_DataArray.empty(), false);
    EXPECT_EQ(attributeI8.m_Name, i8_Single);
    EXPECT_EQ(attributeI8.m_Elements, numberOfElements);
    EXPECT_EQ(attributeI8.m_Type, "signed char");

    ASSERT_EQ(attributeI16.m_IsSingleValue, false);
    ASSERT_EQ(attributeI16.m_DataArray.empty(), false);
    EXPECT_EQ(attributeI16.m_Name, i16_Single);
    EXPECT_EQ(attributeI16.m_Elements, numberOfElements);
    EXPECT_EQ(attributeI16.m_Type, "short");

    ASSERT_EQ(attributeI32.m_IsSingleValue, false);
    ASSERT_EQ(attributeI32.m_DataArray.empty(), false);
    EXPECT_EQ(attributeI32.m_Name, i32_Single);
    EXPECT_EQ(attributeI32.m_Elements, numberOfElements);
    EXPECT_EQ(attributeI32.m_Type, "int");

    ASSERT_EQ(attributeI64.m_IsSingleValue, false);
    ASSERT_EQ(attributeI64.m_DataArray.empty(), false);
    EXPECT_EQ(attributeI64.m_Name, i64_Single);
    EXPECT_EQ(attributeI64.m_Elements, numberOfElements);
    EXPECT_EQ(sizeof(attributeI64.m_DataSingleValue), 8);

    ASSERT_EQ(attributeU8.m_IsSingleValue, false);
    ASSERT_EQ(attributeU8.m_DataArray.empty(), false);
    EXPECT_EQ(attributeU8.m_Name, u8_Single);
    EXPECT_EQ(attributeU8.m_Elements, numberOfElements);
    EXPECT_EQ(attributeU8.m_Type, "unsigned char");

    ASSERT_EQ(attributeU16.m_IsSingleValue, false);
    ASSERT_EQ(attributeU16.m_DataArray.empty(), false);
    EXPECT_EQ(attributeU16.m_Name, u16_Single);
    EXPECT_EQ(attributeU16.m_Elements, numberOfElements);
    EXPECT_EQ(attributeU16.m_Type, "unsigned short");

    ASSERT_EQ(attributeU32.m_IsSingleValue, false);
    ASSERT_EQ(attributeU32.m_DataArray.empty(), false);
    EXPECT_EQ(attributeU32.m_Name, u32_Single);
    EXPECT_EQ(attributeU32.m_Elements, numberOfElements);
    EXPECT_EQ(attributeU32.m_Type, "unsigned int");

    ASSERT_EQ(attributeU64.m_IsSingleValue, false);
    ASSERT_EQ(attributeU64.m_DataArray.empty(), false);
    EXPECT_EQ(attributeU64.m_Name, u64_Single);
    EXPECT_EQ(attributeU64.m_Elements, numberOfElements);
    EXPECT_EQ(sizeof(attributeU64.m_DataSingleValue), 8);

    ASSERT_EQ(attributeFloat.m_IsSingleValue, false);
    ASSERT_EQ(attributeFloat.m_DataArray.empty(), false);
    EXPECT_EQ(attributeFloat.m_Name, float_Single);
    EXPECT_EQ(attributeFloat.m_Elements, numberOfElements);
    EXPECT_EQ(attributeFloat.m_Type, "float");

    ASSERT_EQ(attributeDouble.m_IsSingleValue, false);
    ASSERT_EQ(attributeDouble.m_DataArray.empty(), false);
    EXPECT_EQ(attributeDouble.m_Name, double_Single);
    EXPECT_EQ(attributeDouble.m_Elements, numberOfElements);
    EXPECT_EQ(attributeDouble.m_Type, "double");

    // Verify data
    for (size_t index = 0; index < numberOfElements; index++)
    {
        EXPECT_EQ(attributeI8.m_DataArray[index], currentTestData.I8.at(index));
        EXPECT_EQ(attributeI16.m_DataArray[index],
                  currentTestData.I16.at(index));
        EXPECT_EQ(attributeI32.m_DataArray[index],
                  currentTestData.I32.at(index));
        EXPECT_EQ(attributeU8.m_DataArray[index], currentTestData.U8.at(index));
        EXPECT_EQ(attributeU16.m_DataArray[index],
                  currentTestData.U16.at(index));
        EXPECT_EQ(attributeU32.m_DataArray[index],
                  currentTestData.U32.at(index));
        EXPECT_EQ(attributeFloat.m_DataArray[index],
                  currentTestData.R32.at(index));
        EXPECT_EQ(attributeDouble.m_DataArray[index],
                  currentTestData.R64.at(index));
    }
}

TEST_F(ADIOSDefineAttributeTest, DefineAndRemove)
{
    auto lf_CheckRemove = [&](const std::string variableName) {

        const bool isRemoved = io.RemoveAttribute(variableName);
        EXPECT_EQ(isRemoved, true);
    };

    const adios2::Dims shape = {10};
    const adios2::Dims start = {0};
    const adios2::Dims count = {10};

    io.DefineAttribute<std::string>("iString", "String Attribute");
    io.DefineAttribute<int8_t>("i8", -8);
    io.DefineAttribute<int16_t>("i16", -16);
    io.DefineAttribute<int32_t>("i32", -32);
    io.DefineAttribute<int64_t>("i64", -64);
    io.DefineAttribute<uint8_t>("u8", 8);
    io.DefineAttribute<uint16_t>("u16", 16);
    io.DefineAttribute<uint32_t>("u32", 32);
    io.DefineAttribute<uint64_t>("u64", 64);
    io.DefineAttribute<float>("r32", 32);
    io.DefineAttribute<double>("r64", 64);

    lf_CheckRemove("iString");
    lf_CheckRemove("i8");
    lf_CheckRemove("i16");
    lf_CheckRemove("i32");
    lf_CheckRemove("i64");

    lf_CheckRemove("u8");
    lf_CheckRemove("u16");
    lf_CheckRemove("u32");
    lf_CheckRemove("u64");

    lf_CheckRemove("r32");
    lf_CheckRemove("r64");

    auto attr_iString = io.InquireAttribute<std::string>("iString");
    auto attr_i8 = io.InquireAttribute<int8_t>("i8");
    auto attr_i16 = io.InquireAttribute<int16_t>("i16");
    auto attr_i32 = io.InquireAttribute<int32_t>("i32");
    auto attr_i64 = io.InquireAttribute<int64_t>("i64");
    auto attr_u8 = io.InquireAttribute<uint8_t>("u8");
    auto attr_u16 = io.InquireAttribute<uint16_t>("u16");
    auto attr_u32 = io.InquireAttribute<uint32_t>("u32");
    auto attr_u64 = io.InquireAttribute<uint64_t>("u64");
    auto attr_r32 = io.InquireAttribute<float>("r32");
    auto attr_r64 = io.InquireAttribute<double>("r64");

    EXPECT_EQ(attr_iString, nullptr);
    EXPECT_EQ(attr_i8, nullptr);
    EXPECT_EQ(attr_i16, nullptr);
    EXPECT_EQ(attr_i32, nullptr);
    EXPECT_EQ(attr_i64, nullptr);

    EXPECT_EQ(attr_u8, nullptr);
    EXPECT_EQ(attr_u16, nullptr);
    EXPECT_EQ(attr_u32, nullptr);
    EXPECT_EQ(attr_u64, nullptr);

    EXPECT_EQ(attr_r32, nullptr);
    EXPECT_EQ(attr_r64, nullptr);
}

TEST_F(ADIOSDefineAttributeTest, DefineAndRemoveAll)
{
    const adios2::Dims shape = {10};
    const adios2::Dims start = {0};
    const adios2::Dims count = {10};

    io.DefineAttribute<std::string>("iString", "String Attribute");
    io.DefineAttribute<int8_t>("i8", -8);
    io.DefineAttribute<int16_t>("i16", -16);
    io.DefineAttribute<int32_t>("i32", -32);
    io.DefineAttribute<int64_t>("i64", -64);
    io.DefineAttribute<uint8_t>("u8", 8);
    io.DefineAttribute<uint16_t>("u16", 16);
    io.DefineAttribute<uint32_t>("u32", 32);
    io.DefineAttribute<uint64_t>("u64", 64);
    io.DefineAttribute<float>("r32", 32);
    io.DefineAttribute<double>("r64", 64);

    io.RemoveAllAttributes();

    auto attr_iString = io.InquireAttribute<std::string>("iString");
    auto attr_i8 = io.InquireAttribute<int8_t>("i8");
    auto attr_i16 = io.InquireAttribute<int16_t>("i16");
    auto attr_i32 = io.InquireAttribute<int32_t>("i32");
    auto attr_i64 = io.InquireAttribute<int64_t>("i64");
    auto attr_u8 = io.InquireAttribute<uint8_t>("u8");
    auto attr_u16 = io.InquireAttribute<uint16_t>("u16");
    auto attr_u32 = io.InquireAttribute<uint32_t>("u32");
    auto attr_u64 = io.InquireAttribute<uint64_t>("u64");
    auto attr_r32 = io.InquireAttribute<float>("r32");
    auto attr_r64 = io.InquireAttribute<double>("r64");

    EXPECT_EQ(attr_iString, nullptr);
    EXPECT_EQ(attr_i8, nullptr);
    EXPECT_EQ(attr_i16, nullptr);
    EXPECT_EQ(attr_i32, nullptr);
    EXPECT_EQ(attr_i64, nullptr);

    EXPECT_EQ(attr_u8, nullptr);
    EXPECT_EQ(attr_u16, nullptr);
    EXPECT_EQ(attr_u32, nullptr);
    EXPECT_EQ(attr_u64, nullptr);

    EXPECT_EQ(attr_r32, nullptr);
    EXPECT_EQ(attr_r64, nullptr);
}

int main(int argc, char **argv)
{
#ifdef ADIOS2_HAVE_MPI
    MPI_Init(nullptr, nullptr);
#endif

    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return result;
}
