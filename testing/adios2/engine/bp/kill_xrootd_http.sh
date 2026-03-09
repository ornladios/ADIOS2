#!/bin/bash

# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

# Kill XRootD HTTP server
kill -2 "$(cat /tmp/xrootd-http.pid)"
#rm /tmp/xrootd-http.pid
exit 0
