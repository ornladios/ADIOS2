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
    // If offset, count and memspaceSize are provided, then variable would be
    // read by selection
    void ReadVar(const std::string varName, void *dataArray,
                 hsize_t *offset = nullptr, hsize_t *count = nullptr,
                 const size_t memsspaceSize = 0);

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
        throw std::runtime_error("Unable to open dataset " + varName +
                                 " when getVarInfo");
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

void HDF5NativeReader::ReadVar(const std::string varName, void *dataArray,
                               hsize_t *offset, hsize_t *count,
                               const size_t memspaceSize)
{
    if (m_GroupId < 0)
    {
        throw std::runtime_error("Can't read variable " + varName +
                                 " since a group is not currently open");
    }

    hid_t dataSetId = H5Dopen(m_GroupId, varName.c_str(), H5P_DEFAULT);
    if (dataSetId < 0)
    {
        throw std::runtime_error("Unable to open dataset " + varName +
                                 "when ReadVar");
    }

    hid_t fileSpace = H5Dget_space(dataSetId);
    if (fileSpace < 0)
    {
        throw std::runtime_error("Unable to get filespace for dataset " +
                                 varName);
    }

    hid_t h5type = H5Dget_type(dataSetId);

    // Extend reader to support read by hyperslab selection
    // Reference link: https://support.hdfgroup.org/HDF5/Tutor/select.html
    // Check if hyperspace is provided
    if (offset && count)
    {
        // Get the dataspace
        hid_t dataspace = H5Dget_space(dataSetId);
        // Define hyperslab in the dataset
        hid_t status = H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, offset,
                                           NULL, count, NULL);
        if (status < 0)
        {
            throw std::runtime_error(
                "Unable to create a selection for dataset" + varName);
        }

        hsize_t dimsm[1];
        dimsm[0] = memspaceSize;
        hid_t memspace = H5Screate_simple(1, dimsm, NULL);

        hid_t ret = H5Dread(dataSetId, h5type, memspace, dataspace, H5P_DEFAULT,
                            dataArray);
    }
    else
    {
        hid_t ret = H5Dread(dataSetId, h5type, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                            dataArray);
    }

    H5Sclose(fileSpace);
    H5Dclose(dataSetId);
}

//******************************************************************************
// 1D 1x8 test data
//******************************************************************************

// ADIOS2 write, native HDF5 read
TEST_F(HDF5WriteReadTest, ADIOS2HDF5WriteHDF5Read1D8)
{
    // Each process would write a 1x8 array and all processes would
    // form a mpiSize * Nx 1D array
    std::string fname = "ADIOS2HDF5WriteHDF5Read1D8.h5";

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const std::size_t Nx = 8;

    // Number of steps
    const std::size_t NSteps = 3;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

    // Write test data using ADIOS2
    {
#ifdef ADIOS2_HAVE_MPI
        adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
        adios2::ADIOS adios(true);
#endif
        adios2::IO &io = adios.DeclareIO("TestIO");

        // Declare 1D variables (NumOfProcesses * Nx)
        // The local process' part (start, count) can be defined now or later
        // before Write().
        {
            adios2::Dims shape{static_cast<unsigned int>(Nx * mpiSize)};
            adios2::Dims start{static_cast<unsigned int>(Nx * mpiRank)};
            adios2::Dims count{static_cast<unsigned int>(Nx)};
            auto &var_i8 = io.DefineVariable<int8_t>("i8", shape, start, count);
            auto &var_i16 =
                io.DefineVariable<int16_t>("i16", shape, start, count);
            auto &var_i32 =
                io.DefineVariable<int32_t>("i32", shape, start, count);
            auto &var_i64 =
                io.DefineVariable<int64_t>("i64", shape, start, count);
            auto &var_u8 =
                io.DefineVariable<uint8_t>("u8", shape, start, count);
            auto &var_u16 =
                io.DefineVariable<uint16_t>("u16", shape, start, count);
            auto &var_u32 =
                io.DefineVariable<uint32_t>("u32", shape, start, count);
            auto &var_u64 =
                io.DefineVariable<uint64_t>("u64", shape, start, count);
            auto &var_r32 =
                io.DefineVariable<float>("r32", shape, start, count);
            auto &var_r64 =
                io.DefineVariable<double>("r64", shape, start, count);
        }

        // Create the ADIOS 1 Engine
        io.SetEngine("HDF5Writer");

#ifdef ADIOS2_HAVE_MPI
        io.AddTransport("file", {{"library", "MPI"}});
#else
        io.AddTransport("file");
#endif

        auto engine = io.Open(fname, adios2::OpenMode::Write);
        ASSERT_NE(engine.get(), nullptr);

        for (size_t step = 0; step < NSteps; ++step)
        {
            // Generate test data for each process uniquely
            SmallTestData currentTestData =
                generateNewSmallTestData(m_TestData, step, mpiRank, mpiSize);

            // Retrieve the variables that previously went out of scope
            auto &var_i8 = io.GetVariable<int8_t>("i8");
            auto &var_i16 = io.GetVariable<int16_t>("i16");
            auto &var_i32 = io.GetVariable<int32_t>("i32");
            auto &var_i64 = io.GetVariable<int64_t>("i64");
            auto &var_u8 = io.GetVariable<uint8_t>("u8");
            auto &var_u16 = io.GetVariable<uint16_t>("u16");
            auto &var_u32 = io.GetVariable<uint32_t>("u32");
            auto &var_u64 = io.GetVariable<uint64_t>("u64");
            auto &var_r32 = io.GetVariable<float>("r32");
            auto &var_r64 = io.GetVariable<double>("r64");

            // Make a 1D selection to describe the local dimensions of the
            // variable we write and its offsets in the global spaces
            adios2::SelectionBoundingBox sel({mpiRank * Nx}, {Nx});
            var_i8.SetSelection(sel);
            var_i16.SetSelection(sel);
            var_i32.SetSelection(sel);
            var_i64.SetSelection(sel);
            var_u8.SetSelection(sel);
            var_u16.SetSelection(sel);
            var_u32.SetSelection(sel);
            var_u64.SetSelection(sel);
            var_r32.SetSelection(sel);
            var_r64.SetSelection(sel);

            // Write each one
            // fill in the variable with values from starting index to
            // starting index + count
            engine->Write(var_i8, currentTestData.I8.data());
            engine->Write(var_i16, currentTestData.I16.data());
            engine->Write(var_i32, currentTestData.I32.data());
            engine->Write(var_i64, currentTestData.I64.data());
            engine->Write(var_u8, currentTestData.U8.data());
            engine->Write(var_u16, currentTestData.U16.data());
            engine->Write(var_u32, currentTestData.U32.data());
            engine->Write(var_u64, currentTestData.U64.data());
            engine->Write(var_r32, currentTestData.R32.data());
            engine->Write(var_r64, currentTestData.R64.data());

            // Advance to the next time step
            engine->Advance();
        }

        // Close the file
        engine->Close();
    }

    {
        const size_t arraySize = Nx;
        std::array<int8_t, arraySize> I8;
        std::array<int16_t, arraySize> I16;
        std::array<int32_t, arraySize> I32;
        std::array<int64_t, arraySize> I64;
        std::array<uint8_t, arraySize> U8;
        std::array<uint16_t, arraySize> U16;
        std::array<uint32_t, arraySize> U32;
        std::array<uint64_t, arraySize> U64;
        std::array<float, arraySize> R32;
        std::array<double, arraySize> R64;

        HDF5NativeReader hdf5Reader(fname);
        // 1D
        hsize_t count[1], offset[1];
        count[0] = mpiRank * Nx;
        offset[0] = Nx;
        size_t globalArraySize = Nx * mpiSize;

        // For each variable, we would verify its global size and type.
        // Then we would retrieve the data back which is written by the
        // current process and validate the value
        for (size_t t = 0; t < NSteps; ++t)
        {
            SmallTestData currentTestData =
                generateNewSmallTestData(m_TestData, t, mpiRank, mpiSize);

            std::vector<hsize_t> gDims;
            hid_t h5Type;

            hdf5Reader.GetVarInfo("i8", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_CHAR), 1);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], globalArraySize);
            hdf5Reader.ReadVar("i8", I8.data(), count, offset, arraySize);

            hdf5Reader.GetVarInfo("i16", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_SHORT), 1);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], globalArraySize);
            hdf5Reader.ReadVar("i16", I16.data(), count, offset, arraySize);

            hdf5Reader.GetVarInfo("i32", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_INT), 1);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], globalArraySize);
            hdf5Reader.ReadVar("i32", I32.data(), count, offset, arraySize);

            hdf5Reader.GetVarInfo("i64", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_LONG), 1);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], globalArraySize);
            hdf5Reader.ReadVar("i64", I64.data(), count, offset, arraySize);

            hdf5Reader.GetVarInfo("u8", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_UCHAR), 1);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], globalArraySize);
            hdf5Reader.ReadVar("u8", U8.data(), count, offset, arraySize);

            hdf5Reader.GetVarInfo("u16", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_USHORT), 1);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], globalArraySize);
            hdf5Reader.ReadVar("u16", U16.data(), count, offset, arraySize);

            hdf5Reader.GetVarInfo("u32", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_UINT), 1);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], globalArraySize);
            hdf5Reader.ReadVar("u32", U32.data(), count, offset, arraySize);

            hdf5Reader.GetVarInfo("u64", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_ULONG), 1);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], globalArraySize);
            hdf5Reader.ReadVar("u64", U64.data(), count, offset, arraySize);

            hdf5Reader.GetVarInfo("r32", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_FLOAT), 1);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], globalArraySize);
            hdf5Reader.ReadVar("r32", R32.data(), count, offset, arraySize);

            hdf5Reader.GetVarInfo("r64", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_DOUBLE), 1);
            ASSERT_EQ(gDims.size(), 1);
            ASSERT_EQ(gDims[0], globalArraySize);
            hdf5Reader.ReadVar("r64", R64.data(), count, offset, arraySize);

            // Check if it's correct
            for (size_t i = 0; i < Nx; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i << " rank=" << mpiRank;
                std::string msg = ss.str();

                EXPECT_EQ(I8[i], currentTestData.I8[i]) << msg;
                EXPECT_EQ(I16[i], currentTestData.I16[i]) << msg;
                EXPECT_EQ(I32[i], currentTestData.I32[i]) << msg;
                EXPECT_EQ(I64[i], currentTestData.I64[i]) << msg;
                EXPECT_EQ(U8[i], currentTestData.U8[i]) << msg;
                EXPECT_EQ(U16[i], currentTestData.U16[i]) << msg;
                EXPECT_EQ(U32[i], currentTestData.U32[i]) << msg;
                EXPECT_EQ(U64[i], currentTestData.U64[i]) << msg;
                EXPECT_EQ(R32[i], currentTestData.R32[i]) << msg;
                EXPECT_EQ(R64[i], currentTestData.R64[i]) << msg;
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
    // Each process would write a 2x4 array and all processes would
    // form a 2D 2 * (numberOfProcess*Nx) matrix where Nx is 4 here
    std::string fname = "ADIOS2HDF5WriteHDF5Read2D2x4Test.h5";

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const std::size_t Nx = 4;

    // Number of rows
    const std::size_t Ny = 2;

    // Number of steps
    const std::size_t NSteps = 3;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

    // Write test data using ADIOS2
    {
#ifdef ADIOS2_HAVE_MPI
        adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
        adios2::ADIOS adios(true);
#endif
        adios2::IO &io = adios.DeclareIO("TestIO");

        // Declare 2D variables (Ny * (NumOfProcesses * Nx))
        // The local process' part (start, count) can be defined now or later
        // before Write().
        {
            adios2::Dims shape{static_cast<unsigned int>(Ny),
                               static_cast<unsigned int>(Nx * mpiSize)};
            adios2::Dims start{static_cast<unsigned int>(0),
                               static_cast<unsigned int>(mpiRank * Nx)};
            adios2::Dims count{static_cast<unsigned int>(Ny),
                               static_cast<unsigned int>(Nx)};
            auto &var_i8 = io.DefineVariable<int8_t>("i8", shape, start, count);
            auto &var_i16 =
                io.DefineVariable<int16_t>("i16", shape, start, count);
            auto &var_i32 =
                io.DefineVariable<int32_t>("i32", shape, start, count);
            auto &var_i64 =
                io.DefineVariable<int64_t>("i64", shape, start, count);
            auto &var_u8 =
                io.DefineVariable<uint8_t>("u8", shape, start, count);
            auto &var_u16 =
                io.DefineVariable<uint16_t>("u16", shape, start, count);
            auto &var_u32 =
                io.DefineVariable<uint32_t>("u32", shape, start, count);
            auto &var_u64 =
                io.DefineVariable<uint64_t>("u64", shape, start, count);
            auto &var_r32 =
                io.DefineVariable<float>("r32", shape, start, count);
            auto &var_r64 =
                io.DefineVariable<double>("r64", shape, start, count);
        }

        // Create the ADIOS 1 Engine
        io.SetEngine("HDF5Writer");

#ifdef ADIOS2_HAVE_MPI
        io.AddTransport("file", {{"library", "MPI"}});
#else
        io.AddTransport("file");
#endif

        auto engine = io.Open(fname, adios2::OpenMode::Write);
        ASSERT_NE(engine.get(), nullptr);

        for (size_t step = 0; step < NSteps; ++step)
        {
            // Generate test data for each process uniquely
            SmallTestData currentTestData =
                generateNewSmallTestData(m_TestData, step, mpiRank, mpiSize);

            // Retrieve the variables that previously went out of scope
            auto &var_i8 = io.GetVariable<int8_t>("i8");
            auto &var_i16 = io.GetVariable<int16_t>("i16");
            auto &var_i32 = io.GetVariable<int32_t>("i32");
            auto &var_i64 = io.GetVariable<int64_t>("i64");
            auto &var_u8 = io.GetVariable<uint8_t>("u8");
            auto &var_u16 = io.GetVariable<uint16_t>("u16");
            auto &var_u32 = io.GetVariable<uint32_t>("u32");
            auto &var_u64 = io.GetVariable<uint64_t>("u64");
            auto &var_r32 = io.GetVariable<float>("r32");
            auto &var_r64 = io.GetVariable<double>("r64");

            // Make a 2D selection to describe the local dimensions of the
            // variable we write and its offsets in the global spaces
            adios2::SelectionBoundingBox sel(
                {0, static_cast<unsigned int>(mpiRank * Nx)}, {Ny, Nx});
            var_i8.SetSelection(sel);
            var_i16.SetSelection(sel);
            var_i32.SetSelection(sel);
            var_i64.SetSelection(sel);
            var_u8.SetSelection(sel);
            var_u16.SetSelection(sel);
            var_u32.SetSelection(sel);
            var_u64.SetSelection(sel);
            var_r32.SetSelection(sel);
            var_r64.SetSelection(sel);

            // Write each one
            // fill in the variable with values from starting index to
            // starting index + count
            engine->Write(var_i8, currentTestData.I8.data());
            engine->Write(var_i16, currentTestData.I16.data());
            engine->Write(var_i32, currentTestData.I32.data());
            engine->Write(var_i64, currentTestData.I64.data());
            engine->Write(var_u8, currentTestData.U8.data());
            engine->Write(var_u16, currentTestData.U16.data());
            engine->Write(var_u32, currentTestData.U32.data());
            engine->Write(var_u64, currentTestData.U64.data());
            engine->Write(var_r32, currentTestData.R32.data());
            engine->Write(var_r64, currentTestData.R64.data());

            // Advance to the next time step
            engine->Advance();
        }

        // Close the file
        engine->Close();
    }

    {
        HDF5NativeReader hdf5Reader(fname);

        const size_t arraySize = Nx * Ny;
        std::array<int8_t, arraySize> I8;
        std::array<int16_t, arraySize> I16;
        std::array<int32_t, arraySize> I32;
        std::array<int64_t, arraySize> I64;
        std::array<uint8_t, arraySize> U8;
        std::array<uint16_t, arraySize> U16;
        std::array<uint32_t, arraySize> U32;
        std::array<uint64_t, arraySize> U64;
        std::array<float, arraySize> R32;
        std::array<double, arraySize> R64;
        // 2D
        hsize_t count[2], offset[2];
        count[0] = 0;
        count[1] = mpiRank * Nx;
        offset[0] = Ny;
        offset[1] = Nx;
        size_t globalArraySize = Nx * mpiSize;

        // For each variable, we would verify its global size and type.
        // Then we would retrieve the data back which is written by the
        // current process and validate the value
        for (size_t t = 0; t < NSteps; ++t)
        {
            SmallTestData currentTestData =
                generateNewSmallTestData(m_TestData, t, mpiRank, mpiSize);

            std::vector<hsize_t> gDims;
            hid_t h5Type;

            hdf5Reader.GetVarInfo("i8", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_CHAR), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], globalArraySize);
            hdf5Reader.ReadVar("i8", I8.data(), count, offset, arraySize);

            hdf5Reader.GetVarInfo("i16", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_SHORT), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], globalArraySize);
            hdf5Reader.ReadVar("i16", I16.data(), count, offset, arraySize);

            hdf5Reader.GetVarInfo("i32", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_INT), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], globalArraySize);
            hdf5Reader.ReadVar("i32", I32.data(), count, offset, arraySize);

            hdf5Reader.GetVarInfo("i64", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_LONG), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], globalArraySize);
            hdf5Reader.ReadVar("i64", I64.data(), count, offset, arraySize);

            hdf5Reader.GetVarInfo("u8", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_UCHAR), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], globalArraySize);
            hdf5Reader.ReadVar("u8", U8.data(), count, offset, arraySize);

            hdf5Reader.GetVarInfo("u16", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_USHORT), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], globalArraySize);
            hdf5Reader.ReadVar("u16", U16.data(), count, offset, arraySize);

            hdf5Reader.GetVarInfo("u32", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_UINT), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], globalArraySize);
            hdf5Reader.ReadVar("u32", U32.data(), count, offset, arraySize);

            hdf5Reader.GetVarInfo("u64", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_ULONG), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], globalArraySize);
            hdf5Reader.ReadVar("u64", U64.data(), count, offset, arraySize);

            hdf5Reader.GetVarInfo("r32", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_FLOAT), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], globalArraySize);
            hdf5Reader.ReadVar("r32", R32.data(), count, offset, arraySize);

            hdf5Reader.GetVarInfo("r64", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_DOUBLE), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 2);
            ASSERT_EQ(gDims[1], globalArraySize);
            hdf5Reader.ReadVar("r64", R64.data(), count, offset, arraySize);

            // Check if it's correct
            for (size_t i = 0; i < Nx; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i << " rank=" << mpiRank;
                std::string msg = ss.str();

                EXPECT_EQ(I8[i], currentTestData.I8[i]) << msg;
                EXPECT_EQ(I16[i], currentTestData.I16[i]) << msg;
                EXPECT_EQ(I32[i], currentTestData.I32[i]) << msg;
                EXPECT_EQ(I64[i], currentTestData.I64[i]) << msg;
                EXPECT_EQ(U8[i], currentTestData.U8[i]) << msg;
                EXPECT_EQ(U16[i], currentTestData.U16[i]) << msg;
                EXPECT_EQ(U32[i], currentTestData.U32[i]) << msg;
                EXPECT_EQ(U64[i], currentTestData.U64[i]) << msg;
                EXPECT_EQ(R32[i], currentTestData.R32[i]) << msg;
                EXPECT_EQ(R64[i], currentTestData.R64[i]) << msg;
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

    // Each process would write a 4x2 array and all processes would
    // form a 2D 4 * (NumberOfProcess * Nx) matrix where Nx is 2 here
    std::string fname = "ADIOS2HDF5WriteHDF5Read2D4x2Test.h5";

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const std::size_t Nx = 2;
    // Number of cols
    const std::size_t Ny = 4;

    // Number of steps
    const std::size_t NSteps = 3;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

    // Write test data using ADIOS2
    {
#ifdef ADIOS2_HAVE_MPI
        adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
        adios2::ADIOS adios(true);
#endif
        adios2::IO &io = adios.DeclareIO("TestIO");

        // Declare 2D variables (4 * (NumberOfProcess * Nx))
        // The local process' part (start, count) can be defined now or later
        // before Write().
        {
            adios2::Dims shape{static_cast<unsigned int>(Ny),
                               static_cast<unsigned int>(mpiSize * Nx)};
            adios2::Dims start{static_cast<unsigned int>(0),
                               static_cast<unsigned int>(mpiRank * Nx)};
            adios2::Dims count{static_cast<unsigned int>(Ny),
                               static_cast<unsigned int>(Nx)};
            auto &var_i8 = io.DefineVariable<int8_t>("i8", shape, start, count);
            auto &var_i16 =
                io.DefineVariable<int16_t>("i16", shape, start, count);
            auto &var_i32 =
                io.DefineVariable<int32_t>("i32", shape, start, count);
            auto &var_i64 =
                io.DefineVariable<int64_t>("i64", shape, start, count);
            auto &var_u8 =
                io.DefineVariable<uint8_t>("u8", shape, start, count);
            auto &var_u16 =
                io.DefineVariable<uint16_t>("u16", shape, start, count);
            auto &var_u32 =
                io.DefineVariable<uint32_t>("u32", shape, start, count);
            auto &var_u64 =
                io.DefineVariable<uint64_t>("u64", shape, start, count);
            auto &var_r32 =
                io.DefineVariable<float>("r32", shape, start, count);
            auto &var_r64 =
                io.DefineVariable<double>("r64", shape, start, count);
        }

        // Create the ADIOS 1 Engine
        io.SetEngine("HDF5Writer");

#ifdef ADIOS2_HAVE_MPI
        io.AddTransport("file", {{"library", "MPI"}});
#else
        io.AddTransport("file");
#endif

        auto engine = io.Open(fname, adios2::OpenMode::Write);
        ASSERT_NE(engine.get(), nullptr);

        for (size_t step = 0; step < NSteps; ++step)
        {
            // Generate test data for each process uniquely
            SmallTestData currentTestData =
                generateNewSmallTestData(m_TestData, step, mpiRank, mpiSize);

            // Retrieve the variables that previously went out of scope
            auto &var_i8 = io.GetVariable<int8_t>("i8");
            auto &var_i16 = io.GetVariable<int16_t>("i16");
            auto &var_i32 = io.GetVariable<int32_t>("i32");
            auto &var_i64 = io.GetVariable<int64_t>("i64");
            auto &var_u8 = io.GetVariable<uint8_t>("u8");
            auto &var_u16 = io.GetVariable<uint16_t>("u16");
            auto &var_u32 = io.GetVariable<uint32_t>("u32");
            auto &var_u64 = io.GetVariable<uint64_t>("u64");
            auto &var_r32 = io.GetVariable<float>("r32");
            auto &var_r64 = io.GetVariable<double>("r64");

            // Make a 2D selection to describe the local dimensions of the
            // variable we write and its offsets in the global spaces
            adios2::SelectionBoundingBox sel(
                {0, static_cast<unsigned int>(mpiRank * Nx)}, {Ny, Nx});
            var_i8.SetSelection(sel);
            var_i16.SetSelection(sel);
            var_i32.SetSelection(sel);
            var_i64.SetSelection(sel);
            var_u8.SetSelection(sel);
            var_u16.SetSelection(sel);
            var_u32.SetSelection(sel);
            var_u64.SetSelection(sel);
            var_r32.SetSelection(sel);
            var_r64.SetSelection(sel);

            // Write each one
            // fill in the variable with values from starting index to
            // starting index + count
            engine->Write(var_i8, currentTestData.I8.data());
            engine->Write(var_i16, currentTestData.I16.data());
            engine->Write(var_i32, currentTestData.I32.data());
            engine->Write(var_i64, currentTestData.I64.data());
            engine->Write(var_u8, currentTestData.U8.data());
            engine->Write(var_u16, currentTestData.U16.data());
            engine->Write(var_u32, currentTestData.U32.data());
            engine->Write(var_u64, currentTestData.U64.data());
            engine->Write(var_r32, currentTestData.R32.data());
            engine->Write(var_r64, currentTestData.R64.data());

            // Advance to the next time step
            engine->Advance();
        }

        // Close the file
        engine->Close();
    }

    {

        HDF5NativeReader hdf5Reader(fname);

        const size_t arraySize = Nx * Ny;
        std::array<int8_t, arraySize> I8;
        std::array<int16_t, arraySize> I16;
        std::array<int32_t, arraySize> I32;
        std::array<int64_t, arraySize> I64;
        std::array<uint8_t, arraySize> U8;
        std::array<uint16_t, arraySize> U16;
        std::array<uint32_t, arraySize> U32;
        std::array<uint64_t, arraySize> U64;
        std::array<float, arraySize> R32;
        std::array<double, arraySize> R64;
        // 2D
        hsize_t count[2], offset[2];
        count[0] = 0;
        count[1] = mpiRank * Nx;
        offset[0] = Ny;
        offset[1] = Nx;
        size_t globalArraySize = Nx * mpiSize;

        // For each variable, we would verify its global size and type.
        // Then we would retrieve the data back which is written by the
        // current process and validate the value
        for (size_t t = 0; t < NSteps; ++t)
        {
            SmallTestData currentTestData =
                generateNewSmallTestData(m_TestData, t, mpiRank, mpiSize);

            std::vector<hsize_t> gDims;
            hid_t h5Type;

            hdf5Reader.GetVarInfo("i8", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_CHAR), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], globalArraySize);
            hdf5Reader.ReadVar("i8", I8.data(), count, offset, arraySize);

            hdf5Reader.GetVarInfo("i16", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_SHORT), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], globalArraySize);
            hdf5Reader.ReadVar("i16", I16.data(), count, offset, arraySize);

            hdf5Reader.GetVarInfo("i32", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_INT), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], globalArraySize);
            hdf5Reader.ReadVar("i32", I32.data(), count, offset, arraySize);

            hdf5Reader.GetVarInfo("i64", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_LONG), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], globalArraySize);
            hdf5Reader.ReadVar("i64", I64.data(), count, offset, arraySize);

            hdf5Reader.GetVarInfo("u8", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_UCHAR), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], globalArraySize);
            hdf5Reader.ReadVar("u8", U8.data(), count, offset, arraySize);

            hdf5Reader.GetVarInfo("u16", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_USHORT), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], globalArraySize);
            hdf5Reader.ReadVar("u16", U16.data(), count, offset, arraySize);

            hdf5Reader.GetVarInfo("u32", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_UINT), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], globalArraySize);
            hdf5Reader.ReadVar("u32", U32.data(), count, offset, arraySize);

            hdf5Reader.GetVarInfo("u64", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_ULONG), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], globalArraySize);
            hdf5Reader.ReadVar("u64", U64.data(), count, offset, arraySize);

            hdf5Reader.GetVarInfo("r32", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_FLOAT), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], globalArraySize);
            hdf5Reader.ReadVar("r32", R32.data(), count, offset, arraySize);

            hdf5Reader.GetVarInfo("r64", gDims, h5Type);
            ASSERT_EQ(H5Tequal(h5Type, H5T_NATIVE_DOUBLE), 1);
            ASSERT_EQ(gDims.size(), 2);
            ASSERT_EQ(gDims[0], 4);
            ASSERT_EQ(gDims[1], globalArraySize);
            hdf5Reader.ReadVar("r64", R64.data(), count, offset, arraySize);

            // Check if it's correct
            for (size_t i = 0; i < Nx; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i << " rank=" << mpiRank;
                std::string msg = ss.str();

                EXPECT_EQ(I8[i], currentTestData.I8[i]) << msg;
                EXPECT_EQ(I16[i], currentTestData.I16[i]) << msg;
                EXPECT_EQ(I32[i], currentTestData.I32[i]) << msg;
                EXPECT_EQ(I64[i], currentTestData.I64[i]) << msg;
                EXPECT_EQ(U8[i], currentTestData.U8[i]) << msg;
                EXPECT_EQ(U16[i], currentTestData.U16[i]) << msg;
                EXPECT_EQ(U32[i], currentTestData.U32[i]) << msg;
                EXPECT_EQ(U64[i], currentTestData.U64[i]) << msg;
                EXPECT_EQ(R32[i], currentTestData.R32[i]) << msg;
                EXPECT_EQ(R64[i], currentTestData.R64[i]) << msg;
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
