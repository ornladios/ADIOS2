/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Verifies the client-side `serverpath` URL-prefix plumbing (hosts.yaml
// `serverpath`, or the XRootDServerPath environment variable in the env-var
// access lane): the reader must build request URLs with the configured prefix
// in place of the built-in "/adios".
//
// Runs against the standard HTTPS test server, whose handler answers at the
// default "/adios" prefix:
//   - prefix explicitly set to "/adios": reads succeed with correct data,
//     proving the configured value produces working URLs;
//   - prefix set to anything else: requests miss the ADIOS handler and the
//     read throws, proving the setting actually reaches the URL.

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

// A configured prefix matching the server's works end to end.
TEST_F(XRootDServerPath, MatchingPrefix)
{
    setenv("XRootDServerPath", "/adios", 1);
    EXPECT_EQ(ReadRemote(), s_Expected);
}

// A non-matching prefix must land in the URL and miss the handler.
TEST_F(XRootDServerPath, WrongPrefix)
{
    setenv("XRootDServerPath", "/not-the-adios-handler", 1);
    EXPECT_THROW(ReadRemote(), std::exception);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
