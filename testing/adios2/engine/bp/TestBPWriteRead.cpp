#include <cstdint>

#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#include "../SmallTestData.h"

class BPWriteReadTest : public ::testing::Test
{
public:
    BPWriteReadTest() = default;

    SmallTestData m_TestData;
};

//******************************************************************************
// 1D 1x8 test data
//******************************************************************************
TEST_F(BPWriteReadTest, ADIOS2ADIOS2WriteADIOS1Read1D8)
{
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

        // Create the ADIOS 1 Engine
        auto method = adios.DeclareMethod("TestMethod");
        method.SetEngine("BPFileWriter");
        method.AddTransport("File");

        auto engine =
            adios.Open("ADIOS2ADIOS2WriteADIOS1Read1D8.bp", "w", method);
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
            engine->Write(var_i8, m_TestData.I8.cbegin() + step);
            engine->Write(var_i16, m_TestData.I16.cbegin() + step);
            engine->Write(var_i32, m_TestData.I32.cbegin() + step);
            engine->Write(var_i64, m_TestData.I64.cbegin() + step);
            engine->Write(var_u8, m_TestData.U8.cbegin() + step);
            engine->Write(var_u16, m_TestData.U16.cbegin() + step);
            engine->Write(var_u32, m_TestData.U32.cbegin() + step);
            engine->Write(var_u64, m_TestData.U64.cbegin() + step);
            engine->Write(var_r32, m_TestData.R32.cbegin() + step);
            engine->Write(var_r64, m_TestData.R64.cbegin() + step);

            // Advance to the next time step
            engine->Advance();
        }

        // Close the file
        engine->Close();
    }
}

//******************************************************************************
// 2D 2x4 test data
//******************************************************************************
TEST_F(BPWriteReadTest, ADIOS2ADIOS2WriteADIOS1Read2D2x4)
{
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

        // Create the ADIOS 1 Engine
        auto method = adios.DeclareMethod("TestMethod");
        method.SetEngine("BPFileWriter");
        method.AddTransport("File");

        auto engine =
            adios.Open("ADIOS2BPWriteADIOS1Read2D2x4Test.bp", "w", method);
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
            engine->Write(var_i8, m_TestData.I8.cbegin() + step);
            engine->Write(var_i16, m_TestData.I16.cbegin() + step);
            engine->Write(var_i32, m_TestData.I32.cbegin() + step);
            engine->Write(var_i64, m_TestData.I64.cbegin() + step);
            engine->Write(var_u8, m_TestData.U8.cbegin() + step);
            engine->Write(var_u16, m_TestData.U16.cbegin() + step);
            engine->Write(var_u32, m_TestData.U32.cbegin() + step);
            engine->Write(var_u64, m_TestData.U64.cbegin() + step);
            engine->Write(var_r32, m_TestData.R32.cbegin() + step);
            engine->Write(var_r64, m_TestData.R64.cbegin() + step);

            // Advance to the next time step
            engine->Advance();
        }

        // Close the file
        engine->Close();
    }
}

//******************************************************************************
// 2D 4x2 test data
//******************************************************************************
TEST_F(BPWriteReadTest, ADIOS2ADIOS2WriteADIOS1Read2D4x2)
{
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

        // Create the ADIOS 1 Engine
        auto method = adios.DeclareMethod("TestMethod");
        method.SetEngine("BPFileWriter");
        method.AddTransport("File");

        auto engine =
            adios.Open("ADIOS2BPWriteADIOS1Read2D4x2Test.bp", "w", method);
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
            engine->Write(var_i8, m_TestData.I8.cbegin() + step);
            engine->Write(var_i16, m_TestData.I16.cbegin() + step);
            engine->Write(var_i32, m_TestData.I32.cbegin() + step);
            engine->Write(var_i64, m_TestData.I64.cbegin() + step);
            engine->Write(var_u8, m_TestData.U8.cbegin() + step);
            engine->Write(var_u16, m_TestData.U16.cbegin() + step);
            engine->Write(var_u32, m_TestData.U32.cbegin() + step);
            engine->Write(var_u64, m_TestData.U64.cbegin() + step);
            engine->Write(var_r32, m_TestData.R32.cbegin() + step);
            engine->Write(var_r64, m_TestData.R64.cbegin() + step);

            // Advance to the next time step
            engine->Advance();
        }

        // Close the file
        engine->Close();
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
