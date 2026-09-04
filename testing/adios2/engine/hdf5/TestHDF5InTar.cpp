/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <adios2.h>

#include <hdf5.h>
#include <mpi.h>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace
{

const std::string HDF5File = "TestHDF5InTar.h5";
const std::string ContainerFile = "TestHDF5InTar.tar";
const std::string MemberName = "member.h5";
constexpr size_t TarDataOffset = 512;
constexpr size_t ElementsPerRank = 8;

bool CreateHDF5File(const size_t elements)
{
    const hid_t file = H5Fcreate(HDF5File.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    if (file < 0)
    {
        return false;
    }

    const hsize_t dimensions[] = {static_cast<hsize_t>(elements)};
    const hid_t space = H5Screate_simple(1, dimensions, nullptr);
    if (space < 0)
    {
        H5Fclose(file);
        return false;
    }

    const hid_t dataset =
        H5Dcreate(file, "/values", H5T_NATIVE_INT, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (dataset < 0)
    {
        H5Sclose(space);
        H5Fclose(file);
        return false;
    }

    std::vector<int> values(elements);
    for (size_t i = 0; i < elements; ++i)
    {
        values[i] = static_cast<int>(3 * i + 1);
    }

    const bool success =
        H5Dwrite(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, values.data()) >= 0;
    H5Dclose(dataset);
    H5Sclose(space);
    H5Fclose(file);
    return success;
}

bool EmbedHDF5File(size_t &memberSize)
{
    std::ifstream input(HDF5File, std::ios::binary | std::ios::ate);
    if (!input)
    {
        return false;
    }

    const std::streampos end = input.tellg();
    if (end < 0)
    {
        return false;
    }
    memberSize = static_cast<size_t>(end);
    input.seekg(0);

    std::ofstream output(ContainerFile, std::ios::binary | std::ios::trunc);
    if (!output)
    {
        return false;
    }

    std::array<char, TarDataOffset> tarHeader{};
    std::memcpy(tarHeader.data(), MemberName.data(), MemberName.size());
    std::snprintf(tarHeader.data() + 100, 8, "%07o", 0644);
    std::snprintf(tarHeader.data() + 108, 8, "%07o", 0);
    std::snprintf(tarHeader.data() + 116, 8, "%07o", 0);
    std::snprintf(tarHeader.data() + 124, 12, "%011llo",
                  static_cast<unsigned long long>(memberSize));
    std::snprintf(tarHeader.data() + 136, 12, "%011o", 0);
    std::memset(tarHeader.data() + 148, ' ', 8);
    tarHeader[156] = '0';
    std::memcpy(tarHeader.data() + 257, "ustar", 5);
    std::memcpy(tarHeader.data() + 263, "00", 2);

    unsigned int checksum = 0;
    for (const unsigned char byte : tarHeader)
    {
        checksum += byte;
    }
    std::snprintf(tarHeader.data() + 148, 7, "%06o", checksum);
    tarHeader[155] = ' ';

    output.write(tarHeader.data(), static_cast<std::streamsize>(tarHeader.size()));
    output << input.rdbuf();
    const std::array<char, TarDataOffset> zeroBlock{};
    const size_t padding = (TarDataOffset - memberSize % TarDataOffset) % TarDataOffset;
    output.write(zeroBlock.data(), static_cast<std::streamsize>(padding));
    output.write(zeroBlock.data(), static_cast<std::streamsize>(zeroBlock.size()));
    output.write(zeroBlock.data(), static_cast<std::streamsize>(zeroBlock.size()));
    return output.good();
}

} // end anonymous namespace

TEST(HDF5InTar, ParallelIndependentReads)
{
    int mpiRank = 0;
    int mpiSize = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);

    if (mpiRank == 0)
    {
        std::remove(HDF5File.c_str());
        std::remove(ContainerFile.c_str());
    }
    MPI_Barrier(MPI_COMM_WORLD);

    int setupSuccess = 1;
    uint64_t memberSize = 0;
    if (mpiRank == 0)
    {
        size_t localMemberSize = 0;
        setupSuccess = CreateHDF5File(static_cast<size_t>(mpiSize) * ElementsPerRank) &&
                               EmbedHDF5File(localMemberSize)
                           ? 1
                           : 0;
        memberSize = static_cast<uint64_t>(localMemberSize);
    }
    MPI_Bcast(&setupSuccess, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&memberSize, 1, MPI_UINT64_T, 0, MPI_COMM_WORLD);
    ASSERT_EQ(setupSuccess, 1);
    MPI_Barrier(MPI_COMM_WORLD);

    adios2::ADIOS adios(MPI_COMM_WORLD);
    adios2::IO io = adios.DeclareIO("HDF5InTarReader");
    io.SetEngine("HDF5");
    io.SetParameter("TarInfo", MemberName + "," + std::to_string(TarDataOffset) + "," +
                                   std::to_string(memberSize));

    adios2::Engine reader = io.Open(ContainerFile, adios2::Mode::Read);
    auto variable = io.InquireVariable<int>("/values");
    ASSERT_TRUE(variable);
    ASSERT_EQ(variable.Shape(), adios2::Dims({static_cast<size_t>(mpiSize) * ElementsPerRank}));

    const size_t start = static_cast<size_t>(mpiRank) * ElementsPerRank;
    variable.SetSelection({{start}, {ElementsPerRank}});
    std::vector<int> values(ElementsPerRank);
    reader.Get(variable, values.data());
    reader.PerformGets();
    reader.Close();

    for (size_t i = 0; i < ElementsPerRank; ++i)
    {
        EXPECT_EQ(values[i], static_cast<int>(3 * (start + i) + 1));
    }

    MPI_Barrier(MPI_COMM_WORLD);
    if (mpiRank == 0)
    {
        std::remove(HDF5File.c_str());
        std::remove(ContainerFile.c_str());
    }
}

int main(int argc, char **argv)
{
    int provided = MPI_THREAD_SINGLE;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    ::testing::InitGoogleTest(&argc, argv);
    const int result = RUN_ALL_TESTS();
    MPI_Finalize();
    return result;
}
