#!/bin/sh
# Generate XRootD configuration for HTTP-to-SSI bridge testing
# Usage: generateXRootDHttpConfig.sh <adios2_xrootd_plugin> <adios2_xrootd_http_plugin> [xrd_port] [http_port]

set -e

# Default ports (can be overridden to avoid conflicts in parallel CI)
XRD_PORT="${3:-11094}"
HTTP_PORT="${4:-8443}"

echo "Generating config for XRootD with HTTP handler"
echo "SSI plugin library: $1"
echo "HTTP handler library: $2"
echo "XRootD port: ${XRD_PORT}"
echo "HTTP port: ${HTTP_PORT}"

# Get absolute path of current directory
BASEDIR="$(pwd)"
echo "Base directory: ${BASEDIR}"

mkdir -p xroot-http/var/spool
mkdir -p xroot-http/run/xrootd
mkdir -p xroot-http/etc/xrootd

# Generate self-signed certificate for testing
mkdir -p xroot-http/certs
if [ ! -f xroot-http/certs/server.crt ]; then
    echo "Generating self-signed SSL certificate..."
    # Unset OPENSSL_CONF to avoid issues with spack's corrupted OpenSSL config path
    # The -batch flag and -subj provide all needed info without requiring a config file
    if OPENSSL_CONF=/dev/null openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
        -keyout xroot-http/certs/server.key \
        -out xroot-http/certs/server.crt \
        -subj "/CN=localhost" \
        -batch; then
        echo "Certificate generated successfully"
        # Key permissions will be adjusted by start script if needed for user switching
        chmod 600 xroot-http/certs/server.key
        chmod 644 xroot-http/certs/server.crt
    else
        echo "ERROR: Certificate generation failed"
        exit 1
    fi
fi

{
    # Enable SSI filesystem
    echo "xrootd.fslib libXrdSsi.so"
    echo ""
    echo "all.export /etc nolock r/w"
    echo ""
    echo "oss.statlib -2 libXrdSsi.so"
    echo ""
    # Load SSI service plugin
    echo "ssi.svclib $1"
    echo ""
    # Disable native xrootd protocol (use port 0 to disable or a different port)
    echo "xrd.port ${XRD_PORT}"
    echo ""
    # Enable HTTP protocol
    echo "xrd.protocol http:${HTTP_PORT} libXrdHttp.so"
    echo ""
    # Configure HTTPS with self-signed cert (use absolute paths)
    echo "http.cert ${BASEDIR}/xroot-http/certs/server.crt"
    echo "http.key ${BASEDIR}/xroot-http/certs/server.key"
    echo ""
    # Load HTTP-to-SSI handler for /ssi endpoint
    # Format: http.exthandler <path-prefix> <library> [params]
    # Pass SSI library path as parameter so handler can find it
    echo "http.exthandler /ssi $2 ssilib=$1"
    echo ""
} > xroot-http/etc/xrootd/xrootd-http-ssi.cfg

echo "Config written to: ${BASEDIR}/xroot-http/etc/xrootd/xrootd-http-ssi.cfg"
cat xroot-http/etc/xrootd/xrootd-http-ssi.cfg
