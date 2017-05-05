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

class HDF5Direct
{

public:
    /**
     * Constructor for HDF5 file
     * @param file name
     */
    HDF5Direct(const std::string name, MPI_Comm m_MPIComm);

    bool isValid();
    void H5_Close();
    void H5_Advance();

    void Read(const std::string name, void *data_array, std::vector<int> &);

    hid_t m_Plist_id, m_File_id;
    hid_t m_Group_id;

    // hid_t DefH5T_COMPLEX_DOUBLE;
    // hid_t DefH5T_COMPLEX_FLOAT;
    // hid_t DefH5T_COMPLEX_LongDOUBLE;

    int m_CurrentTimeStep;

private:
    int m_Total_timestep;
};

HDF5Direct::HDF5Direct(const std::string name, MPI_Comm m_MPIComm)
: m_Total_timestep(0), m_CurrentTimeStep(0)
{
    //
    m_Plist_id = H5Pcreate(H5P_FILE_ACCESS);

#ifdef ADIOS2_HAVE_MPI
    H5Pset_fapl_mpio(m_Plist_id, m_MPIComm, MPI_INFO_NULL);
#endif

    std::string ts0 = "/TimeStep0";

    {
        // read a file collectively
        m_File_id = H5Fopen(name.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
        if (m_File_id >= 0)
        {
            m_Group_id = H5Gopen(m_File_id, ts0.c_str(), H5P_DEFAULT);
        }
    }

    H5Pclose(m_Plist_id);

    hid_t attr = H5Aopen(m_File_id, "NumTimeSteps", H5P_DEFAULT);
    H5Aread(attr, H5T_NATIVE_UINT, &m_Total_timestep);
    H5Aclose(attr);
}

bool HDF5Direct::isValid() { return (m_File_id >= 0); }

void HDF5Direct::H5_Close()
{
    if (m_Group_id >= 0)
    {
        H5Gclose(m_Group_id);
    }

    H5Fclose(m_File_id);
}

void HDF5Direct::H5_Advance()
{
    m_CurrentTimeStep++;
    if (m_CurrentTimeStep > 0)
    {
        H5Gclose(m_Group_id);
        m_Group_id = -1;
    }

    std::string tsname = "/TimeStep";
    tsname.append(std::to_string(m_CurrentTimeStep));
    int totalts = m_Total_timestep;
    {
        if ((totalts > 0) && (totalts <= m_CurrentTimeStep))
        {
            return;
        }
        // std::cout<<" ... current  group "<<tsname.c_str()<<std::endl;
        m_Group_id = H5Gopen(m_File_id, tsname.c_str(), H5P_DEFAULT);
    }
}

void HDF5Direct::Read(const std::string name, void *data_array,
                      std::vector<int> &gdims)
{
    hid_t datasetID = H5Dopen(m_Group_id, name.c_str(), H5P_DEFAULT);

    if (datasetID < 0)
    {
        return;
    }

    hid_t filespace = H5Dget_space(datasetID);

    if (filespace < 0)
    {
        return;
    }

    const int ndims = H5Sget_simple_extent_ndims(filespace);
    hsize_t dims[ndims];
    H5Sget_simple_extent_dims(filespace, dims, NULL);

    gdims.clear();
    for (int i = 0; i < ndims; i++)
    {
        gdims.push_back(dims[i]);
    }

    hid_t h5type = H5Dget_type(datasetID);
    hid_t ret =
        H5Dread(datasetID, h5type, H5S_ALL, H5S_ALL, H5P_DEFAULT, data_array);

    H5Sclose(filespace);
    H5Dclose(datasetID);
}

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
        HDF5Direct hdf5Reader(fname, MPI_COMM_WORLD);
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

        // Read stuff
        for (size_t t = 0; t < 3; ++t)
        {
            std::vector<int> gDims;
            hdf5Reader.Read("i8", I8.data(), gDims);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], 8);

            hdf5Reader.Read("i16", I16.data(), gDims);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], 8);

            hdf5Reader.Read("i32", I32.data(), gDims);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], 8);

            hdf5Reader.Read("i64", I64.data(), gDims);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], 8);

            hdf5Reader.Read("u8", U8.data(), gDims);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], 8);

            hdf5Reader.Read("u16", U16.data(), gDims);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], 8);

            hdf5Reader.Read("u32", U32.data(), gDims);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], 8);

            hdf5Reader.Read("u64", U64.data(), gDims);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], 8);

            hdf5Reader.Read("r32", R32.data(), gDims);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], 8);

            hdf5Reader.Read("r64", R64.data(), gDims);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], 8);

            /*
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

            */
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

            hdf5Reader.H5_Advance();
        }

        // Cleanup file
        hdf5Reader.H5_Close();
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
        HDF5Direct hdf5Reader(fname, MPI_COMM_WORLD);
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

        // Read stuff
        for (size_t t = 0; t < 3; ++t)
        {
            std::vector<int> gDims;
            hdf5Reader.Read("i8", I8.data(), gDims);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], 4);

            hdf5Reader.Read("i16", I16.data(), gDims);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], 4);

            hdf5Reader.Read("i32", I32.data(), gDims);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], 4);

            hdf5Reader.Read("i64", I64.data(), gDims);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], 4);

            hdf5Reader.Read("u8", U8.data(), gDims);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], 4);

            hdf5Reader.Read("u16", U16.data(), gDims);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], 4);

            hdf5Reader.Read("u32", U32.data(), gDims);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], 4);

            hdf5Reader.Read("u64", U64.data(), gDims);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], 4);

            hdf5Reader.Read("r32", R32.data(), gDims);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], 4);

            hdf5Reader.Read("r64", R64.data(), gDims);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], 4);

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
            hdf5Reader.H5_Advance();
        }

        // Cleanup file
        hdf5Reader.H5_Close();
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

        HDF5Direct hdf5Reader(fname, MPI_COMM_WORLD);
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

        // Read stuff
        for (size_t t = 0; t < 3; ++t)
        {
            std::vector<int> gDims;
            hdf5Reader.Read("i8", I8.data(), gDims);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], 2);

            hdf5Reader.Read("i16", I16.data(), gDims);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], 2);

            hdf5Reader.Read("i32", I32.data(), gDims);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], 2);

            hdf5Reader.Read("i64", I64.data(), gDims);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], 2);

            hdf5Reader.Read("u8", U8.data(), gDims);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], 2);

            hdf5Reader.Read("u16", U16.data(), gDims);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], 2);

            hdf5Reader.Read("u32", U32.data(), gDims);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], 2);

            hdf5Reader.Read("u64", U64.data(), gDims);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], 2);

            hdf5Reader.Read("r32", R32.data(), gDims);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], 2);

            hdf5Reader.Read("r64", R64.data(), gDims);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], 2);

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
            hdf5Reader.H5_Advance();
        }

        // Cleanup file
        hdf5Reader.H5_Close();
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
