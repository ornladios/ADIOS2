/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include <cstdint>
#include <cstring>

#include <iostream>
#include <stdexcept>

#include <adios2.h>
#include <hdf5.h>

#include <gtest/gtest.h>

#include "../SmallTestData.h"

class HDF5WriteReadTest : public ::testing::Test
{
public:
    HDF5WriteReadTest() = default;

    SmallTestData m_TestData;
};

//******************************************************************************
// 1D 1x8 test data
//******************************************************************************

// ADIOS2 write, native HDF5 read
TEST_F(HDF5WriteReadTest, ADIOS2HDF5WriteHDF5Read1D8)
{
    std::string fname = "ADIOS2HDF5WriteHDF5Read1D8.h5";

    // Write test data using ADIOS2
    {
        adios::ADIOS adios(adios::Verbose::WARN, true);

        // Declare 1D variables
        {
            auto &var_i8 = adios.DefineVariable<char>("i8", adios::Dims{8});
            auto &var_i16 = adios.DefineVariable<short>("i16", adios::Dims{8});
            auto &var_i32 = adios.DefineVariable<int>("i32", adios::Dims{8});
            auto &var_i64 = adios.DefineVariable<long>("i64", adios::Dims{8});
            auto &var_u8 =
                adios.DefineVariable<unsigned char>("u8", adios::Dims{8});
            auto &var_u16 =
                adios.DefineVariable<unsigned short>("u16", adios::Dims{8});
            auto &var_u32 =
                adios.DefineVariable<unsigned int>("u32", adios::Dims{8});
            auto &var_u64 =
                adios.DefineVariable<unsigned long>("u64", adios::Dims{8});
            auto &var_r32 = adios.DefineVariable<float>("r32", adios::Dims{8});
            auto &var_r64 = adios.DefineVariable<double>("r64", adios::Dims{8});
        }

        // Create the HDF5 Engine
        auto method = adios.DeclareMethod("TestMethod");
        method.SetEngine("HDF5Writer");
        method.AddTransport("File");

        auto engine = adios.Open(fname, "w", method);
        ASSERT_NE(engine, nullptr);

        for (size_t step = 0; step < 3; ++step)
        {
            // Retrieve the variables that previously went out of scope
            auto &var_i8 = adios.GetVariable<char>("i8");
            auto &var_i16 = adios.GetVariable<short>("i16");
            auto &var_i32 = adios.GetVariable<int>("i32");
            auto &var_i64 = adios.GetVariable<long>("i64");
            auto &var_u8 = adios.GetVariable<unsigned char>("u8");
            auto &var_u16 = adios.GetVariable<unsigned short>("u16");
            auto &var_u32 = adios.GetVariable<unsigned int>("u32");
            auto &var_u64 = adios.GetVariable<unsigned long>("u64");
            auto &var_r32 = adios.GetVariable<float>("r32");
            auto &var_r64 = adios.GetVariable<double>("r64");

            // Write each one
            engine->Write(var_i8, m_TestData.I8.data() + step);
            engine->Write(var_i16, m_TestData.I16.data() + step);
            engine->Write(var_i32, m_TestData.I32.data() + step);
            engine->Write(var_i64, m_TestData.I64.data() + step);
            engine->Write(var_u8, m_TestData.U8.data() + step);
            engine->Write(var_u16, m_TestData.U16.data() + step);
            engine->Write(var_u32, m_TestData.U32.data() + step);
            engine->Write(var_u64, m_TestData.U64.data() + step);
            engine->Write(var_r32, m_TestData.R32.data() + step);
            engine->Write(var_r64, m_TestData.R64.data() + step);

            // Advance to the next time step
            engine->Advance();
        }

        // Close the file
        engine->Close();
    }

    // Read test data using HDF5
    {
        ASSERT_TRUE(false)
            << "Native HDF5 read validation is not yet implemented";
    }
}

// ADIOS2 write, ADIOS2 read
TEST_F(HDF5WriteReadTest, DISABLED_ADIOS2HDF5WriteADIOS2HDF5Read1D8)
{
    std::string fname = "ADIOS2HDF5WriteADIOS2HDF5Read1D8.h5";

    ASSERT_TRUE(false) << "ADIOS2 read API is not yet implemented";
}

// Native HDF5 write, ADIOS2 read
TEST_F(HDF5WriteReadTest, DISABLED_HDF5WriteADIOS2HDF5Read1D8)
{
    std::string fname = "HDF5WriteADIOS2HDF5Read1D8.h5";

    ASSERT_TRUE(false) << "ADIOS2 read API is not yet implemented";
}

//******************************************************************************
// 2D 2x4 test data
//******************************************************************************

// ADIOS2 write, native HDF5 read
TEST_F(HDF5WriteReadTest, ADIOS2HDF5WriteHDF5Read2D2x4)
{
    std::string fname = "ADIOS2HDF5WriteHDF5Read2D2x4Test.h5";

    // Write test data using ADIOS2
    {
        adios::ADIOS adios(adios::Verbose::WARN, true);

        // Declare 1D variables
        {
            auto &var_i8 = adios.DefineVariable<char>("i8", adios::Dims{2, 4});
            auto &var_i16 =
                adios.DefineVariable<short>("i16", adios::Dims{2, 4});
            auto &var_i32 = adios.DefineVariable<int>("i32", adios::Dims{2, 4});
            auto &var_i64 =
                adios.DefineVariable<long>("i64", adios::Dims{2, 4});
            auto &var_u8 =
                adios.DefineVariable<unsigned char>("u8", adios::Dims{2, 4});
            auto &var_u16 =
                adios.DefineVariable<unsigned short>("u16", adios::Dims{2, 4});
            auto &var_u32 =
                adios.DefineVariable<unsigned int>("u32", adios::Dims{2, 4});
            auto &var_u64 =
                adios.DefineVariable<unsigned long>("u64", adios::Dims{2, 4});
            auto &var_r32 =
                adios.DefineVariable<float>("r32", adios::Dims{2, 4});
            auto &var_r64 =
                adios.DefineVariable<double>("r64", adios::Dims{2, 4});
        }

        // Create the HDF5 Engine
        auto method = adios.DeclareMethod("TestMethod");
        method.SetEngine("HDF5Writer");
        method.AddTransport("File");

        auto engine = adios.Open(fname, "w", method);
        ASSERT_NE(engine, nullptr);

        for (size_t step = 0; step < 3; ++step)
        {
            // Retrieve the variables that previously went out of scope
            auto &var_i8 = adios.GetVariable<char>("i8");
            auto &var_i16 = adios.GetVariable<short>("i16");
            auto &var_i32 = adios.GetVariable<int>("i32");
            auto &var_i64 = adios.GetVariable<long>("i64");
            auto &var_u8 = adios.GetVariable<unsigned char>("u8");
            auto &var_u16 = adios.GetVariable<unsigned short>("u16");
            auto &var_u32 = adios.GetVariable<unsigned int>("u32");
            auto &var_u64 = adios.GetVariable<unsigned long>("u64");
            auto &var_r32 = adios.GetVariable<float>("r32");
            auto &var_r64 = adios.GetVariable<double>("r64");

            // Write each one
            engine->Write(var_i8, m_TestData.I8.data() + step);
            engine->Write(var_i16, m_TestData.I16.data() + step);
            engine->Write(var_i32, m_TestData.I32.data() + step);
            engine->Write(var_i64, m_TestData.I64.data() + step);
            engine->Write(var_u8, m_TestData.U8.data() + step);
            engine->Write(var_u16, m_TestData.U16.data() + step);
            engine->Write(var_u32, m_TestData.U32.data() + step);
            engine->Write(var_u64, m_TestData.U64.data() + step);
            engine->Write(var_r32, m_TestData.R32.data() + step);
            engine->Write(var_r64, m_TestData.R64.data() + step);

            // Advance to the next time step
            engine->Advance();
        }

        // Close the file
        engine->Close();
    }

    // Read test data using HDF5
    {
        ASSERT_TRUE(false)
            << "Native HDF5 read validation is not yet implemented";
    }
}

// ADIOS2 write, ADIOS2 read
TEST_F(HDF5WriteReadTest, DISABLED_ADIOS2HDF5WriteADIOS2HDF5Read2D2x4)
{
    std::string fname = "ADIOS2HDF5WriteADIOS2HDF5Read2D2x4Test.h5";

    ASSERT_TRUE(false) << "ADIOS2 read API is not yet implemented";
}

// Native HDF5 write, ADIOS2 read
TEST_F(HDF5WriteReadTest, DISABLED_HDF5WriteADIOS2HDF5Read2D2x4)
{
    std::string fname = "HDF5WriteADIOS2HDF5Read2D2x4Test.h5";

    ASSERT_TRUE(false) << "ADIOS2 read API is not yet implemented";
}

//******************************************************************************
// 2D 4x2 test data
//******************************************************************************

// ADIOS2 write, native HDF5 read
TEST_F(HDF5WriteReadTest, ADIOS2HDF5WriteHDF5Read2D4x2)
{
    std::string fname = "ADIOS2HDF5WriteHDF5Read2D4x2Test.h5";

    // Write test data using ADIOS2
    {
        adios::ADIOS adios(adios::Verbose::WARN, true);

        // Declare 1D variables
        {
            auto &var_i8 = adios.DefineVariable<char>("i8", adios::Dims{4, 2});
            auto &var_i16 =
                adios.DefineVariable<short>("i16", adios::Dims{4, 2});
            auto &var_i32 = adios.DefineVariable<int>("i32", adios::Dims{4, 2});
            auto &var_i64 =
                adios.DefineVariable<long>("i64", adios::Dims{4, 2});
            auto &var_u8 =
                adios.DefineVariable<unsigned char>("u8", adios::Dims{4, 2});
            auto &var_u16 =
                adios.DefineVariable<unsigned short>("u16", adios::Dims{4, 2});
            auto &var_u32 =
                adios.DefineVariable<unsigned int>("u32", adios::Dims{4, 2});
            auto &var_u64 =
                adios.DefineVariable<unsigned long>("u64", adios::Dims{4, 2});
            auto &var_r32 =
                adios.DefineVariable<float>("r32", adios::Dims{4, 2});
            auto &var_r64 =
                adios.DefineVariable<double>("r64", adios::Dims{4, 2});
        }

        // Create the HDF5 Engine
        auto method = adios.DeclareMethod("TestMethod");
        method.SetEngine("HDF5Writer");
        method.AddTransport("File");

        auto engine = adios.Open(fname, "w", method);
        ASSERT_NE(engine, nullptr);

        for (size_t step = 0; step < 3; ++step)
        {
            // Retrieve the variables that previously went out of scope
            auto &var_i8 = adios.GetVariable<char>("i8");
            auto &var_i16 = adios.GetVariable<short>("i16");
            auto &var_i32 = adios.GetVariable<int>("i32");
            auto &var_i64 = adios.GetVariable<long>("i64");
            auto &var_u8 = adios.GetVariable<unsigned char>("u8");
            auto &var_u16 = adios.GetVariable<unsigned short>("u16");
            auto &var_u32 = adios.GetVariable<unsigned int>("u32");
            auto &var_u64 = adios.GetVariable<unsigned long>("u64");
            auto &var_r32 = adios.GetVariable<float>("r32");
            auto &var_r64 = adios.GetVariable<double>("r64");

            // Write each one
            engine->Write(var_i8, m_TestData.I8.data() + step);
            engine->Write(var_i16, m_TestData.I16.data() + step);
            engine->Write(var_i32, m_TestData.I32.data() + step);
            engine->Write(var_i64, m_TestData.I64.data() + step);
            engine->Write(var_u8, m_TestData.U8.data() + step);
            engine->Write(var_u16, m_TestData.U16.data() + step);
            engine->Write(var_u32, m_TestData.U32.data() + step);
            engine->Write(var_u64, m_TestData.U64.data() + step);
            engine->Write(var_r32, m_TestData.R32.data() + step);
            engine->Write(var_r64, m_TestData.R64.data() + step);

            // Advance to the next time step
            engine->Advance();
        }

        // Close the file
        engine->Close();
    }

    // Read test data using HDF5
    {
        ASSERT_TRUE(false)
            << "Native HDF5 read validation is not yet implemented";
    }
}

// ADIOS2 write, ADIOS2 read
TEST_F(HDF5WriteReadTest, DISABLED_ADIOS2HDF5WriteADIOS2HDF5Read2D4x2)
{
    std::string fname = "ADIOS2HDF5WriteADIOS2HDF5Read2D4x2Test.h5";

    ASSERT_TRUE(false) << "ADIOS2 read API is not yet implemented";
}

// Native HDF5 write, ADIOS2 read
TEST_F(HDF5WriteReadTest, DISABLED_HDF5WriteADIOS2HDF5Read2D4x2)
{
    std::string fname = "HDF5WriteADIOS2HDF5Read2D4x2Test.h5";

    ASSERT_TRUE(false) << "ADIOS2 read API is not yet implemented";
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
