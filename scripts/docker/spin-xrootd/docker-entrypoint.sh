#!/bin/bash
set -e

echo "=========================================="
echo "ADIOS2 XRootD HTTP Service for NERSC Spin"
echo "=========================================="

# Create required directories
mkdir -p /var/log/xrootd /var/spool/xrootd /run/xrootd

# Fix SSL key permissions for XRootD (requires restricted access, not world-readable)
# Copy to new files owned by current UID, then tighten key permissions
cp /tmp/server.key /tmp/xrootd-server.key
chmod 600 /tmp/xrootd-server.key
cp /tmp/server.crt /tmp/xrootd-server.crt

# Copy config to writable location and update cert paths
cp /etc/xrootd/xrootd-http.cfg /tmp/xrootd-http.cfg
sed -i 's|/tmp/server.key|/tmp/xrootd-server.key|' /tmp/xrootd-http.cfg
sed -i 's|/tmp/server.crt|/tmp/xrootd-server.crt|' /tmp/xrootd-http.cfg

echo ""
echo "XRootD Configuration:"
echo "--------------------"
cat /tmp/xrootd-http.cfg
echo ""
echo "=========================================="

# Determine user specification for xrootd
# If running as root, switch to xrootd user for security
if (( EUID == 0 )); then
    USER_SPEC=("-R" "xrootd")
    echo "Running as root, will switch to xrootd user"
    # Ensure xrootd user can access data directory
    chown -R xrootd:xrootd /data 2>/dev/null || true
else
    USER_SPEC=()
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
exec xrootd "${USER_SPEC[@]}" -n adios -c /tmp/xrootd-http.cfg
