/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_XRDCLBACKEND_H
#define ADIOS2_XRDCLBACKEND_H

#include "RemoteHttpBackend.h"

namespace adios2
{

/**
 * @brief XrdCl implementation of RemoteHttpBackend.
 *
 * Fetches a path-encoded URL with XrdCl::File: Open(url) followed by a single
 * Read(0, expectedSize) of the whole response.  The URL scheme selects the
 * XrdCl plugin at runtime, so the same backend serves a Pelican federation
 * (pelican://) or a direct HTTPS origin (https://, via xrdcl-curl).
 *
 * Each request runs on its own worker thread issuing synchronous XrdCl calls;
 * XrdCl manages connection pooling internally.  (A bounded pool or XrdCl-native
 * async submission is a future optimization.)
 */
class XrdClBackend : public RemoteHttpBackend
{
public:
    void SetConfig(const Config &config) override;
    void SubmitGet(const std::string &url, AsyncGet *op) override;

private:
    Config m_Config;
};

} // end namespace adios2

#endif // ADIOS2_XRDCLBACKEND_H
