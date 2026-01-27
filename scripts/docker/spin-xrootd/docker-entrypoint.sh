#!/bin/bash
set -e

echo "=========================================="
echo "ADIOS2 XRootD HTTP Service for NERSC Spin"
echo "=========================================="

# Create required directories and set ownership
mkdir -p /var/log/xrootd /var/spool/xrootd /run/xrootd
chown -R xrootd:xrootd /var/log/xrootd /var/spool/xrootd /run/xrootd /etc/xrootd/certs 2>/dev/null || true

echo ""
echo "XRootD Configuration:"
echo "--------------------"
cat /etc/xrootd/xrootd-http.cfg
echo ""
echo "=========================================="

# Determine user specification for xrootd
# If running as root, switch to xrootd user for security
if (( EUID == 0 )); then
    USER_SPEC="-R xrootd"
    echo "Running as root, will switch to xrootd user"
    # Ensure xrootd user can access data directory
    chown -R xrootd:xrootd /data 2>/dev/null || true
else
    USER_SPEC=""
    echo "Running as non-root user (UID: $EUID)"
fi

echo ""
echo "Starting XRootD HTTP server on port 8080..."
echo "Data directory: /data"
echo ""

# Start XRootD with HTTP configuration in foreground
# -n adios: instance name
# -c: configuration file
# Note: Running without -l to let logs go to stdout/stderr
exec xrootd $USER_SPEC -n adios -c /etc/xrootd/xrootd-http.cfg
