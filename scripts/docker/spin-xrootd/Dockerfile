FROM fedora:latest

# Install development tools
RUN dnf update -y && \
    dnf upgrade -y && \
    dnf group -y install c-development development-tools

# Install XRootD and dependencies
# Note: xrootd-* installs all XRootD components including HTTP plugin
RUN dnf install -y cmake git 'xrootd-*' libcurl-devel openssl && \
    dnf clean all && \
    rm -rf /var/cache/dnf

# Copy configuration files from build context (scripts/docker/spin-xrootd/)
COPY xrootd-http.cfg /etc/xrootd/xrootd-http.cfg
COPY docker-entrypoint.sh /docker-entrypoint.sh
RUN chmod +x /docker-entrypoint.sh

# Clone and build ADIOS2 with XRootD and CURL support from GitHub
ARG ADIOS2_REPO=https://github.com/ornladios/ADIOS2.git
ARG ADIOS2_BRANCH=master

# Cache bust: change this arg to force re-clone when branch has new commits
ARG CACHEBUST=0

# Build, install, create test data, then remove source entirely
RUN git clone --depth 1 --branch ${ADIOS2_BRANCH} ${ADIOS2_REPO} /ADIOS2-src && \
    mkdir -p /ADIOS2-src/build && \
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
    make install && \
    mkdir -p /data && \
    cd /data && \
    /ADIOS2-src/build/bin/TestCommonWrite bp5 sample && \
    rm -rf /ADIOS2-src

# Create directories for XRootD with world-writable permissions
# Spin runs containers with arbitrary UIDs, so we can't rely on ownership
RUN mkdir -p /var/spool/xrootd/adios /run/xrootd/adios /etc/xrootd/certs /var/log/xrootd /tmp/adios && \
    chmod -R 1777 /var/spool/xrootd /run/xrootd /var/log/xrootd /tmp/adios /data

# Generate self-signed SSL certificate at build time
# Written to /tmp so any UID can read them at startup
# The entrypoint will copy and fix permissions for XRootD
RUN openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
    -keyout /tmp/server.key \
    -out /tmp/server.crt \
    -subj "/CN=localhost" \
    -batch && \
    chmod 644 /tmp/server.key /tmp/server.crt

# Expose HTTP port (Spin Ingress will terminate TLS and forward to this port)
EXPOSE 8080

# Health check - verify XRootD HTTP is responding
# We check port 8080 is accepting connections (the /ssi endpoint requires POST data)
HEALTHCHECK --interval=30s --timeout=10s --start-period=10s --retries=3 \
    CMD curl -sf -o /dev/null http://localhost:8080/ || exit 1

ENTRYPOINT ["/docker-entrypoint.sh"]
