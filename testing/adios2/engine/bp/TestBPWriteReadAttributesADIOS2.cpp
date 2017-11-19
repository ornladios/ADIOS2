/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include <cstdint>
#include <string>

#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#include "../SmallTestData.h"

class BPWriteReadAttributeTestADIOS2 : public ::testing::Test
{
public:
    BPWriteReadAttributeTestADIOS2() = default;

    SmallTestData m_TestData;
};

// ADIOS2 write, read for single value attributes
TEST_F(BPWriteReadAttributeTestADIOS2, ADIOS2BPWriteReadSingleTypes)
{
    const std::string fName = "foo" + std::string(&adios2::PathSeparator, 1) +
                              "ADIOS2BPWriteAttributeReadSingleTypes.bp";

    int mpiRank = 0, mpiSize = 1;
#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

    const std::string mpiRankString = std::to_string(mpiRank);
    const std::string s1_Single = std::string("s1_Single_") + mpiRankString;
    const std::string i8_Single = std::string("i8_Single_") + mpiRankString;
    const std::string i16_Single = std::string("i16_Single_") + mpiRankString;
    const std::string i32_Single = std::string("i32_Single_") + mpiRankString;
    const std::string i64_Single = std::string("i64_Single_") + mpiRankString;
    const std::string u8_Single = std::string("u8_Single_") + mpiRankString;
    const std::string u16_Single = std::string("u16_Single_") + mpiRankString;
    const std::string u32_Single = std::string("u32_Single_") + mpiRankString;
    const std::string u64_Single = std::string("u64_Single_") + mpiRankString;
    const std::string r32_Single = std::string("r32_Single_") + mpiRankString;
    const std::string r64_Single = std::string("r64_Single_") + mpiRankString;

    // When collective meta generation has landed, use
    // generateNewSmallTestData(m_TestData, 0, mpiRank, mpiSize);
    // Generate current testing data
    SmallTestData currentTestData =
        generateNewSmallTestData(m_TestData, 0, 0, 0);

// Write test data using BP
#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(true);
#endif
    {
        adios2::IO &io = adios.DeclareIO("TestIO");

        // Declare Single Value Attributes
        io.DefineAttribute<std::string>(s1_Single, currentTestData.S1);
        io.DefineAttribute<int8_t>(i8_Single, currentTestData.I8.front());
        io.DefineAttribute<int16_t>(i16_Single, currentTestData.I16.front());
        io.DefineAttribute<int32_t>(i32_Single, currentTestData.I32.front());
        io.DefineAttribute<int64_t>(i64_Single, currentTestData.I64.front());

        io.DefineAttribute<uint8_t>(u8_Single, currentTestData.U8.front());
        io.DefineAttribute<uint16_t>(u16_Single, currentTestData.U16.front());
        io.DefineAttribute<uint32_t>(u32_Single, currentTestData.U32.front());
        io.DefineAttribute<uint64_t>(u64_Single, currentTestData.U64.front());

        io.DefineAttribute<float>(r32_Single, currentTestData.R32.front());
        io.DefineAttribute<double>(r64_Single, currentTestData.R64.front());

        io.SetEngine("BPFileWriter");
        io.AddTransport("file");

        adios2::Engine &engine = io.Open(fName, adios2::Mode::Write);
        // only attributes are written
        engine.Close();
    }

    {
        adios2::IO &ioRead = adios.DeclareIO("ioRead");

        adios2::Engine &bpRead = ioRead.Open(fName, adios2::Mode::Read);

        auto attr_s1 = ioRead.InquireAttribute<std::string>(s1_Single);
        auto attr_i8 = ioRead.InquireAttribute<int8_t>(i8_Single);
        auto attr_i16 = ioRead.InquireAttribute<int16_t>(i16_Single);
        auto attr_i32 = ioRead.InquireAttribute<int32_t>(i32_Single);
        auto attr_i64 = ioRead.InquireAttribute<int64_t>(i64_Single);

        auto attr_u8 = ioRead.InquireAttribute<uint8_t>(u8_Single);
        auto attr_u16 = ioRead.InquireAttribute<uint16_t>(u16_Single);
        auto attr_u32 = ioRead.InquireAttribute<uint32_t>(u32_Single);
        auto attr_u64 = ioRead.InquireAttribute<uint64_t>(u64_Single);

        auto attr_r32 = ioRead.InquireAttribute<float>(r32_Single);
        auto attr_r64 = ioRead.InquireAttribute<double>(r64_Single);

        ASSERT_NE(attr_s1, nullptr);
        ASSERT_EQ(attr_s1->m_Name, s1_Single);
        ASSERT_EQ(attr_s1->m_IsSingleValue, true);
        ASSERT_EQ(attr_s1->m_Type, "string");
        ASSERT_EQ(attr_s1->m_DataSingleValue, currentTestData.S1);

        ASSERT_NE(attr_i8, nullptr);
        ASSERT_EQ(attr_i8->m_Name, i8_Single);
        ASSERT_EQ(attr_i8->m_IsSingleValue, true);
        ASSERT_EQ(attr_i8->m_Type, "signed char");
        ASSERT_EQ(attr_i8->m_DataSingleValue, currentTestData.I8.front());

        ASSERT_NE(attr_i16, nullptr);
        ASSERT_EQ(attr_i16->m_Name, i16_Single);
        ASSERT_EQ(attr_i16->m_IsSingleValue, true);
        ASSERT_EQ(attr_i16->m_Type, "short");
        ASSERT_EQ(attr_i16->m_DataSingleValue, currentTestData.I16.front());

        ASSERT_NE(attr_i32, nullptr);
        ASSERT_EQ(attr_i32->m_Name, i32_Single);
        ASSERT_EQ(attr_i32->m_IsSingleValue, true);
        ASSERT_EQ(attr_i32->m_Type, "int");
        ASSERT_EQ(attr_i32->m_DataSingleValue, currentTestData.I32.front());

        ASSERT_NE(attr_i64, nullptr);
        ASSERT_EQ(attr_i64->m_Name, i64_Single);
        ASSERT_EQ(attr_i64->m_IsSingleValue, true);
#ifdef _WIN32
        ASSERT_EQ(attr_i64->m_Type, "long long int");
#else
        ASSERT_EQ(attr_i64->m_Type, "long int");
#endif
        ASSERT_EQ(attr_i64->m_DataSingleValue, currentTestData.I64.front());

        ASSERT_NE(attr_u8, nullptr);
        ASSERT_EQ(attr_u8->m_Name, u8_Single);
        ASSERT_EQ(attr_u8->m_IsSingleValue, true);
        ASSERT_EQ(attr_u8->m_Type, "unsigned char");
        ASSERT_EQ(attr_u8->m_DataSingleValue, currentTestData.U8.front());

        ASSERT_NE(attr_u16, nullptr);
        ASSERT_EQ(attr_u16->m_Name, u16_Single);
        ASSERT_EQ(attr_u16->m_IsSingleValue, true);
        ASSERT_EQ(attr_u16->m_Type, "unsigned short");
        ASSERT_EQ(attr_u16->m_DataSingleValue, currentTestData.U16.front());

        ASSERT_NE(attr_u32, nullptr);
        ASSERT_EQ(attr_u32->m_Name, u32_Single);
        ASSERT_EQ(attr_u32->m_IsSingleValue, true);
        ASSERT_EQ(attr_u32->m_Type, "unsigned int");
        ASSERT_EQ(attr_u32->m_DataSingleValue, currentTestData.U32.front());

        ASSERT_NE(attr_u64, nullptr);
        ASSERT_EQ(attr_u64->m_Name, u64_Single);
        ASSERT_EQ(attr_u64->m_IsSingleValue, true);

#ifdef _WIN32
        ASSERT_EQ(attr_u64->m_Type, "unsigned long long int");
#else
        ASSERT_EQ(attr_u64->m_Type, "unsigned long int");
#endif
        ASSERT_EQ(attr_u64->m_DataSingleValue, currentTestData.U64.front());

        ASSERT_NE(attr_r32, nullptr);
        ASSERT_EQ(attr_r32->m_Name, r32_Single);
        ASSERT_EQ(attr_r32->m_IsSingleValue, true);
        ASSERT_EQ(attr_r32->m_Type, "float");
        ASSERT_EQ(attr_r32->m_DataSingleValue, currentTestData.R32.front());

        ASSERT_NE(attr_r64, nullptr);
        ASSERT_EQ(attr_r64->m_Name, r64_Single);
        ASSERT_EQ(attr_r64->m_IsSingleValue, true);
        ASSERT_EQ(attr_r64->m_Type, "double");
        ASSERT_EQ(attr_r64->m_DataSingleValue, currentTestData.R64.front());

        bpRead.Close();
    }
}

// ADIOS2 write read for array attributes
TEST_F(BPWriteReadAttributeTestADIOS2, ADIOS2BPWriteReadArrayTypes)
{
    const std::string fName = "foo" + std::string(&adios2::PathSeparator, 1) +
                              "ADIOS2BPWriteAttributeReadArrayTypes.bp";

    int mpiRank = 0, mpiSize = 1;
#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

    const std::string mpiRankString = std::to_string(mpiRank);
    const std::string s1_Array = std::string("s1_Array_") + mpiRankString;
    const std::string i8_Array = std::string("i8_Array_") + mpiRankString;
    const std::string i16_Array = std::string("i16_Array_") + mpiRankString;
    const std::string i32_Array = std::string("i32_Array_") + mpiRankString;
    const std::string i64_Array = std::string("i64_Array_") + mpiRankString;
    const std::string u8_Array = std::string("u8_Array_") + mpiRankString;
    const std::string u16_Array = std::string("u16_Array_") + mpiRankString;
    const std::string u32_Array = std::string("u32_Array_") + mpiRankString;
    const std::string u64_Array = std::string("u64_Array_") + mpiRankString;
    const std::string r32_Array = std::string("r32_Array_") + mpiRankString;
    const std::string r64_Array = std::string("r64_Array_") + mpiRankString;

    // When collective meta generation has landed, use
    // generateNewSmallTestData(m_TestData, 0, mpiRank, mpiSize);
    // Generate current testing data
    SmallTestData currentTestData =
        generateNewSmallTestData(m_TestData, 0, 0, 0);

// Write test data using BP
#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(true);
#endif
    {
        adios2::IO &io = adios.DeclareIO("TestIO");

        // Declare Single Value Attributes
        io.DefineAttribute<std::string>(s1_Array, currentTestData.S3.data(),
                                        currentTestData.S3.size());

        io.DefineAttribute<int8_t>(i8_Array, currentTestData.I8.data(),
                                   currentTestData.I8.size());
        io.DefineAttribute<int16_t>(i16_Array, currentTestData.I16.data(),
                                    currentTestData.I16.size());
        io.DefineAttribute<int32_t>(i32_Array, currentTestData.I32.data(),
                                    currentTestData.I32.size());
        io.DefineAttribute<int64_t>(i64_Array, currentTestData.I64.data(),
                                    currentTestData.I64.size());

        io.DefineAttribute<uint8_t>(u8_Array, currentTestData.U8.data(),
                                    currentTestData.U8.size());
        io.DefineAttribute<uint16_t>(u16_Array, currentTestData.U16.data(),
                                     currentTestData.U16.size());
        io.DefineAttribute<uint32_t>(u32_Array, currentTestData.U32.data(),
                                     currentTestData.U32.size());
        io.DefineAttribute<uint64_t>(u64_Array, currentTestData.U64.data(),
                                     currentTestData.U64.size());

        io.DefineAttribute<float>(r32_Array, currentTestData.R32.data(),
                                  currentTestData.R32.size());
        io.DefineAttribute<double>(r64_Array, currentTestData.R64.data(),
                                   currentTestData.R64.size());

        io.SetEngine("BPFileWriter");
        io.AddTransport("file");

        adios2::Engine &engine = io.Open(fName, adios2::Mode::Write);
        // only attributes are written
        engine.Close();
    }

    {
        adios2::IO &ioRead = adios.DeclareIO("ioRead");

        adios2::Engine &bpRead = ioRead.Open(fName, adios2::Mode::Read);

        auto attr_s1 = ioRead.InquireAttribute<std::string>(s1_Array);

        auto attr_i8 = ioRead.InquireAttribute<int8_t>(i8_Array);
        auto attr_i16 = ioRead.InquireAttribute<int16_t>(i16_Array);
        auto attr_i32 = ioRead.InquireAttribute<int32_t>(i32_Array);
        auto attr_i64 = ioRead.InquireAttribute<int64_t>(i64_Array);

        auto attr_u8 = ioRead.InquireAttribute<uint8_t>(u8_Array);
        auto attr_u16 = ioRead.InquireAttribute<uint16_t>(u16_Array);
        auto attr_u32 = ioRead.InquireAttribute<uint32_t>(u32_Array);
        auto attr_u64 = ioRead.InquireAttribute<uint64_t>(u64_Array);

        auto attr_r32 = ioRead.InquireAttribute<float>(r32_Array);
        auto attr_r64 = ioRead.InquireAttribute<double>(r64_Array);

        ASSERT_NE(attr_s1, nullptr);
        ASSERT_EQ(attr_s1->m_Name, s1_Array);
        ASSERT_EQ(attr_s1->m_IsSingleValue, false);
        ASSERT_EQ(attr_s1->m_Type, "string");

        ASSERT_NE(attr_i8, nullptr);
        ASSERT_EQ(attr_i8->m_Name, i8_Array);
        ASSERT_EQ(attr_i8->m_IsSingleValue, false);
        ASSERT_EQ(attr_i8->m_Type, "signed char");

        ASSERT_NE(attr_i16, nullptr);
        ASSERT_EQ(attr_i16->m_Name, i16_Array);
        ASSERT_EQ(attr_i16->m_IsSingleValue, false);
        ASSERT_EQ(attr_i16->m_Type, "short");

        ASSERT_NE(attr_i32, nullptr);
        ASSERT_EQ(attr_i32->m_Name, i32_Array);
        ASSERT_EQ(attr_i32->m_IsSingleValue, false);
        ASSERT_EQ(attr_i32->m_Type, "int");

        ASSERT_NE(attr_i64, nullptr);
        ASSERT_EQ(attr_i64->m_Name, i64_Array);
        ASSERT_EQ(attr_i64->m_IsSingleValue, false);
#ifdef _WIN32
        ASSERT_EQ(attr_i64->m_Type, "long long int");
#else
        ASSERT_EQ(attr_i64->m_Type, "long int");
#endif

        ASSERT_NE(attr_u8, nullptr);
        ASSERT_EQ(attr_u8->m_Name, u8_Array);
        ASSERT_EQ(attr_u8->m_IsSingleValue, false);
        ASSERT_EQ(attr_u8->m_Type, "unsigned char");

        ASSERT_NE(attr_u16, nullptr);
        ASSERT_EQ(attr_u16->m_Name, u16_Array);
        ASSERT_EQ(attr_u16->m_IsSingleValue, false);
        ASSERT_EQ(attr_u16->m_Type, "unsigned short");

        ASSERT_NE(attr_u32, nullptr);
        ASSERT_EQ(attr_u32->m_Name, u32_Array);
        ASSERT_EQ(attr_u32->m_IsSingleValue, false);
        ASSERT_EQ(attr_u32->m_Type, "unsigned int");

        ASSERT_NE(attr_u64, nullptr);
        ASSERT_EQ(attr_u64->m_Name, u64_Array);
        ASSERT_EQ(attr_u64->m_IsSingleValue, false);
#ifdef _WIN32
        ASSERT_EQ(attr_u64->m_Type, "unsigned long long int");
#else
        ASSERT_EQ(attr_u64->m_Type, "unsigned long int");
#endif
        ASSERT_NE(attr_r32, nullptr);
        ASSERT_EQ(attr_r32->m_Name, r32_Array);
        ASSERT_EQ(attr_r32->m_IsSingleValue, false);
        ASSERT_EQ(attr_r32->m_Type, "float");

        ASSERT_NE(attr_r64, nullptr);
        ASSERT_EQ(attr_r64->m_Name, r64_Array);
        ASSERT_EQ(attr_r64->m_IsSingleValue, false);
        ASSERT_EQ(attr_r64->m_Type, "double");

        auto &I8 = attr_i8->m_DataArray;
        auto &I16 = attr_i16->m_DataArray;
        auto &I32 = attr_i32->m_DataArray;
        auto &I64 = attr_i64->m_DataArray;

        auto &U8 = attr_u8->m_DataArray;
        auto &U16 = attr_u16->m_DataArray;
        auto &U32 = attr_u32->m_DataArray;
        auto &U64 = attr_u64->m_DataArray;

        const size_t Nx = 10;
        for (size_t i = 0; i < Nx; ++i)
        {
            EXPECT_EQ(I8[i], currentTestData.I8[i]);
            EXPECT_EQ(I16[i], currentTestData.I16[i]);
            EXPECT_EQ(I32[i], currentTestData.I32[i]);
            EXPECT_EQ(I64[i], currentTestData.I64[i]);

            EXPECT_EQ(U8[i], currentTestData.U8[i]);
            EXPECT_EQ(U16[i], currentTestData.U16[i]);
            EXPECT_EQ(U32[i], currentTestData.U32[i]);
            EXPECT_EQ(U64[i], currentTestData.U64[i]);
        }

        bpRead.Close();
    }
}

//******************************************************************************
// main
//******************************************************************************

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
