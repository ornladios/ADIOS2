/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Raw byte-range reads of an arbitrary file through the XRootD remote lanes,
// the path the campaign reader takes for image and text replicas.  Runs
// against whichever server the environment selects (DoXRootD for native SSI,
// DoXRootDHttps for the HTTPS bridge, DoXRootDXrdCl for the XrdCl client) via
// the same GetRemoteSimpleFile() factory the campaign reader uses.  The server
// must return exactly the requested range and reject anything else.

#include <array>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <unistd.h> // getcwd; the server plugin is POSIX-only

#include "adios2/toolkit/remote/Remote.h"

#include <gtest/gtest.h>

namespace
{
// Larger than the native client's 1 MiB receive chunk, so a response that
// arrives in several pieces is exercised.
constexpr size_t kFileSize = 3 * 1024 * 1024 + 517;

// One campaign-style read: open the file as a byte stream, read a range,
// close.  Throws on any failure, as the campaign reader would.
std::vector<char> ReadRange(const std::string &path, size_t offset, size_t count)
{
    adios2::RemoteSetup rs = adios2::GetRemoteSetup("");
    std::shared_ptr<adios2::Remote> remote = adios2::GetRemoteSimpleFile(rs, path);
    if (!remote || !(*remote))
    {
        throw std::runtime_error("no usable remote for " + path);
    }
    std::vector<char> buffer(count);
    adios2::Remote::GetHandle handle = remote->Read(offset, count, buffer.data());
    if (handle == nullptr)
    {
        throw std::runtime_error("Read() returned no handle for " + path);
    }
    remote->WaitForGet(handle);
    remote->Close();
    return buffer;
}
} // anonymous namespace

class XRootDSimpleFile : public ::testing::Test
{
protected:
    // Write one file of pseudo-random bytes; every test reads ranges of it.
    static void SetUpTestSuite()
    {
        ASSERT_TRUE(getenv("DoXRootD") || getenv("DoXRootDHttps") || getenv("DoXRootDXrdCl"))
            << "one of DoXRootD, DoXRootDHttps, DoXRootDXrdCl must select the server lane";

        s_Data.resize(kFileSize);
        uint32_t x = 0x9E3779B9u;
        for (char &c : s_Data)
        {
            x ^= x << 13;
            x ^= x >> 17;
            x ^= x << 5;
            c = static_cast<char>(x);
        }

        std::array<char, PATH_MAX> cwd{};
        ASSERT_NE(getcwd(cwd.data(), cwd.size()), nullptr);
        s_Path = std::string(cwd.data()) + "/simplefile.bin";
        std::ofstream out(s_Path, std::ios::binary | std::ios::trunc);
        out.write(s_Data.data(), static_cast<std::streamsize>(s_Data.size()));
        ASSERT_TRUE(out.good());
    }

    static void ExpectRange(const std::vector<char> &got, size_t offset)
    {
        ASSERT_LE(offset + got.size(), s_Data.size());
        EXPECT_EQ(memcmp(got.data(), s_Data.data() + offset, got.size()), 0)
            << "bytes differ in range [" << offset << ", " << offset + got.size() << ")";
    }

    static std::vector<char> s_Data;
    static std::string s_Path;
};

std::vector<char> XRootDSimpleFile::s_Data;
std::string XRootDSimpleFile::s_Path;

TEST_F(XRootDSimpleFile, WholeFile)
{
    std::vector<char> got = ReadRange(s_Path, 0, kFileSize);
    ASSERT_EQ(got.size(), kFileSize);
    ExpectRange(got, 0);
}

// A range that straddles the 1 MiB and 2 MiB chunk boundaries.
TEST_F(XRootDSimpleFile, MiddleRange)
{
    const size_t offset = 1024 * 1024 - 7;
    const size_t count = 1024 * 1024 + 99;
    std::vector<char> got = ReadRange(s_Path, offset, count);
    ASSERT_EQ(got.size(), count);
    ExpectRange(got, offset);
}

// A short range at the very end (the common image/text case: small files).
TEST_F(XRootDSimpleFile, TailRange)
{
    const size_t count = 100;
    const size_t offset = kFileSize - count;
    std::vector<char> got = ReadRange(s_Path, offset, count);
    ASSERT_EQ(got.size(), count);
    ExpectRange(got, offset);
}

// A range running past EOF is an error, not a short read: the campaign
// recorded a byte range this replica does not have.
TEST_F(XRootDSimpleFile, RangePastEndFails)
{
    EXPECT_THROW(ReadRange(s_Path, kFileSize - 10, 20), std::exception);
}

TEST_F(XRootDSimpleFile, MissingFileFails)
{
    EXPECT_THROW(ReadRange(s_Path + ".missing", 0, 16), std::exception);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
