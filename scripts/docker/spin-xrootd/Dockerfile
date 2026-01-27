FROM fedora:latest

# Install development tools
RUN dnf update -y && \
    dnf upgrade -y && \
    dnf group -y install c-development development-tools

# Install XRootD and dependencies
# Note: xrootd-* installs all XRootD components including HTTP plugin
RUN dnf install -y cmake 'xrootd-*' libcurl-devel openssl && \
    dnf clean all && \
    rm -rf /var/cache/dnf

# Clone and build ADIOS2 with XRootD and CURL support
# Using local copy for development - in production, use a tagged release
COPY . /ADIOS2-src
RUN mkdir -p /ADIOS2-src/build && \
    cd /ADIOS2-src/build && \
    cmake \
        -DADIOS2_USE_XRootD=ON \
        -DADIOS2_USE_CURL=ON \
        -DADIOS2_USE_MPI=OFF \
        -DADIOS2_USE_Fortran=OFF \
        -DADIOS2_USE_Python=OFF \
        -DADIOS2_USE_SST=OFF \
        -DBUILD_TESTING=ON \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        .. && \
    make -j$(nproc) && \
    make install

# Create test data directory with sample BP5 file
RUN mkdir -p /data && \
    cd /data && \
    /ADIOS2-src/build/bin/TestCommonWrite bp5 sample

# Clean up source to reduce image size
RUN rm -rf /ADIOS2-src

# Create directories for XRootD
RUN mkdir -p /var/spool/xrootd /run/xrootd /etc/xrootd /etc/xrootd/certs

# Generate self-signed SSL certificate at build time
# This is needed because XRootD's http.exthandler requires TLS to be configured
RUN openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
    -keyout /etc/xrootd/certs/server.key \
    -out /etc/xrootd/certs/server.crt \
    -subj "/CN=localhost" \
    -batch && \
    chmod 600 /etc/xrootd/certs/server.key && \
    chmod 644 /etc/xrootd/certs/server.crt

# Copy configuration files
COPY scripts/docker/spin-xrootd/xrootd-http.cfg /etc/xrootd/xrootd-http.cfg
COPY scripts/docker/spin-xrootd/docker-entrypoint.sh /docker-entrypoint.sh
RUN chmod +x /docker-entrypoint.sh

# Expose HTTP port (Spin Ingress will terminate TLS and forward to this port)
EXPOSE 8080

# Health check - verify XRootD HTTP is responding
# We check port 8080 is accepting connections (the /ssi endpoint requires POST data)
HEALTHCHECK --interval=30s --timeout=10s --start-period=10s --retries=3 \
    CMD curl -sf -o /dev/null http://localhost:8080/ || exit 1

ENTRYPOINT ["/docker-entrypoint.sh"]
