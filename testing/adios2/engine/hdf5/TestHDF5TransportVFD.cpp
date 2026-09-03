/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adios2/helper/adiosCommDummy.h"
#include "adios2/toolkit/interop/hdf5/HDF5TransportVFD.h"
#include "adios2/toolkit/transport/OpenFile.h"

#include <hdf5.h>

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace
{

const std::string HDF5File = "TestHDF5TransportVFD.h5";
const std::string ContainerFile = "TestHDF5TransportVFD.container";
constexpr size_t PrefixSize = 513;

} // end anonymous namespace

class HDF5TransportVFDTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const hid_t file = H5Fcreate(HDF5File.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
        ASSERT_GE(file, 0);

        const hsize_t dimensions[] = {256};
        const hid_t space = H5Screate_simple(1, dimensions, nullptr);
        ASSERT_GE(space, 0);
        const hid_t dataset = H5Dcreate(file, "/values", H5T_NATIVE_INT, space, H5P_DEFAULT,
                                        H5P_DEFAULT, H5P_DEFAULT);
        ASSERT_GE(dataset, 0);

        std::vector<int> values(dimensions[0]);
        for (size_t i = 0; i < values.size(); ++i)
        {
            values[i] = static_cast<int>(3 * i + 1);
        }
        ASSERT_GE(H5Dwrite(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, values.data()),
                  0);
        ASSERT_GE(H5Dclose(dataset), 0);
        ASSERT_GE(H5Sclose(space), 0);
        ASSERT_GE(H5Fclose(file), 0);
    }

    void TearDown() override
    {
        std::remove(HDF5File.c_str());
        std::remove(ContainerFile.c_str());
    }

    static void Verify(const std::string &path, const adios2::Params &parameters)
    {
        adios2::helper::Comm comm = adios2::helper::CommDummy();
        auto transport =
            adios2::transport::OpenFile(comm, path, adios2::Mode::Read, parameters, false);
        const hid_t fapl =
            H5Pset_fapl_adios2_transport(transport, static_cast<haddr_t>(transport->GetSize()));
        ASSERT_GE(fapl, 0);

        const hid_t file = H5Fopen(path.c_str(), H5F_ACC_RDONLY, fapl);
        ASSERT_GE(H5Pclose(fapl), 0);
        ASSERT_GE(file, 0);
        const hid_t dataset = H5Dopen(file, "/values", H5P_DEFAULT);
        ASSERT_GE(dataset, 0);

        std::vector<int> values(256);
        ASSERT_GE(H5Dread(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, values.data()),
                  0);
        for (size_t i = 0; i < values.size(); ++i)
        {
            EXPECT_EQ(values[i], static_cast<int>(3 * i + 1));
        }

        EXPECT_GE(H5Dclose(dataset), 0);
        EXPECT_GE(H5Fclose(file), 0);
    }
};

TEST_F(HDF5TransportVFDTest, File) { Verify(HDF5File, {{"library", "posix"}}); }

TEST_F(HDF5TransportVFDTest, EmbeddedFile)
{
    std::ifstream input(HDF5File, std::ios::binary);
    ASSERT_TRUE(input);
    const std::vector<char> contents((std::istreambuf_iterator<char>(input)),
                                     std::istreambuf_iterator<char>());
    input.close();

    std::ofstream output(ContainerFile, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(output);
    const std::vector<char> prefix(PrefixSize, 'x');
    output.write(prefix.data(), static_cast<std::streamsize>(prefix.size()));
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    output.close();

    Verify(ContainerFile, {{"library", "posix"},
                           {"taroffset", std::to_string(PrefixSize)},
                           {"tarsize", std::to_string(contents.size())}});
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
