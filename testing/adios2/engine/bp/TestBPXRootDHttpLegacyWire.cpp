/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// REGRESSION TEST FOR THE LEGACY XROOTD-HTTP WIRE FORMAT.
//
// Released clients up through v2.12.x encode remote-read requests as a URL
// query string:
//     /adios/<filename>?get&Varname=<v>&Count=...&Start=...
//     /adios/<filename>?batchget&NVars=<n>&Varname=...&Varname=...
// The current client speaks the path-encoded form instead, but the server
// must keep answering the legacy form until those clients age out.  This
// test hand-builds requests exactly as the v2.12.0 client did and verifies
// the bytes that come back.
//
// REMOVE this test (and its registration in CMakeLists.txt) when legacy
// query-string support is retired from the server.

#include <array>
#include <climits>
#include <cstdint>
#include <cstring>
#include <numeric>
#include <string>
#include <vector>

#include <unistd.h> // getcwd; the server plugin is POSIX-only

#include <adios2.h>
#include <curl/curl.h>
#include <gtest/gtest.h>

namespace
{

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    auto *buffer = static_cast<std::vector<char> *>(userp);
    const char *data = static_cast<const char *>(contents);
    buffer->insert(buffer->end(), data, data + size * nmemb);
    return size * nmemb;
}

constexpr long kHttpOK = 200;
constexpr size_t kNElems = 16; // elements in the test variable

// Synchronous GET; returns true on HTTP 200 with the body in `out`.
bool LegacyFetch(const std::string &url, std::vector<char> &out)
{
    CURL *curl = curl_easy_init();
    if (!curl)
    {
        return false;
    }
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
    // The test fixture's certificate is self-signed.
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    CURLcode rc = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);
    return rc == CURLE_OK && httpCode == kHttpOK;
}

} // anonymous namespace

class XRootDHttpLegacyWire : public ::testing::Test
{
protected:
    // Write one BP5 file and compute the legacy base URL,
    // https://<host>/adios<absolute-filename>, shared by both tests.
    static void SetUpTestSuite()
    {
        const char *host = getenv("XRootDHttpsHost");
        ASSERT_NE(host, nullptr) << "XRootDHttpsHost must be set (host:port of the test server)";

        s_Expected.resize(kNElems);
        std::iota(s_Expected.begin(), s_Expected.end(), 0.0);

        adios2::ADIOS adios;
        adios2::IO io = adios.DeclareIO("legacywire");
        io.SetEngine("BP5");
        auto var = io.DefineVariable<double>("t", {kNElems}, {0}, {kNElems});
        adios2::Engine writer = io.Open("legacywire.bp", adios2::Mode::Write);
        writer.BeginStep();
        writer.Put(var, s_Expected.data());
        writer.EndStep();
        writer.Close();

        std::array<char, PATH_MAX> cwd{};
        ASSERT_NE(getcwd(cwd.data(), cwd.size()), nullptr);
        s_BaseUrl = "https://" + std::string(host) + "/adios" + cwd.data() + "/legacywire.bp";
    }

    static std::vector<double> s_Expected;
    static std::string s_BaseUrl;
};

std::vector<double> XRootDHttpLegacyWire::s_Expected;
std::string XRootDHttpLegacyWire::s_BaseUrl;

// Single get: raw variable bytes come back, nothing else.
TEST_F(XRootDHttpLegacyWire, SingleGet)
{
    std::vector<char> body;
    ASSERT_TRUE(LegacyFetch(s_BaseUrl + "?get&Varname=t&RMOrder=1&Count=16&Start=0", body));
    ASSERT_EQ(body.size(), s_Expected.size() * sizeof(double));
    EXPECT_EQ(memcmp(body.data(), s_Expected.data(), body.size()), 0);
}

// Batch get: [uint64 NVars][uint64 size_i ...][data_i ...] framing.
TEST_F(XRootDHttpLegacyWire, BatchGet)
{
    std::vector<char> body;
    ASSERT_TRUE(LegacyFetch(s_BaseUrl + "?batchget&NVars=2&RMOrder=1"
                                        "&Varname=t&Count=4&Start=0"
                                        "&Varname=t&Count=4&Start=12",
                            body));

    const size_t chunkBytes = 4 * sizeof(double);
    ASSERT_EQ(body.size(), 3 * sizeof(uint64_t) + 2 * chunkBytes);

    std::array<uint64_t, 3> frame{};
    memcpy(frame.data(), body.data(), sizeof(frame));
    EXPECT_EQ(frame[0], 2u); // NVars
    EXPECT_EQ(frame[1], chunkBytes);
    EXPECT_EQ(frame[2], chunkBytes);

    const char *chunk0 = body.data() + sizeof(frame);
    const char *chunk1 = chunk0 + chunkBytes;
    EXPECT_EQ(memcmp(chunk0, &s_Expected[0], chunkBytes), 0);
    EXPECT_EQ(memcmp(chunk1, &s_Expected[12], chunkBytes), 0);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
