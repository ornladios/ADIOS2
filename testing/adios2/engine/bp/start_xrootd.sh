#!/bin/bash

# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

set -x

if [ "$(id -u)" -eq 0 ]; then
    # we run as root in CI in docker images, this is OK, but we have to tell XRootD that it's OK.
    XROOTD_USER=("-R" "daemon")
fi

"$1" -b -l /tmp/xroot.log -c "$2"/xroot/etc/xrootd/xrootd-ssi.cfg "${XROOTD_USER[@]}"
exit 0
