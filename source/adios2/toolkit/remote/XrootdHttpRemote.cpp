/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * XrootdHttpRemote.cpp - HTTP/HTTPS-based client for XRootD SSI services
 *
 * Uses CURL multi interface for efficient parallel requests with connection pooling.
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

#endif // ADIOS2_HAVE_CURL

XrootdHttpRemote::XrootdHttpRemote(const adios2::HostOptions &hostOptions) : Remote(hostOptions)
{
#ifdef ADIOS2_HAVE_CURL
    curl_global_init(CURL_GLOBAL_DEFAULT);
#endif
}

XrootdHttpRemote::~XrootdHttpRemote()
{
    Close();
    ShutdownCurlMulti();
}

bool XrootdHttpRemote::InitCurlMulti()
{
#ifdef ADIOS2_HAVE_CURL
    if (m_MultiHandle)
        return true;

    m_MultiHandle = curl_multi_init();
    if (!m_MultiHandle)
    {
        helper::Log("Remote", "XrootdHttpRemote", "InitCurlMulti",
                    "Failed to initialize CURL multi handle", helper::LogMode::FATALERROR);
        return false;
    }

    curl_multi_setopt(m_MultiHandle, CURLMOPT_MAXCONNECTS, m_MaxConnections);
    curl_multi_setopt(m_MultiHandle, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);

    m_Running = true;
    m_WorkerThread = std::thread(&XrootdHttpRemote::WorkerLoop, this);
    return true;
#else
    helper::Log("Remote", "XrootdHttpRemote", "InitCurlMulti",
                "ADIOS2 was not built with CURL support", helper::LogMode::FATALERROR);
    return false;
#endif
}

void XrootdHttpRemote::ShutdownCurlMulti()
{
#ifdef ADIOS2_HAVE_CURL
    {
        std::lock_guard<std::mutex> lock(m_QueueMutex);
        m_Running = false;
    }
    m_QueueCV.notify_all();

    if (m_WorkerThread.joinable())
        m_WorkerThread.join();

    if (m_MultiHandle)
    {
        curl_multi_cleanup(m_MultiHandle);
        m_MultiHandle = nullptr;
    }
#endif
}

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

    it = params.find("MaxConnections");
    if (it != params.end())
        m_MaxConnections = std::stol(it->second);

    std::ostringstream urlStream;
    urlStream << (m_UseHttps ? "https" : "http") << "://" << hostname << ":" << port << "/ssi";
    m_BaseUrl = urlStream.str();

    if (!InitCurlMulti())
    {
        m_OpenSuccess = false;
        return;
    }

    std::string protocol = m_UseHttps ? "HTTPS" : "HTTP";
    helper::Log("Remote", "XrootdHttpRemote", "Open",
                "Opened " + protocol + " connection to " + m_BaseUrl + " for file " + m_Filename +
                    " (max " + std::to_string(m_MaxConnections) + " connections)",
                helper::LogMode::INFO);

    m_OpenSuccess = true;
}

void XrootdHttpRemote::Close() { m_OpenSuccess = false; }

std::string XrootdHttpRemote::BuildRequestString(const char *VarName, size_t Step, size_t StepCount,
                                                 size_t BlockID, const Dims &Count,
                                                 const Dims &Start)
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

void XrootdHttpRemote::ProcessCompletedTransfers()
{
#ifdef ADIOS2_HAVE_CURL
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
                            ss << ": "
                               << std::string(asyncOp->responseData.begin(),
                                              asyncOp->responseData.end());
                        asyncOp->errorMsg = ss.str();
                    }
                }
                else
                {
                    asyncOp->errorMsg = curl_easy_strerror(result);
                }

                asyncOp->promise.set_value(success);

                if (asyncOp->headers)
                {
                    curl_slist_free_all(asyncOp->headers);
                    asyncOp->headers = nullptr;
                }
            }

            curl_multi_remove_handle(m_MultiHandle, easy);
            curl_easy_cleanup(easy);
        }
    }
#endif
}

void XrootdHttpRemote::WorkerLoop()
{
#ifdef ADIOS2_HAVE_CURL
    while (true)
    {
        // Drain pending queue into curl multi handle
        {
            std::unique_lock<std::mutex> lock(m_QueueMutex);

            while (!m_PendingQueue.empty())
            {
                PendingRequest req = std::move(m_PendingQueue.front());
                m_PendingQueue.pop_front();

                CURL *easy = CreateEasyHandle(req.asyncOp, req.url, req.postData);
                if (easy)
                    curl_multi_add_handle(m_MultiHandle, easy);
                else
                {
                    req.asyncOp->errorMsg = "Failed to create CURL handle";
                    req.asyncOp->promise.set_value(false);
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
                break;
            if (m_PendingQueue.empty())
                m_QueueCV.wait(lock);
            continue;
        }

        // Wait for socket activity (up to 100ms)
        int numfds;
        CURLMcode mc = curl_multi_wait(m_MultiHandle, nullptr, 0, 100, &numfds);
        if (mc != CURLM_OK)
            helper::Log("Remote", "XrootdHttpRemote", "WorkerLoop",
                        "curl_multi_wait failed: " + std::string(curl_multi_strerror(mc)),
                        helper::LogMode::WARNING);
    }

    ProcessCompletedTransfers();
#endif
}

void XrootdHttpRemote::SubmitRequest(AsyncGet *asyncOp, const std::string &url,
                                     const std::string &postData)
{
    {
        std::lock_guard<std::mutex> lock(m_QueueMutex);
        m_PendingQueue.push_back({asyncOp, url, postData});
    }
    m_QueueCV.notify_one();
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

    AsyncGet *asyncOp = new AsyncGet();
    asyncOp->destBuffer = dest;

    std::string postData = BuildRequestString(VarName, Step, StepCount, BlockID, Count, Start);
    SubmitRequest(asyncOp, m_BaseUrl, postData);

    return static_cast<GetHandle>(asyncOp);
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
