#!/bin/bash

# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

kill -2 "$(cat /tmp/xrootd.pid)"
#rm /tmp/xrootd.pid
exit 0
