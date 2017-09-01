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

// 1x10
TEST_F(ADIOSInterfaceWriteTest, DefineVar_int8_t_1x10)
{
    int mpiRank = 0, mpiSize = 1;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif
    std::string name = std::string("i8_") + std::to_string(mpiRank);

    // Define ADIOS variables for each type

    auto &var_int8_t =
        io.DefineVariable<int8_t>(name, {}, {}, adios2::Dims{10});

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(var_int8_t),
                                  adios2::Variable<int8_t> &>();

    // Verify exceptions are thrown upon duplicate variable names
    EXPECT_THROW(auto &foo =
                     io.DefineVariable<int8_t>(name, {}, {}, adios2::Dims{10}),
                 std::invalid_argument);

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(var_int8_t.m_Shape.size(), 0);
    EXPECT_EQ(var_int8_t.m_Start.size(), 0);
    EXPECT_EQ(var_int8_t.m_Count.size(), 1);
    EXPECT_EQ(var_int8_t.m_Count[0], 10);
    EXPECT_EQ(var_int8_t.m_Name, name);
    EXPECT_EQ(var_int8_t.m_Type, "signed char");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVar_int16_t_1x10)
{
    int mpiRank = 0, mpiSize = 1;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif
    std::string name = std::string("i16_") + std::to_string(mpiRank);

    // Define ADIOS variables for each type

    auto &var_int16_t =
        io.DefineVariable<int16_t>(name, {}, {}, adios2::Dims{10});

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(var_int16_t),
                                  adios2::Variable<int16_t> &>();

    // Verify exceptions are thrown upon duplicate variable names
    EXPECT_THROW(auto &foo =
                     io.DefineVariable<int16_t>(name, {}, {}, adios2::Dims{10}),
                 std::invalid_argument);

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(var_int16_t.m_Shape.size(), 0);
    EXPECT_EQ(var_int16_t.m_Start.size(), 0);
    EXPECT_EQ(var_int16_t.m_Count.size(), 1);
    EXPECT_EQ(var_int16_t.m_Count[0], 10);
    EXPECT_EQ(var_int16_t.m_Name, name);
    EXPECT_EQ(var_int16_t.m_Type, "short");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVar_int32_t_1x10)
{
    int mpiRank = 0, mpiSize = 1;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif
    std::string name = std::string("i32_") + std::to_string(mpiRank);

    // Define ADIOS variables for each type

    auto &var_int32_t =
        io.DefineVariable<int32_t>(name, {}, {}, adios2::Dims{10});

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(var_int32_t),
                                  adios2::Variable<int32_t> &>();

    // Verify exceptions are thrown upon duplicate variable names
    EXPECT_THROW(auto &foo =
                     io.DefineVariable<int32_t>(name, {}, {}, adios2::Dims{10}),
                 std::invalid_argument);

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(var_int32_t.m_Shape.size(), 0);
    EXPECT_EQ(var_int32_t.m_Start.size(), 0);
    EXPECT_EQ(var_int32_t.m_Count.size(), 1);
    EXPECT_EQ(var_int32_t.m_Count[0], 10);
    EXPECT_EQ(var_int32_t.m_Name, name);
    EXPECT_EQ(var_int32_t.m_Type, "int");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVar_int64_t_1x10)
{
    int mpiRank = 0, mpiSize = 1;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif
    std::string name = std::string("i64_") + std::to_string(mpiRank);

    // Define ADIOS variables for each type

    auto &var_int64_t =
        io.DefineVariable<int64_t>(name, {}, {}, adios2::Dims{10});

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(var_int64_t),
                                  adios2::Variable<int64_t> &>();

    // Verify exceptions are thrown upon duplicate variable names
    EXPECT_THROW(auto &foo =
                     io.DefineVariable<int64_t>(name, {}, {}, adios2::Dims{10}),
                 std::invalid_argument);

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(var_int64_t.m_Shape.size(), 0);
    EXPECT_EQ(var_int64_t.m_Start.size(), 0);
    EXPECT_EQ(var_int64_t.m_Count.size(), 1);
    EXPECT_EQ(var_int64_t.m_Count[0], 10);
    EXPECT_EQ(var_int64_t.m_Name, name);
    EXPECT_EQ(var_int64_t.m_Type, "long int");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVar_uint8_t_1x10)
{
    int mpiRank = 0, mpiSize = 1;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif
    std::string name = std::string("u8_") + std::to_string(mpiRank);

    // Define ADIOS variables for each type

    auto &var_uint8_t =
        io.DefineVariable<uint8_t>(name, {}, {}, adios2::Dims{10});

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(var_uint8_t),
                                  adios2::Variable<uint8_t> &>();

    // Verify exceptions are thrown upon duplicate variable names
    EXPECT_THROW(auto &foo =
                     io.DefineVariable<uint8_t>(name, {}, {}, adios2::Dims{10}),
                 std::invalid_argument);

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(var_uint8_t.m_Shape.size(), 0);
    EXPECT_EQ(var_uint8_t.m_Start.size(), 0);
    EXPECT_EQ(var_uint8_t.m_Count.size(), 1);
    EXPECT_EQ(var_uint8_t.m_Count[0], 10);
    EXPECT_EQ(var_uint8_t.m_Name, name);
    EXPECT_EQ(var_uint8_t.m_Type, "unsigned char");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVar_uint16_t_1x10)
{
    int mpiRank = 0, mpiSize = 1;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif
    std::string name = std::string("u16_") + std::to_string(mpiRank);

    // Define ADIOS variables for each type

    auto &var_uint16_t =
        io.DefineVariable<uint16_t>(name, {}, {}, adios2::Dims{10});

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(var_uint16_t),
                                  adios2::Variable<uint16_t> &>();

    // Verify exceptions are thrown upon duplicate variable names
    EXPECT_THROW(
        auto &foo = io.DefineVariable<uint16_t>(name, {}, {}, adios2::Dims{10}),
        std::invalid_argument);

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(var_uint16_t.m_Shape.size(), 0);
    EXPECT_EQ(var_uint16_t.m_Start.size(), 0);
    EXPECT_EQ(var_uint16_t.m_Count.size(), 1);
    EXPECT_EQ(var_uint16_t.m_Count[0], 10);
    EXPECT_EQ(var_uint16_t.m_Name, name);
    EXPECT_EQ(var_uint16_t.m_Type, "unsigned short");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVar_uint32_t_1x10)
{
    int mpiRank = 0, mpiSize = 1;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif
    std::string name = std::string("u32_") + std::to_string(mpiRank);

    // Define ADIOS variables for each type

    auto &var_uint32_t =
        io.DefineVariable<uint32_t>(name, {}, {}, adios2::Dims{10});

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(var_uint32_t),
                                  adios2::Variable<uint32_t> &>();

    // Verify exceptions are thrown upon duplicate variable names
    EXPECT_THROW(
        auto &foo = io.DefineVariable<uint32_t>(name, {}, {}, adios2::Dims{10}),
        std::invalid_argument);

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(var_uint32_t.m_Shape.size(), 0);
    EXPECT_EQ(var_uint32_t.m_Start.size(), 0);
    EXPECT_EQ(var_uint32_t.m_Count.size(), 1);
    EXPECT_EQ(var_uint32_t.m_Count[0], 10);
    EXPECT_EQ(var_uint32_t.m_Name, name);
    EXPECT_EQ(var_uint32_t.m_Type, "unsigned int");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVar_uint64_t_1x10)
{
    int mpiRank = 0, mpiSize = 1;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif
    std::string name = std::string("u64_") + std::to_string(mpiRank);

    // Define ADIOS variables for each type

    auto &var_uint64_t =
        io.DefineVariable<uint64_t>(name, {}, {}, adios2::Dims{10});

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(var_uint64_t),
                                  adios2::Variable<uint64_t> &>();

    // Verify exceptions are thrown upon duplicate variable names
    EXPECT_THROW(
        auto &foo = io.DefineVariable<uint64_t>(name, {}, {}, adios2::Dims{10}),
        std::invalid_argument);

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(var_uint64_t.m_Shape.size(), 0);
    EXPECT_EQ(var_uint64_t.m_Start.size(), 0);
    EXPECT_EQ(var_uint64_t.m_Count.size(), 1);
    EXPECT_EQ(var_uint64_t.m_Count[0], 10);
    EXPECT_EQ(var_uint64_t.m_Name, name);
    EXPECT_EQ(var_uint64_t.m_Type, "unsigned long int");
}

// Rinse  and repeat for remaining types

// 2x5
TEST_F(ADIOSInterfaceWriteTest, DefineVar_int8_t_2x5)
{
    int mpiRank = 0, mpiSize = 1;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif
    std::string name = std::string("i8_") + std::to_string(mpiRank);

    // Define ADIOS variables for each type

    auto &var_int8_t =
        io.DefineVariable<int8_t>(name, {}, {}, adios2::Dims{2, 5});

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(var_int8_t),
                                  adios2::Variable<int8_t> &>();

    // Verify exceptions are thrown upon duplicate variable names
    EXPECT_THROW(
        auto &foo = io.DefineVariable<int8_t>(name, {}, {}, adios2::Dims{2, 5}),
        std::invalid_argument);

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(var_int8_t.m_Shape.size(), 0);
    EXPECT_EQ(var_int8_t.m_Start.size(), 0);
    EXPECT_EQ(var_int8_t.m_Count.size(), 2);
    EXPECT_EQ(var_int8_t.m_Count[0], 2);
    EXPECT_EQ(var_int8_t.m_Count[1], 5);
    EXPECT_EQ(var_int8_t.m_Name, name);
    EXPECT_EQ(var_int8_t.m_Type, "signed char");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVar_int16_t_2x5)
{
    int mpiRank = 0, mpiSize = 1;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif
    std::string name = std::string("i16_") + std::to_string(mpiRank);

    // Define ADIOS variables for each type

    auto &var_int16_t =
        io.DefineVariable<int16_t>(name, {}, {}, adios2::Dims{2, 5});

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(var_int16_t),
                                  adios2::Variable<int16_t> &>();

    // Verify exceptions are thrown upon duplicate variable names
    EXPECT_THROW(auto &foo = io.DefineVariable<int16_t>(name, {}, {},
                                                        adios2::Dims{2, 5}),
                 std::invalid_argument);

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(var_int16_t.m_Shape.size(), 0);
    EXPECT_EQ(var_int16_t.m_Start.size(), 0);
    EXPECT_EQ(var_int16_t.m_Count.size(), 2);
    EXPECT_EQ(var_int16_t.m_Count[0], 2);
    EXPECT_EQ(var_int16_t.m_Count[1], 5);
    EXPECT_EQ(var_int16_t.m_Name, name);
    EXPECT_EQ(var_int16_t.m_Type, "short");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVar_int32_t_2x5)
{
    int mpiRank = 0, mpiSize = 1;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif
    std::string name = std::string("i32_") + std::to_string(mpiRank);

    // Define ADIOS variables for each type

    auto &var_int32_t =
        io.DefineVariable<int32_t>(name, {}, {}, adios2::Dims{2, 5});

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(var_int32_t),
                                  adios2::Variable<int32_t> &>();

    // Verify exceptions are thrown upon duplicate variable names
    EXPECT_THROW(auto &foo = io.DefineVariable<int32_t>(name, {}, {},
                                                        adios2::Dims{2, 5}),
                 std::invalid_argument);

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(var_int32_t.m_Shape.size(), 0);
    EXPECT_EQ(var_int32_t.m_Start.size(), 0);
    EXPECT_EQ(var_int32_t.m_Count.size(), 2);
    EXPECT_EQ(var_int32_t.m_Count[0], 2);
    EXPECT_EQ(var_int32_t.m_Count[1], 5);
    EXPECT_EQ(var_int32_t.m_Name, name);
    EXPECT_EQ(var_int32_t.m_Type, "int");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVar_int64_t_2x5)
{
    int mpiRank = 0, mpiSize = 1;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif
    std::string name = std::string("i64_") + std::to_string(mpiRank);

    // Define ADIOS variables for each type

    auto &var_int64_t =
        io.DefineVariable<int64_t>(name, {}, {}, adios2::Dims{2, 5});

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(var_int64_t),
                                  adios2::Variable<int64_t> &>();

    // Verify exceptions are thrown upon duplicate variable names
    EXPECT_THROW(auto &foo = io.DefineVariable<int64_t>(name, {}, {},
                                                        adios2::Dims{2, 5}),
                 std::invalid_argument);

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(var_int64_t.m_Shape.size(), 0);
    EXPECT_EQ(var_int64_t.m_Start.size(), 0);
    EXPECT_EQ(var_int64_t.m_Count.size(), 2);
    EXPECT_EQ(var_int64_t.m_Count[0], 2);
    EXPECT_EQ(var_int64_t.m_Count[1], 5);
    EXPECT_EQ(var_int64_t.m_Name, name);
    EXPECT_EQ(var_int64_t.m_Type, "long int");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVar_uint8_t_2x5)
{
    int mpiRank = 0, mpiSize = 1;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif
    std::string name = std::string("u8_") + std::to_string(mpiRank);

    // Define ADIOS variables for each type

    auto &var_uint8_t =
        io.DefineVariable<uint8_t>(name, {}, {}, adios2::Dims{2, 5});

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(var_uint8_t),
                                  adios2::Variable<uint8_t> &>();

    // Verify exceptions are thrown upon duplicate variable names
    EXPECT_THROW(auto &foo = io.DefineVariable<uint8_t>(name, {}, {},
                                                        adios2::Dims{2, 5}),
                 std::invalid_argument);

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(var_uint8_t.m_Shape.size(), 0);
    EXPECT_EQ(var_uint8_t.m_Start.size(), 0);
    EXPECT_EQ(var_uint8_t.m_Count.size(), 2);
    EXPECT_EQ(var_uint8_t.m_Count[0], 2);
    EXPECT_EQ(var_uint8_t.m_Count[1], 5);
    EXPECT_EQ(var_uint8_t.m_Name, name);
    EXPECT_EQ(var_uint8_t.m_Type, "unsigned char");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVar_uint16_t_2x5)
{
    int mpiRank = 0, mpiSize = 1;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif
    std::string name = std::string("u16_") + std::to_string(mpiRank);

    // Define ADIOS variables for each type

    auto &var_uint16_t =
        io.DefineVariable<uint16_t>(name, {}, {}, adios2::Dims{2, 5});

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(var_uint16_t),
                                  adios2::Variable<uint16_t> &>();

    // Verify exceptions are thrown upon duplicate variable names
    EXPECT_THROW(auto &foo = io.DefineVariable<uint16_t>(name, {}, {},
                                                         adios2::Dims{2, 5}),
                 std::invalid_argument);

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(var_uint16_t.m_Shape.size(), 0);
    EXPECT_EQ(var_uint16_t.m_Start.size(), 0);
    EXPECT_EQ(var_uint16_t.m_Count.size(), 2);
    EXPECT_EQ(var_uint16_t.m_Count[0], 2);
    EXPECT_EQ(var_uint16_t.m_Count[1], 5);
    EXPECT_EQ(var_uint16_t.m_Name, name);
    EXPECT_EQ(var_uint16_t.m_Type, "unsigned short");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVar_uint32_t_2x5)
{
    int mpiRank = 0, mpiSize = 1;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif
    std::string name = std::string("u32_") + std::to_string(mpiRank);

    // Define ADIOS variables for each type

    auto &var_uint32_t =
        io.DefineVariable<uint32_t>(name, {}, {}, adios2::Dims{2, 5});

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(var_uint32_t),
                                  adios2::Variable<uint32_t> &>();

    // Verify exceptions are thrown upon duplicate variable names
    EXPECT_THROW(auto &foo = io.DefineVariable<uint32_t>(name, {}, {},
                                                         adios2::Dims{2, 5}),
                 std::invalid_argument);

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(var_uint32_t.m_Shape.size(), 0);
    EXPECT_EQ(var_uint32_t.m_Start.size(), 0);
    EXPECT_EQ(var_uint32_t.m_Count.size(), 2);
    EXPECT_EQ(var_uint32_t.m_Count[0], 2);
    EXPECT_EQ(var_uint32_t.m_Count[1], 5);
    EXPECT_EQ(var_uint32_t.m_Name, name);
    EXPECT_EQ(var_uint32_t.m_Type, "unsigned int");
}

TEST_F(ADIOSInterfaceWriteTest, DefineVar_uint64_t_2x5)
{
    int mpiRank = 0, mpiSize = 1;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif
    std::string name = std::string("u64_") + std::to_string(mpiRank);

    // Define ADIOS variables for each type

    auto &var_uint64_t =
        io.DefineVariable<uint64_t>(name, {}, {}, adios2::Dims{2, 5});

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(var_uint64_t),
                                  adios2::Variable<uint64_t> &>();

    // Verify exceptions are thrown upon duplicate variable names
    EXPECT_THROW(auto &foo = io.DefineVariable<uint64_t>(name, {}, {},
                                                         adios2::Dims{2, 5}),
                 std::invalid_argument);

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(var_uint64_t.m_Shape.size(), 0);
    EXPECT_EQ(var_uint64_t.m_Start.size(), 0);
    EXPECT_EQ(var_uint64_t.m_Count.size(), 2);
    EXPECT_EQ(var_uint64_t.m_Count[0], 2);
    EXPECT_EQ(var_uint64_t.m_Count[1], 5);
    EXPECT_EQ(var_uint64_t.m_Name, name);
    EXPECT_EQ(var_uint64_t.m_Type, "unsigned long int");
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
