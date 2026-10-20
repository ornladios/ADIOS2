/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Verifies the client-side `serverpath` plumbing (hosts.yaml `serverpath`,
// or the XRootDServerPath environment variable in the env-var access lane):
// the reader must place the configured prefix before the dataset path in
// every request URL.  The prefix no longer routes to the ADIOS handler (the
// reserved /_adios/ segment does); it exists for deployments whose URL
// namespace does not start at the server's file system root.
//
// Runs against the standard HTTPS test server, which serves the test tree
// at its real path:
//   - prefix "/" (none): reads succeed with correct data;
//   - prefix set to anything else: the dataset path in the URL no longer
//     exists on the server and the read throws, proving the setting reaches
//     the URL.

#include <cstdlib>
#include <exception>
#include <numeric>
#include <string>
#include <vector>

#include <adios2.h>
#include <gtest/gtest.h>

namespace
{
constexpr size_t kNElems = 16;

// One remote-read cycle: open the file, read variable "t", close.
std::vector<double> ReadRemote()
{
    std::vector<double> data(kNElems);
    adios2::ADIOS adios;
    adios2::IO io = adios.DeclareIO("serverpathread");
    io.SetEngine("BP5");
    adios2::Engine reader = io.Open("serverpath.bp", adios2::Mode::Read);
    reader.BeginStep();
    auto var = io.InquireVariable<double>("t");
    EXPECT_TRUE(var);
    reader.Get(var, data.data());
    reader.PerformGets();
    reader.EndStep();
    reader.Close();
    return data;
}
} // anonymous namespace

class XRootDServerPath : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        ASSERT_NE(getenv("XRootDHttpsHost"), nullptr)
            << "XRootDHttpsHost must be set (host:port of the test server)";

        s_Expected.resize(kNElems);
        std::iota(s_Expected.begin(), s_Expected.end(), 0.0);

        adios2::ADIOS adios;
        adios2::IO io = adios.DeclareIO("serverpathwrite");
        io.SetEngine("BP5");
        auto var = io.DefineVariable<double>("t", {kNElems}, {0}, {kNElems});
        adios2::Engine writer = io.Open("serverpath.bp", adios2::Mode::Write);
        writer.BeginStep();
        writer.Put(var, s_Expected.data());
        writer.EndStep();
        writer.Close();
    }

    void TearDown() override { unsetenv("XRootDServerPath"); }

    static std::vector<double> s_Expected;
};

std::vector<double> XRootDServerPath::s_Expected;

// An explicit "no prefix" works end to end.
TEST_F(XRootDServerPath, NoPrefix)
{
    setenv("XRootDServerPath", "/", 1);
    EXPECT_EQ(ReadRemote(), s_Expected);
}

// A prefix must land in the URL: the dataset then lives at a path the
// server does not have.
TEST_F(XRootDServerPath, WrongPrefix)
{
    setenv("XRootDServerPath", "/not-a-real-namespace", 1);
    EXPECT_THROW(ReadRemote(), std::exception);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
