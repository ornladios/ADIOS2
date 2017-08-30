#include <cstdint>

#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

class ADIOSDefineVariableTest : public ::testing::Test
{
public:
    ADIOSDefineVariableTest() : adios(true), io(adios.DeclareIO("TestIO")) {}

protected:
    // virtual void SetUp() { }

    // virtual void TearDown() { }

    adios2::ADIOS adios;
    adios2::IO &io;
};

TEST_F(ADIOSDefineVariableTest, DefineGlobalValue)
{
    // Define ADIOS global value
    auto &globalvalue = io.DefineVariable<int>("globalvalue");

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(globalvalue),
                                  adios2::Variable<int> &>();

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(globalvalue.m_Shape.size(), 0);
    EXPECT_EQ(globalvalue.m_Start.size(), 0);
    EXPECT_EQ(globalvalue.m_Count.size(), 0);
    EXPECT_EQ(globalvalue.m_Name, "globalvalue");
    EXPECT_EQ(globalvalue.m_Type, "int");
}

TEST_F(ADIOSDefineVariableTest, DefineLocalValue)
{
    // Define ADIOS local value (a value changing across processes)
    auto &localvalue =
        io.DefineVariable<int>("localvalue", {adios2::LocalValueDim});

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(localvalue),
                                  adios2::Variable<int> &>();

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(localvalue.m_Shape.size(), 1);
    EXPECT_EQ(localvalue.m_Shape[0], adios2::LocalValueDim);
    EXPECT_EQ(localvalue.m_Start.size(), 0);
    EXPECT_EQ(localvalue.m_Count.size(), 0);
    EXPECT_EQ(localvalue.m_Name, "localvalue");
    EXPECT_EQ(localvalue.m_Type, "int");
}

TEST_F(ADIOSDefineVariableTest, DefineGlobalArray)
{
    // Define ADIOS global array
    std::size_t n = 50;
    auto &globalarray = io.DefineVariable<int>("globalarray", {100, n, 30},
                                               {50, n / 2, 0}, {10, n / 2, 30});

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(globalarray),
                                  adios2::Variable<int> &>();

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(globalarray.m_Shape.size(), 3);
    EXPECT_EQ(globalarray.m_Shape[0], 100);
    EXPECT_EQ(globalarray.m_Shape[1], n);
    EXPECT_EQ(globalarray.m_Shape[2], 30);
    EXPECT_EQ(globalarray.m_Start.size(), 3);
    EXPECT_EQ(globalarray.m_Start[0], 50);
    EXPECT_EQ(globalarray.m_Start[1], n / 2);
    EXPECT_EQ(globalarray.m_Start[2], 0);
    EXPECT_EQ(globalarray.m_Count.size(), 3);
    EXPECT_EQ(globalarray.m_Count[0], 10);
    EXPECT_EQ(globalarray.m_Count[1], n / 2);
    EXPECT_EQ(globalarray.m_Count[2], 30);
    EXPECT_EQ(globalarray.m_Name, "globalarray");
    EXPECT_EQ(globalarray.m_Type, "int");
}

TEST_F(ADIOSDefineVariableTest, DefineGlobalArrayWithSelections)
{
    // Define ADIOS global array with postponed size definition in SetSelection
    std::size_t n = 50;
    auto &globalarray = io.DefineVariable<int>("globalarray", {100, n, 30});

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(globalarray),
                                  adios2::Variable<int> &>();

    // Make a 3D selection to describe the local dimensions of the
    // variable we write and its offsets in the global spaces
    globalarray.SetSelection(
        adios2::Box<adios2::Dims>({50, n / 2, 0}, {10, n / 2, 30}));

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(globalarray.m_Shape.size(), 3);
    EXPECT_EQ(globalarray.m_Shape[0], 100);
    EXPECT_EQ(globalarray.m_Shape[1], n);
    EXPECT_EQ(globalarray.m_Shape[2], 30);
    EXPECT_EQ(globalarray.m_Start.size(), 3);
    EXPECT_EQ(globalarray.m_Start[0], 50);
    EXPECT_EQ(globalarray.m_Start[1], n / 2);
    EXPECT_EQ(globalarray.m_Start[2], 0);
    EXPECT_EQ(globalarray.m_Count.size(), 3);
    EXPECT_EQ(globalarray.m_Count[0], 10);
    EXPECT_EQ(globalarray.m_Count[1], n / 2);
    EXPECT_EQ(globalarray.m_Count[2], 30);
    EXPECT_EQ(globalarray.m_Name, "globalarray");
    EXPECT_EQ(globalarray.m_Type, "int");
}

TEST_F(ADIOSDefineVariableTest, DefineGlobalArrayConstantDims)
{
    // Define ADIOS global array with locked-down dimensions
    std::size_t n = 50;
    auto &globalarray = io.DefineVariable<int>(
        "globalarray", {100, n, 30}, {50, n / 2, 0}, {10, n / 2, 30}, true);

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(globalarray),
                                  adios2::Variable<int> &>();

    adios2::Box<adios2::Dims> sel({50, n / 2, 0}, {10, n / 2, 30});
    EXPECT_THROW(globalarray.SetSelection(sel), std::invalid_argument);

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(globalarray.m_Shape.size(), 3);
    EXPECT_EQ(globalarray.m_Shape[0], 100);
    EXPECT_EQ(globalarray.m_Shape[1], n);
    EXPECT_EQ(globalarray.m_Shape[2], 30);
    EXPECT_EQ(globalarray.m_Start.size(), 3);
    EXPECT_EQ(globalarray.m_Start[0], 50);
    EXPECT_EQ(globalarray.m_Start[1], n / 2);
    EXPECT_EQ(globalarray.m_Start[2], 0);
    EXPECT_EQ(globalarray.m_Count.size(), 3);
    EXPECT_EQ(globalarray.m_Count[0], 10);
    EXPECT_EQ(globalarray.m_Count[1], n / 2);
    EXPECT_EQ(globalarray.m_Count[2], 30);
    EXPECT_EQ(globalarray.m_Name, "globalarray");
    EXPECT_EQ(globalarray.m_Type, "int");
}

TEST_F(ADIOSDefineVariableTest, DefineGlobalArrayInvalidLocalValueDim)
{
    // Define ADIOS global array
    std::size_t n = 50;
    adios2::Variable<int> *globalarray;
    EXPECT_THROW(globalarray = &io.DefineVariable<int>(
                     "globalarray", {100, adios2::LocalValueDim, 30},
                     {50, n / 2, 0}, {10, n / 2, 30}),
                 std::invalid_argument);
}

TEST_F(ADIOSDefineVariableTest, DefineLocalArray)
{
    // Define ADIOS local array (no global dimensions, no offsets)
    std::size_t n = 50;
    auto &localarray =
        io.DefineVariable<int>("localarray", {}, {}, {10, n / 2, 30});

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(localarray),
                                  adios2::Variable<int> &>();

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(localarray.m_Shape.size(), 0);
    EXPECT_EQ(localarray.m_Start.size(), 0);
    EXPECT_EQ(localarray.m_Count.size(), 3);
    EXPECT_EQ(localarray.m_Count[0], 10);
    EXPECT_EQ(localarray.m_Count[1], n / 2);
    EXPECT_EQ(localarray.m_Count[2], 30);
    EXPECT_EQ(localarray.m_Name, "localarray");
    EXPECT_EQ(localarray.m_Type, "int");
    EXPECT_EQ(localarray.m_ShapeID, adios2::ShapeID::LocalArray);
}

TEST_F(ADIOSDefineVariableTest, DefineLocalArrayWithSelection)
{
    // Define ADIOS local array with postponed size definition in SetSelection
    std::size_t n = 50;
    auto &localarray = io.DefineVariable<int>(
        "localarray", {}, {},
        {adios2::UnknownDim, adios2::UnknownDim, adios2::UnknownDim});

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(localarray),
                                  adios2::Variable<int> &>();

    ASSERT_EQ(localarray.m_Shape.size(), 0);
    EXPECT_EQ(localarray.m_Start.size(), 0);
    EXPECT_EQ(localarray.m_Count.size(), 3);
    EXPECT_EQ(localarray.m_Count[0], 0);
    EXPECT_EQ(localarray.m_Count[1], 0);
    EXPECT_EQ(localarray.m_Count[2], 0);
    EXPECT_EQ(localarray.m_Name, "localarray");
    EXPECT_EQ(localarray.m_Type, "int");
    EXPECT_EQ(localarray.m_ShapeID, adios2::ShapeID::LocalArray);

    // Make a 3D selection to describe the local dimensions of the
    // variable we write
    localarray.SetSelection(adios2::Box<adios2::Dims>({}, {10, n / 2, 30}));

    adios2::Box<adios2::Dims> selbad({50, n / 2, 0}, {10, n / 2, 30});
    EXPECT_THROW(localarray.SetSelection(selbad), std::invalid_argument);

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(localarray.m_Shape.size(), 0);
    EXPECT_EQ(localarray.m_Start.size(), 0);
    EXPECT_EQ(localarray.m_Count.size(), 3);
    EXPECT_EQ(localarray.m_Count[0], 10);
    EXPECT_EQ(localarray.m_Count[1], n / 2);
    EXPECT_EQ(localarray.m_Count[2], 30);
    EXPECT_EQ(localarray.m_Name, "localarray");
    EXPECT_EQ(localarray.m_Type, "int");
    EXPECT_EQ(localarray.m_ShapeID, adios2::ShapeID::LocalArray);
}

TEST_F(ADIOSDefineVariableTest, DefineLocalArrayConstantDims)
{
    // Define ADIOS local array with locked down dimensions
    std::size_t n = 50;
    auto &localarray =
        io.DefineVariable<int>("localarray", {}, {}, {10, n / 2, 30}, true);

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(localarray),
                                  adios2::Variable<int> &>();

    adios2::Box<adios2::Dims> sel({}, {10, n / 2, 30});
    EXPECT_THROW(localarray.SetSelection(sel), std::invalid_argument);

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(localarray.m_Shape.size(), 0);
    EXPECT_EQ(localarray.m_Start.size(), 0);
    EXPECT_EQ(localarray.m_Count.size(), 3);
    EXPECT_EQ(localarray.m_Count[0], 10);
    EXPECT_EQ(localarray.m_Count[1], n / 2);
    EXPECT_EQ(localarray.m_Count[2], 30);
    EXPECT_EQ(localarray.m_Name, "localarray");
    EXPECT_EQ(localarray.m_Type, "int");
    EXPECT_EQ(localarray.m_ShapeID, adios2::ShapeID::LocalArray);
}

TEST_F(ADIOSDefineVariableTest, DefineLocalArrayInvalidOffsets)
{
    // Define ADIOS local array but try to add offsets
    std::size_t n = 50;

    adios2::Variable<int> *localarray;
    EXPECT_THROW(localarray = &io.DefineVariable<int>(
                     "localarray", {}, {50, n / 2, 0}, {10, n / 2, 30}),
                 std::invalid_argument);
}

TEST_F(ADIOSDefineVariableTest, DefineJoinedArrayFirstDim)
{
    // Define ADIOS joined array
    std::size_t n = 50;
    auto &joinedarray = io.DefineVariable<int>(
        "joinedarray", {adios2::JoinedDim, n, 30}, {}, {10, n, 30});

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(joinedarray),
                                  adios2::Variable<int> &>();

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(joinedarray.m_Shape.size(), 3);
    EXPECT_EQ(joinedarray.m_Shape[0], adios2::JoinedDim);
    EXPECT_EQ(joinedarray.m_Shape[1], n);
    EXPECT_EQ(joinedarray.m_Shape[2], 30);
    EXPECT_EQ(joinedarray.m_Start.size(), 0);
    EXPECT_EQ(joinedarray.m_Count.size(), 3);
    EXPECT_EQ(joinedarray.m_Count[0], 10);
    EXPECT_EQ(joinedarray.m_Count[1], n);
    EXPECT_EQ(joinedarray.m_Count[2], 30);
    EXPECT_EQ(joinedarray.m_Name, "joinedarray");
    EXPECT_EQ(joinedarray.m_Type, "int");
}

TEST_F(ADIOSDefineVariableTest, DefineJoinedArraySecondDim)
{
    // Define ADIOS joined array
    std::size_t n = 50;
    auto &joinedarray = io.DefineVariable<int>(
        "joinedarray", {n, adios2::JoinedDim, 30}, {0, 0, 0}, {n, 10, 30});

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(joinedarray),
                                  adios2::Variable<int> &>();

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(joinedarray.m_Shape.size(), 3);
    EXPECT_EQ(joinedarray.m_Shape[0], n);
    EXPECT_EQ(joinedarray.m_Shape[1], adios2::JoinedDim);
    EXPECT_EQ(joinedarray.m_Shape[2], 30);
    EXPECT_EQ(joinedarray.m_Start.size(), 3);
    EXPECT_EQ(joinedarray.m_Start[0], 0);
    EXPECT_EQ(joinedarray.m_Start[1], 0);
    EXPECT_EQ(joinedarray.m_Start[2], 0);
    EXPECT_EQ(joinedarray.m_Count.size(), 3);
    EXPECT_EQ(joinedarray.m_Count[0], n);
    EXPECT_EQ(joinedarray.m_Count[1], 10);
    EXPECT_EQ(joinedarray.m_Count[2], 30);
    EXPECT_EQ(joinedarray.m_Name, "joinedarray");
    EXPECT_EQ(joinedarray.m_Type, "int");
}

TEST_F(ADIOSDefineVariableTest, DefineJoinedArrayTooManyJoinedDims)
{
    // Define ADIOS joined array
    std::size_t n = 50;
    adios2::Variable<int> *joinedarray;
    EXPECT_THROW(joinedarray = &io.DefineVariable<int>(
                     "joinedarray", {n, adios2::JoinedDim, adios2::JoinedDim},
                     {}, {n, 50, 30}),
                 std::invalid_argument);
}

TEST_F(ADIOSDefineVariableTest, DefineJoinedArrayInvalidStart)
{
    // Define ADIOS joined array
    std::size_t n = 10;
    std::size_t WrongValue = 1;
    adios2::Variable<int> *joinedarray;
    // Start must be empty or full zero array
    EXPECT_THROW(
        joinedarray = &io.DefineVariable<int>(
            "joinedarray", {adios2::JoinedDim, 50}, {0, WrongValue}, {n, 50}),
        std::invalid_argument);
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
