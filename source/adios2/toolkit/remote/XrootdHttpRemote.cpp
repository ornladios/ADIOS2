/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * XrootdHttpRemote.cpp - HTTP/HTTPS-based client for XRootD SSI services
 */

#include "XrootdHttpRemote.h"
#include "adios2/helper/adiosLog.h"

#include <cstring>
#include <sstream>
#include <stdexcept>
#include <thread>

#ifdef ADIOS2_HAVE_CURL
#include <curl/curl.h>
#endif

namespace adios2
{

/******************************************************************************/
/*                        C U R L   C a l l b a c k s                         */
/******************************************************************************/

#ifdef ADIOS2_HAVE_CURL

namespace
{

// Callback for receiving response data
size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t totalSize = size * nmemb;
    std::vector<char> *buffer = static_cast<std::vector<char> *>(userp);
    char *data = static_cast<char *>(contents);
    buffer->insert(buffer->end(), data, data + totalSize);
    return totalSize;
}

// URL-encode a string using curl_easy_escape
// This handles special characters like spaces, &, =, /, etc.
std::string UrlEncode(CURL *curl, const std::string &str)
{
    if (!curl || str.empty())
    {
        return str;
    }
    char *encoded = curl_easy_escape(curl, str.c_str(), static_cast<int>(str.length()));
    if (!encoded)
    {
        return str;
    }
    std::string result(encoded);
    curl_free(encoded);
    return result;
}

} // anonymous namespace

#endif // ADIOS2_HAVE_CURL

/******************************************************************************/
/*                     C o n s t r u c t o r / D e s t r u c t o r            */
/******************************************************************************/

XrootdHttpRemote::XrootdHttpRemote(const adios2::HostOptions &hostOptions) : Remote(hostOptions)
{
#ifdef ADIOS2_HAVE_CURL
    // Global CURL initialization (should be called once per process)
    // This is safe to call multiple times
    curl_global_init(CURL_GLOBAL_DEFAULT);
#endif
}

XrootdHttpRemote::~XrootdHttpRemote()
{
    Close();
#ifdef ADIOS2_HAVE_CURL
    CleanupCurl();
#endif
}

/******************************************************************************/
/*                              I n i t C u r l                               */
/******************************************************************************/

bool XrootdHttpRemote::InitCurl()
{
#ifdef ADIOS2_HAVE_CURL
    std::lock_guard<std::mutex> lock(m_CurlMutex);

    if (m_CurlInitialized)
    {
        return true;
    }

    m_Curl = curl_easy_init();
    if (!m_Curl)
    {
        helper::Log("Remote", "XrootdHttpRemote", "InitCurl", "Failed to initialize CURL",
                    helper::LogMode::FATALERROR);
        return false;
    }

    m_CurlInitialized = true;
    return true;
#else
    helper::Log("Remote", "XrootdHttpRemote", "InitCurl", "ADIOS2 was not built with CURL support",
                helper::LogMode::FATALERROR);
    return false;
#endif
}

/******************************************************************************/
/*                          C l e a n u p C u r l                             */
/******************************************************************************/

void XrootdHttpRemote::CleanupCurl()
{
#ifdef ADIOS2_HAVE_CURL
    std::lock_guard<std::mutex> lock(m_CurlMutex);

    if (m_Curl)
    {
        curl_easy_cleanup(m_Curl);
        m_Curl = nullptr;
    }
    m_CurlInitialized = false;
#endif
}

/******************************************************************************/
/*                                 O p e n                                    */
/******************************************************************************/

void XrootdHttpRemote::Open(const std::string hostname, const int32_t port,
                            const std::string filename, const Mode mode, bool RowMajorOrdering,
                            const Params &params)
{
    m_Filename = filename;
    m_Mode = mode;
    m_RowMajorOrdering = RowMajorOrdering;

    // Check optional parameters - do this before building URL
    // so UseHttps can be set via params
    auto it = params.find("UseHttps");
    if (it != params.end())
    {
        m_UseHttps = (it->second == "true" || it->second == "1" || it->second == "yes");
    }

    it = params.find("CAPath");
    if (it != params.end())
    {
        m_CACertPath = it->second;
    }

    it = params.find("VerifySSL");
    if (it != params.end())
    {
        m_VerifySSL = (it->second == "true" || it->second == "1" || it->second == "yes");
    }

    it = params.find("ConnectTimeout");
    if (it != params.end())
    {
        m_ConnectTimeout = std::stol(it->second);
    }

    it = params.find("RequestTimeout");
    if (it != params.end())
    {
        m_RequestTimeout = std::stol(it->second);
    }

    // Build base URL for HTTP/HTTPS connections
    // Format: http[s]://hostname:port/ssi
    std::ostringstream urlStream;
    urlStream << (m_UseHttps ? "https" : "http") << "://" << hostname << ":" << port << "/ssi";
    m_BaseUrl = urlStream.str();

    // Initialize CURL
    if (!InitCurl())
    {
        m_OpenSuccess = false;
        return;
    }

    std::string protocol = m_UseHttps ? "HTTPS" : "HTTP";
    helper::Log("Remote", "XrootdHttpRemote", "Open",
                "Opened " + protocol + " connection to " + m_BaseUrl + " for file " + m_Filename,
                helper::LogMode::INFO);

    m_OpenSuccess = true;
}

/******************************************************************************/
/*                                C l o s e                                   */
/******************************************************************************/

void XrootdHttpRemote::Close() { m_OpenSuccess = false; }

/******************************************************************************/
/*                   B u i l d R e q u e s t S t r i n g                      */
/******************************************************************************/

std::string XrootdHttpRemote::BuildRequestString(const char *VarName, size_t Step, size_t StepCount,
                                                 size_t BlockID, const Dims &Count,
                                                 const Dims &Start, const Accuracy &accuracy)
{
    // Build request string in the same format as XrootdRemote
    // Format: get
    // Filename=xxx&RMOrder=n&Varname=yyy&StepStart=n&StepCount=n&Block=n&Dims=n&Count=n&Start=n...
    //
    // String parameters (Filename, Varname) are URL-encoded to handle special characters
    // like spaces, &, =, /, etc. that could break the parsing.

    std::ostringstream reqStream;

#ifdef ADIOS2_HAVE_CURL
    // URL-encode string parameters to handle special characters
    std::string encodedFilename = UrlEncode(m_Curl, m_Filename);
    std::string encodedVarName = UrlEncode(m_Curl, std::string(VarName));
#else
    // Fallback: use raw strings (may break with special characters)
    std::string encodedFilename = m_Filename;
    std::string encodedVarName = std::string(VarName);
#endif

    reqStream << "get Filename=" << encodedFilename;
    reqStream << "&RMOrder=" << (m_RowMajorOrdering ? 1 : 0);
    reqStream << "&Varname=" << encodedVarName;
    reqStream << "&StepStart=" << Step;
    reqStream << "&StepCount=" << StepCount;
    reqStream << "&Block=" << BlockID;
    reqStream << "&Dims=" << Count.size();

    for (const auto &c : Count)
    {
        reqStream << "&Count=" << c;
    }

    for (const auto &s : Start)
    {
        reqStream << "&Start=" << s;
    }

    // Add accuracy parameters
    reqStream << "&AccuracyError=" << accuracy.error;
    reqStream << "&AccuracyNorm=" << accuracy.norm;
    reqStream << "&AccuracyRelative=" << (accuracy.relative ? 1 : 0);

    return reqStream.str();
}

/******************************************************************************/
/*                             H t t p P o s t                                */
/******************************************************************************/

bool XrootdHttpRemote::HttpPost(const std::string &endpoint, const std::string &requestData,
                                std::vector<char> &responseData, std::string &errorMsg)
{
#ifdef ADIOS2_HAVE_CURL
    std::lock_guard<std::mutex> lock(m_CurlMutex);

    if (!m_Curl)
    {
        errorMsg = "CURL not initialized";
        return false;
    }

    // Reset CURL handle for new request
    curl_easy_reset(m_Curl);

    // Set URL
    curl_easy_setopt(m_Curl, CURLOPT_URL, endpoint.c_str());

    // Set POST data
    curl_easy_setopt(m_Curl, CURLOPT_POSTFIELDS, requestData.c_str());
    curl_easy_setopt(m_Curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(requestData.length()));

    // Set response callback
    responseData.clear();
    curl_easy_setopt(m_Curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(m_Curl, CURLOPT_WRITEDATA, &responseData);

    // Set timeouts
    curl_easy_setopt(m_Curl, CURLOPT_CONNECTTIMEOUT, m_ConnectTimeout);
    curl_easy_setopt(m_Curl, CURLOPT_TIMEOUT, m_RequestTimeout);

    // SSL configuration - only relevant for HTTPS
    if (m_UseHttps)
    {
        if (!m_VerifySSL)
        {
            curl_easy_setopt(m_Curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(m_Curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }
        else
        {
            curl_easy_setopt(m_Curl, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(m_Curl, CURLOPT_SSL_VERIFYHOST, 2L);

            if (!m_CACertPath.empty())
            {
                curl_easy_setopt(m_Curl, CURLOPT_CAINFO, m_CACertPath.c_str());
            }
        }
    }

    // Set headers
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    curl_easy_setopt(m_Curl, CURLOPT_HTTPHEADER, headers);

    // Perform request
    CURLcode res = curl_easy_perform(m_Curl);

    // Cleanup headers
    curl_slist_free_all(headers);

    if (res != CURLE_OK)
    {
        errorMsg = curl_easy_strerror(res);
        return false;
    }

    // Check HTTP response code
    long httpCode = 0;
    curl_easy_getinfo(m_Curl, CURLINFO_RESPONSE_CODE, &httpCode);

    if (httpCode != 200 && httpCode != 201)
    {
        std::ostringstream ss;
        ss << "HTTP error " << httpCode;
        if (!responseData.empty())
        {
            ss << ": " << std::string(responseData.begin(), responseData.end());
        }
        errorMsg = ss.str();
        return false;
    }

    return true;
#else
    errorMsg = "ADIOS2 was not built with CURL support";
    return false;
#endif
}

/******************************************************************************/
/*              B u i l d B a t c h R e q u e s t S t r i n g                 */
/******************************************************************************/

std::string XrootdHttpRemote::BuildBatchRequestString(const std::vector<BatchGetRequest> &requests)
{
    std::ostringstream reqStream;

#ifdef ADIOS2_HAVE_CURL
    std::string encodedFilename = UrlEncode(m_Curl, m_Filename);
#else
    std::string encodedFilename = m_Filename;
#endif

    // Shared header: command, filename, array order, number of variables
    reqStream << "batchget Filename=" << encodedFilename;
    reqStream << "&RMOrder=" << (m_RowMajorOrdering ? 1 : 0);
    reqStream << "&NVars=" << requests.size();

    // Per-variable sections, pipe-delimited
    for (const auto &req : requests)
    {
#ifdef ADIOS2_HAVE_CURL
        std::string encodedVarName = UrlEncode(m_Curl, std::string(req.VarName));
#else
        std::string encodedVarName = std::string(req.VarName);
#endif
        reqStream << "|Varname=" << encodedVarName;
        reqStream << "&StepStart=" << req.Step;
        reqStream << "&StepCount=" << req.StepCount;
        reqStream << "&Block=" << req.BlockID;
        reqStream << "&Dims=" << req.Count.size();
        for (const auto &c : req.Count)
        {
            reqStream << "&Count=" << c;
        }
        for (const auto &s : req.Start)
        {
            reqStream << "&Start=" << s;
        }
        reqStream << "&AccuracyError=" << req.accuracy.error;
        reqStream << "&AccuracyNorm=" << req.accuracy.norm;
        reqStream << "&AccuracyRelative=" << (req.accuracy.relative ? 1 : 0);
    }

    return reqStream.str();
}

/******************************************************************************/
/*                            B a t c h G e t                                 */
/******************************************************************************/

bool XrootdHttpRemote::BatchGet(const std::vector<BatchGetRequest> &requests)
{
    if (!m_OpenSuccess || requests.empty())
    {
        return false;
    }

    std::string requestData = BuildBatchRequestString(requests);

    std::vector<char> responseData;
    std::string errorMsg;

    bool success = HttpPost(m_BaseUrl, requestData, responseData, errorMsg);

    if (!success)
    {
        helper::Log("Remote", "XrootdHttpRemote", "BatchGet",
                    "BatchGet failed: " + errorMsg + " (falling back to individual Gets)",
                    helper::LogMode::INFO);
        return false;
    }

    // Parse binary response: [uint64_t NVars][uint64_t size_0]...[size_N-1][data_0]...[data_N-1]
    size_t nVars = requests.size();
    size_t headerSize = sizeof(uint64_t) + nVars * sizeof(uint64_t);

    if (responseData.size() < headerSize)
    {
        helper::Log("Remote", "XrootdHttpRemote", "BatchGet",
                    "Response too small for header, falling back", helper::LogMode::INFO);
        return false;
    }

    const char *ptr = responseData.data();
    uint64_t responseNVars;
    memcpy(&responseNVars, ptr, sizeof(uint64_t));
    ptr += sizeof(uint64_t);

    if (responseNVars != nVars)
    {
        helper::Log("Remote", "XrootdHttpRemote", "BatchGet",
                    "Response NVars mismatch, falling back", helper::LogMode::INFO);
        return false;
    }

    // Read the size table
    std::vector<uint64_t> sizes(nVars);
    memcpy(sizes.data(), ptr, nVars * sizeof(uint64_t));
    ptr += nVars * sizeof(uint64_t);

    // Verify total size
    uint64_t totalDataSize = 0;
    for (auto s : sizes)
    {
        totalDataSize += s;
    }
    if (responseData.size() != headerSize + totalDataSize)
    {
        helper::Log("Remote", "XrootdHttpRemote", "BatchGet",
                    "Response size mismatch, falling back", helper::LogMode::INFO);
        return false;
    }

    // Demux data into destination buffers
    for (size_t i = 0; i < nVars; i++)
    {
        if (requests[i].dest && sizes[i] > 0)
        {
            memcpy(requests[i].dest, ptr, sizes[i]);
        }
        ptr += sizes[i];
    }

    return true;
}

/******************************************************************************/
/*                                  G e t                                     */
/******************************************************************************/

Remote::GetHandle XrootdHttpRemote::Get(const char *VarName, size_t Step, size_t StepCount,
                                        size_t BlockID, Dims &Count, Dims &Start,
                                        Accuracy &accuracy, void *dest)
{
    if (!m_OpenSuccess)
    {
        helper::Log("Remote", "XrootdHttpRemote", "Get", "Connection not open",
                    helper::LogMode::WARNING);
        return nullptr;
    }

    // Create async operation structure
    AsyncGet *asyncOp = new AsyncGet();
    asyncOp->destBuffer = dest;
    asyncOp->destSize = 0;

    // Build request string
    std::string requestData =
        BuildRequestString(VarName, Step, StepCount, BlockID, Count, Start, accuracy);

    // Launch async HTTP request
    std::thread requestThread([this, asyncOp, requestData]() {
        std::vector<char> responseData;
        std::string errorMsg;

        bool success = HttpPost(m_BaseUrl, requestData, responseData, errorMsg);

        if (success && asyncOp->destBuffer && !responseData.empty())
        {
            // Copy response data to destination buffer
            memcpy(asyncOp->destBuffer, responseData.data(), responseData.size());
            asyncOp->destSize = responseData.size();
            asyncOp->promise.set_value(true);
        }
        else
        {
            asyncOp->errorMsg = errorMsg;
            asyncOp->promise.set_value(false);
        }
    });

    requestThread.detach();

    return static_cast<GetHandle>(asyncOp);
}

/******************************************************************************/
/*                           W a i t F o r G e t                              */
/******************************************************************************/

bool XrootdHttpRemote::WaitForGet(GetHandle handle)
{
    if (!handle)
    {
        return false;
    }

    AsyncGet *asyncOp = static_cast<AsyncGet *>(handle);

    bool result = asyncOp->promise.get_future().get();

    if (!result)
    {
        helper::Log("Remote", "XrootdHttpRemote", "WaitForGet", "Get failed: " + asyncOp->errorMsg,
                    helper::LogMode::WARNING);
    }

    delete asyncOp;
    return result;
}

/******************************************************************************/
/*                                 R e a d                                    */
/******************************************************************************/

Remote::GetHandle XrootdHttpRemote::Read(size_t Start, size_t Size, void *Dest)
{
    // Raw byte read not implemented for HTTP
    // This would require a different server-side implementation
    helper::Log("Remote", "XrootdHttpRemote", "Read",
                "Raw byte Read not implemented for HTTP transport", helper::LogMode::WARNING);
    return nullptr;
}

} // end namespace adios2
