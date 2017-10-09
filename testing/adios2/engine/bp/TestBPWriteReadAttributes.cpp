/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include <cstdint>
#include <cstring>

#include <iostream>
#include <stdexcept>

#include <adios2.h>
#include <adios_read.h>

#include <gtest/gtest.h>

#include "../SmallTestData.h"

class BPWriteReadAttributeTest : public ::testing::Test
{
public:
    BPWriteReadAttributeTest() = default;

    SmallTestData m_TestData;
};

//******************************************************************************
// 1D 1x8 test data
//******************************************************************************

// ADIOS2 write, native ADIOS1 read for single value attributes
TEST_F(BPWriteReadAttributeTest, ADIOS2BPWriteADIOS1ReadSingleTypes)
{
    std::string fname = "ADIOS2BPWriteAttributeADIOS1ReadSingleTypes.bp";

    // Write test data using ADIOS2
    {
        adios2::ADIOS adios(true);
        adios2::IO &io = adios.DeclareIO("TestIO");

        // Declare Single Value Attributes
        {
            io.DefineAttribute<std::string>("s1_Single", m_TestData.S1);
            io.DefineAttribute<int8_t>("i8_Single", m_TestData.I8.front());
            io.DefineAttribute<int16_t>("i16_Single", m_TestData.I16.front());
            io.DefineAttribute<int32_t>("i32_Single", m_TestData.I32.front());
            io.DefineAttribute<int64_t>("i64_Single", m_TestData.I64.front());

            io.DefineAttribute<uint8_t>("u8_Single", m_TestData.U8.front());
            io.DefineAttribute<uint16_t>("u16_Single", m_TestData.U16.front());
            io.DefineAttribute<uint32_t>("u32_Single", m_TestData.U32.front());
            io.DefineAttribute<uint64_t>("u64_Single", m_TestData.U64.front());

            io.DefineAttribute<float>("float_Single", m_TestData.R32.front());
            io.DefineAttribute<double>("double_Single", m_TestData.R64.front());
        }

        // Create the BP Engine
        io.SetEngine("BPFileWriter");
        io.AddTransport("File");

        adios2::Engine &engine = io.Open(fname, adios2::Mode::Write);

        // Close the file
        engine.Close();
    }

// Read test data using ADIOS1
#ifdef ADIOS2_HAVE_MPI
    // Read everything from rank 0
    int rank;
    MPI_Comm_rank();
    if (rank == 0)
#endif
    {
        adios_read_init_method(ADIOS_READ_METHOD_BP, MPI_COMM_WORLD,
                               "verbose=3");

        // Open the file for reading
        ADIOS_FILE *f =
            adios_read_open_file((fname + ".dir/" + fname + ".0").c_str(),
                                 ADIOS_READ_METHOD_BP, MPI_COMM_WORLD);
        ASSERT_NE(f, nullptr);

        int size, status;
        enum ADIOS_DATATYPES type;
        void *data = nullptr;

        status = adios_get_attr(f, "s1_Single", &type, &size, &data);
        ASSERT_EQ(status, 0);
        ASSERT_NE(data, nullptr);
        ASSERT_EQ(type, adios_string);
        std::string singleStringAttribute(reinterpret_cast<char *>(data), size);
        ASSERT_STREQ(singleStringAttribute.c_str(), m_TestData.S1.c_str());

        status = adios_get_attr(f, "i8_Single", &type, &size, &data);
        ASSERT_EQ(status, 0);
        ASSERT_NE(data, nullptr);
        ASSERT_EQ(type, adios_byte);
        ASSERT_EQ(*reinterpret_cast<int8_t *>(data), m_TestData.I8.front());

        status = adios_get_attr(f, "i16_Single", &type, &size, &data);
        ASSERT_EQ(status, 0);
        ASSERT_NE(data, nullptr);
        ASSERT_EQ(type, adios_short);
        ASSERT_EQ(*reinterpret_cast<int16_t *>(data), m_TestData.I16.front());

        status = adios_get_attr(f, "i32_Single", &type, &size, &data);
        ASSERT_EQ(status, 0);
        ASSERT_NE(data, nullptr);
        ASSERT_EQ(type, adios_integer);
        ASSERT_EQ(*reinterpret_cast<int32_t *>(data), m_TestData.I32.front());

        status = adios_get_attr(f, "i64_Single", &type, &size, &data);
        ASSERT_EQ(status, 0);
        ASSERT_NE(data, nullptr);
        ASSERT_EQ(type, adios_long);
        ASSERT_EQ(*reinterpret_cast<int64_t *>(data), m_TestData.I64.front());

        status = adios_get_attr(f, "u8_Single", &type, &size, &data);
        ASSERT_EQ(status, 0);
        ASSERT_NE(data, nullptr);
        ASSERT_EQ(type, adios_unsigned_byte);
        ASSERT_EQ(*reinterpret_cast<uint8_t *>(data), m_TestData.U8.front());

        status = adios_get_attr(f, "u16_Single", &type, &size, &data);
        ASSERT_EQ(status, 0);
        ASSERT_NE(data, nullptr);
        ASSERT_EQ(type, adios_unsigned_short);
        ASSERT_EQ(*reinterpret_cast<uint16_t *>(data), m_TestData.U16.front());

        status = adios_get_attr(f, "u32_Single", &type, &size, &data);
        ASSERT_EQ(status, 0);
        ASSERT_NE(data, nullptr);
        ASSERT_EQ(type, adios_unsigned_integer);
        ASSERT_EQ(*reinterpret_cast<uint32_t *>(data), m_TestData.U32.front());

        status = adios_get_attr(f, "u64_Single", &type, &size, &data);
        ASSERT_EQ(status, 0);
        ASSERT_NE(data, nullptr);
        ASSERT_EQ(type, adios_unsigned_long);
        ASSERT_EQ(*reinterpret_cast<uint64_t *>(data), m_TestData.U64.front());

        status = adios_get_attr(f, "float_Single", &type, &size, &data);
        ASSERT_EQ(status, 0);
        ASSERT_NE(data, nullptr);
        ASSERT_EQ(type, adios_real);
        ASSERT_EQ(*reinterpret_cast<float *>(data), m_TestData.R32.front());

        status = adios_get_attr(f, "double_Single", &type, &size, &data);
        ASSERT_EQ(status, 0);
        ASSERT_NE(data, nullptr);
        ASSERT_EQ(type, adios_double);
        ASSERT_EQ(*reinterpret_cast<double *>(data), m_TestData.R64.front());

        // Cleanup file
        adios_read_close(f);
    }
}

// ADIOS2 write, native ADIOS1 read for array attributes
TEST_F(BPWriteReadAttributeTest, ADIOS2BPWriteADIOS1ReadArrayTypes)
{
    std::string fname = "ADIOS2BPWriteAttributeADIOS1ReadArrayTypes.bp";

    // Write test data using ADIOS2
    {
        adios2::ADIOS adios(true);
        adios2::IO &io = adios.DeclareIO("TestIO");

        // Declare Array Attributes
        {
            io.DefineAttribute<std::string>("s3_Array", m_TestData.S3.data(),
                                            m_TestData.S3.size());
            io.DefineAttribute<int8_t>("i8_Array", m_TestData.I8.data(),
                                       m_TestData.I8.size());
            io.DefineAttribute<int16_t>("i16_Array", m_TestData.I16.data(),
                                        m_TestData.I16.size());
            io.DefineAttribute<int32_t>("i32_Array", m_TestData.I32.data(),
                                        m_TestData.I32.size());
            io.DefineAttribute<int64_t>("i64_Array", m_TestData.I64.data(),
                                        m_TestData.I64.size());

            io.DefineAttribute<uint8_t>("u8_Array", m_TestData.U8.data(),
                                        m_TestData.U8.size());
            io.DefineAttribute<uint16_t>("u16_Array", m_TestData.U16.data(),
                                         m_TestData.U16.size());
            io.DefineAttribute<uint32_t>("u32_Array", m_TestData.U32.data(),
                                         m_TestData.U32.size());
            io.DefineAttribute<uint64_t>("u64_Array", m_TestData.U64.data(),
                                         m_TestData.U64.size());

            io.DefineAttribute<float>("float_Array", m_TestData.R32.data(),
                                      m_TestData.R32.size());
            io.DefineAttribute<double>("double_Array", m_TestData.R64.data(),
                                       m_TestData.R64.size());
        }

        // Create the BP Engine
        io.SetEngine("BPFileWriter");
        io.AddTransport("File");

        adios2::Engine &engine = io.Open(fname, adios2::Mode::Write);

        // Close the file
        engine.Close();
    }

// Read test data using ADIOS1
#ifdef ADIOS2_HAVE_MPI
    // Read everything from rank 0
    int rank;
    MPI_Comm_rank();
    if (rank == 0)
#endif
    {
        adios_read_init_method(ADIOS_READ_METHOD_BP, MPI_COMM_WORLD,
                               "verbose=3");

        // Open the file for reading
        ADIOS_FILE *f =
            adios_read_open_file((fname + ".dir/" + fname + ".0").c_str(),
                                 ADIOS_READ_METHOD_BP, MPI_COMM_WORLD);
        ASSERT_NE(f, nullptr);

        int size, status;
        enum ADIOS_DATATYPES type;
        void *data = nullptr;

        status = adios_get_attr(f, "s3_Array", &type, &size, &data);
        ASSERT_EQ(status, 0);
        ASSERT_NE(data, nullptr);
        ASSERT_EQ(type, adios_string_array);
        char **stringArray = reinterpret_cast<char **>(data);

        for (unsigned int i = 0; i < 3; ++i)
        {
            ASSERT_STREQ(std::string(stringArray[i]).c_str(),
                         m_TestData.S3[i].c_str());
        }

        status = adios_get_attr(f, "i8_Array", &type, &size, &data);
        ASSERT_EQ(status, 0);
        ASSERT_EQ(size, 10 * sizeof(int8_t));
        ASSERT_NE(data, nullptr);
        ASSERT_EQ(type, adios_byte);
        int8_t *I8 = reinterpret_cast<int8_t *>(data);

        for (unsigned int i = 0; i < 10; ++i)
        {
            ASSERT_EQ(I8[i], m_TestData.I8[i]);
        }

        status = adios_get_attr(f, "i16_Array", &type, &size, &data);
        ASSERT_EQ(status, 0);
        ASSERT_EQ(size, 10 * sizeof(int16_t));
        ASSERT_NE(data, nullptr);
        ASSERT_EQ(type, adios_short);
        int16_t *I16 = reinterpret_cast<int16_t *>(data);

        for (unsigned int i = 0; i < 10; ++i)
        {
            ASSERT_EQ(I16[i], m_TestData.I16[i]);
        }

        status = adios_get_attr(f, "i32_Array", &type, &size, &data);
        ASSERT_EQ(status, 0);
        ASSERT_EQ(size, 10 * sizeof(int32_t));
        ASSERT_NE(data, nullptr);
        ASSERT_EQ(type, adios_integer);

        int32_t *I32 = reinterpret_cast<int32_t *>(data);

        for (unsigned int i = 0; i < 10; ++i)
        {
            ASSERT_EQ(I32[i], m_TestData.I32[i]);
        }

        status = adios_get_attr(f, "i64_Array", &type, &size, &data);
        ASSERT_EQ(status, 0);
        ASSERT_EQ(size, 10 * sizeof(int64_t));
        ASSERT_NE(data, nullptr);
        ASSERT_EQ(type, adios_long);
        int64_t *I64 = reinterpret_cast<int64_t *>(data);

        for (unsigned int i = 0; i < 10; ++i)
        {
            ASSERT_EQ(I64[i], m_TestData.I64[i]);
        }

        // uint
        status = adios_get_attr(f, "u8_Array", &type, &size, &data);
        ASSERT_EQ(status, 0);
        ASSERT_EQ(size, 10 * sizeof(uint8_t));
        ASSERT_NE(data, nullptr);
        ASSERT_EQ(type, adios_unsigned_byte);
        uint8_t *U8 = reinterpret_cast<uint8_t *>(data);

        for (unsigned int i = 0; i < 10; ++i)
        {
            ASSERT_EQ(U8[i], m_TestData.U8[i]);
        }

        status = adios_get_attr(f, "u16_Array", &type, &size, &data);
        ASSERT_EQ(status, 0);
        ASSERT_EQ(size, 10 * sizeof(uint16_t));
        ASSERT_NE(data, nullptr);
        ASSERT_EQ(type, adios_unsigned_short);
        uint16_t *U16 = reinterpret_cast<uint16_t *>(data);

        for (unsigned int i = 0; i < 10; ++i)
        {
            ASSERT_EQ(U16[i], m_TestData.U16[i]);
        }

        status = adios_get_attr(f, "u32_Array", &type, &size, &data);
        ASSERT_EQ(status, 0);
        ASSERT_EQ(size, 10 * sizeof(uint32_t));
        ASSERT_NE(data, nullptr);
        ASSERT_EQ(type, adios_unsigned_integer);
        uint32_t *U32 = reinterpret_cast<uint32_t *>(data);

        for (unsigned int i = 0; i < 10; ++i)
        {
            ASSERT_EQ(U32[i], m_TestData.U32[i]);
        }

        status = adios_get_attr(f, "u64_Array", &type, &size, &data);
        ASSERT_EQ(status, 0);
        ASSERT_EQ(size, 10 * sizeof(uint64_t));
        ASSERT_NE(data, nullptr);
        ASSERT_EQ(type, adios_unsigned_long);
        uint64_t *U64 = reinterpret_cast<uint64_t *>(data);

        for (unsigned int i = 0; i < 10; ++i)
        {
            ASSERT_EQ(U64[i], m_TestData.U64[i]);
        }

        status = adios_get_attr(f, "float_Array", &type, &size, &data);
        ASSERT_EQ(status, 0);
        ASSERT_EQ(size, 10 * sizeof(float));
        ASSERT_NE(data, nullptr);
        ASSERT_EQ(type, adios_real);
        float *R32 = reinterpret_cast<float *>(data);

        for (unsigned int i = 0; i < 10; ++i)
        {
            ASSERT_EQ(R32[i], m_TestData.R32[i]);
        }

        status = adios_get_attr(f, "double_Array", &type, &size, &data);
        ASSERT_EQ(status, 0);
        ASSERT_EQ(size, 10 * sizeof(double));
        ASSERT_NE(data, nullptr);
        ASSERT_EQ(type, adios_double);
        double *R64 = reinterpret_cast<double *>(data);

        for (unsigned int i = 0; i < 10; ++i)
        {
            ASSERT_EQ(R64[i], m_TestData.R64[i]);
        }

        // Cleanup file
        adios_read_close(f);
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
