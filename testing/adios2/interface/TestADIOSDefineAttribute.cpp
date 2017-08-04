#include <cstdint>

#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

class ADIOSDefineAttributeTest : public ::testing::Test
{
public:
    ADIOSDefineAttributeTest() : adios(true), io(adios.DeclareIO("TestIO")) {}

protected:
    // virtual void SetUp() { }

    // virtual void TearDown() { }

    adios2::ADIOS adios;
    adios2::IO &io;
};

TEST_F(ADIOSDefineAttributeTest, DefineAttributeNameException)
{
    auto &attributeString1 =
        io.DefineAttribute<std::string>("attributeString", "-1");

    EXPECT_THROW(auto &attributeString2 =
                     io.DefineAttribute<std::string>("attributeString", "0"),
                 std::invalid_argument);

    EXPECT_THROW(auto &attributeString2 =
                     io.GetAttribute<std::string>("NoExistingAttribute"),
                 std::invalid_argument);

    EXPECT_NO_THROW(auto &attributeString3 =
                        io.GetAttribute<std::string>("attributeString"));
}

TEST_F(ADIOSDefineAttributeTest, DefineAttributeTypeByValue)
{
    // Define ADIOS global value
    auto &attributeString =
        io.DefineAttribute<std::string>("attributeString", "-1");
    auto &attributeChar = io.DefineAttribute<char>("attributeChar", '0');
    auto &attributeUChar =
        io.DefineAttribute<unsigned char>("attributeUChar", '1');
    auto &attributeShort = io.DefineAttribute<short>("attributeShort", 2);
    auto &attributeUShort =
        io.DefineAttribute<unsigned short>("attributeUShort", 3);
    auto &attributeInt = io.DefineAttribute<int>("attributeInt", 4);
    auto &attributeUInt = io.DefineAttribute<unsigned int>("attributeUInt", 5);
    auto &attributeLInt = io.DefineAttribute<long int>("attributeLInt", 6);
    auto &attributeULInt =
        io.DefineAttribute<unsigned long int>("attributeULInt", 7);
    auto &attributeLLInt =
        io.DefineAttribute<long long int>("attributeLLInt", 8);
    auto &attributeULLInt =
        io.DefineAttribute<unsigned long long int>("attributeULLInt", 9);
    auto &attributeFloat = io.DefineAttribute<float>("attributeFloat", 10);
    auto &attributeDouble = io.DefineAttribute<double>("attributeDouble", 11);
    auto &attributeLDouble =
        io.DefineAttribute<long double>("attributeLDouble", 12);

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(attributeString),
                                  adios2::Attribute<std::string> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeChar),
                                  adios2::Attribute<char> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeUChar),
                                  adios2::Attribute<unsigned char> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeShort),
                                  adios2::Attribute<short> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeUShort),
                                  adios2::Attribute<unsigned short> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeInt),
                                  adios2::Attribute<int> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeUInt),
                                  adios2::Attribute<unsigned int> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeLInt),
                                  adios2::Attribute<long int> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeULInt),
                                  adios2::Attribute<unsigned long int> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeLLInt),
                                  adios2::Attribute<long long int> &>();
    ::testing::StaticAssertTypeEq<
        decltype(attributeULLInt),
        adios2::Attribute<unsigned long long int> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeFloat),
                                  adios2::Attribute<float> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeDouble),
                                  adios2::Attribute<double> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeLDouble),
                                  adios2::Attribute<long double> &>();

    // Verify the members are correct
    ASSERT_EQ(attributeString.m_DataArray, nullptr);
    EXPECT_EQ(attributeString.m_Name, "attributeString");
    EXPECT_EQ(attributeString.m_DataValue, "-1");
    EXPECT_EQ(attributeString.m_Elements, 1);
    EXPECT_EQ(attributeString.m_Type, "string");

    ASSERT_EQ(attributeChar.m_DataArray, nullptr);
    EXPECT_EQ(attributeChar.m_Name, "attributeChar");
    EXPECT_EQ(attributeChar.m_DataValue, '0');
    EXPECT_EQ(attributeChar.m_Elements, 1);
    EXPECT_EQ(attributeChar.m_Type, "char");

    ASSERT_EQ(attributeUChar.m_DataArray, nullptr);
    EXPECT_EQ(attributeUChar.m_Name, "attributeUChar");
    EXPECT_EQ(attributeUChar.m_DataValue, '1');
    EXPECT_EQ(attributeUChar.m_Elements, 1);
    EXPECT_EQ(attributeUChar.m_Type, "unsigned char");

    ASSERT_EQ(attributeShort.m_DataArray, nullptr);
    EXPECT_EQ(attributeShort.m_Name, "attributeShort");
    EXPECT_EQ(attributeShort.m_DataValue, 2);
    EXPECT_EQ(attributeShort.m_Elements, 1);
    EXPECT_EQ(attributeShort.m_Type, "short");

    ASSERT_EQ(attributeUShort.m_DataArray, nullptr);
    EXPECT_EQ(attributeUShort.m_Name, "attributeUShort");
    EXPECT_EQ(attributeUShort.m_DataValue, 3);
    EXPECT_EQ(attributeUShort.m_Elements, 1);
    EXPECT_EQ(attributeUShort.m_Type, "unsigned short");

    ASSERT_EQ(attributeInt.m_DataArray, nullptr);
    EXPECT_EQ(attributeInt.m_Name, "attributeInt");
    EXPECT_EQ(attributeInt.m_DataValue, 4);
    EXPECT_EQ(attributeInt.m_Elements, 1);
    EXPECT_EQ(attributeInt.m_Type, "int");

    ASSERT_EQ(attributeUInt.m_DataArray, nullptr);
    EXPECT_EQ(attributeUInt.m_Name, "attributeUInt");
    EXPECT_EQ(attributeUInt.m_DataValue, 5);
    EXPECT_EQ(attributeUInt.m_Elements, 1);
    EXPECT_EQ(attributeUInt.m_Type, "unsigned int");

    ASSERT_EQ(attributeLInt.m_DataArray, nullptr);
    EXPECT_EQ(attributeLInt.m_Name, "attributeLInt");
    EXPECT_EQ(attributeLInt.m_DataValue, 6);
    EXPECT_EQ(attributeLInt.m_Elements, 1);
    EXPECT_EQ(attributeLInt.m_Type, "long int");

    ASSERT_EQ(attributeULInt.m_DataArray, nullptr);
    EXPECT_EQ(attributeULInt.m_Name, "attributeULInt");
    EXPECT_EQ(attributeULInt.m_DataValue, 7);
    EXPECT_EQ(attributeULInt.m_Elements, 1);
    EXPECT_EQ(attributeULInt.m_Type, "unsigned long int");

    ASSERT_EQ(attributeLLInt.m_DataArray, nullptr);
    EXPECT_EQ(attributeLLInt.m_Name, "attributeLLInt");
    EXPECT_EQ(attributeLLInt.m_DataValue, 8);
    EXPECT_EQ(attributeLLInt.m_Elements, 1);
    EXPECT_EQ(attributeLLInt.m_Type, "long long int");

    ASSERT_EQ(attributeULLInt.m_DataArray, nullptr);
    EXPECT_EQ(attributeULLInt.m_Name, "attributeULLInt");
    EXPECT_EQ(attributeULLInt.m_DataValue, 9);
    EXPECT_EQ(attributeULLInt.m_Elements, 1);
    EXPECT_EQ(attributeULLInt.m_Type, "unsigned long long int");

    ASSERT_EQ(attributeFloat.m_DataArray, nullptr);
    EXPECT_EQ(attributeFloat.m_Name, "attributeFloat");
    EXPECT_EQ(attributeFloat.m_DataValue, 10);
    EXPECT_EQ(attributeFloat.m_Elements, 1);
    EXPECT_EQ(attributeFloat.m_Type, "float");

    ASSERT_EQ(attributeDouble.m_DataArray, nullptr);
    EXPECT_EQ(attributeDouble.m_Name, "attributeDouble");
    EXPECT_EQ(attributeDouble.m_DataValue, 11);
    EXPECT_EQ(attributeDouble.m_Elements, 1);
    EXPECT_EQ(attributeDouble.m_Type, "double");

    ASSERT_EQ(attributeLDouble.m_DataArray, nullptr);
    EXPECT_EQ(attributeLDouble.m_Name, "attributeLDouble");
    EXPECT_EQ(attributeLDouble.m_DataValue, 12);
    EXPECT_EQ(attributeLDouble.m_Elements, 1);
    EXPECT_EQ(attributeLDouble.m_Type, "long double");
}

TEST_F(ADIOSDefineAttributeTest, DefineAttributeTypeByReference)
{
    // Define ADIOS global value
    const std::vector<std::string> vString{"-1", "0", "+1"};
    const std::vector<char> vChar = {0, 0 + 1, 0 + 2};
    const std::vector<unsigned char> vUChar = {1, 1 + 1, 1 + 2};
    const std::vector<short> vShort = {2, 2 + 1, 2 + 2};
    const std::vector<unsigned short> vUShort = {3, 3 + 1, 3 + 2};
    const std::vector<int> vInt = {4, 4 + 1, 4 + 2};
    const std::vector<unsigned int> vUInt = {5, 5 + 1, 5 + 2};
    const std::vector<long int> vLInt = {6, 6 + 1, 6 + 2};
    const std::vector<unsigned long int> vULInt = {7, 7 + 1, 7 + 2};
    const std::vector<long long int> vLLInt = {8, 8 + 1, 8 + 2};
    const std::vector<unsigned long long int> vULLInt = {9, 9 + 1, 9 + 2};
    const std::vector<float> vFloat = {10, 10 + 1, 10 + 2};
    const std::vector<double> vDouble = {11, 11 + 1, 11 + 2};
    const std::vector<long double> vLDouble = {12, 12 + 1, 12 + 2};

    auto &attributeString =
        io.DefineAttribute<std::string>("attributeString", vString.data(), 3);

    auto &attributeChar =
        io.DefineAttribute<char>("attributeChar", vChar.data(), 3);
    auto &attributeUChar =
        io.DefineAttribute<unsigned char>("attributeUChar", vUChar.data(), 3);
    auto &attributeShort =
        io.DefineAttribute<short>("attributeShort", vShort.data(), 3);
    auto &attributeUShort = io.DefineAttribute<unsigned short>(
        "attributeUShort", vUShort.data(), 3);
    auto &attributeInt =
        io.DefineAttribute<int>("attributeInt", vInt.data(), 3);
    auto &attributeUInt =
        io.DefineAttribute<unsigned int>("attributeUInt", vUInt.data(), 3);
    auto &attributeLInt =
        io.DefineAttribute<long int>("attributeLInt", vLInt.data(), 3);
    auto &attributeULInt = io.DefineAttribute<unsigned long int>(
        "attributeULInt", vULInt.data(), 3);
    auto &attributeLLInt =
        io.DefineAttribute<long long int>("attributeLLInt", vLLInt.data(), 3);
    auto &attributeULLInt = io.DefineAttribute<unsigned long long int>(
        "attributeULLInt", vULLInt.data(), 3);
    auto &attributeFloat =
        io.DefineAttribute<float>("attributeFloat", vFloat.data(), 3);
    auto &attributeDouble =
        io.DefineAttribute<double>("attributeDouble", vDouble.data(), 3);
    auto &attributeLDouble =
        io.DefineAttribute<long double>("attributeLDouble", vLDouble.data(), 3);

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(attributeString),
                                  adios2::Attribute<std::string> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeChar),
                                  adios2::Attribute<char> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeUChar),
                                  adios2::Attribute<unsigned char> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeShort),
                                  adios2::Attribute<short> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeUShort),
                                  adios2::Attribute<unsigned short> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeInt),
                                  adios2::Attribute<int> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeUInt),
                                  adios2::Attribute<unsigned int> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeLInt),
                                  adios2::Attribute<long int> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeULInt),
                                  adios2::Attribute<unsigned long int> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeLLInt),
                                  adios2::Attribute<long long int> &>();
    ::testing::StaticAssertTypeEq<
        decltype(attributeULLInt),
        adios2::Attribute<unsigned long long int> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeFloat),
                                  adios2::Attribute<float> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeDouble),
                                  adios2::Attribute<double> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeLDouble),
                                  adios2::Attribute<long double> &>();

    // Verify the dimensions, name, and type are correct
    ASSERT_NE(attributeString.m_DataArray, nullptr);
    EXPECT_EQ(attributeString.m_DataArray[0], "-1");
    EXPECT_EQ(attributeString.m_DataArray[1], "0");
    EXPECT_EQ(attributeString.m_DataArray[2], "+1");
    EXPECT_EQ(attributeString.m_Name, "attributeString");
    ASSERT_EQ(attributeString.m_DataValue.empty(), true);
    EXPECT_EQ(attributeString.m_Elements, 3);
    EXPECT_EQ(attributeString.m_Type, "string");

    ASSERT_NE(attributeChar.m_DataArray, nullptr);
    EXPECT_EQ(attributeChar.m_DataArray[0], 0);
    EXPECT_EQ(attributeChar.m_DataArray[1], 0 + 1);
    EXPECT_EQ(attributeChar.m_DataArray[2], 0 + 2);
    EXPECT_EQ(attributeChar.m_Name, "attributeChar");
    EXPECT_EQ(attributeChar.m_Elements, 3);
    EXPECT_EQ(attributeChar.m_Type, "char");

    ASSERT_NE(attributeUChar.m_DataArray, nullptr);
    EXPECT_EQ(attributeUChar.m_DataArray[0], 1);
    EXPECT_EQ(attributeUChar.m_DataArray[1], 1 + 1);
    EXPECT_EQ(attributeUChar.m_DataArray[2], 1 + 2);
    EXPECT_EQ(attributeUChar.m_Name, "attributeUChar");
    EXPECT_EQ(attributeUChar.m_Elements, 3);
    EXPECT_EQ(attributeUChar.m_Type, "unsigned char");

    ASSERT_NE(attributeShort.m_DataArray, nullptr);
    EXPECT_EQ(attributeShort.m_DataArray[0], 2);
    EXPECT_EQ(attributeShort.m_DataArray[1], 2 + 1);
    EXPECT_EQ(attributeShort.m_DataArray[2], 2 + 2);
    EXPECT_EQ(attributeShort.m_Name, "attributeShort");
    EXPECT_EQ(attributeShort.m_Elements, 3);
    EXPECT_EQ(attributeShort.m_Type, "short");

    ASSERT_NE(attributeUShort.m_DataArray, nullptr);
    EXPECT_EQ(attributeUShort.m_DataArray[0], 3);
    EXPECT_EQ(attributeUShort.m_DataArray[1], 3 + 1);
    EXPECT_EQ(attributeUShort.m_DataArray[2], 3 + 2);
    EXPECT_EQ(attributeUShort.m_Name, "attributeUShort");
    EXPECT_EQ(attributeUShort.m_Elements, 3);
    EXPECT_EQ(attributeUShort.m_Type, "unsigned short");

    ASSERT_NE(attributeInt.m_DataArray, nullptr);
    EXPECT_EQ(attributeInt.m_DataArray[0], 4);
    EXPECT_EQ(attributeInt.m_DataArray[1], 4 + 1);
    EXPECT_EQ(attributeInt.m_DataArray[2], 4 + 2);
    EXPECT_EQ(attributeInt.m_Name, "attributeInt");
    EXPECT_EQ(attributeInt.m_Elements, 3);
    EXPECT_EQ(attributeInt.m_Type, "int");

    ASSERT_NE(attributeUInt.m_DataArray, nullptr);
    EXPECT_EQ(attributeUInt.m_DataArray[0], 5);
    EXPECT_EQ(attributeUInt.m_DataArray[1], 5 + 1);
    EXPECT_EQ(attributeUInt.m_DataArray[2], 5 + 2);
    EXPECT_EQ(attributeUInt.m_Name, "attributeUInt");
    EXPECT_EQ(attributeUInt.m_Elements, 3);
    EXPECT_EQ(attributeUInt.m_Type, "unsigned int");

    ASSERT_NE(attributeLInt.m_DataArray, nullptr);
    EXPECT_EQ(attributeLInt.m_DataArray[0], 6);
    EXPECT_EQ(attributeLInt.m_DataArray[1], 6 + 1);
    EXPECT_EQ(attributeLInt.m_DataArray[2], 6 + 2);
    EXPECT_EQ(attributeLInt.m_Name, "attributeLInt");
    EXPECT_EQ(attributeLInt.m_Elements, 3);
    EXPECT_EQ(attributeLInt.m_Type, "long int");

    ASSERT_NE(attributeULInt.m_DataArray, nullptr);
    EXPECT_EQ(attributeULInt.m_DataArray[0], 7);
    EXPECT_EQ(attributeULInt.m_DataArray[1], 7 + 1);
    EXPECT_EQ(attributeULInt.m_DataArray[2], 7 + 2);
    EXPECT_EQ(attributeULInt.m_Name, "attributeULInt");
    EXPECT_EQ(attributeULInt.m_Elements, 3);
    EXPECT_EQ(attributeULInt.m_Type, "unsigned long int");

    ASSERT_NE(attributeLLInt.m_DataArray, nullptr);
    EXPECT_EQ(attributeLLInt.m_DataArray[0], 8);
    EXPECT_EQ(attributeLLInt.m_DataArray[1], 8 + 1);
    EXPECT_EQ(attributeLLInt.m_DataArray[2], 8 + 2);
    EXPECT_EQ(attributeLLInt.m_Name, "attributeLLInt");
    EXPECT_EQ(attributeLLInt.m_Elements, 3);
    EXPECT_EQ(attributeLLInt.m_Type, "long long int");

    ASSERT_NE(attributeULLInt.m_DataArray, nullptr);
    EXPECT_EQ(attributeULLInt.m_DataArray[0], 9);
    EXPECT_EQ(attributeULLInt.m_DataArray[1], 9 + 1);
    EXPECT_EQ(attributeULLInt.m_DataArray[2], 9 + 2);
    EXPECT_EQ(attributeULLInt.m_Name, "attributeULLInt");
    EXPECT_EQ(attributeULLInt.m_Elements, 3);
    EXPECT_EQ(attributeULLInt.m_Type, "unsigned long long int");

    ASSERT_NE(attributeFloat.m_DataArray, nullptr);
    EXPECT_EQ(attributeFloat.m_DataArray[0], 10);
    EXPECT_EQ(attributeFloat.m_DataArray[1], 10 + 1);
    EXPECT_EQ(attributeFloat.m_DataArray[2], 10 + 2);
    EXPECT_EQ(attributeFloat.m_Name, "attributeFloat");
    EXPECT_EQ(attributeFloat.m_Elements, 3);
    EXPECT_EQ(attributeFloat.m_Type, "float");

    ASSERT_NE(attributeDouble.m_DataArray, nullptr);
    EXPECT_EQ(attributeDouble.m_DataArray[0], 11);
    EXPECT_EQ(attributeDouble.m_DataArray[1], 11 + 1);
    EXPECT_EQ(attributeDouble.m_DataArray[2], 11 + 2);
    EXPECT_EQ(attributeDouble.m_Name, "attributeDouble");
    EXPECT_EQ(attributeDouble.m_Elements, 3);
    EXPECT_EQ(attributeDouble.m_Type, "double");

    ASSERT_NE(attributeLDouble.m_DataArray, nullptr);
    EXPECT_EQ(attributeLDouble.m_DataArray[0], 12);
    EXPECT_EQ(attributeLDouble.m_DataArray[1], 12 + 1);
    EXPECT_EQ(attributeLDouble.m_DataArray[2], 12 + 2);
    EXPECT_EQ(attributeLDouble.m_Name, "attributeLDouble");
    EXPECT_EQ(attributeLDouble.m_Elements, 3);
    EXPECT_EQ(attributeLDouble.m_Type, "long double");
}

TEST_F(ADIOSDefineAttributeTest, GetAttribute)
{
    // Define ADIOS global value
    const std::vector<std::string> vString{"-1", "0", "+1"};
    const std::vector<char> vChar = {0, 0 + 1, 0 + 2};
    const std::vector<unsigned char> vUChar = {1, 1 + 1, 1 + 2};
    const std::vector<short> vShort = {2, 2 + 1, 2 + 2};
    const std::vector<unsigned short> vUShort = {3, 3 + 1, 3 + 2};
    const std::vector<int> vInt = {4, 4 + 1, 4 + 2};
    const std::vector<unsigned int> vUInt = {5, 5 + 1, 5 + 2};
    const std::vector<long int> vLInt = {6, 6 + 1, 6 + 2};
    const std::vector<unsigned long int> vULInt = {7, 7 + 1, 7 + 2};
    const std::vector<long long int> vLLInt = {8, 8 + 1, 8 + 2};
    const std::vector<unsigned long long int> vULLInt = {9, 9 + 1, 9 + 2};
    const std::vector<float> vFloat = {10, 10 + 1, 10 + 2};
    const std::vector<double> vDouble = {11, 11 + 1, 11 + 2};
    const std::vector<long double> vLDouble = {12, 12 + 1, 12 + 2};

    {
        io.DefineAttribute<std::string>("attributeString", vString.data(), 3);
        io.DefineAttribute<char>("attributeChar", vChar.data(), 3);
        io.DefineAttribute<unsigned char>("attributeUChar", vUChar.data(), 3);
        io.DefineAttribute<short>("attributeShort", vShort.data(), 3);
        io.DefineAttribute<unsigned short>("attributeUShort", vUShort.data(),
                                           3);
        io.DefineAttribute<int>("attributeInt", vInt.data(), 3);
        io.DefineAttribute<unsigned int>("attributeUInt", vUInt.data(), 3);
        io.DefineAttribute<long int>("attributeLInt", vLInt.data(), 3);
        io.DefineAttribute<unsigned long int>("attributeULInt", vULInt.data(),
                                              3);
        io.DefineAttribute<long long int>("attributeLLInt", vLLInt.data(), 3);
        io.DefineAttribute<unsigned long long int>("attributeULLInt",
                                                   vULLInt.data(), 3);
        io.DefineAttribute<float>("attributeFloat", vFloat.data(), 3);
        io.DefineAttribute<double>("attributeDouble", vDouble.data(), 3);
        io.DefineAttribute<long double>("attributeLDouble", vLDouble.data(), 3);
    }

    auto &attributeString = io.GetAttribute<std::string>("attributeString");
    auto &attributeChar = io.GetAttribute<char>("attributeChar");
    auto &attributeUChar = io.GetAttribute<unsigned char>("attributeUChar");
    auto &attributeShort = io.GetAttribute<short>("attributeShort");
    auto &attributeUShort = io.GetAttribute<unsigned short>("attributeUShort");
    auto &attributeInt = io.GetAttribute<int>("attributeInt");
    auto &attributeUInt = io.GetAttribute<unsigned int>("attributeUInt");
    auto &attributeLInt = io.GetAttribute<long int>("attributeLInt");
    auto &attributeULInt = io.GetAttribute<unsigned long int>("attributeULInt");
    auto &attributeLLInt = io.GetAttribute<long long int>("attributeLLInt");
    auto &attributeULLInt =
        io.GetAttribute<unsigned long long int>("attributeULLInt");
    auto &attributeFloat = io.GetAttribute<float>("attributeFloat");
    auto &attributeDouble = io.GetAttribute<double>("attributeDouble");
    auto &attributeLDouble = io.GetAttribute<long double>("attributeLDouble");

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(attributeString),
                                  adios2::Attribute<std::string> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeChar),
                                  adios2::Attribute<char> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeUChar),
                                  adios2::Attribute<unsigned char> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeShort),
                                  adios2::Attribute<short> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeUShort),
                                  adios2::Attribute<unsigned short> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeInt),
                                  adios2::Attribute<int> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeUInt),
                                  adios2::Attribute<unsigned int> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeLInt),
                                  adios2::Attribute<long int> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeULInt),
                                  adios2::Attribute<unsigned long int> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeLLInt),
                                  adios2::Attribute<long long int> &>();
    ::testing::StaticAssertTypeEq<
        decltype(attributeULLInt),
        adios2::Attribute<unsigned long long int> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeFloat),
                                  adios2::Attribute<float> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeDouble),
                                  adios2::Attribute<double> &>();
    ::testing::StaticAssertTypeEq<decltype(attributeLDouble),
                                  adios2::Attribute<long double> &>();

    // Verify the dimensions, name, and type are correct
    ASSERT_NE(attributeString.m_DataArray, nullptr);
    EXPECT_EQ(attributeString.m_DataArray[0], "-1");
    EXPECT_EQ(attributeString.m_DataArray[1], "0");
    EXPECT_EQ(attributeString.m_DataArray[2], "+1");
    EXPECT_EQ(attributeString.m_Name, "attributeString");
    ASSERT_EQ(attributeString.m_DataValue.empty(), true);
    EXPECT_EQ(attributeString.m_Elements, 3);
    EXPECT_EQ(attributeString.m_Type, "string");

    ASSERT_NE(attributeChar.m_DataArray, nullptr);
    EXPECT_EQ(attributeChar.m_DataArray[0], 0);
    EXPECT_EQ(attributeChar.m_DataArray[1], 0 + 1);
    EXPECT_EQ(attributeChar.m_DataArray[2], 0 + 2);
    EXPECT_EQ(attributeChar.m_Name, "attributeChar");
    EXPECT_EQ(attributeChar.m_Elements, 3);
    EXPECT_EQ(attributeChar.m_Type, "char");

    ASSERT_NE(attributeUChar.m_DataArray, nullptr);
    EXPECT_EQ(attributeUChar.m_DataArray[0], 1);
    EXPECT_EQ(attributeUChar.m_DataArray[1], 1 + 1);
    EXPECT_EQ(attributeUChar.m_DataArray[2], 1 + 2);
    EXPECT_EQ(attributeUChar.m_Name, "attributeUChar");
    EXPECT_EQ(attributeUChar.m_Elements, 3);
    EXPECT_EQ(attributeUChar.m_Type, "unsigned char");

    ASSERT_NE(attributeShort.m_DataArray, nullptr);
    EXPECT_EQ(attributeShort.m_DataArray[0], 2);
    EXPECT_EQ(attributeShort.m_DataArray[1], 2 + 1);
    EXPECT_EQ(attributeShort.m_DataArray[2], 2 + 2);
    EXPECT_EQ(attributeShort.m_Name, "attributeShort");
    EXPECT_EQ(attributeShort.m_Elements, 3);
    EXPECT_EQ(attributeShort.m_Type, "short");

    ASSERT_NE(attributeUShort.m_DataArray, nullptr);
    EXPECT_EQ(attributeUShort.m_DataArray[0], 3);
    EXPECT_EQ(attributeUShort.m_DataArray[1], 3 + 1);
    EXPECT_EQ(attributeUShort.m_DataArray[2], 3 + 2);
    EXPECT_EQ(attributeUShort.m_Name, "attributeUShort");
    EXPECT_EQ(attributeUShort.m_Elements, 3);
    EXPECT_EQ(attributeUShort.m_Type, "unsigned short");

    ASSERT_NE(attributeInt.m_DataArray, nullptr);
    EXPECT_EQ(attributeInt.m_DataArray[0], 4);
    EXPECT_EQ(attributeInt.m_DataArray[1], 4 + 1);
    EXPECT_EQ(attributeInt.m_DataArray[2], 4 + 2);
    EXPECT_EQ(attributeInt.m_Name, "attributeInt");
    EXPECT_EQ(attributeInt.m_Elements, 3);
    EXPECT_EQ(attributeInt.m_Type, "int");

    ASSERT_NE(attributeUInt.m_DataArray, nullptr);
    EXPECT_EQ(attributeUInt.m_DataArray[0], 5);
    EXPECT_EQ(attributeUInt.m_DataArray[1], 5 + 1);
    EXPECT_EQ(attributeUInt.m_DataArray[2], 5 + 2);
    EXPECT_EQ(attributeUInt.m_Name, "attributeUInt");
    EXPECT_EQ(attributeUInt.m_Elements, 3);
    EXPECT_EQ(attributeUInt.m_Type, "unsigned int");

    ASSERT_NE(attributeLInt.m_DataArray, nullptr);
    EXPECT_EQ(attributeLInt.m_DataArray[0], 6);
    EXPECT_EQ(attributeLInt.m_DataArray[1], 6 + 1);
    EXPECT_EQ(attributeLInt.m_DataArray[2], 6 + 2);
    EXPECT_EQ(attributeLInt.m_Name, "attributeLInt");
    EXPECT_EQ(attributeLInt.m_Elements, 3);
    EXPECT_EQ(attributeLInt.m_Type, "long int");

    ASSERT_NE(attributeULInt.m_DataArray, nullptr);
    EXPECT_EQ(attributeULInt.m_DataArray[0], 7);
    EXPECT_EQ(attributeULInt.m_DataArray[1], 7 + 1);
    EXPECT_EQ(attributeULInt.m_DataArray[2], 7 + 2);
    EXPECT_EQ(attributeULInt.m_Name, "attributeULInt");
    EXPECT_EQ(attributeULInt.m_Elements, 3);
    EXPECT_EQ(attributeULInt.m_Type, "unsigned long int");

    ASSERT_NE(attributeLLInt.m_DataArray, nullptr);
    EXPECT_EQ(attributeLLInt.m_DataArray[0], 8);
    EXPECT_EQ(attributeLLInt.m_DataArray[1], 8 + 1);
    EXPECT_EQ(attributeLLInt.m_DataArray[2], 8 + 2);
    EXPECT_EQ(attributeLLInt.m_Name, "attributeLLInt");
    EXPECT_EQ(attributeLLInt.m_Elements, 3);
    EXPECT_EQ(attributeLLInt.m_Type, "long long int");

    ASSERT_NE(attributeULLInt.m_DataArray, nullptr);
    EXPECT_EQ(attributeULLInt.m_DataArray[0], 9);
    EXPECT_EQ(attributeULLInt.m_DataArray[1], 9 + 1);
    EXPECT_EQ(attributeULLInt.m_DataArray[2], 9 + 2);
    EXPECT_EQ(attributeULLInt.m_Name, "attributeULLInt");
    EXPECT_EQ(attributeULLInt.m_Elements, 3);
    EXPECT_EQ(attributeULLInt.m_Type, "unsigned long long int");

    ASSERT_NE(attributeFloat.m_DataArray, nullptr);
    EXPECT_EQ(attributeFloat.m_DataArray[0], 10);
    EXPECT_EQ(attributeFloat.m_DataArray[1], 10 + 1);
    EXPECT_EQ(attributeFloat.m_DataArray[2], 10 + 2);
    EXPECT_EQ(attributeFloat.m_Name, "attributeFloat");
    EXPECT_EQ(attributeFloat.m_Elements, 3);
    EXPECT_EQ(attributeFloat.m_Type, "float");

    ASSERT_NE(attributeDouble.m_DataArray, nullptr);
    EXPECT_EQ(attributeDouble.m_DataArray[0], 11);
    EXPECT_EQ(attributeDouble.m_DataArray[1], 11 + 1);
    EXPECT_EQ(attributeDouble.m_DataArray[2], 11 + 2);
    EXPECT_EQ(attributeDouble.m_Name, "attributeDouble");
    EXPECT_EQ(attributeDouble.m_Elements, 3);
    EXPECT_EQ(attributeDouble.m_Type, "double");

    ASSERT_NE(attributeLDouble.m_DataArray, nullptr);
    EXPECT_EQ(attributeLDouble.m_DataArray[0], 12);
    EXPECT_EQ(attributeLDouble.m_DataArray[1], 12 + 1);
    EXPECT_EQ(attributeLDouble.m_DataArray[2], 12 + 2);
    EXPECT_EQ(attributeLDouble.m_Name, "attributeLDouble");
    EXPECT_EQ(attributeLDouble.m_Elements, 3);
    EXPECT_EQ(attributeLDouble.m_Type, "long double");
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
