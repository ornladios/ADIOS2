/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "XrdClBackend.h"

#include <climits> // INT_MAX
#include <cstdint>
#include <string>
#include <thread>

#include <XrdCl/XrdClFile.hh>

namespace adios2
{

namespace
{

// XrdCl declares Open/Close warn_unused_result, which GCC enforces even
// through a (void) cast; deliberately-ignored statuses are routed here.
void IgnoreStatus(const XrdCl::XRootDStatus &) {}

// Fetch one URL synchronously into op: Open(url) then a single Read(0,
// expectedSize) of the whole response into the destination buffer (single Get)
// or into responseData (batch, which the caller then frames/parses).  Returns
// true on success; on failure sets op->errorMsg and returns false.  Does not
// touch op->promise -- RunGet sets it exactly once, so the many exit paths here
// cannot forget to.
bool Fetch(const std::string &url, AsyncGet *op)
{
    XrdCl::File file;

    // The xrdcl-curl plugin fetches the whole response in one GET (full_download
    // mode; the origin returns a complete body per GET).  This no-op Open
    // (Compress/None) is the plugin's documented hook to instantiate itself so
    // the following SetProperty reaches the plugin object; then the real Open runs.
    IgnoreStatus(file.Open(url, XrdCl::OpenFlags::Compress, XrdCl::Access::None, nullptr, 0));
    (void)file.SetProperty("XrdClCurlFullDownload", "true");

    XrdCl::XRootDStatus status = file.Open(url, XrdCl::OpenFlags::Read);
    if (!status.IsOK())
    {
        op->errorMsg = "XrdCl Open failed: " + status.ToString();
        return false;
    }

    // The read needs the exact byte count up front: single Get carries it as
    // expectedSize, batch as the framed total.
    if (op->expectedSize == 0)
    {
        op->errorMsg = "XrdCl backend requires a known response size";
        IgnoreStatus(file.Close());
        return false;
    }
    // XrdCl::File::Read takes a uint32_t count (and the server caps a single
    // response at INT_MAX), so reject anything the cast below would truncate.
    if (op->expectedSize > static_cast<size_t>(INT_MAX))
    {
        op->errorMsg = "XrdCl backend response size " + std::to_string(op->expectedSize) +
                       " exceeds the 2 GiB single-request limit";
        IgnoreStatus(file.Close());
        return false;
    }
    const auto size = static_cast<uint32_t>(op->expectedSize);

    char *target = nullptr;
    if (op->destBuffer)
    {
        target = static_cast<char *>(op->destBuffer);
    }
    else
    {
        op->responseData.resize(size);
        target = op->responseData.data();
    }

    uint32_t bytesRead = 0;
    status = file.Read(0, size, target, bytesRead);
    IgnoreStatus(file.Close());

    if (!status.IsOK())
    {
        op->errorMsg = "XrdCl Read failed: " + status.ToString();
        return false;
    }

    if (op->destBuffer)
    {
        op->destSize = bytesRead;
    }
    else
    {
        op->responseData.resize(bytesRead);
    }
    return true;
}

// Run one fetch on its own thread and deliver the result through op->promise.
// The single set_value lives here so Fetch's exit paths each just return.
void RunGet(const std::string &url, AsyncGet *op) { op->promise.set_value(Fetch(url, op)); }

} // anonymous namespace

// Stored but not yet consumed: xrdcl-curl has no per-File equivalents for
// these settings.  CA trust comes from the process-global XrdCl environment
// (XRD_CURLCERTFILE, falling back to X509_CERT_FILE); there is no insecure
// mode (verifySSL=false cannot be honored); timeouts ride on the plugin's
// own defaults.  Revisit as the XrdCl/Pelican picture firms up.
void XrdClBackend::SetConfig(const Config &config) { m_Config = config; }

void XrdClBackend::SubmitGet(const std::string &url, AsyncGet *op)
{
    std::thread(RunGet, url, op).detach();
}

} // end namespace adios2
