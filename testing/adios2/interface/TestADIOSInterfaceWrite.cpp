#include <cstdint>

#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

class ADIOSInterfaceWriteTest : public ::testing::Test
{
public:
    ADIOSInterfaceWriteTest() : adios(true), io(adios.DeclareIO("TestIO")) {}

protected:
    // virtual void SetUp() { }

    // virtual void TearDown() { }

    adios2::ADIOS adios;
    adios2::IO &io;
};

TEST_F(ADIOSInterfaceWriteTest, DefineVarChar1x10)
{
    // Define ADIOS variables for each type
    auto &var_i8 = io.DefineVariable<char>("i8", {}, {}, adios2::Dims{10});

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(var_i8), adios2::Variable<char> &>();

    // Verify exceptions are thrown upon duplicate variable names
    EXPECT_THROW(auto &foo =
                     io.DefineVariable<char>("i8", {}, {}, adios2::Dims{10}),
                 std::invalid_argument);

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(var_i8.m_Shape.size(), 0);
    EXPECT_EQ(var_i8.m_Start.size(), 0);
    EXPECT_EQ(var_i8.m_Count.size(), 1);
    EXPECT_EQ(var_i8.m_Count[0], 10);
    EXPECT_EQ(var_i8.m_Name, "i8");
    EXPECT_EQ(var_i8.m_Type, "char");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarSChar1x10)
{
    // Define ADIOS variables for each type
    auto &var_si8 =
        io.DefineVariable<signed char>("si8", {}, {}, adios2::Dims{10});

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(var_si8),
                                  adios2::Variable<signed char> &>();

    // Verify exceptions are thrown upon duplicate variable names
    EXPECT_THROW(auto &foo =
                     io.DefineVariable<char>("si8", {}, {}, adios2::Dims{10}),
                 std::invalid_argument);

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(var_si8.m_Shape.size(), 0);
    EXPECT_EQ(var_si8.m_Start.size(), 0);
    EXPECT_EQ(var_si8.m_Count.size(), 1);
    EXPECT_EQ(var_si8.m_Count[0], 10);
    EXPECT_EQ(var_si8.m_Name, "si8");
    EXPECT_EQ(var_si8.m_Type, "signed char");
}

// Rinse  and repeat for remaining types
TEST_F(ADIOSInterfaceWriteTest, DefineVarShort1x10)
{
    auto &var_i16 = io.DefineVariable<short>("i16", {}, {}, adios2::Dims{10});
    ::testing::StaticAssertTypeEq<decltype(var_i16),
                                  adios2::Variable<short> &>();
    EXPECT_THROW(auto &foo =
                     io.DefineVariable<short>("i16", {}, {}, adios2::Dims{10}),
                 std::invalid_argument);
    ASSERT_EQ(var_i16.m_Shape.size(), 0);
    EXPECT_EQ(var_i16.m_Start.size(), 0);
    EXPECT_EQ(var_i16.m_Count.size(), 1);
    EXPECT_EQ(var_i16.m_Count[0], 10);
    EXPECT_EQ(var_i16.m_Name, "i16");
    EXPECT_EQ(var_i16.m_Type, "short");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarInt1x10)
{
    auto &var_i32 = io.DefineVariable<int>("i32", {}, {}, adios2::Dims{10});
    ::testing::StaticAssertTypeEq<decltype(var_i32), adios2::Variable<int> &>();
    EXPECT_THROW(auto &foo =
                     io.DefineVariable<int>("i32", {}, {}, adios2::Dims{10}),
                 std::invalid_argument);
    ASSERT_EQ(var_i32.m_Shape.size(), 0);
    EXPECT_EQ(var_i32.m_Start.size(), 0);
    EXPECT_EQ(var_i32.m_Count.size(), 1);
    EXPECT_EQ(var_i32.m_Count[0], 10);
    EXPECT_EQ(var_i32.m_Name, "i32");
    EXPECT_EQ(var_i32.m_Type, "int");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarLong1x10)
{
    auto &var_u16 = io.DefineVariable<long>("u16", {}, {}, adios2::Dims{10});
    ::testing::StaticAssertTypeEq<decltype(var_u16),
                                  adios2::Variable<long> &>();
    EXPECT_THROW(auto &foo =
                     io.DefineVariable<long>("u16", {}, {}, adios2::Dims{10}),
                 std::invalid_argument);
    ASSERT_EQ(var_u16.m_Shape.size(), 0);
    EXPECT_EQ(var_u16.m_Start.size(), 0);
    EXPECT_EQ(var_u16.m_Count.size(), 1);
    EXPECT_EQ(var_u16.m_Count[0], 10);
    EXPECT_EQ(var_u16.m_Name, "u16");
    EXPECT_EQ(var_u16.m_Type, "long int");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarUChar1x10)
{
    auto &var_u8 =
        io.DefineVariable<unsigned char>("u8", {}, {}, adios2::Dims{10});
    ::testing::StaticAssertTypeEq<decltype(var_u8),
                                  adios2::Variable<unsigned char> &>();
    EXPECT_THROW(auto &foo = io.DefineVariable<unsigned char>("u8", {}, {},
                                                              adios2::Dims{10}),
                 std::invalid_argument);
    ASSERT_EQ(var_u8.m_Shape.size(), 0);
    EXPECT_EQ(var_u8.m_Start.size(), 0);
    EXPECT_EQ(var_u8.m_Count.size(), 1);
    EXPECT_EQ(var_u8.m_Count[0], 10);
    EXPECT_EQ(var_u8.m_Name, "u8");
    EXPECT_EQ(var_u8.m_Type, "unsigned char");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarUShort1x10)
{
    auto &var_u16 =
        io.DefineVariable<unsigned short>("u16", {}, {}, adios2::Dims{10});
    ::testing::StaticAssertTypeEq<decltype(var_u16),
                                  adios2::Variable<unsigned short> &>();
    EXPECT_THROW(auto &foo = io.DefineVariable<unsigned short>(
                     "u16", {}, {}, adios2::Dims{10}),
                 std::invalid_argument);
    ASSERT_EQ(var_u16.m_Shape.size(), 0);
    EXPECT_EQ(var_u16.m_Start.size(), 0);
    EXPECT_EQ(var_u16.m_Count.size(), 1);
    EXPECT_EQ(var_u16.m_Count[0], 10);
    EXPECT_EQ(var_u16.m_Name, "u16");
    EXPECT_EQ(var_u16.m_Type, "unsigned short");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarUInt1x10)
{
    auto &var_u32 =
        io.DefineVariable<unsigned int>("u32", {}, {}, adios2::Dims{10});
    ::testing::StaticAssertTypeEq<decltype(var_u32),
                                  adios2::Variable<unsigned int> &>();
    EXPECT_THROW(auto &foo = io.DefineVariable<unsigned int>("u32", {}, {},
                                                             adios2::Dims{10}),
                 std::invalid_argument);
    ASSERT_EQ(var_u32.m_Shape.size(), 0);
    EXPECT_EQ(var_u32.m_Start.size(), 0);
    EXPECT_EQ(var_u32.m_Count.size(), 1);
    EXPECT_EQ(var_u32.m_Count[0], 10);
    EXPECT_EQ(var_u32.m_Name, "u32");
    EXPECT_EQ(var_u32.m_Type, "unsigned int");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarULong1x10)
{
    auto &var_u64 =
        io.DefineVariable<unsigned long>("u64", {}, {}, adios2::Dims{10});
    ::testing::StaticAssertTypeEq<decltype(var_u64),
                                  adios2::Variable<unsigned long> &>();
    EXPECT_THROW(auto &foo = io.DefineVariable<unsigned long>("u64", {}, {},
                                                              adios2::Dims{10}),
                 std::invalid_argument);
    ASSERT_EQ(var_u64.m_Shape.size(), 0);
    EXPECT_EQ(var_u64.m_Start.size(), 0);
    EXPECT_EQ(var_u64.m_Count.size(), 1);
    EXPECT_EQ(var_u64.m_Count[0], 10);
    EXPECT_EQ(var_u64.m_Name, "u64");
    EXPECT_EQ(var_u64.m_Type, "unsigned long int");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarFloat1x10)
{
    auto &var_r32 = io.DefineVariable<float>("r32", {}, {}, adios2::Dims{10});
    ::testing::StaticAssertTypeEq<decltype(var_r32),
                                  adios2::Variable<float> &>();
    EXPECT_THROW(auto &foo =
                     io.DefineVariable<float>("r32", {}, {}, adios2::Dims{10}),
                 std::invalid_argument);
    ASSERT_EQ(var_r32.m_Shape.size(), 0);
    EXPECT_EQ(var_r32.m_Start.size(), 0);
    EXPECT_EQ(var_r32.m_Count.size(), 1);
    EXPECT_EQ(var_r32.m_Count[0], 10);
    EXPECT_EQ(var_r32.m_Name, "r32");
    EXPECT_EQ(var_r32.m_Type, "float");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarDouble1x10)
{
    auto &var_r64 = io.DefineVariable<double>("r64", {}, {}, adios2::Dims{10});
    ::testing::StaticAssertTypeEq<decltype(var_r64),
                                  adios2::Variable<double> &>();
    EXPECT_THROW(auto &foo =
                     io.DefineVariable<double>("r64", {}, {}, adios2::Dims{10}),
                 std::invalid_argument);
    ASSERT_EQ(var_r64.m_Shape.size(), 0);
    EXPECT_EQ(var_r64.m_Start.size(), 0);
    EXPECT_EQ(var_r64.m_Count.size(), 1);
    EXPECT_EQ(var_r64.m_Count[0], 10);
    EXPECT_EQ(var_r64.m_Name, "r64");
    EXPECT_EQ(var_r64.m_Type, "double");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarChar2x5)
{
    auto &var_i8 = io.DefineVariable<char>("i8", {}, {}, adios2::Dims{2, 5});
    ::testing::StaticAssertTypeEq<decltype(var_i8), adios2::Variable<char> &>();
    EXPECT_THROW(auto &foo =
                     io.DefineVariable<char>("i8", {}, {}, adios2::Dims{2, 5}),
                 std::invalid_argument);
    ASSERT_EQ(var_i8.m_Shape.size(), 0);
    EXPECT_EQ(var_i8.m_Start.size(), 0);
    EXPECT_EQ(var_i8.m_Count.size(), 2);
    EXPECT_EQ(var_i8.m_Count[0], 2);
    EXPECT_EQ(var_i8.m_Count[1], 5);
    EXPECT_EQ(var_i8.m_Name, "i8");
    EXPECT_EQ(var_i8.m_Type, "char");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarShort2x5)
{
    auto &var_i16 = io.DefineVariable<short>("i16", {}, {}, adios2::Dims{2, 5});
    ::testing::StaticAssertTypeEq<decltype(var_i16),
                                  adios2::Variable<short> &>();
    EXPECT_THROW(
        auto &foo = io.DefineVariable<short>("i16", {}, {}, adios2::Dims{2, 5}),
        std::invalid_argument);
    ASSERT_EQ(var_i16.m_Shape.size(), 0);
    EXPECT_EQ(var_i16.m_Start.size(), 0);
    EXPECT_EQ(var_i16.m_Count.size(), 2);
    EXPECT_EQ(var_i16.m_Count[0], 2);
    EXPECT_EQ(var_i16.m_Count[1], 5);
    EXPECT_EQ(var_i16.m_Name, "i16");
    EXPECT_EQ(var_i16.m_Type, "short");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarInt2x5)
{
    auto &var_i32 = io.DefineVariable<int>("i32", {}, {}, adios2::Dims{2, 5});
    ::testing::StaticAssertTypeEq<decltype(var_i32), adios2::Variable<int> &>();
    EXPECT_THROW(auto &foo =
                     io.DefineVariable<int>("i32", {}, {}, adios2::Dims{2, 5}),
                 std::invalid_argument);
    ASSERT_EQ(var_i32.m_Shape.size(), 0);
    EXPECT_EQ(var_i32.m_Start.size(), 0);
    EXPECT_EQ(var_i32.m_Count.size(), 2);
    EXPECT_EQ(var_i32.m_Count[0], 2);
    EXPECT_EQ(var_i32.m_Count[1], 5);
    EXPECT_EQ(var_i32.m_Name, "i32");
    EXPECT_EQ(var_i32.m_Type, "int");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarLong2x5)
{
    auto &var_u16 = io.DefineVariable<long>("u16", {}, {}, adios2::Dims{2, 5});
    ::testing::StaticAssertTypeEq<decltype(var_u16),
                                  adios2::Variable<long> &>();
    EXPECT_THROW(auto &foo =
                     io.DefineVariable<long>("u16", {}, {}, adios2::Dims{2, 5}),
                 std::invalid_argument);
    ASSERT_EQ(var_u16.m_Shape.size(), 0);
    EXPECT_EQ(var_u16.m_Start.size(), 0);
    EXPECT_EQ(var_u16.m_Count.size(), 2);
    EXPECT_EQ(var_u16.m_Count[0], 2);
    EXPECT_EQ(var_u16.m_Count[1], 5);
    EXPECT_EQ(var_u16.m_Name, "u16");
    EXPECT_EQ(var_u16.m_Type, "long int");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarUChar2x5)
{
    auto &var_u8 =
        io.DefineVariable<unsigned char>("u8", {}, {}, adios2::Dims{2, 5});
    ::testing::StaticAssertTypeEq<decltype(var_u8),
                                  adios2::Variable<unsigned char> &>();
    EXPECT_THROW(auto &foo = io.DefineVariable<unsigned char>(
                     "u8", {}, {}, adios2::Dims{2, 5}),
                 std::invalid_argument);
    ASSERT_EQ(var_u8.m_Shape.size(), 0);
    EXPECT_EQ(var_u8.m_Start.size(), 0);
    EXPECT_EQ(var_u8.m_Count.size(), 2);
    EXPECT_EQ(var_u8.m_Count[0], 2);
    EXPECT_EQ(var_u8.m_Count[1], 5);
    EXPECT_EQ(var_u8.m_Name, "u8");
    EXPECT_EQ(var_u8.m_Type, "unsigned char");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarUShort2x5)
{
    auto &var_u16 =
        io.DefineVariable<unsigned short>("u16", {}, {}, adios2::Dims{2, 5});
    ::testing::StaticAssertTypeEq<decltype(var_u16),
                                  adios2::Variable<unsigned short> &>();
    EXPECT_THROW(auto &foo = io.DefineVariable<unsigned short>(
                     "u16", {}, {}, adios2::Dims{2, 5}),
                 std::invalid_argument);
    ASSERT_EQ(var_u16.m_Shape.size(), 0);
    EXPECT_EQ(var_u16.m_Start.size(), 0);
    EXPECT_EQ(var_u16.m_Count.size(), 2);
    EXPECT_EQ(var_u16.m_Count[0], 2);
    EXPECT_EQ(var_u16.m_Count[1], 5);
    EXPECT_EQ(var_u16.m_Name, "u16");
    EXPECT_EQ(var_u16.m_Type, "unsigned short");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarUInt2x5)
{
    auto &var_u32 =
        io.DefineVariable<unsigned int>("u32", {}, {}, adios2::Dims{2, 5});
    ::testing::StaticAssertTypeEq<decltype(var_u32),
                                  adios2::Variable<unsigned int> &>();
    EXPECT_THROW(auto &foo = io.DefineVariable<unsigned int>(
                     "u32", {}, {}, adios2::Dims{2, 5}),
                 std::invalid_argument);
    ASSERT_EQ(var_u32.m_Shape.size(), 0);
    EXPECT_EQ(var_u32.m_Start.size(), 0);
    EXPECT_EQ(var_u32.m_Count.size(), 2);
    EXPECT_EQ(var_u32.m_Count[0], 2);
    EXPECT_EQ(var_u32.m_Count[1], 5);
    EXPECT_EQ(var_u32.m_Name, "u32");
    EXPECT_EQ(var_u32.m_Type, "unsigned int");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarULong2x5)
{
    auto &var_u64 =
        io.DefineVariable<unsigned long>("u64", {}, {}, adios2::Dims{2, 5});
    ::testing::StaticAssertTypeEq<decltype(var_u64),
                                  adios2::Variable<unsigned long> &>();
    EXPECT_THROW(auto &foo = io.DefineVariable<unsigned long>(
                     "u64", {}, {}, adios2::Dims{2, 5}),
                 std::invalid_argument);
    ASSERT_EQ(var_u64.m_Shape.size(), 0);
    EXPECT_EQ(var_u64.m_Start.size(), 0);
    EXPECT_EQ(var_u64.m_Count.size(), 2);
    EXPECT_EQ(var_u64.m_Count[0], 2);
    EXPECT_EQ(var_u64.m_Count[1], 5);
    EXPECT_EQ(var_u64.m_Name, "u64");
    EXPECT_EQ(var_u64.m_Type, "unsigned long int");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarFloat2x5)
{
    auto &var_r32 = io.DefineVariable<float>("r32", {}, {}, adios2::Dims{2, 5});
    ::testing::StaticAssertTypeEq<decltype(var_r32),
                                  adios2::Variable<float> &>();
    EXPECT_THROW(
        auto &foo = io.DefineVariable<float>("r32", {}, {}, adios2::Dims{2, 5}),
        std::invalid_argument);
    ASSERT_EQ(var_r32.m_Shape.size(), 0);
    EXPECT_EQ(var_r32.m_Start.size(), 0);
    EXPECT_EQ(var_r32.m_Count.size(), 2);
    EXPECT_EQ(var_r32.m_Count[0], 2);
    EXPECT_EQ(var_r32.m_Count[1], 5);
    EXPECT_EQ(var_r32.m_Name, "r32");
    EXPECT_EQ(var_r32.m_Type, "float");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVarDouble2x5)
{
    auto &var_r64 =
        io.DefineVariable<double>("r64", {}, {}, adios2::Dims{2, 5});
    ::testing::StaticAssertTypeEq<decltype(var_r64),
                                  adios2::Variable<double> &>();
    EXPECT_THROW(auto &foo = io.DefineVariable<double>("r64", {}, {},
                                                       adios2::Dims{2, 5}),
                 std::invalid_argument);
    ASSERT_EQ(var_r64.m_Shape.size(), 0);
    EXPECT_EQ(var_r64.m_Start.size(), 0);
    EXPECT_EQ(var_r64.m_Count.size(), 2);
    EXPECT_EQ(var_r64.m_Count[0], 2);
    EXPECT_EQ(var_r64.m_Count[1], 5);
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
