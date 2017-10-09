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

class HDF5NativeReader
{

public:
    HDF5NativeReader(const std::string fileName);
    ~HDF5NativeReader();

    bool Advance();

    void GetVarInfo(const std::string varName, std::vector<hsize_t> &dims,
                    hid_t &h5Type);
    void ReadVar(const std::string varName, void *dataArray);

    int m_CurrentTimeStep;
    unsigned int m_TotalTimeSteps;

private:
    hid_t m_FilePropertyListId;
    hid_t m_FileId;
    hid_t m_GroupId;
};

HDF5NativeReader::HDF5NativeReader(const std::string fileName)
: m_CurrentTimeStep(0), m_TotalTimeSteps(0)
{
    m_FilePropertyListId = H5Pcreate(H5P_FILE_ACCESS);

#ifdef ADIOS2_HAVE_MPI
    // read a file collectively
    H5Pset_fapl_mpio(m_FilePropertyListId, MPI_COMM_WORLD, MPI_INFO_NULL);
#endif

    m_FileId = H5Fopen(fileName.c_str(), H5F_ACC_RDONLY, m_FilePropertyListId);
    if (m_FileId < 0)
    {
        throw std::runtime_error("Unable to open " + fileName + " for reading");
    }

    std::string ts0 = "/TimeStep0";
    m_GroupId = H5Gopen(m_FileId, ts0.c_str(), H5P_DEFAULT);
    if (m_GroupId < 0)
    {
        throw std::runtime_error("Unable to open group " + ts0 +
                                 " for reading");
    }

    hid_t attrId = H5Aopen(m_FileId, "NumTimeSteps", H5P_DEFAULT);
    if (attrId < 0)
    {
        throw std::runtime_error("Unable to open attribute NumTimeSteps");
    }
    H5Aread(attrId, H5T_NATIVE_UINT, &m_TotalTimeSteps);
    H5Aclose(attrId);
}

HDF5NativeReader::~HDF5NativeReader()
{
    if (m_GroupId >= 0)
    {
        H5Gclose(m_GroupId);
    }

    H5Fclose(m_FileId);
    H5Pclose(m_FilePropertyListId);
}

void HDF5NativeReader::GetVarInfo(const std::string varName,
                                  std::vector<hsize_t> &dims, hid_t &h5Type)
{
    hid_t dataSetId = H5Dopen(m_GroupId, varName.c_str(), H5P_DEFAULT);
    if (dataSetId < 0)
    {
        throw std::runtime_error("Unable to open dataset " + varName);
    }

    hid_t fileSpaceId = H5Dget_space(dataSetId);
    if (fileSpaceId < 0)
    {
        throw std::runtime_error("Unable to get filespace for dataset " +
                                 varName);
    }

    const int ndims = H5Sget_simple_extent_ndims(fileSpaceId);
    if (ndims < 0)
    {
        throw std::runtime_error(
            "Unable to get number of dimensions for dataset " + varName);
    }

    dims.resize(ndims);
    if (H5Sget_simple_extent_dims(fileSpaceId, dims.data(), NULL) != ndims)
    {
        throw std::runtime_error("Unable to get dimensions for dataset " +
                                 varName);
    }

    h5Type = H5Dget_type(dataSetId);

    H5Sclose(fileSpaceId);
    H5Dclose(dataSetId);
}

bool HDF5NativeReader::Advance()
{
    if (m_GroupId >= 0)
    {
        H5Gclose(m_GroupId);
        m_GroupId = -1;
    }

    if (m_CurrentTimeStep + 1 >= m_TotalTimeSteps)
    {
        return false;
    }

    std::string tsName = "/TimeStep" + std::to_string(m_CurrentTimeStep + 1);
    m_GroupId = H5Gopen(m_FileId, tsName.c_str(), H5P_DEFAULT);
    if (m_GroupId < 0)
    {
        throw std::runtime_error("Unable to open group " + tsName +
                                 " for reading");
    }
    ++m_CurrentTimeStep;

    return true;
}

void HDF5NativeReader::ReadVar(const std::string varName, void *dataArray)
{
    if (m_GroupId < 0)
    {
        throw std::runtime_error("Can't read variable " + varName +
                                 " since a group is not currently open");
    }

    hid_t dataSetId = H5Dopen(m_GroupId, varName.c_str(), H5P_DEFAULT);
    if (dataSetId < 0)
    {
        throw std::runtime_error("Unable to open dataset " + varName);
    }

    hid_t fileSpace = H5Dget_space(dataSetId);
    if (fileSpace < 0)
    {
        throw std::runtime_error("Unable to get filespace for dataset " +
                                 varName);
    }

    hid_t h5type = H5Dget_type(dataSetId);
    hid_t ret =
        H5Dread(dataSetId, h5type, H5S_ALL, H5S_ALL, H5P_DEFAULT, dataArray);

    H5Sclose(fileSpace);
    H5Dclose(dataSetId);
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
        adios2::ADIOS adios(true); // moved up
        adios2::IO &io = adios.DeclareIO("TestIO");
        // Declare 1D variables
        {
            auto &var_i8 =
                io.DefineVariable<int8_t>("i8", {}, {}, adios2::Dims{8});
            auto &var_i16 =
                io.DefineVariable<int16_t>("i16", {}, {}, adios2::Dims{8});
            auto &var_i32 =
                io.DefineVariable<int32_t>("i32", {}, {}, adios2::Dims{8});
            auto &var_i64 =
                io.DefineVariable<int64_t>("i64", {}, {}, adios2::Dims{8});
            auto &var_u8 =
                io.DefineVariable<uint8_t>("u8", {}, {}, adios2::Dims{8});
            auto &var_u16 =
                io.DefineVariable<uint16_t>("u16", {}, {}, adios2::Dims{8});
            auto &var_u32 =
                io.DefineVariable<uint32_t>("u32", {}, {}, adios2::Dims{8});
            auto &var_u64 =
                io.DefineVariable<uint64_t>("u64", {}, {}, adios2::Dims{8});
            auto &var_r32 =
                io.DefineVariable<float>("r32", {}, {}, adios2::Dims{8});
            auto &var_r64 =
                io.DefineVariable<double>("r64", {}, {}, adios2::Dims{8});
        }

        // Create the HDF5 Engine
        io.SetEngine("HDF5Writer");

        adios2::Engine &engine = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < 3; ++step)
        {
            engine.BeginStep();

            // Retrieve the variables that previously went out of scope
            auto &var_i8 = *io.InquireVariable<int8_t>("i8");
            auto &var_i16 = *io.InquireVariable<int16_t>("i16");
            auto &var_i32 = *io.InquireVariable<int32_t>("i32");
            auto &var_i64 = *io.InquireVariable<int64_t>("i64");
            auto &var_u8 = *io.InquireVariable<uint8_t>("u8");
            auto &var_u16 = *io.InquireVariable<uint16_t>("u16");
            auto &var_u32 = *io.InquireVariable<uint32_t>("u32");
            auto &var_u64 = *io.InquireVariable<uint64_t>("u64");
            auto &var_r32 = *io.InquireVariable<float>("r32");
            auto &var_r64 = *io.InquireVariable<double>("r64");

            // Write each one
            engine.PutSync(var_i8, m_TestData.I8.data() + step);
            engine.PutSync(var_i16, m_TestData.I16.data() + step);
            engine.PutSync(var_i32, m_TestData.I32.data() + step);
            engine.PutSync(var_i64, m_TestData.I64.data() + step);
            engine.PutSync(var_u8, m_TestData.U8.data() + step);
            engine.PutSync(var_u16, m_TestData.U16.data() + step);
            engine.PutSync(var_u32, m_TestData.U32.data() + step);
            engine.PutSync(var_u64, m_TestData.U64.data() + step);
            engine.PutSync(var_r32, m_TestData.R32.data() + step);
            engine.PutSync(var_r64, m_TestData.R64.data() + step);

            // Advance to the next time step
            engine.EndStep();
        }

        // Close the file
        engine.Close();
    }

// Read test data using HDF5
#ifdef ADIOS2_HAVE_MPI
    // Read everything from rank 0
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0)
#endif
    {
        std::array<int8_t, 8> I8;
        std::array<int16_t, 8> I16;
        std::array<int32_t, 8> I32;
        std::array<int64_t, 8> I64;
        std::array<uint8_t, 8> U8;
        std::array<uint16_t, 8> U16;
        std::array<uint32_t, 8> U32;
        std::array<uint64_t, 8> U64;
        std::array<float, 8> R32;
        std::array<double, 8> R64;

        HDF5NativeReader hdf5Reader(fname);

        // Read stuff
        for (size_t t = 0; t < 3; ++t)
        {
            std::vector<hsize_t> gDims;
            hid_t h5Type;

            hdf5Reader.GetVarInfo("i8", gDims, h5Type);

            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_CHAR), 1);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], 8);
            hdf5Reader.ReadVar("i8", I8.data());

            hdf5Reader.GetVarInfo("i16", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_SHORT), 1);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], 8);
            hdf5Reader.ReadVar("i16", I16.data());

            hdf5Reader.GetVarInfo("i32", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_INT), 1);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], 8);
            hdf5Reader.ReadVar("i32", I32.data());

            hdf5Reader.GetVarInfo("i64", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_LONG), 1);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], 8);
            hdf5Reader.ReadVar("i64", I64.data());

            hdf5Reader.GetVarInfo("u8", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_UCHAR), 1);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], 8);
            hdf5Reader.ReadVar("u8", U8.data());

            hdf5Reader.GetVarInfo("u16", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_USHORT), 1);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], 8);
            hdf5Reader.ReadVar("u16", U16.data());

            hdf5Reader.GetVarInfo("u32", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_UINT), 1);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], 8);
            hdf5Reader.ReadVar("u32", U32.data());

            hdf5Reader.GetVarInfo("u64", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_ULONG), 1);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], 8);
            hdf5Reader.ReadVar("u64", U64.data());

            hdf5Reader.GetVarInfo("r32", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_FLOAT), 1);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], 8);
            hdf5Reader.ReadVar("r32", R32.data());

            hdf5Reader.GetVarInfo("r64", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_DOUBLE), 1);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], 8);
            hdf5Reader.ReadVar("r64", R64.data());

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
        adios2::ADIOS adios(true);
        adios2::IO &io = adios.DeclareIO("TestIO");

        // Declare 1D variables
        {
            auto &var_i8 =
                io.DefineVariable<int8_t>("i8", {}, {}, adios2::Dims{2, 4});
            auto &var_i16 =
                io.DefineVariable<int16_t>("i16", {}, {}, adios2::Dims{2, 4});
            auto &var_i32 =
                io.DefineVariable<int32_t>("i32", {}, {}, adios2::Dims{2, 4});
            auto &var_i64 =
                io.DefineVariable<int64_t>("i64", {}, {}, adios2::Dims{2, 4});
            auto &var_u8 =
                io.DefineVariable<uint8_t>("u8", {}, {}, adios2::Dims{2, 4});
            auto &var_u16 =
                io.DefineVariable<uint16_t>("u16", {}, {}, adios2::Dims{2, 4});
            auto &var_u32 =
                io.DefineVariable<uint32_t>("u32", {}, {}, adios2::Dims{2, 4});
            auto &var_u64 =
                io.DefineVariable<uint64_t>("u64", {}, {}, adios2::Dims{2, 4});
            auto &var_r32 =
                io.DefineVariable<float>("r32", {}, {}, adios2::Dims{2, 4});
            auto &var_r64 =
                io.DefineVariable<double>("r64", {}, {}, adios2::Dims{2, 4});
        }

        io.SetEngine("HDF5Writer");
        io.AddTransport("file");

        // Create the HDF5 Engine
        adios2::Engine &engine = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < 3; ++step)
        {
            engine.BeginStep();
            // Retrieve the variables that previously went out of scope
            auto &var_i8 = *io.InquireVariable<int8_t>("i8");
            auto &var_i16 = *io.InquireVariable<int16_t>("i16");
            auto &var_i32 = *io.InquireVariable<int32_t>("i32");
            auto &var_i64 = *io.InquireVariable<int64_t>("i64");
            auto &var_u8 = *io.InquireVariable<uint8_t>("u8");
            auto &var_u16 = *io.InquireVariable<uint16_t>("u16");
            auto &var_u32 = *io.InquireVariable<uint32_t>("u32");
            auto &var_u64 = *io.InquireVariable<uint64_t>("u64");
            auto &var_r32 = *io.InquireVariable<float>("r32");
            auto &var_r64 = *io.InquireVariable<double>("r64");

            // Write each one
            engine.PutSync(var_i8, m_TestData.I8.data() + step);
            engine.PutSync(var_i16, m_TestData.I16.data() + step);
            engine.PutSync(var_i32, m_TestData.I32.data() + step);
            engine.PutSync(var_i64, m_TestData.I64.data() + step);
            engine.PutSync(var_u8, m_TestData.U8.data() + step);
            engine.PutSync(var_u16, m_TestData.U16.data() + step);
            engine.PutSync(var_u32, m_TestData.U32.data() + step);
            engine.PutSync(var_u64, m_TestData.U64.data() + step);
            engine.PutSync(var_r32, m_TestData.R32.data() + step);
            engine.PutSync(var_r64, m_TestData.R64.data() + step);

            // Advance to the next time step
            engine.EndStep();
        }

        // Close the file
        engine.Close();
    }

// Read test data using HDF5
#ifdef ADIOS2_HAVE_MPI
    // Read everything from rank 0
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0)
#endif
    {
        HDF5NativeReader hdf5Reader(fname);

        std::array<int8_t, 8> I8;
        std::array<int16_t, 8> I16;
        std::array<int32_t, 8> I32;
        std::array<int64_t, 8> I64;
        std::array<uint8_t, 8> U8;
        std::array<uint16_t, 8> U16;
        std::array<uint32_t, 8> U32;
        std::array<uint64_t, 8> U64;
        std::array<float, 8> R32;
        std::array<double, 8> R64;

        // Read stuff
        for (size_t t = 0; t < 3; ++t)
        {
            std::vector<hsize_t> gDims;
            hid_t h5Type;

            hdf5Reader.GetVarInfo("i8", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_CHAR), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], 4);
            hdf5Reader.ReadVar("i8", I8.data());

            hdf5Reader.GetVarInfo("i16", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_SHORT), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], 4);
            hdf5Reader.ReadVar("i16", I16.data());

            hdf5Reader.GetVarInfo("i32", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_INT), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], 4);
            hdf5Reader.ReadVar("i32", I32.data());

            hdf5Reader.GetVarInfo("i64", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_LONG), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], 4);
            hdf5Reader.ReadVar("i64", I64.data());

            hdf5Reader.GetVarInfo("u8", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_UCHAR), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], 4);
            hdf5Reader.ReadVar("u8", U8.data());

            hdf5Reader.GetVarInfo("u16", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_USHORT), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], 4);
            hdf5Reader.ReadVar("u16", U16.data());

            hdf5Reader.GetVarInfo("u32", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_UINT), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], 4);
            hdf5Reader.ReadVar("u32", U32.data());

            hdf5Reader.GetVarInfo("u64", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_ULONG), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], 4);
            hdf5Reader.ReadVar("u64", U64.data());

            hdf5Reader.GetVarInfo("r32", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_FLOAT), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], 4);
            hdf5Reader.ReadVar("r32", R32.data());

            hdf5Reader.GetVarInfo("r64", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_DOUBLE), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], 4);
            hdf5Reader.ReadVar("r64", R64.data());

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
        adios2::ADIOS adios(true);
        adios2::IO &io = adios.DeclareIO("TestIO");

        // Declare 1D variables
        {
            auto &var_i8 =
                io.DefineVariable<int8_t>("i8", {}, {}, adios2::Dims{4, 2});
            auto &var_i16 =
                io.DefineVariable<int16_t>("i16", {}, {}, adios2::Dims{4, 2});
            auto &var_i32 =
                io.DefineVariable<int32_t>("i32", {}, {}, adios2::Dims{4, 2});
            auto &var_i64 =
                io.DefineVariable<int64_t>("i64", {}, {}, adios2::Dims{4, 2});
            auto &var_u8 =
                io.DefineVariable<uint8_t>("u8", {}, {}, adios2::Dims{4, 2});
            auto &var_u16 =
                io.DefineVariable<uint16_t>("u16", {}, {}, adios2::Dims{4, 2});
            auto &var_u32 =
                io.DefineVariable<uint32_t>("u32", {}, {}, adios2::Dims{4, 2});
            auto &var_u64 =
                io.DefineVariable<uint64_t>("u64", {}, {}, adios2::Dims{4, 2});
            auto &var_r32 =
                io.DefineVariable<float>("r32", {}, {}, adios2::Dims{4, 2});
            auto &var_r64 =
                io.DefineVariable<double>("r64", {}, {}, adios2::Dims{4, 2});
        }

        // Create the HDF5 Engine
        io.SetEngine("HDF5Writer");
        io.AddTransport("file");

        adios2::Engine &engine = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < 3; ++step)
        {
            engine.BeginStep();

            // Retrieve the variables that previously went out of scope
            auto &var_i8 = *io.InquireVariable<int8_t>("i8");
            auto &var_i16 = *io.InquireVariable<int16_t>("i16");
            auto &var_i32 = *io.InquireVariable<int32_t>("i32");
            auto &var_i64 = *io.InquireVariable<int64_t>("i64");
            auto &var_u8 = *io.InquireVariable<uint8_t>("u8");
            auto &var_u16 = *io.InquireVariable<uint16_t>("u16");
            auto &var_u32 = *io.InquireVariable<uint32_t>("u32");
            auto &var_u64 = *io.InquireVariable<uint64_t>("u64");
            auto &var_r32 = *io.InquireVariable<float>("r32");
            auto &var_r64 = *io.InquireVariable<double>("r64");

            // Write each one
            engine.PutSync(var_i8, m_TestData.I8.data() + step);
            engine.PutSync(var_i16, m_TestData.I16.data() + step);
            engine.PutSync(var_i32, m_TestData.I32.data() + step);
            engine.PutSync(var_i64, m_TestData.I64.data() + step);
            engine.PutSync(var_u8, m_TestData.U8.data() + step);
            engine.PutSync(var_u16, m_TestData.U16.data() + step);
            engine.PutSync(var_u32, m_TestData.U32.data() + step);
            engine.PutSync(var_u64, m_TestData.U64.data() + step);
            engine.PutSync(var_r32, m_TestData.R32.data() + step);
            engine.PutSync(var_r64, m_TestData.R64.data() + step);

            // Advance to the next time step
            engine.EndStep();
        }

        // Close the file
        engine.Close();
    }

// Read test data using HDF5
#ifdef ADIOS2_HAVE_MPI
    // Read everything from rank 0
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0)
#endif
    {

        HDF5NativeReader hdf5Reader(fname);

        std::array<int8_t, 8> I8;
        std::array<int16_t, 8> I16;
        std::array<int32_t, 8> I32;
        std::array<int64_t, 8> I64;
        std::array<uint8_t, 8> U8;
        std::array<uint16_t, 8> U16;
        std::array<uint32_t, 8> U32;
        std::array<uint64_t, 8> U64;
        std::array<float, 8> R32;
        std::array<double, 8> R64;

        // Read stuff
        for (size_t t = 0; t < 3; ++t)
        {
            std::vector<hsize_t> gDims;
            hid_t h5Type;

            hdf5Reader.GetVarInfo("i8", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_CHAR), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], 2);
            hdf5Reader.ReadVar("i8", I8.data());

            hdf5Reader.GetVarInfo("i16", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_SHORT), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], 2);
            hdf5Reader.ReadVar("i16", I16.data());

            hdf5Reader.GetVarInfo("i32", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_INT), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], 2);
            hdf5Reader.ReadVar("i32", I32.data());

            hdf5Reader.GetVarInfo("i64", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_LONG), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], 2);
            hdf5Reader.ReadVar("i64", I64.data());

            hdf5Reader.GetVarInfo("u8", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_UCHAR), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], 2);
            hdf5Reader.ReadVar("u8", U8.data());

            hdf5Reader.GetVarInfo("u16", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_USHORT), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], 2);
            hdf5Reader.ReadVar("u16", U16.data());

            hdf5Reader.GetVarInfo("u32", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_UINT), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], 2);
            hdf5Reader.ReadVar("u32", U32.data());

            hdf5Reader.GetVarInfo("u64", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_ULONG), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], 2);
            hdf5Reader.ReadVar("u64", U64.data());

            hdf5Reader.GetVarInfo("r32", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_FLOAT), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], 2);
            hdf5Reader.ReadVar("r32", R32.data());

            hdf5Reader.GetVarInfo("r64", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_DOUBLE), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], 2);
            hdf5Reader.ReadVar("r64", R64.data());

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
