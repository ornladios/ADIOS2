/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_LIBCURLBACKEND_H
#define ADIOS2_LIBCURLBACKEND_H

#include "RemoteHttpBackend.h"

namespace adios2
{

/**
 * @brief libcurl implementation of RemoteHttpBackend.
 *
 * All instances share a single process-wide curl_multi worker (one thread,
 * one multi handle) for connection pooling; see the CurlMultiPool singleton
 * in the .cpp.
 */
class LibcurlBackend : public RemoteHttpBackend
{
public:
    void SetConfig(const Config &config) override;
    void SubmitGet(const std::string &url, AsyncGet *op) override;

private:
    Config m_Config;
};

} // end namespace adios2

#endif // ADIOS2_LIBCURLBACKEND_H
