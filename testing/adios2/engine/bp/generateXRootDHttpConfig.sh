#!/bin/sh
# Generate XRootD configuration for HTTP-to-SSI bridge testing
# Usage: generateXRootDHttpConfig.sh <adios2_xrootd_plugin> <adios2_xrootd_http_plugin>

echo "Generating config for XRootD with HTTP handler"
echo "SSI plugin library: $1"
echo "HTTP handler library: $2"

# Get absolute path of current directory
BASEDIR="$(pwd)"

mkdir -p xroot-http/var/spool
mkdir -p xroot-http/run/xrootd
mkdir -p xroot-http/etc/xrootd

# Generate self-signed certificate for testing
mkdir -p xroot-http/certs
if [ ! -f xroot-http/certs/server.crt ]; then
    openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
        -keyout xroot-http/certs/server.key \
        -out xroot-http/certs/server.crt \
        -subj "/CN=localhost" 2>/dev/null
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
    echo "xrd.port 11094"
    echo ""
    # Enable HTTP protocol on port 8443
    echo "xrd.protocol http:8443 libXrdHttp.so"
    echo ""
    # Configure HTTPS with self-signed cert (use absolute paths)
    echo "http.cert ${BASEDIR}/xroot-http/certs/server.crt"
    echo "http.key ${BASEDIR}/xroot-http/certs/server.key"
    echo ""
    # Load HTTP-to-SSI handler (external handler for /ssi endpoint)
    echo "http.exthandler xrdadioshttp $2"
    echo ""
} > xroot-http/etc/xrootd/xrootd-http-ssi.cfg

echo "Config written to: ${BASEDIR}/xroot-http/etc/xrootd/xrootd-http-ssi.cfg"
