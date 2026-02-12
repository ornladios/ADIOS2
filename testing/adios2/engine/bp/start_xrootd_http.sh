#!/bin/bash
# Start XRootD server with HTTP-to-SSI bridge enabled
# Usage: start_xrootd_http.sh <xrootd_binary> <config_base_dir> [http_port]

set -x

XROOTD_BINARY="$1"
CONFIG_BASE="$2"
HTTP_PORT="${3:-8443}"
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
    # We run as root in CI in docker images, this is OK, but we have to tell XRootD that it's OK.
    # Try to find a user to run as (daemon or nobody), or skip -R if neither exists
    if id daemon >/dev/null 2>&1; then
        XROOTD_USER=("-R" "daemon")
        XROOTD_USER_NAME="daemon"
        echo "Running as daemon user"
    elif id nobody >/dev/null 2>&1; then
        XROOTD_USER=("-R" "nobody")
        XROOTD_USER_NAME="nobody"
        echo "Running as nobody user"
    else
        # No suitable user found, let XRootD run as root
        echo "WARNING: No suitable non-root user found, running as root"
        XROOTD_USER=()
        XROOTD_USER_NAME=""
    fi
    # Change ownership of certificate files so the target user can read them
    if [ -n "${XROOTD_USER_NAME}" ]; then
        echo "Changing ownership of certs to ${XROOTD_USER_NAME}"
        chown -R "${XROOTD_USER_NAME}" "${CONFIG_BASE}/xroot-http/certs"
    fi
fi

# Remove old log file
rm -f "${LOG_FILE}"

# Start XRootD with HTTP-SSI config
"${XROOTD_BINARY}" -b -l "${LOG_FILE}" -s "${PID_FILE}" -c "${CONFIG_FILE}" "${XROOTD_USER[@]}"
START_RESULT=$?

if [ $START_RESULT -ne 0 ]; then
    echo "ERROR: XRootD failed to start (exit code: $START_RESULT)"
    echo "=== Server log (on failure) ==="
    cat "${LOG_FILE}" 2>/dev/null || echo "No log file created"
    exit 1
fi

# Wait for the HTTPS listener to accept connections (up to 30 seconds).
# A simple kill -0 only checks if the process is alive, not whether the
# HTTP/TLS stack has finished initializing.  Polling the port avoids a
# race where tests start before the listener is ready.
MAX_WAIT=30
for i in $(seq 1 ${MAX_WAIT}); do
    if curl -sk -o /dev/null --max-time 2 "https://localhost:${HTTP_PORT}/" 2>/dev/null; then
        echo "HTTPS listener ready on port ${HTTP_PORT} after ${i}s"
        break
    fi
    # Fall back: if the process died, bail out immediately
    if [ -f "${PID_FILE}" ]; then
        PID=$(cat "${PID_FILE}")
        if ! kill -0 "${PID}" 2>/dev/null; then
            echo "ERROR: Server process (PID ${PID}) died during startup"
            echo "=== Server log ==="
            cat "${LOG_FILE}" 2>/dev/null || echo "No log file"
            exit 1
        fi
    fi
    if [ "${i}" -eq ${MAX_WAIT} ]; then
        echo "ERROR: HTTPS listener not ready after ${MAX_WAIT}s"
        echo "=== Server log ==="
        cat "${LOG_FILE}" 2>/dev/null || echo "No log file"
        exit 1
    fi
    sleep 1
done

# Show log for debugging
echo "=== Server log (first 50 lines) ==="
head -50 "${LOG_FILE}" 2>/dev/null || echo "No log file"

exit 0
