#include <cstdint>

#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

class ADIOSInterfaceWriteTest : public ::testing::Test
{
public:
    ADIOSInterfaceWriteTest() : adios(adios::Verbose::WARN, true) {}

protected:
    // virtual void SetUp() { }

    // virtual void TearDown() { }

    adios::ADIOS adios;
};

TEST_F(ADIOSInterfaceWriteTest, DefineVarChar1x10)
{
    // Define ADIOS variables for each type
    auto &var_i8 = adios.DefineVariable<char>("i8", adios::Dims{10});

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(var_i8), adios::Variable<char> &>();

    // Verify exceptions are thrown upon duplicate variable names
    EXPECT_THROW(auto &foo = adios.DefineVariable<char>("i8", adios::Dims{10}),
                 std::invalid_argument);

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(var_i8.m_Shape.size(), 1);
    EXPECT_EQ(var_i8.m_Shape[0], 10);
    EXPECT_EQ(var_i8.m_Count.size(), 0);
    EXPECT_EQ(var_i8.m_Start.size(), 0);
    EXPECT_EQ(var_i8.m_Name, "i8");
    EXPECT_EQ(var_i8.m_Type, "char");
}

// Rinse  and repeat for remaining types
TEST_F(ADIOSInterfaceWriteTest, DefineVarShort1x10)
{
    auto &var_i16 = adios.DefineVariable<short>("i16", adios::Dims{10});
    ::testing::StaticAssertTypeEq<decltype(var_i16),
                                  adios::Variable<short> &>();
    EXPECT_THROW(auto &foo =
                     adios.DefineVariable<short>("i16", adios::Dims{10}),
                 std::invalid_argument);
    ASSERT_EQ(var_i16.m_Shape.size(), 1);
    EXPECT_EQ(var_i16.m_Shape[0], 10);
    EXPECT_EQ(var_i16.m_Count.size(), 0);
    EXPECT_EQ(var_i16.m_Start.size(), 0);
    EXPECT_EQ(var_i16.m_Name, "i16");
    EXPECT_EQ(var_i16.m_Type, "short");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarInt1x10)
{
    auto &var_i32 = adios.DefineVariable<int>("i32", adios::Dims{10});
    ::testing::StaticAssertTypeEq<decltype(var_i32), adios::Variable<int> &>();
    EXPECT_THROW(auto &foo = adios.DefineVariable<int>("i32", adios::Dims{10}),
                 std::invalid_argument);
    ASSERT_EQ(var_i32.m_Shape.size(), 1);
    EXPECT_EQ(var_i32.m_Shape[0], 10);
    EXPECT_EQ(var_i32.m_Count.size(), 0);
    EXPECT_EQ(var_i32.m_Start.size(), 0);
    EXPECT_EQ(var_i32.m_Name, "i32");
    EXPECT_EQ(var_i32.m_Type, "int");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarLong1x10)
{
    auto &var_i64 = adios.DefineVariable<long>("i64", adios::Dims{10});
    ::testing::StaticAssertTypeEq<decltype(var_i64), adios::Variable<long> &>();
    EXPECT_THROW(auto &foo = adios.DefineVariable<long>("i64", adios::Dims{10}),
                 std::invalid_argument);
    ASSERT_EQ(var_i64.m_Shape.size(), 1);
    EXPECT_EQ(var_i64.m_Shape[0], 10);
    EXPECT_EQ(var_i64.m_Count.size(), 0);
    EXPECT_EQ(var_i64.m_Start.size(), 0);
    EXPECT_EQ(var_i64.m_Name, "i64");
    EXPECT_EQ(var_i64.m_Type, "long int");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarUChar1x10)
{
    auto &var_u8 = adios.DefineVariable<unsigned char>("u8", adios::Dims{10});
    ::testing::StaticAssertTypeEq<decltype(var_u8),
                                  adios::Variable<unsigned char> &>();
    EXPECT_THROW(auto &foo =
                     adios.DefineVariable<unsigned char>("u8", adios::Dims{10}),
                 std::invalid_argument);
    ASSERT_EQ(var_u8.m_Shape.size(), 1);
    EXPECT_EQ(var_u8.m_Shape[0], 10);
    EXPECT_EQ(var_u8.m_Count.size(), 0);
    EXPECT_EQ(var_u8.m_Start.size(), 0);
    EXPECT_EQ(var_u8.m_Name, "u8");
    EXPECT_EQ(var_u8.m_Type, "unsigned char");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarUShort1x10)
{
    auto &var_u16 =
        adios.DefineVariable<unsigned short>("u16", adios::Dims{10});
    ::testing::StaticAssertTypeEq<decltype(var_u16),
                                  adios::Variable<unsigned short> &>();
    EXPECT_THROW(auto &foo = adios.DefineVariable<unsigned short>(
                     "u16", adios::Dims{10}),
                 std::invalid_argument);
    ASSERT_EQ(var_u16.m_Shape.size(), 1);
    EXPECT_EQ(var_u16.m_Shape[0], 10);
    EXPECT_EQ(var_u16.m_Count.size(), 0);
    EXPECT_EQ(var_u16.m_Start.size(), 0);
    EXPECT_EQ(var_u16.m_Name, "u16");
    EXPECT_EQ(var_u16.m_Type, "unsigned short");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarUInt1x10)
{
    auto &var_u32 = adios.DefineVariable<unsigned int>("u32", adios::Dims{10});
    ::testing::StaticAssertTypeEq<decltype(var_u32),
                                  adios::Variable<unsigned int> &>();
    EXPECT_THROW(auto &foo =
                     adios.DefineVariable<unsigned int>("u32", adios::Dims{10}),
                 std::invalid_argument);
    ASSERT_EQ(var_u32.m_Shape.size(), 1);
    EXPECT_EQ(var_u32.m_Shape[0], 10);
    EXPECT_EQ(var_u32.m_Count.size(), 0);
    EXPECT_EQ(var_u32.m_Start.size(), 0);
    EXPECT_EQ(var_u32.m_Name, "u32");
    EXPECT_EQ(var_u32.m_Type, "unsigned int");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarULong1x10)
{
    auto &var_u64 = adios.DefineVariable<unsigned long>("u64", adios::Dims{10});
    ::testing::StaticAssertTypeEq<decltype(var_u64),
                                  adios::Variable<unsigned long> &>();
    EXPECT_THROW(
        auto &foo = adios.DefineVariable<unsigned long>("u64", adios::Dims{10}),
        std::invalid_argument);
    ASSERT_EQ(var_u64.m_Shape.size(), 1);
    EXPECT_EQ(var_u64.m_Shape[0], 10);
    EXPECT_EQ(var_u64.m_Count.size(), 0);
    EXPECT_EQ(var_u64.m_Start.size(), 0);
    EXPECT_EQ(var_u64.m_Name, "u64");
    EXPECT_EQ(var_u64.m_Type, "unsigned long int");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarFloat1x10)
{
    auto &var_r32 = adios.DefineVariable<float>("r32", adios::Dims{10});
    ::testing::StaticAssertTypeEq<decltype(var_r32),
                                  adios::Variable<float> &>();
    EXPECT_THROW(auto &foo =
                     adios.DefineVariable<float>("r32", adios::Dims{10}),
                 std::invalid_argument);
    ASSERT_EQ(var_r32.m_Shape.size(), 1);
    EXPECT_EQ(var_r32.m_Shape[0], 10);
    EXPECT_EQ(var_r32.m_Count.size(), 0);
    EXPECT_EQ(var_r32.m_Start.size(), 0);
    EXPECT_EQ(var_r32.m_Name, "r32");
    EXPECT_EQ(var_r32.m_Type, "float");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarDouble1x10)
{
    auto &var_r64 = adios.DefineVariable<double>("r64", adios::Dims{10});
    ::testing::StaticAssertTypeEq<decltype(var_r64),
                                  adios::Variable<double> &>();
    EXPECT_THROW(auto &foo =
                     adios.DefineVariable<double>("r64", adios::Dims{10}),
                 std::invalid_argument);
    ASSERT_EQ(var_r64.m_Shape.size(), 1);
    EXPECT_EQ(var_r64.m_Shape[0], 10);
    EXPECT_EQ(var_r64.m_Count.size(), 0);
    EXPECT_EQ(var_r64.m_Start.size(), 0);
    EXPECT_EQ(var_r64.m_Name, "r64");
    EXPECT_EQ(var_r64.m_Type, "double");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarChar2x5)
{
    auto &var_i8 = adios.DefineVariable<char>("i8", adios::Dims{2, 5});
    ::testing::StaticAssertTypeEq<decltype(var_i8), adios::Variable<char> &>();
    EXPECT_THROW(auto &foo =
                     adios.DefineVariable<char>("i8", adios::Dims{2, 5}),
                 std::invalid_argument);
    ASSERT_EQ(var_i8.m_Shape.size(), 2);
    EXPECT_EQ(var_i8.m_Shape[0], 2);
    EXPECT_EQ(var_i8.m_Shape[1], 5);
    EXPECT_EQ(var_i8.m_Count.size(), 0);
    EXPECT_EQ(var_i8.m_Start.size(), 0);
    EXPECT_EQ(var_i8.m_Name, "i8");
    EXPECT_EQ(var_i8.m_Type, "char");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarShort2x5)
{
    auto &var_i16 = adios.DefineVariable<short>("i16", adios::Dims{2, 5});
    ::testing::StaticAssertTypeEq<decltype(var_i16),
                                  adios::Variable<short> &>();
    EXPECT_THROW(auto &foo =
                     adios.DefineVariable<short>("i16", adios::Dims{2, 5}),
                 std::invalid_argument);
    ASSERT_EQ(var_i16.m_Shape.size(), 2);
    EXPECT_EQ(var_i16.m_Shape[0], 2);
    EXPECT_EQ(var_i16.m_Shape[1], 5);
    EXPECT_EQ(var_i16.m_Count.size(), 0);
    EXPECT_EQ(var_i16.m_Start.size(), 0);
    EXPECT_EQ(var_i16.m_Name, "i16");
    EXPECT_EQ(var_i16.m_Type, "short");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarInt2x5)
{
    auto &var_i32 = adios.DefineVariable<int>("i32", adios::Dims{2, 5});
    ::testing::StaticAssertTypeEq<decltype(var_i32), adios::Variable<int> &>();
    EXPECT_THROW(auto &foo =
                     adios.DefineVariable<int>("i32", adios::Dims{2, 5}),
                 std::invalid_argument);
    ASSERT_EQ(var_i32.m_Shape.size(), 2);
    EXPECT_EQ(var_i32.m_Shape[0], 2);
    EXPECT_EQ(var_i32.m_Shape[1], 5);
    EXPECT_EQ(var_i32.m_Count.size(), 0);
    EXPECT_EQ(var_i32.m_Start.size(), 0);
    EXPECT_EQ(var_i32.m_Name, "i32");
    EXPECT_EQ(var_i32.m_Type, "int");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarLong2x5)
{
    auto &var_i64 = adios.DefineVariable<long>("i64", adios::Dims{2, 5});
    ::testing::StaticAssertTypeEq<decltype(var_i64), adios::Variable<long> &>();
    EXPECT_THROW(auto &foo =
                     adios.DefineVariable<long>("i64", adios::Dims{2, 5}),
                 std::invalid_argument);
    ASSERT_EQ(var_i64.m_Shape.size(), 2);
    EXPECT_EQ(var_i64.m_Shape[0], 2);
    EXPECT_EQ(var_i64.m_Shape[1], 5);
    EXPECT_EQ(var_i64.m_Count.size(), 0);
    EXPECT_EQ(var_i64.m_Start.size(), 0);
    EXPECT_EQ(var_i64.m_Name, "i64");
    EXPECT_EQ(var_i64.m_Type, "long int");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarUChar2x5)
{
    auto &var_u8 = adios.DefineVariable<unsigned char>("u8", adios::Dims{2, 5});
    ::testing::StaticAssertTypeEq<decltype(var_u8),
                                  adios::Variable<unsigned char> &>();
    EXPECT_THROW(auto &foo = adios.DefineVariable<unsigned char>(
                     "u8", adios::Dims{2, 5}),
                 std::invalid_argument);
    ASSERT_EQ(var_u8.m_Shape.size(), 2);
    EXPECT_EQ(var_u8.m_Shape[0], 2);
    EXPECT_EQ(var_u8.m_Shape[1], 5);
    EXPECT_EQ(var_u8.m_Count.size(), 0);
    EXPECT_EQ(var_u8.m_Start.size(), 0);
    EXPECT_EQ(var_u8.m_Name, "u8");
    EXPECT_EQ(var_u8.m_Type, "unsigned char");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarUShort2x5)
{
    auto &var_u16 =
        adios.DefineVariable<unsigned short>("u16", adios::Dims{2, 5});
    ::testing::StaticAssertTypeEq<decltype(var_u16),
                                  adios::Variable<unsigned short> &>();
    EXPECT_THROW(auto &foo = adios.DefineVariable<unsigned short>(
                     "u16", adios::Dims{2, 5}),
                 std::invalid_argument);
    ASSERT_EQ(var_u16.m_Shape.size(), 2);
    EXPECT_EQ(var_u16.m_Shape[0], 2);
    EXPECT_EQ(var_u16.m_Shape[1], 5);
    EXPECT_EQ(var_u16.m_Count.size(), 0);
    EXPECT_EQ(var_u16.m_Start.size(), 0);
    EXPECT_EQ(var_u16.m_Name, "u16");
    EXPECT_EQ(var_u16.m_Type, "unsigned short");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarUInt2x5)
{
    auto &var_u32 =
        adios.DefineVariable<unsigned int>("u32", adios::Dims{2, 5});
    ::testing::StaticAssertTypeEq<decltype(var_u32),
                                  adios::Variable<unsigned int> &>();
    EXPECT_THROW(auto &foo = adios.DefineVariable<unsigned int>(
                     "u32", adios::Dims{2, 5}),
                 std::invalid_argument);
    ASSERT_EQ(var_u32.m_Shape.size(), 2);
    EXPECT_EQ(var_u32.m_Shape[0], 2);
    EXPECT_EQ(var_u32.m_Shape[1], 5);
    EXPECT_EQ(var_u32.m_Count.size(), 0);
    EXPECT_EQ(var_u32.m_Start.size(), 0);
    EXPECT_EQ(var_u32.m_Name, "u32");
    EXPECT_EQ(var_u32.m_Type, "unsigned int");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarULong2x5)
{
    auto &var_u64 =
        adios.DefineVariable<unsigned long>("u64", adios::Dims{2, 5});
    ::testing::StaticAssertTypeEq<decltype(var_u64),
                                  adios::Variable<unsigned long> &>();
    EXPECT_THROW(auto &foo = adios.DefineVariable<unsigned long>(
                     "u64", adios::Dims{2, 5}),
                 std::invalid_argument);
    ASSERT_EQ(var_u64.m_Shape.size(), 2);
    EXPECT_EQ(var_u64.m_Shape[0], 2);
    EXPECT_EQ(var_u64.m_Shape[1], 5);
    EXPECT_EQ(var_u64.m_Count.size(), 0);
    EXPECT_EQ(var_u64.m_Start.size(), 0);
    EXPECT_EQ(var_u64.m_Name, "u64");
    EXPECT_EQ(var_u64.m_Type, "unsigned long int");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarFloat2x5)
{
    auto &var_r32 = adios.DefineVariable<float>("r32", adios::Dims{2, 5});
    ::testing::StaticAssertTypeEq<decltype(var_r32),
                                  adios::Variable<float> &>();
    EXPECT_THROW(auto &foo =
                     adios.DefineVariable<float>("r32", adios::Dims{2, 5}),
                 std::invalid_argument);
    ASSERT_EQ(var_r32.m_Shape.size(), 2);
    EXPECT_EQ(var_r32.m_Shape[0], 2);
    EXPECT_EQ(var_r32.m_Shape[1], 5);
    EXPECT_EQ(var_r32.m_Count.size(), 0);
    EXPECT_EQ(var_r32.m_Start.size(), 0);
    EXPECT_EQ(var_r32.m_Name, "r32");
    EXPECT_EQ(var_r32.m_Type, "float");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarDouble2x5)
{
    auto &var_r64 = adios.DefineVariable<double>("r64", adios::Dims{2, 5});
    ::testing::StaticAssertTypeEq<decltype(var_r64),
                                  adios::Variable<double> &>();
    EXPECT_THROW(auto &foo =
                     adios.DefineVariable<double>("r64", adios::Dims{2, 5}),
                 std::invalid_argument);
    ASSERT_EQ(var_r64.m_Shape.size(), 2);
    EXPECT_EQ(var_r64.m_Shape[0], 2);
    EXPECT_EQ(var_r64.m_Shape[1], 5);
    EXPECT_EQ(var_r64.m_Count.size(), 0);
    EXPECT_EQ(var_r64.m_Start.size(), 0);
    EXPECT_EQ(var_r64.m_Name, "r64");
    EXPECT_EQ(var_r64.m_Type, "double");
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
