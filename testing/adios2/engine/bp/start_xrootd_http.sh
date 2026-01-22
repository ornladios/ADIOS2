#!/bin/bash
# Start XRootD server with HTTP-to-SSI bridge enabled
# Usage: start_xrootd_http.sh <xrootd_binary> <config_base_dir>

set -x

if [ "$(id -u)" -eq 0 ]; then
    # we run as root in CI in docker images, this is OK, but we have to tell XRootD that it's OK.
    XROOTD_USER=("-R" "daemon")
fi

# Start XRootD with HTTP-SSI config
# The PID file is written to /tmp/xrootd-http.pid
"$1" -b -l /tmp/xroot-http.log -s /tmp/xrootd-http.pid -c "$2"/xroot-http/etc/xrootd/xrootd-http-ssi.cfg "${XROOTD_USER[@]}"

# Give the server a moment to start
sleep 2

exit 0
