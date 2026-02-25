/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * XrootdHttpRemote.cpp - HTTP/HTTPS-based client for XRootD SSI services
 *
 * Uses a shared CurlMultiPool singleton for efficient parallel requests
 * with connection pooling across all XrootdHttpRemote instances.
 */

#include "XrootdHttpRemote.h"
#include "adios2/helper/adiosLog.h"

#include <cstring>
#include <sstream>
#include <stdexcept>

#ifdef ADIOS2_HAVE_CURL
#include <curl/curl.h>
#endif

namespace adios2
{

#ifdef ADIOS2_HAVE_CURL

namespace
{

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t totalSize = size * nmemb;
    std::vector<char> *buffer = static_cast<std::vector<char> *>(userp);
    char *data = static_cast<char *>(contents);
    buffer->insert(buffer->end(), data, data + totalSize);
    return totalSize;
}

} // anonymous namespace

// ======================================================================
// CurlMultiPool implementation
// ======================================================================

CurlMultiPool &CurlMultiPool::getInstance()
{
    static CurlMultiPool instance;
    return instance;
}

CurlMultiPool::CurlMultiPool()
{
    curl_global_init(CURL_GLOBAL_DEFAULT);

    m_MultiHandle = curl_multi_init();
    if (!m_MultiHandle)
    {
        helper::Log("Remote", "CurlMultiPool", "CurlMultiPool",
                    "Failed to initialize CURL multi handle", helper::LogMode::FATALERROR);
        return;
    }

    curl_multi_setopt(m_MultiHandle, CURLMOPT_MAXCONNECTS, 50L);
    curl_multi_setopt(m_MultiHandle, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);

    m_Running = true;
    m_WorkerThread = std::thread(&CurlMultiPool::WorkerLoop, this);

    helper::Log("Remote", "CurlMultiPool", "CurlMultiPool",
                "Shared CURL multi pool initialized (max 50 connections)", helper::LogMode::INFO);
}

CurlMultiPool::~CurlMultiPool()
{
    {
        std::lock_guard<std::mutex> lock(m_QueueMutex);
        m_Running = false;
    }
    m_QueueCV.notify_all();

    if (m_WorkerThread.joinable())
    {
        m_WorkerThread.join();
    }

    if (m_MultiHandle)
    {
        curl_multi_cleanup(m_MultiHandle);
        m_MultiHandle = nullptr;
    }
}

void CurlMultiPool::Submit(CURL *easyHandle, AsyncGet *asyncOp)
{
    {
        std::lock_guard<std::mutex> lock(m_QueueMutex);
        m_PendingQueue.push_back({easyHandle, asyncOp});
    }
    m_QueueCV.notify_one();
}

void CurlMultiPool::ProcessCompletedTransfers()
{
    CURLMsg *msg;
    int msgsLeft;

    while ((msg = curl_multi_info_read(m_MultiHandle, &msgsLeft)))
    {
        if (msg->msg == CURLMSG_DONE)
        {
            CURL *easy = msg->easy_handle;
            CURLcode result = msg->data.result;

            AsyncGet *asyncOp = nullptr;
            curl_easy_getinfo(easy, CURLINFO_PRIVATE, &asyncOp);

            if (asyncOp)
            {
                bool success = false;

                if (result == CURLE_OK)
                {
                    long httpCode = 0;
                    curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &httpCode);

                    if (httpCode == 200 || httpCode == 201)
                    {
                        if (asyncOp->destBuffer && !asyncOp->responseData.empty())
                        {
                            memcpy(asyncOp->destBuffer, asyncOp->responseData.data(),
                                   asyncOp->responseData.size());
                            asyncOp->destSize = asyncOp->responseData.size();
                        }
                        success = true;
                    }
                    else
                    {
                        std::ostringstream ss;
                        ss << "HTTP error " << httpCode;
                        if (!asyncOp->responseData.empty())
                        {
                            ss << ": "
                               << std::string(asyncOp->responseData.begin(),
                                              asyncOp->responseData.end());
                        }
                        asyncOp->errorMsg = ss.str();
                    }
                }
                else
                {
                    asyncOp->errorMsg = curl_easy_strerror(result);
                }

                // Clean up headers and easy handle BEFORE signaling completion.
                // Once set_value() is called, the waiting thread in WaitForGet()
                // may immediately delete asyncOp, causing use-after-free.
                if (asyncOp->headers)
                {
                    curl_slist_free_all(asyncOp->headers);
                    asyncOp->headers = nullptr;
                }

                curl_multi_remove_handle(m_MultiHandle, easy);
                curl_easy_cleanup(easy);
                easy = nullptr;

                asyncOp->promise.set_value(success);
            }

            if (easy)
            {
                curl_multi_remove_handle(m_MultiHandle, easy);
                curl_easy_cleanup(easy);
            }
        }
    }
}

void CurlMultiPool::WorkerLoop()
{
    while (true)
    {
        // Drain pending queue into curl multi handle
        {
            std::unique_lock<std::mutex> lock(m_QueueMutex);

            while (!m_PendingQueue.empty())
            {
                PendingSubmit req = m_PendingQueue.front();
                m_PendingQueue.pop_front();

                CURLMcode rc = curl_multi_add_handle(m_MultiHandle, req.easyHandle);
                if (rc != CURLM_OK)
                {
                    req.asyncOp->errorMsg =
                        std::string("curl_multi_add_handle failed: ") + curl_multi_strerror(rc);
                    req.asyncOp->promise.set_value(false);
                    if (req.asyncOp->headers)
                    {
                        curl_slist_free_all(req.asyncOp->headers);
                        req.asyncOp->headers = nullptr;
                    }
                    curl_easy_cleanup(req.easyHandle);
                }
            }
        }

        // Drive transfers and process completions
        int runningHandles = 0;
        curl_multi_perform(m_MultiHandle, &runningHandles);
        ProcessCompletedTransfers();

        // Check if we should block waiting for new work
        if (runningHandles == 0)
        {
            std::unique_lock<std::mutex> lock(m_QueueMutex);
            if (!m_Running && m_PendingQueue.empty())
            {
                break;
            }
            if (m_PendingQueue.empty())
            {
                m_QueueCV.wait(lock);
            }
            continue;
        }

        // Wait for socket activity (up to 100ms)
        int numfds;
        CURLMcode mc = curl_multi_wait(m_MultiHandle, nullptr, 0, 100, &numfds);
        if (mc != CURLM_OK)
        {
            helper::Log("Remote", "CurlMultiPool", "WorkerLoop",
                        "curl_multi_wait failed: " + std::string(curl_multi_strerror(mc)),
                        helper::LogMode::WARNING);
        }
    }

    ProcessCompletedTransfers();
}

#endif // ADIOS2_HAVE_CURL

// ======================================================================
// XrootdHttpRemote implementation
// ======================================================================

XrootdHttpRemote::XrootdHttpRemote(const adios2::HostOptions &hostOptions) : Remote(hostOptions) {}

XrootdHttpRemote::~XrootdHttpRemote() { Close(); }

std::string XrootdHttpRemote::UrlEncode(const std::string &str)
{
#ifdef ADIOS2_HAVE_CURL
    if (str.empty())
        return str;

    CURL *curl = curl_easy_init();
    if (!curl)
        return str;

    char *encoded = curl_easy_escape(curl, str.c_str(), static_cast<int>(str.length()));
    std::string result = encoded ? std::string(encoded) : str;

    if (encoded)
        curl_free(encoded);
    curl_easy_cleanup(curl);
    return result;
#else
    return str;
#endif
}

void XrootdHttpRemote::Open(const std::string hostname, const int32_t port,
                            const std::string filename, const Mode mode, bool RowMajorOrdering,
                            const Params &params)
{
    m_Filename = filename;
    m_Mode = mode;
    m_RowMajorOrdering = RowMajorOrdering;

    auto it = params.find("UseHttps");
    if (it != params.end())
        m_UseHttps = (it->second == "true" || it->second == "1" || it->second == "yes");

    it = params.find("CAPath");
    if (it != params.end())
        m_CACertPath = it->second;

    it = params.find("VerifySSL");
    if (it != params.end())
        m_VerifySSL = (it->second == "true" || it->second == "1" || it->second == "yes");

    it = params.find("ConnectTimeout");
    if (it != params.end())
        m_ConnectTimeout = std::stol(it->second);

    it = params.find("RequestTimeout");
    if (it != params.end())
        m_RequestTimeout = std::stol(it->second);

    std::ostringstream urlStream;
    urlStream << (m_UseHttps ? "https" : "http") << "://" << hostname << ":" << port << "/ssi";
    m_BaseUrl = urlStream.str();

    std::string protocol = m_UseHttps ? "HTTPS" : "HTTP";
    helper::Log("Remote", "XrootdHttpRemote", "Open",
                "Opened " + protocol + " connection to " + m_BaseUrl + " for file " + m_Filename,
                helper::LogMode::INFO);

    m_OpenSuccess = true;
}

void XrootdHttpRemote::Close() { m_OpenSuccess = false; }

std::string XrootdHttpRemote::BuildRequestString(const char *VarName, size_t Step, size_t StepCount,
                                                 size_t BlockID, const Dims &Count,
                                                 const Dims &Start, const Accuracy &accuracy)
{
    std::ostringstream reqStream;
    std::string encodedFilename = UrlEncode(m_Filename);
    std::string encodedVarName = UrlEncode(std::string(VarName));

    reqStream << "get Filename=" << encodedFilename;
    reqStream << "&RMOrder=" << (m_RowMajorOrdering ? 1 : 0);
    reqStream << "&Varname=" << encodedVarName;
    reqStream << "&StepStart=" << Step;
    reqStream << "&StepCount=" << StepCount;
    reqStream << "&Block=" << BlockID;
    reqStream << "&Dims=" << Count.size();

    for (const auto &c : Count)
        reqStream << "&Count=" << c;
    for (const auto &s : Start)
        reqStream << "&Start=" << s;

    reqStream << "&AccuracyError=" << accuracy.error;
    reqStream << "&AccuracyNorm=" << accuracy.norm;
    reqStream << "&AccuracyRelative=" << (accuracy.relative ? 1 : 0);

    return reqStream.str();
}

CURL *XrootdHttpRemote::CreateEasyHandle(AsyncGet *asyncOp, const std::string &url,
                                         const std::string &postData)
{
#ifdef ADIOS2_HAVE_CURL
    CURL *easy = curl_easy_init();
    if (!easy)
        return nullptr;

    curl_easy_setopt(easy, CURLOPT_PRIVATE, asyncOp);
    asyncOp->easyHandle = easy;

    curl_easy_setopt(easy, CURLOPT_URL, url.c_str());
    curl_easy_setopt(easy, CURLOPT_COPYPOSTFIELDS, postData.c_str());
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, &asyncOp->responseData);
    curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT, m_ConnectTimeout);
    curl_easy_setopt(easy, CURLOPT_TIMEOUT, m_RequestTimeout);

    if (m_UseHttps)
    {
        if (!m_VerifySSL)
        {
            curl_easy_setopt(easy, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(easy, CURLOPT_SSL_VERIFYHOST, 0L);
        }
        else
        {
            curl_easy_setopt(easy, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(easy, CURLOPT_SSL_VERIFYHOST, 2L);
            if (!m_CACertPath.empty())
                curl_easy_setopt(easy, CURLOPT_CAINFO, m_CACertPath.c_str());
        }
    }

    asyncOp->headers =
        curl_slist_append(nullptr, "Content-Type: application/x-www-form-urlencoded");
    curl_easy_setopt(easy, CURLOPT_HTTPHEADER, asyncOp->headers);
    curl_easy_setopt(easy, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);

    return easy;
#else
    return nullptr;
#endif
}

std::string XrootdHttpRemote::BuildBatchRequestString(const std::vector<BatchGetRequest> &requests)
{
    std::ostringstream reqStream;
    std::string encodedFilename = UrlEncode(m_Filename);

    reqStream << "batchget Filename=" << encodedFilename;
    reqStream << "&RMOrder=" << (m_RowMajorOrdering ? 1 : 0);
    reqStream << "&NVars=" << requests.size();

    for (const auto &req : requests)
    {
        std::string encodedVarName = UrlEncode(std::string(req.VarName));
        reqStream << "|Varname=" << encodedVarName;
        reqStream << "&StepStart=" << req.Step;
        reqStream << "&StepCount=" << req.StepCount;
        reqStream << "&Block=" << req.BlockID;
        reqStream << "&Dims=" << req.Count.size();
        for (const auto &c : req.Count)
            reqStream << "&Count=" << c;
        for (const auto &s : req.Start)
            reqStream << "&Start=" << s;
        reqStream << "&AccuracyError=" << req.accuracy.error;
        reqStream << "&AccuracyNorm=" << req.accuracy.norm;
        reqStream << "&AccuracyRelative=" << (req.accuracy.relative ? 1 : 0);
    }

    return reqStream.str();
}

bool XrootdHttpRemote::BatchGet(const std::vector<BatchGetRequest> &requests)
{
    if (!m_OpenSuccess || requests.empty())
    {
        return false;
    }

    // Allow disabling batch get for performance comparison
    static bool disabled = (getenv("ADIOS2_DISABLE_BATCHGET") != nullptr);
    if (disabled)
    {
        return false;
    }

#ifdef ADIOS2_HAVE_CURL
    // Split requests into sub-batches for server-side parallelism.
    // Each sub-batch becomes a separate HTTP POST handled by its own
    // server thread, giving us parallelism without individual-Get overhead.
    const size_t subBatchSize = 10;
    size_t nBatches = (requests.size() + subBatchSize - 1) / subBatchSize;

    CurlMultiPool &pool = CurlMultiPool::getInstance();

    // Track each sub-batch: its offset into requests[], count, and async state
    struct SubBatch
    {
        size_t startIdx;
        size_t count;
        AsyncGet *asyncOp;
    };
    std::vector<SubBatch> subBatches;
    subBatches.reserve(nBatches);

    for (size_t b = 0; b < nBatches; b++)
    {
        size_t startIdx = b * subBatchSize;
        size_t count = (std::min)(subBatchSize, requests.size() - startIdx);

        std::vector<BatchGetRequest> subRequests(requests.begin() + startIdx,
                                                 requests.begin() + startIdx + count);
        std::string postData = BuildBatchRequestString(subRequests);

        AsyncGet *asyncOp = new AsyncGet();
        CURL *easy = CreateEasyHandle(asyncOp, m_BaseUrl, postData);
        if (!easy)
        {
            delete asyncOp;
            // Wait for already-submitted sub-batches before returning
            for (auto &sb : subBatches)
            {
                sb.asyncOp->promise.get_future().get();
                delete sb.asyncOp;
            }
            helper::Log("Remote", "XrootdHttpRemote", "BatchGet",
                        "Failed to create CURL handle for sub-batch", helper::LogMode::INFO);
            return false;
        }

        pool.Submit(easy, asyncOp);
        subBatches.push_back({startIdx, count, asyncOp});
    }

    // Wait for all sub-batches and parse responses
    bool allOk = true;
    for (auto &sb : subBatches)
    {
        bool success = sb.asyncOp->promise.get_future().get();

        if (!success || !allOk)
        {
            if (!success && allOk)
            {
                helper::Log("Remote", "XrootdHttpRemote", "BatchGet",
                            "Sub-batch failed: " + sb.asyncOp->errorMsg +
                                " (falling back to individual Gets)",
                            helper::LogMode::INFO);
            }
            allOk = false;
            delete sb.asyncOp;
            continue;
        }

        // Parse binary response:
        // [uint64_t NVars][uint64_t size_0]...[size_N-1][data_0]...[data_N-1]
        auto &responseData = sb.asyncOp->responseData;
        size_t nVars = sb.count;
        size_t headerSize = sizeof(uint64_t) + nVars * sizeof(uint64_t);

        if (responseData.size() < headerSize)
        {
            helper::Log("Remote", "XrootdHttpRemote", "BatchGet",
                        "Sub-batch response too small for header", helper::LogMode::INFO);
            allOk = false;
            delete sb.asyncOp;
            continue;
        }

        const char *ptr = responseData.data();
        uint64_t responseNVars = 0;
        memcpy(&responseNVars, ptr, sizeof(uint64_t));
        ptr += sizeof(uint64_t);

        if (responseNVars != nVars)
        {
            helper::Log("Remote", "XrootdHttpRemote", "BatchGet", "Sub-batch NVars mismatch",
                        helper::LogMode::INFO);
            allOk = false;
            delete sb.asyncOp;
            continue;
        }

        std::vector<uint64_t> sizes(nVars);
        memcpy(sizes.data(), ptr, nVars * sizeof(uint64_t));
        ptr += nVars * sizeof(uint64_t);

        uint64_t totalDataSize = 0;
        for (auto s : sizes)
        {
            totalDataSize += s;
        }
        if (responseData.size() != headerSize + totalDataSize)
        {
            helper::Log("Remote", "XrootdHttpRemote", "BatchGet",
                        "Sub-batch response size mismatch", helper::LogMode::INFO);
            allOk = false;
            delete sb.asyncOp;
            continue;
        }

        for (size_t i = 0; i < nVars; i++)
        {
            if (requests[sb.startIdx + i].dest && sizes[i] > 0)
            {
                memcpy(requests[sb.startIdx + i].dest, ptr, sizes[i]);
            }
            ptr += sizes[i];
        }

        delete sb.asyncOp;
    }

    return allOk;
#else
    helper::Log("Remote", "XrootdHttpRemote", "BatchGet", "ADIOS2 was not built with CURL support",
                helper::LogMode::WARNING);
    return false;
#endif
}

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

#ifdef ADIOS2_HAVE_CURL
    // Get pool reference first to ensure curl_global_init() runs
    // before any curl_easy_init() calls in CreateEasyHandle()
    CurlMultiPool &pool = CurlMultiPool::getInstance();

    AsyncGet *asyncOp = new AsyncGet();
    asyncOp->destBuffer = dest;

    std::string postData =
        BuildRequestString(VarName, Step, StepCount, BlockID, Count, Start, accuracy);
    CURL *easy = CreateEasyHandle(asyncOp, m_BaseUrl, postData);
    if (!easy)
    {
        delete asyncOp;
        helper::Log("Remote", "XrootdHttpRemote", "Get", "Failed to create CURL handle",
                    helper::LogMode::WARNING);
        return nullptr;
    }

    pool.Submit(easy, asyncOp);
    return static_cast<GetHandle>(asyncOp);
#else
    helper::Log("Remote", "XrootdHttpRemote", "Get", "ADIOS2 was not built with CURL support",
                helper::LogMode::WARNING);
    return nullptr;
#endif
}

bool XrootdHttpRemote::WaitForGet(GetHandle handle)
{
    if (!handle)
        return false;

    AsyncGet *asyncOp = static_cast<AsyncGet *>(handle);
    bool result = asyncOp->promise.get_future().get();

    if (!result)
        helper::Log("Remote", "XrootdHttpRemote", "WaitForGet", "Get failed: " + asyncOp->errorMsg,
                    helper::LogMode::WARNING);

    delete asyncOp;
    return result;
}

Remote::GetHandle XrootdHttpRemote::Read(size_t Start, size_t Size, void *Dest)
{
    helper::Log("Remote", "XrootdHttpRemote", "Read",
                "Raw byte Read not implemented for HTTP transport", helper::LogMode::WARNING);
    return nullptr;
}

} // end namespace adios2
