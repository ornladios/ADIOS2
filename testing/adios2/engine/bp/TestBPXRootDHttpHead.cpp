/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// HEAD support on the path-encoded wire form: a data-request URL must answer
// HEAD with the exact Content-Length the corresponding GET would carry and no
// body, so an HTTP cache can size a response without fetching it.

#include <array>
#include <climits>
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

struct FetchResult
{
    long httpCode = 0;
    long long contentLength = -1; // from the Content-Length header
    std::vector<char> body;
};

// Synchronous GET or HEAD; returns false only on curl-level failure.
bool Fetch(const std::string &url, bool head, FetchResult &out)
{
    CURL *curl = curl_easy_init();
    if (!curl)
    {
        return false;
    }
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    if (head)
    {
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    }
    else
    {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out.body);
    // The test fixture's certificate is self-signed.
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    CURLcode rc = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &out.httpCode);
    curl_off_t clen = -1;
    curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &clen);
    out.contentLength = static_cast<long long>(clen);
    curl_easy_cleanup(curl);
    return rc == CURLE_OK;
}

} // anonymous namespace

class XRootDHttpHead : public ::testing::Test
{
protected:
    // Write one BP5 file and build the path-encoded single-get URL for
    // variable "t" (base64url "dA"), shared by all tests.
    static void SetUpTestSuite()
    {
        const char *host = getenv("XRootDHttpsHost");
        ASSERT_NE(host, nullptr) << "XRootDHttpsHost must be set (host:port of the test server)";

        s_Expected.resize(kNElems);
        std::iota(s_Expected.begin(), s_Expected.end(), 0.0);

        adios2::ADIOS adios;
        adios2::IO io = adios.DeclareIO("headtest");
        io.SetEngine("BP5");
        auto var = io.DefineVariable<double>("t", {kNElems}, {0}, {kNElems});
        adios2::Engine writer = io.Open("headtest.bp", adios2::Mode::Write);
        writer.BeginStep();
        writer.Put(var, s_Expected.data());
        writer.EndStep();
        writer.Close();

        std::array<char, PATH_MAX> cwd{};
        ASSERT_NE(getcwd(cwd.data(), cwd.size()), nullptr);
        s_GetUrl =
            "https://" + std::string(host) + "/adios" + cwd.data() + "/headtest.bp/r1/g~dA~c16o0";
    }

    static std::vector<double> s_Expected;
    static std::string s_GetUrl;
};

std::vector<double> XRootDHttpHead::s_Expected;
std::string XRootDHttpHead::s_GetUrl;

// HEAD: 200, Content-Length of the would-be body, no body bytes.
TEST_F(XRootDHttpHead, Head)
{
    FetchResult r;
    ASSERT_TRUE(Fetch(s_GetUrl, true, r));
    EXPECT_EQ(r.httpCode, kHttpOK);
    EXPECT_EQ(r.contentLength, (long long)(kNElems * sizeof(double)));
    EXPECT_TRUE(r.body.empty());
}

// The GET the HEAD described: same length, correct bytes.
TEST_F(XRootDHttpHead, GetMatchesHead)
{
    FetchResult r;
    ASSERT_TRUE(Fetch(s_GetUrl, false, r));
    EXPECT_EQ(r.httpCode, kHttpOK);
    ASSERT_EQ(r.body.size(), s_Expected.size() * sizeof(double));
    EXPECT_EQ(memcmp(r.body.data(), s_Expected.data(), r.body.size()), 0);
}

// Batch HEAD: Content-Length is the framed size, 8 + N*8 + sum(data bytes).
TEST_F(XRootDHttpHead, BatchHead)
{
    const std::string batchUrl =
        s_GetUrl.substr(0, s_GetUrl.rfind("/g~")) + "/b~2~dA~c16o0~dA~c8o4";
    FetchResult r;
    ASSERT_TRUE(Fetch(batchUrl, true, r));
    EXPECT_EQ(r.httpCode, kHttpOK);
    const long long framed = 3 * 8 + (16 + 8) * (long long)sizeof(double);
    EXPECT_EQ(r.contentLength, framed);
    EXPECT_TRUE(r.body.empty());
}

// HEAD of a failing request (unknown variable "x"): error status, no body.
TEST_F(XRootDHttpHead, HeadError)
{
    const std::string badUrl = s_GetUrl.substr(0, s_GetUrl.rfind("/g~")) + "/g~eA~c16o0";
    FetchResult r;
    ASSERT_TRUE(Fetch(badUrl, true, r));
    EXPECT_NE(r.httpCode, kHttpOK);
    EXPECT_TRUE(r.body.empty());
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
