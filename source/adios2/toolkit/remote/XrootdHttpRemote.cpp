/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "XrootdHttpRemote.h"
#include "adios2/helper/adiosLog.h"
#include "adios2/helper/adiosSystem.h" // IsLittleEndian

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <thread>

#ifdef _WIN32
#include <direct.h>
#include <stdlib.h>
#define getcwd _getcwd
#define PATH_MAX _MAX_PATH
#else
#include <climits>
#include <unistd.h>
#endif

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
    curl_multi_setopt(m_MultiHandle, CURLMOPT_MAX_TOTAL_CONNECTIONS, 50L);
    curl_multi_setopt(m_MultiHandle, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);

    m_Running = true;
    m_WorkerThread = std::thread(&CurlMultiPool::WorkerLoop, this);
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

        // Skip waiting if new requests are queued -- submit them immediately
        {
            std::lock_guard<std::mutex> lock(m_QueueMutex);
            if (!m_PendingQueue.empty())
            {
                continue;
            }
        }

        // Wait for socket activity (up to 10ms)
        int numfds;
        CURLMcode mc = curl_multi_wait(m_MultiHandle, nullptr, 0, 10, &numfds);
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

// Base64url encoding (RFC 4648 §5).  Uses only A-Za-z0-9, '-', '_' —
// all path-safe characters that no HTTP intermediary will mangle.
// No padding ('=') is emitted; the decoder infers it from length.
std::string XrootdHttpRemote::Base64urlEncode(const std::string &str)
{
    static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::string out;
    const auto *src = reinterpret_cast<const unsigned char *>(str.data());
    size_t len = str.size();
    out.reserve((len * 4 + 2) / 3);
    size_t i = 0;
    for (; i + 2 < len; i += 3)
    {
        out += table[(src[i] >> 2) & 0x3F];
        out += table[((src[i] & 0x03) << 4) | (src[i + 1] >> 4)];
        out += table[((src[i + 1] & 0x0F) << 2) | (src[i + 2] >> 6)];
        out += table[src[i + 2] & 0x3F];
    }
    if (i < len)
    {
        out += table[(src[i] >> 2) & 0x3F];
        if (i + 1 < len)
        {
            out += table[((src[i] & 0x03) << 4) | (src[i + 1] >> 4)];
            out += table[(src[i + 1] & 0x0F) << 2];
        }
        else
        {
            out += table[(src[i] & 0x03) << 4];
        }
    }
    return out;
}

void XrootdHttpRemote::Open(const std::string hostname, const int32_t port,
                            const std::string filename, const Mode mode, bool RowMajorOrdering,
                            const Params &params)
{
    // Ensure filename is absolute so the URL path is clean (no double-slash ambiguity)
    if (!filename.empty() && filename[0] != '/')
    {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)))
        {
            m_Filename = std::string(cwd) + "/" + filename;
        }
        else
        {
            m_Filename = filename;
        }
    }
    else
    {
        m_Filename = filename;
    }
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

    // File id for the server's identity check.
    it = params.find("FileUUID");
    if (it != params.end())
        m_FileUUID = static_cast<uint32_t>(std::stoul(it->second));

    // Collect non-HTTP engine params (TarInfo, SelectSteps, IgnoreFlattenSteps)
    // and encode them as a TAB-separated string for transmission
    Params engineParams;
    for (const auto &p : params)
    {
        if (p.first != "UseHttps" && p.first != "CAPath" && p.first != "VerifySSL" &&
            p.first != "ConnectTimeout" && p.first != "RequestTimeout" && p.first != "FileUUID")
        {
            engineParams[p.first] = p.second;
        }
    }
    if (!engineParams.empty())
    {
        m_EngineParams = ParamsToEncodedString(engineParams);
    }

    std::ostringstream urlStream;
    urlStream << (m_UseHttps ? "https" : "http") << "://" << hostname << ":" << port << "/adios"
              << m_Filename;
    m_BaseUrl = urlStream.str();

    // Per-engine constants (RMOrder + EngineParams) are packed once into
    // the file-config path segment and reused on every request.
    m_FileConfigSegment = BuildFileConfigSegment();

    m_OpenSuccess = true;
}

void XrootdHttpRemote::Close() { m_OpenSuccess = false; }

CURL *XrootdHttpRemote::CreateEasyHandle(AsyncGet *asyncOp, const std::string &url)
{
#ifdef ADIOS2_HAVE_CURL
    CURL *easy = curl_easy_init();
    if (!easy)
        return nullptr;

    curl_easy_setopt(easy, CURLOPT_PRIVATE, asyncOp);
    asyncOp->easyHandle = easy;

    curl_easy_setopt(easy, CURLOPT_URL, url.c_str());
    curl_easy_setopt(easy, CURLOPT_HTTPGET, 1L);
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

    curl_easy_setopt(easy, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);

    return easy;
#else
    return nullptr;
#endif
}

// ---------------------------------------------------------------------
// Path-encoded URL builders (Pelican/XCache-friendly form).
//
// Full URL: <scheme>://<host>:<port>/adios<filename>/<file-config>/<request>
//
// The filename keeps its literal slashes (it is not URL-encoded), so
// it occupies multiple path segments.  The server identifies the last
// two segments as file-config and request and rejoins everything
// before them as the filename.
//
// Per-segment grammar:
//   file-config:   r<digit>(p<base64url-EP>)?       (always at least r0/r1)
//   single get:    g~<base64url-var>~<paramstring>  (`_` if no params)
//   batch get:     b~N~<v1>~<p1>~<v2>~<p2>~...~<vN>~<pN>
//
// Variable names and EngineParams are base64url-encoded (RFC 4648 §5)
// so they never collide with our delimiters and so HTTP intermediaries
// can't normalize them in transit.  Paramstring fields are letter-
// prefixed and self-delimiting; vector elements use `,` (e.g.
// `c4,8,12`); outer delimiter is `~`; floats keep `.` as decimal
// separator without colliding with anything.
// ---------------------------------------------------------------------

std::string XrootdHttpRemote::BuildFileConfigSegment()
{
    std::ostringstream s;
    // Always emit RMOrder so the server doesn't have to guess the default.
    s << "r" << (m_RowMajorOrdering ? "1" : "0");
    // File id for the identity check (omit when 0). Must precede the greedy `p`.
    if (m_FileUUID != 0)
        s << "u" << m_FileUUID;
    // Client byte order (omit when little-endian). Must precede the greedy `p`.
    if (!helper::IsLittleEndian())
        s << "e1";
    if (!m_EngineParams.empty())
        s << "p" << Base64urlEncode(m_EngineParams);
    std::string out = s.str();
    // Empty file-config gets a placeholder so the path segment is non-
    // empty; some intermediaries normalize empty segments away.
    return out.empty() ? std::string("_") : out;
}

std::string XrootdHttpRemote::BuildPerRequestParamString(size_t Step, size_t StepCount,
                                                         size_t BlockID, const Dims &Count,
                                                         const Dims &Start,
                                                         const Accuracy &accuracy)
{
    std::ostringstream s;
    if (BlockID != static_cast<size_t>(-1))
        s << "b" << BlockID;
    if (Step != 0)
        s << "s" << Step;
    if (StepCount != 1)
        s << "S" << StepCount;
    if (!Count.empty())
    {
        s << "c" << Count[0];
        for (size_t i = 1; i < Count.size(); ++i)
            s << "," << Count[i];
    }
    if (!Start.empty())
    {
        s << "o" << Start[0];
        for (size_t i = 1; i < Start.size(); ++i)
            s << "," << Start[i];
    }
    // `a`/`N` (not `e`/`n`) so the letters don't collide with characters
    // that appear in float formatting: `e` in scientific notation
    // (`1e-5`), `n` in `nan`/`inf`.  None of {a, N, R} appear in float
    // output, so a value is always self-delimiting.
    if (accuracy.error != 0.0)
        s << "a" << accuracy.error;
    if (accuracy.norm != 0.0)
        s << "N" << accuracy.norm;
    if (accuracy.relative)
        s << "R1";
    std::string out = s.str();
    // Same `_` placeholder convention as file-config: an all-default
    // request is the empty string and would collapse adjacent `~`s in
    // batch sub-requests.
    return out.empty() ? std::string("_") : out;
}

std::string XrootdHttpRemote::BuildSingleGetSegment(const char *VarName, size_t Step,
                                                    size_t StepCount, size_t BlockID,
                                                    const Dims &Count, const Dims &Start,
                                                    const Accuracy &accuracy)
{
    return "g~" + Base64urlEncode(std::string(VarName)) + "~" +
           BuildPerRequestParamString(Step, StepCount, BlockID, Count, Start, accuracy);
}

std::string XrootdHttpRemote::BuildBatchGetSegment(const std::vector<BatchGetRequest> &requests)
{
    std::ostringstream s;
    s << "b~" << requests.size();
    for (const auto &r : requests)
    {
        s << "~" << Base64urlEncode(std::string(r.VarName)) << "~"
          << BuildPerRequestParamString(r.Step, r.StepCount, r.BlockID, r.Count, r.Start,
                                        r.accuracy);
    }
    return s.str();
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
        std::string url =
            m_BaseUrl + "/" + m_FileConfigSegment + "/" + BuildBatchGetSegment(subRequests);

        AsyncGet *asyncOp = new AsyncGet();
        CURL *easy = CreateEasyHandle(asyncOp, url);
        if (!easy)
        {
            delete asyncOp;
            // Wait for already-submitted sub-batches before returning
            for (auto &sb : subBatches)
            {
                sb.asyncOp->promise.get_future().get();
                delete sb.asyncOp;
            }
            return false;
        }

        pool.Submit(easy, asyncOp);
        subBatches.push_back({startIdx, count, asyncOp});
    }

    // Wait for all sub-batches, retry transient failures
    const int maxRetries = 3;
    bool allOk = true;
    for (auto &sb : subBatches)
    {
        bool success = sb.asyncOp->promise.get_future().get();
        std::string lastErr;

        if (!success)
        {
            lastErr = sb.asyncOp->errorMsg;
            delete sb.asyncOp;
            sb.asyncOp = nullptr;

            // Retry this sub-batch
            for (int retry = 1; retry <= maxRetries; retry++)
            {
                helper::Log("Remote", "XrootdHttpRemote", "BatchGet",
                            "Sub-batch failed (" + lastErr + "), retry " + std::to_string(retry) +
                                "/" + std::to_string(maxRetries),
                            helper::LogMode::WARNING);

                std::this_thread::sleep_for(std::chrono::milliseconds(100 * retry));

                std::vector<BatchGetRequest> subRequests(requests.begin() + sb.startIdx,
                                                         requests.begin() + sb.startIdx + sb.count);
                std::string url =
                    m_BaseUrl + "/" + m_FileConfigSegment + "/" + BuildBatchGetSegment(subRequests);

                AsyncGet *retryOp = new AsyncGet();
                CURL *easy = CreateEasyHandle(retryOp, url);
                if (!easy)
                {
                    delete retryOp;
                    continue;
                }
                pool.Submit(easy, retryOp);
                success = retryOp->promise.get_future().get();
                if (success)
                {
                    sb.asyncOp = retryOp;
                    break;
                }
                lastErr = retryOp->errorMsg;
                delete retryOp;
            }

            if (!success)
            {
                helper::Throw<std::runtime_error>("Remote", "XrootdHttpRemote", "BatchGet",
                                                  "Sub-batch failed after " +
                                                      std::to_string(maxRetries) +
                                                      " retries: " + lastErr);
            }
        }

        // Parse binary response:
        // [uint64_t NVars][uint64_t size_0]...[size_N-1][data_0]...[data_N-1]
        auto &responseData = sb.asyncOp->responseData;
        size_t nVars = sb.count;
        size_t headerSize = sizeof(uint64_t) + nVars * sizeof(uint64_t);

        if (responseData.size() < headerSize)
        {
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
        return nullptr;
    }

#ifdef ADIOS2_HAVE_CURL
    // Get pool reference first to ensure curl_global_init() runs
    // before any curl_easy_init() calls in CreateEasyHandle()
    CurlMultiPool &pool = CurlMultiPool::getInstance();

    AsyncGet *asyncOp = new AsyncGet();
    asyncOp->destBuffer = dest;

    std::string url =
        m_BaseUrl + "/" + m_FileConfigSegment + "/" +
        BuildSingleGetSegment(VarName, Step, StepCount, BlockID, Count, Start, accuracy);
    CURL *easy = CreateEasyHandle(asyncOp, url);
    if (!easy)
    {
        delete asyncOp;
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

    delete asyncOp;
    return result;
}

Remote::GetHandle XrootdHttpRemote::Read(size_t Start, size_t Size, void *Dest) { return nullptr; }

} // end namespace adios2
