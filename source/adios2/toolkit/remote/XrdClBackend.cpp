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

// Fetch one URL synchronously: Open(url) then a single Read(0, expectedSize)
// of the whole response into the destination buffer (single Get) or into
// responseData (batch, which the caller then frames/parses).  Runs on its own
// thread; the result is delivered through op->promise.
void DoGet(const std::string &url, AsyncGet *op)
{
    XrdCl::File file;

    // The xrdcl-curl plugin fetches the whole response in one GET (full_download
    // mode; the origin returns a complete body per GET).  Properties only reach
    // the plugin once its object exists, so a no-op Open (Compress/None) loads
    // it, then SetProperty enables full_download, then the real Open runs.
    IgnoreStatus(file.Open(url, XrdCl::OpenFlags::Compress, XrdCl::Access::None, nullptr, 0));
    (void)file.SetProperty("XrdClCurlFullDownload", "true");

    XrdCl::XRootDStatus status = file.Open(url, XrdCl::OpenFlags::Read);
    if (!status.IsOK())
    {
        op->errorMsg = "XrdCl Open failed: " + status.ToString();
        op->promise.set_value(false);
        return;
    }

    // The read needs the exact byte count up front: single Get carries it as
    // expectedSize, batch as the framed total.
    if (op->expectedSize == 0)
    {
        op->errorMsg = "XrdCl backend requires a known response size";
        IgnoreStatus(file.Close());
        op->promise.set_value(false);
        return;
    }
    // XrdCl::File::Read takes a uint32_t count (and the server caps a single
    // response at INT_MAX), so reject anything the cast below would truncate.
    if (op->expectedSize > static_cast<size_t>(INT_MAX))
    {
        op->errorMsg = "XrdCl backend response size " + std::to_string(op->expectedSize) +
                       " exceeds the 2 GiB single-request limit";
        IgnoreStatus(file.Close());
        op->promise.set_value(false);
        return;
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
        op->promise.set_value(false);
        return;
    }

    if (op->destBuffer)
    {
        op->destSize = bytesRead;
    }
    else
    {
        op->responseData.resize(bytesRead);
    }
    op->promise.set_value(true);
}

} // anonymous namespace

// Stored but not yet consumed: xrdcl-curl has no per-File equivalents for
// these settings.  CA trust comes from the process-global XrdCl environment
// (XRD_CURLCERTFILE, falling back to X509_CERT_FILE); there is no insecure
// mode (verifySSL=false cannot be honored); timeouts ride on the plugin's
// own defaults.  Revisit as the XrdCl/Pelican picture firms up.
void XrdClBackend::SetConfig(const Config &config) { m_Config = config; }

void XrdClBackend::SubmitGet(const std::string &url, AsyncGet *op)
{
    std::thread(DoGet, url, op).detach();
}

} // end namespace adios2
