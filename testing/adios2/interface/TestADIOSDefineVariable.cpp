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
    adios2::ADIOS adios;
    adios2::IO &io;
};

TEST_F(ADIOSDefineVariableTest, DefineGlobalValue)
{
    int mpiRank = 0, mpiSize = 1;
#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif
    std::string name = std::string("globalValue");

    // Define ADIOS global value
    auto &globalvalue = io.DefineVariable<int>(name);

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(globalvalue),
                                  adios2::Variable<int> &>();

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(globalvalue.m_Shape.size(), 0);
    EXPECT_EQ(globalvalue.m_Start.size(), 0);
    EXPECT_EQ(globalvalue.m_Count.size(), 0);
    EXPECT_EQ(globalvalue.m_Name, name);
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
    int mpiRank = 0, mpiSize = 1;
#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif
    const std::size_t Nx(10), Ny(20), Nz(30);

    adios2::Dims shape{static_cast<unsigned int>(Nx * mpiSize),
                       static_cast<unsigned int>(Ny * mpiSize),
                       static_cast<unsigned int>(Nz * mpiSize)};
    adios2::Dims start{static_cast<unsigned int>(Nx * mpiRank),
                       static_cast<unsigned int>(Ny * mpiRank),
                       static_cast<unsigned int>(Nz * mpiRank)};
    adios2::Dims count{static_cast<unsigned int>(Nx),
                       static_cast<unsigned int>(Ny),
                       static_cast<unsigned int>(Nz)};
    // Define ADIOS global array
    auto &globalarray =
        io.DefineVariable<int>("globalarray", shape, start, count);

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(globalarray),
                                  adios2::Variable<int> &>();

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(globalarray.m_Shape.size(), 3);
    EXPECT_EQ(globalarray.m_Shape[0], Nx * mpiSize);
    EXPECT_EQ(globalarray.m_Shape[1], Ny * mpiSize);
    EXPECT_EQ(globalarray.m_Shape[2], Nz * mpiSize);
    EXPECT_EQ(globalarray.m_Start.size(), 3);
    EXPECT_EQ(globalarray.m_Start[0], Nx * mpiRank);
    EXPECT_EQ(globalarray.m_Start[1], Ny * mpiRank);
    EXPECT_EQ(globalarray.m_Start[2], Nz * mpiRank);
    EXPECT_EQ(globalarray.m_Count.size(), 3);
    EXPECT_EQ(globalarray.m_Count[0], Nx);
    EXPECT_EQ(globalarray.m_Count[1], Ny);
    EXPECT_EQ(globalarray.m_Count[2], Nz);
    EXPECT_EQ(globalarray.m_Name, "globalarray");
    EXPECT_EQ(globalarray.m_Type, "int");
}

TEST_F(ADIOSDefineVariableTest, DefineGlobalArrayWithSelections)
{
    // Define ADIOS global array with postponed size definition in SetSelection

    int mpiRank = 0, mpiSize = 1;
#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif
    const std::size_t Nx(10), Ny(20), Nz(30);

    adios2::Dims shape{static_cast<unsigned int>(Nx * mpiSize),
                       static_cast<unsigned int>(Ny * mpiSize),
                       static_cast<unsigned int>(Nz * mpiSize)};
    adios2::Dims start{static_cast<unsigned int>(Nx * mpiRank),
                       static_cast<unsigned int>(Ny * mpiRank),
                       static_cast<unsigned int>(Nz * mpiRank)};
    adios2::Dims count{static_cast<unsigned int>(Nx),
                       static_cast<unsigned int>(Ny),
                       static_cast<unsigned int>(Nz)};
    // Define ADIOS global array
    auto &globalarray = io.DefineVariable<int>("globalarray", shape);

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(globalarray),
                                  adios2::Variable<int> &>();

    // Make a 3D selection to describe the local dimensions of the
    // variable we write and its offsets in the global spaces
    adios2::Box<adios2::Dims> sel(start, count);
    globalarray.SetSelection(sel);

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(globalarray.m_Shape.size(), 3);
    EXPECT_EQ(globalarray.m_Shape[0], Nx * mpiSize);
    EXPECT_EQ(globalarray.m_Shape[1], Ny * mpiSize);
    EXPECT_EQ(globalarray.m_Shape[2], Nz * mpiSize);
    EXPECT_EQ(globalarray.m_Start.size(), 3);
    EXPECT_EQ(globalarray.m_Start[0], Nx * mpiRank);
    EXPECT_EQ(globalarray.m_Start[1], Ny * mpiRank);
    EXPECT_EQ(globalarray.m_Start[2], Nz * mpiRank);
    EXPECT_EQ(globalarray.m_Count.size(), 3);
    EXPECT_EQ(globalarray.m_Count[0], Nx);
    EXPECT_EQ(globalarray.m_Count[1], Ny);
    EXPECT_EQ(globalarray.m_Count[2], Nz);
    EXPECT_EQ(globalarray.m_Name, "globalarray");
    EXPECT_EQ(globalarray.m_Type, "int");
}

TEST_F(ADIOSDefineVariableTest, DefineGlobalArrayConstantDims)
{
    // Define ADIOS global array with locked-down dimensions
    int mpiRank = 0, mpiSize = 1;
#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif
    const std::size_t Nx(10), Ny(20), Nz(30);

    adios2::Dims shape{static_cast<unsigned int>(Nx * mpiSize),
                       static_cast<unsigned int>(Ny * mpiSize),
                       static_cast<unsigned int>(Nz * mpiSize)};
    adios2::Dims start{static_cast<unsigned int>(Nx * mpiRank),
                       static_cast<unsigned int>(Ny * mpiRank),
                       static_cast<unsigned int>(Nz * mpiRank)};
    adios2::Dims count{static_cast<unsigned int>(Nx),
                       static_cast<unsigned int>(Ny),
                       static_cast<unsigned int>(Nz)};
    // Define ADIOS global array
    auto &globalarray =
        io.DefineVariable<int>("globalarray", shape, start, count, true);

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(globalarray),
                                  adios2::Variable<int> &>();

    // Make a 3D selection to describe the local dimensions of the
    // variable we write and its offsets in the global spaces
    adios2::Box<adios2::Dims> sel(start, count);
    EXPECT_THROW(globalarray.SetSelection(sel), std::invalid_argument);

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(globalarray.m_Shape.size(), 3);
    EXPECT_EQ(globalarray.m_Shape[0], Nx * mpiSize);
    EXPECT_EQ(globalarray.m_Shape[1], Ny * mpiSize);
    EXPECT_EQ(globalarray.m_Shape[2], Nz * mpiSize);
    EXPECT_EQ(globalarray.m_Start.size(), 3);
    EXPECT_EQ(globalarray.m_Start[0], Nx * mpiRank);
    EXPECT_EQ(globalarray.m_Start[1], Ny * mpiRank);
    EXPECT_EQ(globalarray.m_Start[2], Nz * mpiRank);
    EXPECT_EQ(globalarray.m_Count.size(), 3);
    EXPECT_EQ(globalarray.m_Count[0], Nx);
    EXPECT_EQ(globalarray.m_Count[1], Ny);
    EXPECT_EQ(globalarray.m_Count[2], Nz);
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
    int mpiRank = 0, mpiSize = 1;
#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif
    const std::size_t Nx(10), Ny(20), Nz(30);

    adios2::Dims shape{static_cast<unsigned int>(Nx * mpiSize),
                       static_cast<unsigned int>(Ny * mpiSize),
                       static_cast<unsigned int>(Nz * mpiSize)};
    adios2::Dims start{static_cast<unsigned int>(Nx * mpiRank),
                       static_cast<unsigned int>(Ny * mpiRank),
                       static_cast<unsigned int>(Nz * mpiRank)};
    adios2::Dims count{static_cast<unsigned int>(Nx),
                       static_cast<unsigned int>(Ny),
                       static_cast<unsigned int>(Nz)};
    // Define ADIOS global array
    auto &localArray = io.DefineVariable<int>(
        "localArray", {}, {},
        {adios2::UnknownDim, adios2::UnknownDim, adios2::UnknownDim});

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(localArray),
                                  adios2::Variable<int> &>();
    ASSERT_EQ(localArray.m_Shape.size(), 0);
    EXPECT_EQ(localArray.m_Start.size(), 0);
    EXPECT_EQ(localArray.m_Count.size(), 3);
    EXPECT_EQ(localArray.m_Count[0], 0);
    EXPECT_EQ(localArray.m_Count[1], 0);
    EXPECT_EQ(localArray.m_Count[2], 0);
    EXPECT_EQ(localArray.m_Name, "localArray");
    EXPECT_EQ(localArray.m_Type, "int");
    EXPECT_EQ(localArray.m_ShapeID, adios2::ShapeID::LocalArray);

    // Make a 3D selection to describe the local dimensions of the
    // variable we write
    adios2::Box<adios2::Dims> sel({}, {Nx, Ny, Nz});
    localArray.SetSelection(sel);

    adios2::Box<adios2::Dims> selbad(start, count);
    EXPECT_THROW(localArray.SetSelection(selbad), std::invalid_argument);

    // Verify the dimensions, name, and type are correct
    ASSERT_EQ(localArray.m_Shape.size(), 0);
    EXPECT_EQ(localArray.m_Start.size(), 0);
    EXPECT_EQ(localArray.m_Count.size(), 3);
    EXPECT_EQ(localArray.m_Count[0], Nx);
    EXPECT_EQ(localArray.m_Count[1], Ny);
    EXPECT_EQ(localArray.m_Count[2], Nz);
    EXPECT_EQ(localArray.m_Name, "localArray");
    EXPECT_EQ(localArray.m_Type, "int");
    EXPECT_EQ(localArray.m_ShapeID, adios2::ShapeID::LocalArray);
}

TEST_F(ADIOSDefineVariableTest, DefineLocalArrayConstantDims)
{
    int mpiRank = 0, mpiSize = 1;
#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif
    const std::size_t Nx(10), Ny(20), Nz(30);

    adios2::Dims count{static_cast<unsigned int>(Nx),
                       static_cast<unsigned int>(Ny),
                       static_cast<unsigned int>(Nz)};
    // Define ADIOS global array
    auto &localArray =
        io.DefineVariable<int>("localArray", {}, {}, count, true);

    // Verify the return type is as expected
    ::testing::StaticAssertTypeEq<decltype(localArray),
                                  adios2::Variable<int> &>();

    adios2::Box<adios2::Dims> sel({}, count);
    EXPECT_THROW(localArray.SetSelection(sel), std::invalid_argument);

    ASSERT_EQ(localArray.m_Shape.size(), 0);
    EXPECT_EQ(localArray.m_Start.size(), 0);
    EXPECT_EQ(localArray.m_Count.size(), 3);
    EXPECT_EQ(localArray.m_Count[0], Nx);
    EXPECT_EQ(localArray.m_Count[1], Ny);
    EXPECT_EQ(localArray.m_Count[2], Nz);
    EXPECT_EQ(localArray.m_Name, "localArray");
    EXPECT_EQ(localArray.m_Type, "int");
    EXPECT_EQ(localArray.m_ShapeID, adios2::ShapeID::LocalArray);
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

TEST_F(ADIOSDefineVariableTest, DefineString)
{
    // Define ADIOS local array but try to add offsets
    const std::size_t n = 50;
    EXPECT_THROW(io.DefineVariable<std::string>(
                     "invalidString1", {}, {50, n / 2, 0}, {10, n / 2, 30}),
                 std::invalid_argument);
    EXPECT_THROW(io.DefineVariable<std::string>("invalidString2", {}, {}, {1}),
                 std::invalid_argument);

    EXPECT_NO_THROW(io.DefineVariable<std::string>("validString1"));
    EXPECT_NO_THROW(io.DefineVariable<std::string>("validString2", {}, {}, {}));
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
