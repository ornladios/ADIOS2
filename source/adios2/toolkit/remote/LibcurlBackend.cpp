/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "LibcurlBackend.h"
#include "adios2/helper/adiosLog.h"

#include <atomic>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <mutex>
#include <sstream>
#include <thread>

#include <curl/curl.h>

namespace adios2
{

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

/** A fully-configured easy handle waiting to be added to the multi handle. */
struct PendingSubmit
{
    CURL *easyHandle;
    AsyncGet *asyncOp;
};

/**
 * @brief Shared CURL multi pool singleton.
 *
 * All LibcurlBackend instances submit fully-configured easy handles to this
 * pool.  A single worker thread drives all transfers via one curl_multi
 * handle, bounding the total number of concurrent TCP connections.
 */
class CurlMultiPool
{
public:
    static CurlMultiPool &getInstance()
    {
        static CurlMultiPool instance;
        return instance;
    }

    /** Submit a fully-configured easy handle for async execution. Thread-safe. */
    void Submit(CURL *easyHandle, AsyncGet *asyncOp)
    {
        {
            std::lock_guard<std::mutex> lock(m_QueueMutex);
            m_PendingQueue.push_back({easyHandle, asyncOp});
        }
        m_QueueCV.notify_one();
    }

    CurlMultiPool(const CurlMultiPool &) = delete;
    CurlMultiPool &operator=(const CurlMultiPool &) = delete;

private:
    CurlMultiPool()
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

    ~CurlMultiPool()
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

    void ProcessCompletedTransfers()
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
                            // Reject a response that would overrun dest; 0 = caller
                            // gave no expected size.
                            if (asyncOp->expectedSize != 0 &&
                                asyncOp->responseData.size() > asyncOp->expectedSize)
                            {
                                asyncOp->errorMsg = "response size " +
                                                    std::to_string(asyncOp->responseData.size()) +
                                                    " exceeds expected " +
                                                    std::to_string(asyncOp->expectedSize) +
                                                    " bytes";
                            }
                            else
                            {
                                if (asyncOp->destBuffer && !asyncOp->responseData.empty())
                                {
                                    memcpy(asyncOp->destBuffer, asyncOp->responseData.data(),
                                           asyncOp->responseData.size());
                                    asyncOp->destSize = asyncOp->responseData.size();
                                }
                                success = true;
                            }
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

                    // Clean up the easy handle BEFORE signaling completion.  Once
                    // set_value() is called, the waiting thread may immediately
                    // delete asyncOp, causing use-after-free.
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

    void WorkerLoop()
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

    CURLM *m_MultiHandle = nullptr;
    std::thread m_WorkerThread;
    std::atomic<bool> m_Running{false};
    std::mutex m_QueueMutex;
    std::condition_variable m_QueueCV;
    std::deque<PendingSubmit> m_PendingQueue;
};

CURL *CreateEasyHandle(const RemoteHttpBackend::Config &config, AsyncGet *asyncOp,
                       const std::string &url)
{
    CURL *easy = curl_easy_init();
    if (!easy)
        return nullptr;

    curl_easy_setopt(easy, CURLOPT_PRIVATE, asyncOp);

    curl_easy_setopt(easy, CURLOPT_URL, url.c_str());
    curl_easy_setopt(easy, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, &asyncOp->responseData);
    curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT, config.connectTimeout);
    curl_easy_setopt(easy, CURLOPT_TIMEOUT, config.requestTimeout);

    if (config.useHttps)
    {
        if (!config.verifySSL)
        {
            curl_easy_setopt(easy, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(easy, CURLOPT_SSL_VERIFYHOST, 0L);
        }
        else
        {
            curl_easy_setopt(easy, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(easy, CURLOPT_SSL_VERIFYHOST, 2L);
            if (!config.caCertPath.empty())
                curl_easy_setopt(easy, CURLOPT_CAINFO, config.caCertPath.c_str());
        }
    }

    curl_easy_setopt(easy, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);

    return easy;
}

} // anonymous namespace

void LibcurlBackend::SetConfig(const Config &config) { m_Config = config; }

void LibcurlBackend::SubmitGet(const std::string &url, AsyncGet *op)
{
    // Get pool reference first to ensure curl_global_init() runs before any
    // curl_easy_init() call in CreateEasyHandle().
    CurlMultiPool &pool = CurlMultiPool::getInstance();

    CURL *easy = CreateEasyHandle(m_Config, op, url);
    if (!easy)
    {
        op->errorMsg = "failed to create curl handle";
        op->promise.set_value(false);
        return;
    }

    pool.Submit(easy, op);
}

} // end namespace adios2
