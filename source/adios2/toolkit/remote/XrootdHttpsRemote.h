/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * XrootdHttpsRemote.h - HTTPS-based client for XRootD SSI services
 *
 * This provides the same functionality as XrootdRemote but uses HTTPS
 * instead of the native XRootD SSI protocol. It connects to an XRootD
 * server running the HTTP-to-SSI bridge handler.
 */

#ifndef ADIOS2_XROOTDHTTPSREMOTE_H
#define ADIOS2_XROOTDHTTPSREMOTE_H

#include "Remote.h"
#include "adios2/common/ADIOSConfig.h"
#include "adios2/common/ADIOSTypes.h"
#include "adios2/toolkit/profiling/iochrono/IOChrono.h"

#include <future>
#include <memory>
#include <mutex>
#include <string>

// Forward declaration for CURL handle
typedef void CURL;

namespace adios2
{

/**
 * @brief HTTPS-based remote access to ADIOS data via XRootD HTTP-SSI bridge
 *
 * This class provides the same interface as XrootdRemote but uses HTTPS
 * for transport instead of the native XRootD SSI protocol. This enables:
 * - Access through HTTPS (port 443/8443) which is commonly allowed through firewalls
 * - TLS encryption for data in transit
 * - Compatibility with NERSC Spin and other Kubernetes deployments
 *
 * The server must be running XRootD with the HTTP-to-SSI bridge handler
 * (libadios2_xrootd_http.so) loaded.
 *
 * Usage:
 *   XrootdHttpsRemote remote(hostOptions);
 *   remote.Open("server.example.com", 8443, "/path/to/file.bp", Mode::Read, true);
 *   auto handle = remote.Get("temperature", step, 1, blockID, count, start, accuracy, buffer);
 *   remote.WaitForGet(handle);
 */
class XrootdHttpsRemote : public Remote
{
public:
    profiling::IOChrono m_Profiler;

    /**
     * @brief Construct an HTTPS remote client
     * @param hostOptions Host configuration options
     */
    XrootdHttpsRemote(const adios2::HostOptions &hostOptions);

    /**
     * @brief Destructor - cleans up CURL resources
     */
    ~XrootdHttpsRemote();

    /**
     * @brief Check if connection is open and valid
     */
    explicit operator bool() const override { return m_OpenSuccess; }

    /**
     * @brief Open a remote ADIOS file via HTTPS
     * @param hostname Server hostname
     * @param port HTTPS port (typically 443 or 8443)
     * @param filename Path to the ADIOS file on the server
     * @param mode Open mode (Read, Write, etc.)
     * @param RowMajorOrdering Array ordering preference
     * @param params Additional parameters
     */
    void Open(const std::string hostname, const int32_t port, const std::string filename,
              const Mode mode, bool RowMajorOrdering, const Params &params = Params()) override;

    /**
     * @brief Asynchronously get variable data from the remote file
     * @param VarName Variable name
     * @param Step Time step
     * @param StepCount Number of steps to read
     * @param BlockID Block ID for block selection
     * @param Count Selection count in each dimension
     * @param Start Selection start in each dimension
     * @param accuracy Accuracy requirements
     * @param dest Destination buffer for data
     * @return Handle for waiting on the async operation
     */
    GetHandle Get(const char *VarName, size_t Step, size_t StepCount, size_t BlockID, Dims &Count,
                  Dims &Start, Accuracy &accuracy, void *dest) override;

    /**
     * @brief Wait for an async Get operation to complete
     * @param handle Handle returned from Get()
     * @return true if successful, false otherwise
     */
    bool WaitForGet(GetHandle handle) override;

    /**
     * @brief Read raw bytes from the remote file (not implemented for HTTPS)
     * @param Start Byte offset
     * @param Size Number of bytes
     * @param Dest Destination buffer
     * @return Handle for waiting (currently returns null handle)
     */
    GetHandle Read(size_t Start, size_t Size, void *Dest) override;

    /**
     * @brief Close the remote connection
     */
    void Close() override;

    // Configuration options

    /**
     * @brief Set connection timeout in seconds (default: 30)
     */
    void SetConnectTimeout(long seconds) { m_ConnectTimeout = seconds; }

    /**
     * @brief Set request timeout in seconds (default: 300)
     */
    void SetRequestTimeout(long seconds) { m_RequestTimeout = seconds; }

    /**
     * @brief Set CA certificate path for server verification
     */
    void SetCACertPath(const std::string &path) { m_CACertPath = path; }

    /**
     * @brief Disable SSL certificate verification (use with caution!)
     */
    void SetVerifySSL(bool verify) { m_VerifySSL = verify; }

private:
    /**
     * @brief Internal structure for async GET operations
     */
    struct AsyncGet
    {
        std::promise<bool> promise;
        std::string errorMsg;
        void *destBuffer;
        size_t destSize;
    };

    /**
     * @brief Build the SSI request string
     */
    std::string BuildRequestString(const char *VarName, size_t Step, size_t StepCount,
                                   size_t BlockID, const Dims &Count, const Dims &Start);

    /**
     * @brief Perform an HTTP POST request
     * @param endpoint URL endpoint
     * @param requestData POST body data
     * @param responseData Output buffer for response
     * @param errorMsg Output for error message on failure
     * @return true if successful
     */
    bool HttpPost(const std::string &endpoint, const std::string &requestData,
                  std::vector<char> &responseData, std::string &errorMsg);

    /**
     * @brief Initialize CURL (called once)
     */
    bool InitCurl();

    /**
     * @brief Cleanup CURL resources
     */
    void CleanupCurl();

    // Connection state
    std::string m_BaseUrl;      ///< Base URL (e.g., "https://server:8443/ssi")
    std::string m_Filename;     ///< Remote filename
    Mode m_Mode;                ///< Open mode
    bool m_RowMajorOrdering;    ///< Array ordering
    bool m_OpenSuccess = false; ///< Connection state

    // CURL state
    CURL *m_Curl = nullptr;      ///< CURL easy handle
    std::mutex m_CurlMutex;      ///< Mutex for CURL operations (not thread-safe)
    bool m_CurlInitialized = false;

    // Configuration
    long m_ConnectTimeout = 30;  ///< Connection timeout in seconds
    long m_RequestTimeout = 300; ///< Request timeout in seconds
    std::string m_CACertPath;    ///< CA certificate path
    bool m_VerifySSL = true;     ///< Whether to verify SSL certificates
};

} // end namespace adios2

#endif // ADIOS2_XROOTDHTTPSREMOTE_H
