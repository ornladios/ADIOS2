#!/bin/bash
# Start XRootD server with HTTP-to-SSI bridge enabled
# Usage: start_xrootd_http.sh <xrootd_binary> <config_base_dir>

set -x

XROOTD_BINARY="$1"
CONFIG_BASE="$2"
CONFIG_FILE="${CONFIG_BASE}/xroot-http/etc/xrootd/xrootd-http-ssi.cfg"
LOG_FILE="/tmp/xroot-http.log"
PID_FILE="/tmp/xrootd-http.pid"

echo "Starting XRootD HTTP server..."
echo "Binary: ${XROOTD_BINARY}"
echo "Config: ${CONFIG_FILE}"

# Verify config file exists
if [ ! -f "${CONFIG_FILE}" ]; then
    echo "ERROR: Config file not found: ${CONFIG_FILE}"
    ls -la "${CONFIG_BASE}/xroot-http/etc/xrootd/" 2>/dev/null || echo "Directory does not exist"
    exit 1
fi

# Show config contents for debugging
echo "=== Config file contents ==="
cat "${CONFIG_FILE}"
echo "=== End config ==="

# Verify certificates exist
if [ ! -f "${CONFIG_BASE}/xroot-http/certs/server.crt" ]; then
    echo "ERROR: SSL certificate not found"
    exit 1
fi

if [ "$(id -u)" -eq 0 ]; then
    # we run as root in CI in docker images, this is OK, but we have to tell XRootD that it's OK.
    XROOTD_USER=("-R" "daemon")
fi

# Remove old log file
rm -f "${LOG_FILE}"

# Start XRootD with HTTP-SSI config
"${XROOTD_BINARY}" -b -l "${LOG_FILE}" -s "${PID_FILE}" -c "${CONFIG_FILE}" "${XROOTD_USER[@]}"
START_RESULT=$?

if [ $START_RESULT -ne 0 ]; then
    echo "ERROR: XRootD failed to start (exit code: $START_RESULT)"
    exit 1
fi

# Give the server a moment to start
sleep 3

# Check if server is running
if [ -f "${PID_FILE}" ]; then
    PID=$(cat "${PID_FILE}")
    echo "XRootD started with PID: ${PID}"
    if kill -0 "${PID}" 2>/dev/null; then
        echo "Server is running"
    else
        echo "ERROR: Server process not running"
        echo "=== Server log ==="
        cat "${LOG_FILE}" 2>/dev/null || echo "No log file"
        exit 1
    fi
else
    echo "ERROR: PID file not created"
    echo "=== Server log ==="
    cat "${LOG_FILE}" 2>/dev/null || echo "No log file"
    exit 1
fi

# Show log for debugging
echo "=== Server log (first 50 lines) ==="
head -50 "${LOG_FILE}" 2>/dev/null || echo "No log file"

exit 0
