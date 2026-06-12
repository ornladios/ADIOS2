/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_REMOTEHTTPBACKEND_H
#define ADIOS2_REMOTEHTTPBACKEND_H

#include <future>
#include <string>
#include <vector>

namespace adios2
{

/**
 * @brief Async state for one HTTP GET, shared across HTTP transport backends.
 *
 * A backend fetches the body of a path-encoded URL into @ref responseData and
 * sets @ref promise: true on HTTP success, or false with @ref errorMsg set on
 * a transport/HTTP failure.  For a single Get the backend also checks the
 * response against @ref expectedSize and copies it into @ref destBuffer; batch
 * requests leave @ref destBuffer null and are framed/parsed by the caller
 * (XrootdHttpRemote), which owns the wire format.
 */
struct AsyncGet
{
    std::promise<bool> promise;
    std::string errorMsg;
    void *destBuffer = nullptr;     ///< single-get dest; null for batch
    size_t destSize = 0;            ///< bytes delivered to destBuffer on success
    size_t expectedSize = 0;        ///< expected response bytes; 0 = unchecked
    std::vector<char> responseData; ///< full response body
};

/**
 * @brief Transport seam for the HTTPS remote lane.
 *
 * Implementations move bytes for a path-encoded URL.  URL building, response
 * framing/parsing, and overrun checks live in XrootdHttpRemote and are shared
 * across backends; a backend is only responsible for the HTTP round trip.
 */
class RemoteHttpBackend
{
public:
    struct Config
    {
        bool useHttps = true;
        long connectTimeout = 30;
        long requestTimeout = 300;
        std::string caCertPath;
        bool verifySSL = true;
    };

    virtual ~RemoteHttpBackend() = default;

    /** Apply per-engine transport settings (called once at Open()). */
    virtual void SetConfig(const Config &config) = 0;

    /**
     * Asynchronously GET @p url.  On completion the implementation has filled
     * @c op->responseData with the body and set @c op->promise (true on HTTP
     * success; false with @c op->errorMsg set otherwise).  When @c op->destBuffer
     * is set, the body is checked against @c op->expectedSize and copied in.
     * @p op is owned by the caller and must outlive the request.
     */
    virtual void SubmitGet(const std::string &url, AsyncGet *op) = 0;
};

} // end namespace adios2

#endif // ADIOS2_REMOTEHTTPBACKEND_H
