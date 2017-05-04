/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include <cstdint>
#include <cstring>

#include <iostream>
#include <stdexcept>

#include <adios2.h>
#include "adios2/engine/hdf5/HDF5ReaderP.h"
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
        adios::ADIOS adios(adios::Verbose::WARN, true); // moved up
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
#ifdef ADIOS2_HAVE_MPI
    // Read everything from rank 0
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0)
#endif
    {
      adios::ADIOS adios(adios::Verbose::WARN, true); 
      // call reader directly
      auto method = adios.DeclareMethod("TestMethod");
      method.SetEngine("HDF5Reader");

      adios::HDF5Reader hdf5Reader(adios, fname, "r", MPI_COMM_WORLD, method);
				   
      ASSERT_TRUE(hdf5Reader.isValid());

      
        std::array<char, 8> I8;
        std::array<int16_t, 8> I16;
        std::array<int32_t, 8> I32;
        std::array<int64_t, 8> I64;
        std::array<unsigned char, 8> U8;
        std::array<uint16_t, 8> U16;
        std::array<uint32_t, 8> U32;
        std::array<uint64_t, 8> U64;
        std::array<float, 8> R32;
        std::array<double, 8> R64;


	auto &var_i8 = adios.DefineVariable<char>("i8");
	auto &var_i16 = adios.DefineVariable<short>("i16");
	auto &var_i32 = adios.DefineVariable<int>("i32" );
	auto &var_i64 = adios.DefineVariable<long>("i64" );
	auto &var_u8 =
	  adios.DefineVariable<unsigned char>("u8");
	auto &var_u16 =
	  adios.DefineVariable<unsigned short>("u16");
	auto &var_u32 =
	  adios.DefineVariable<unsigned int>("u32");
	auto &var_u64 =
	  adios.DefineVariable<unsigned long>("u64");
	auto &var_r32 = adios.DefineVariable<float>("r32");
	auto &var_r64 = adios.DefineVariable<double>("r64");
	
        // Read stuff
        for (size_t t = 0; t < 3; ++t)
        {
            // Read the current step
	    hdf5Reader.ReadMe(var_i8, I8.data(), H5T_NATIVE_CHAR);	    
	    hdf5Reader.ReadMe(var_i16, I16.data(), H5T_NATIVE_SHORT);
	    hdf5Reader.ReadMe(var_i32, I32.data(), H5T_NATIVE_INT);
	    hdf5Reader.ReadMe(var_i64, I64.data(), H5T_NATIVE_LONG);
	    hdf5Reader.ReadMe(var_u8,  U8.data(),  H5T_NATIVE_UCHAR);
	    hdf5Reader.ReadMe(var_u16, U16.data(), H5T_NATIVE_USHORT);
	    hdf5Reader.ReadMe(var_u32, U32.data(), H5T_NATIVE_UINT);
	    hdf5Reader.ReadMe(var_u64, U64.data(), H5T_NATIVE_ULONG);
	    hdf5Reader.ReadMe(var_r32, R32.data(), H5T_NATIVE_FLOAT);
	    hdf5Reader.ReadMe(var_r64, R64.data(), H5T_NATIVE_DOUBLE);

	    // Check the variables exist      
	    ASSERT_EQ(var_i8.m_GlobalDimensions.size(), 1);
	    ASSERT_EQ(var_i8.m_GlobalDimensions[0], 8);

	    ASSERT_EQ(var_i16.m_GlobalDimensions.size(), 1);
	    ASSERT_EQ(var_i16.m_GlobalDimensions[0], 8);

	    ASSERT_EQ(var_i32.m_GlobalDimensions.size(), 1);
	    ASSERT_EQ(var_i32.m_GlobalDimensions[0], 8);

	    ASSERT_EQ(var_i64.m_GlobalDimensions.size(), 1);
	    ASSERT_EQ(var_i64.m_GlobalDimensions[0], 8);

	    ASSERT_EQ(var_u8.m_GlobalDimensions.size(), 1);
	    ASSERT_EQ(var_u8.m_GlobalDimensions[0], 8);

	    ASSERT_EQ(var_u16.m_GlobalDimensions.size(), 1);
	    ASSERT_EQ(var_u16.m_GlobalDimensions[0], 8);

	    ASSERT_EQ(var_u32.m_GlobalDimensions.size(), 1);
	    ASSERT_EQ(var_u32.m_GlobalDimensions[0], 8);

	    ASSERT_EQ(var_u64.m_GlobalDimensions.size(), 1);
	    ASSERT_EQ(var_u64.m_GlobalDimensions[0], 8);

	    ASSERT_EQ(var_r32.m_GlobalDimensions.size(), 1);
	    ASSERT_EQ(var_r32.m_GlobalDimensions[0], 8);

	    ASSERT_EQ(var_r64.m_GlobalDimensions.size(), 1);
	    ASSERT_EQ(var_r64.m_GlobalDimensions[0], 8);


            // Check if it's correct
            for (size_t i = 0; i < 8; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i;
                std::string msg = ss.str();

                EXPECT_EQ(I8[i], m_TestData.I8[i + t]) << msg;
                EXPECT_EQ(I16[i], m_TestData.I16[i + t]) << msg;
                EXPECT_EQ(I32[i], m_TestData.I32[i + t]) << msg;
                EXPECT_EQ(I64[i], m_TestData.I64[i + t]) << msg;
                EXPECT_EQ(U8[i], m_TestData.U8[i + t]) << msg;
                EXPECT_EQ(U16[i], m_TestData.U16[i + t]) << msg;
                EXPECT_EQ(U32[i], m_TestData.U32[i + t]) << msg;
                EXPECT_EQ(U64[i], m_TestData.U64[i + t]) << msg;
                EXPECT_EQ(R32[i], m_TestData.R32[i + t]) << msg;
                EXPECT_EQ(R64[i], m_TestData.R64[i + t]) << msg;
            }
	    hdf5Reader.Advance();
        }

        // Cleanup file
	hdf5Reader.Close();
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
#ifdef ADIOS2_HAVE_MPI
    // Read everything from rank 0
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0)
#endif
    {
      adios::ADIOS adios(adios::Verbose::WARN, true); 
      // call reader directly
      auto method = adios.DeclareMethod("TestMethod");
      method.SetEngine("HDF5Reader");

      adios::HDF5Reader hdf5Reader(adios, fname, "r", MPI_COMM_WORLD, method);
				   
      ASSERT_TRUE(hdf5Reader.isValid());
      
      std::array<char, 8> I8;
      std::array<int16_t, 8> I16;
      std::array<int32_t, 8> I32;
      std::array<int64_t, 8> I64;
      std::array<unsigned char, 8> U8;
      std::array<uint16_t, 8> U16;
      std::array<uint32_t, 8> U32;
      std::array<uint64_t, 8> U64;
      std::array<float, 8> R32;
      std::array<double, 8> R64;
      
      
      auto &var_i8 = adios.DefineVariable<char>("i8");
      auto &var_i16 = adios.DefineVariable<short>("i16");
      auto &var_i32 = adios.DefineVariable<int>("i32" );
      auto &var_i64 = adios.DefineVariable<long>("i64" );
      auto &var_u8 =
	adios.DefineVariable<unsigned char>("u8");
      auto &var_u16 =
	adios.DefineVariable<unsigned short>("u16");
      auto &var_u32 =
	adios.DefineVariable<unsigned int>("u32");
      auto &var_u64 =
	adios.DefineVariable<unsigned long>("u64");
      auto &var_r32 = adios.DefineVariable<float>("r32");
      auto &var_r64 = adios.DefineVariable<double>("r64");
      
      // Read stuff
      for (size_t t = 0; t < 3; ++t)
        {
	  // Read the current step
	  hdf5Reader.ReadMe(var_i8, I8.data(), H5T_NATIVE_CHAR);	    
	  hdf5Reader.ReadMe(var_i16, I16.data(), H5T_NATIVE_SHORT);
	  hdf5Reader.ReadMe(var_i32, I32.data(), H5T_NATIVE_INT);
	  hdf5Reader.ReadMe(var_i64, I64.data(), H5T_NATIVE_LONG);
	  hdf5Reader.ReadMe(var_u8,  U8.data(),  H5T_NATIVE_UCHAR);
	  hdf5Reader.ReadMe(var_u16, U16.data(), H5T_NATIVE_USHORT);
	  hdf5Reader.ReadMe(var_u32, U32.data(), H5T_NATIVE_UINT);
	  hdf5Reader.ReadMe(var_u64, U64.data(), H5T_NATIVE_ULONG);
	  hdf5Reader.ReadMe(var_r32, R32.data(), H5T_NATIVE_FLOAT);
	  hdf5Reader.ReadMe(var_r64, R64.data(), H5T_NATIVE_DOUBLE);
	  
	  // Check the variables exist      
	  ASSERT_EQ(var_i8.m_GlobalDimensions.size(), 2);
	  ASSERT_EQ(var_i8.m_GlobalDimensions[0], 2);
	  ASSERT_EQ(var_i8.m_GlobalDimensions[1], 4);
	  
	    ASSERT_EQ(var_i16.m_GlobalDimensions.size(), 2);
	    ASSERT_EQ(var_i16.m_GlobalDimensions[0], 2);
	    ASSERT_EQ(var_i16.m_GlobalDimensions[1], 4);

	    ASSERT_EQ(var_i32.m_GlobalDimensions.size(), 2);
	    ASSERT_EQ(var_i32.m_GlobalDimensions[0], 2);
	    ASSERT_EQ(var_i32.m_GlobalDimensions[1], 4);

	    ASSERT_EQ(var_i64.m_GlobalDimensions.size(), 2);
	    ASSERT_EQ(var_i64.m_GlobalDimensions[0], 2);
	    ASSERT_EQ(var_i64.m_GlobalDimensions[1], 4);

	    ASSERT_EQ(var_u8.m_GlobalDimensions.size(), 2);
	    ASSERT_EQ(var_u8.m_GlobalDimensions[0], 2);
	    ASSERT_EQ(var_u8.m_GlobalDimensions[1], 4);

	    ASSERT_EQ(var_u16.m_GlobalDimensions.size(), 2);
	    ASSERT_EQ(var_u16.m_GlobalDimensions[0], 2);
	    ASSERT_EQ(var_u16.m_GlobalDimensions[1], 4);

	    ASSERT_EQ(var_u32.m_GlobalDimensions.size(), 2);
	    ASSERT_EQ(var_u32.m_GlobalDimensions[0], 2);
	    ASSERT_EQ(var_u32.m_GlobalDimensions[1], 4);

	    ASSERT_EQ(var_u64.m_GlobalDimensions.size(), 2);
	    ASSERT_EQ(var_u64.m_GlobalDimensions[0], 2);
	    ASSERT_EQ(var_u64.m_GlobalDimensions[1], 4);

	    ASSERT_EQ(var_r32.m_GlobalDimensions.size(), 2);
	    ASSERT_EQ(var_r32.m_GlobalDimensions[0], 2);
	    ASSERT_EQ(var_r32.m_GlobalDimensions[1], 4);

	    ASSERT_EQ(var_r64.m_GlobalDimensions.size(), 2);
	    ASSERT_EQ(var_r64.m_GlobalDimensions[0], 2);
	    ASSERT_EQ(var_r64.m_GlobalDimensions[1], 4);


            // Check if it's correct
            for (size_t i = 0; i < 8; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i;
                std::string msg = ss.str();

                EXPECT_EQ(I8[i], m_TestData.I8[i + t]) << msg;
                EXPECT_EQ(I16[i], m_TestData.I16[i + t]) << msg;
                EXPECT_EQ(I32[i], m_TestData.I32[i + t]) << msg;
                EXPECT_EQ(I64[i], m_TestData.I64[i + t]) << msg;
                EXPECT_EQ(U8[i], m_TestData.U8[i + t]) << msg;
                EXPECT_EQ(U16[i], m_TestData.U16[i + t]) << msg;
                EXPECT_EQ(U32[i], m_TestData.U32[i + t]) << msg;
                EXPECT_EQ(U64[i], m_TestData.U64[i + t]) << msg;
                EXPECT_EQ(R32[i], m_TestData.R32[i + t]) << msg;
                EXPECT_EQ(R64[i], m_TestData.R64[i + t]) << msg;
            }
	    hdf5Reader.Advance();
        }

        // Cleanup file
	hdf5Reader.Close();
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
#ifdef ADIOS2_HAVE_MPI
    // Read everything from rank 0
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0)
#endif
    {
      adios::ADIOS adios(adios::Verbose::WARN, true); 
      // call reader directly
      auto method = adios.DeclareMethod("TestMethod");
      method.SetEngine("HDF5Reader");

      adios::HDF5Reader hdf5Reader(adios, fname, "r", MPI_COMM_WORLD, method);
				   
      ASSERT_TRUE(hdf5Reader.isValid());
      
      std::array<char, 8> I8;
      std::array<int16_t, 8> I16;
      std::array<int32_t, 8> I32;
      std::array<int64_t, 8> I64;
      std::array<unsigned char, 8> U8;
      std::array<uint16_t, 8> U16;
      std::array<uint32_t, 8> U32;
      std::array<uint64_t, 8> U64;
      std::array<float, 8> R32;
      std::array<double, 8> R64;
      
      
      auto &var_i8 = adios.DefineVariable<char>("i8");
      auto &var_i16 = adios.DefineVariable<short>("i16");
      auto &var_i32 = adios.DefineVariable<int>("i32" );
      auto &var_i64 = adios.DefineVariable<long>("i64" );
      auto &var_u8 =
	adios.DefineVariable<unsigned char>("u8");
      auto &var_u16 =
	adios.DefineVariable<unsigned short>("u16");
      auto &var_u32 =
	adios.DefineVariable<unsigned int>("u32");
      auto &var_u64 =
	adios.DefineVariable<unsigned long>("u64");
      auto &var_r32 = adios.DefineVariable<float>("r32");
      auto &var_r64 = adios.DefineVariable<double>("r64");
      
      // Read stuff
      for (size_t t = 0; t < 3; ++t)
        {
	  // Read the current step
	  hdf5Reader.ReadMe(var_i8, I8.data(), H5T_NATIVE_CHAR);	    
	  hdf5Reader.ReadMe(var_i16, I16.data(), H5T_NATIVE_SHORT);
	  hdf5Reader.ReadMe(var_i32, I32.data(), H5T_NATIVE_INT);
	  hdf5Reader.ReadMe(var_i64, I64.data(), H5T_NATIVE_LONG);
	  hdf5Reader.ReadMe(var_u8,  U8.data(),  H5T_NATIVE_UCHAR);
	  hdf5Reader.ReadMe(var_u16, U16.data(), H5T_NATIVE_USHORT);
	  hdf5Reader.ReadMe(var_u32, U32.data(), H5T_NATIVE_UINT);
	  hdf5Reader.ReadMe(var_u64, U64.data(), H5T_NATIVE_ULONG);
	  hdf5Reader.ReadMe(var_r32, R32.data(), H5T_NATIVE_FLOAT);
	  hdf5Reader.ReadMe(var_r64, R64.data(), H5T_NATIVE_DOUBLE);
	  
	  // Check the variables exist      
	  ASSERT_EQ(var_i8.m_GlobalDimensions.size(), 2);
	  ASSERT_EQ(var_i8.m_GlobalDimensions[0], 4);
	  ASSERT_EQ(var_i8.m_GlobalDimensions[1], 2);
	  
	    ASSERT_EQ(var_i16.m_GlobalDimensions.size(), 2);
	    ASSERT_EQ(var_i16.m_GlobalDimensions[0], 4);
	    ASSERT_EQ(var_i16.m_GlobalDimensions[1], 2);

	    ASSERT_EQ(var_i32.m_GlobalDimensions.size(), 2);
	    ASSERT_EQ(var_i32.m_GlobalDimensions[0], 4);
	    ASSERT_EQ(var_i32.m_GlobalDimensions[1], 2);

	    ASSERT_EQ(var_i64.m_GlobalDimensions.size(), 2);
	    ASSERT_EQ(var_i64.m_GlobalDimensions[0], 4);
	    ASSERT_EQ(var_i64.m_GlobalDimensions[1], 2);

	    ASSERT_EQ(var_u8.m_GlobalDimensions.size(), 2);
	    ASSERT_EQ(var_u8.m_GlobalDimensions[0], 4);
	    ASSERT_EQ(var_u8.m_GlobalDimensions[1], 2);

	    ASSERT_EQ(var_u16.m_GlobalDimensions.size(), 2);
	    ASSERT_EQ(var_u16.m_GlobalDimensions[0], 4);
	    ASSERT_EQ(var_u16.m_GlobalDimensions[1], 2);

	    ASSERT_EQ(var_u32.m_GlobalDimensions.size(), 2);
	    ASSERT_EQ(var_u32.m_GlobalDimensions[0], 4);
	    ASSERT_EQ(var_u32.m_GlobalDimensions[1], 2);

	    ASSERT_EQ(var_u64.m_GlobalDimensions.size(), 2);
	    ASSERT_EQ(var_u64.m_GlobalDimensions[0], 4);
	    ASSERT_EQ(var_u64.m_GlobalDimensions[1], 2);

	    ASSERT_EQ(var_r32.m_GlobalDimensions.size(), 2);
	    ASSERT_EQ(var_r32.m_GlobalDimensions[0], 4);
	    ASSERT_EQ(var_r32.m_GlobalDimensions[1], 2);

	    ASSERT_EQ(var_r64.m_GlobalDimensions.size(), 2);
	    ASSERT_EQ(var_r64.m_GlobalDimensions[0], 4);
	    ASSERT_EQ(var_r64.m_GlobalDimensions[1], 2);


            // Check if it's correct
            for (size_t i = 0; i < 8; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i;
                std::string msg = ss.str();

                EXPECT_EQ(I8[i], m_TestData.I8[i + t]) << msg;
                EXPECT_EQ(I16[i], m_TestData.I16[i + t]) << msg;
                EXPECT_EQ(I32[i], m_TestData.I32[i + t]) << msg;
                EXPECT_EQ(I64[i], m_TestData.I64[i + t]) << msg;
                EXPECT_EQ(U8[i], m_TestData.U8[i + t]) << msg;
                EXPECT_EQ(U16[i], m_TestData.U16[i + t]) << msg;
                EXPECT_EQ(U32[i], m_TestData.U32[i + t]) << msg;
                EXPECT_EQ(U64[i], m_TestData.U64[i + t]) << msg;
                EXPECT_EQ(R32[i], m_TestData.R32[i + t]) << msg;
                EXPECT_EQ(R64[i], m_TestData.R64[i + t]) << msg;
            }
	    hdf5Reader.Advance();
        }

        // Cleanup file
	hdf5Reader.Close();
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
