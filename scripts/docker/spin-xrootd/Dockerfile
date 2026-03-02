FROM fedora:latest@sha256:781b7642e8bf256e9cf75d2aa58d86f5cc695fd2df113517614e181a5eee9138

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

# Create directories for XRootD with correct ownership for non-root operation
RUN mkdir -p /var/spool/xrootd /run/xrootd /etc/xrootd/certs /var/log/xrootd && \
    chown -R 194:194 /var/spool/xrootd /run/xrootd /var/log/xrootd /data

# Generate self-signed SSL certificate at build time
# This is needed because XRootD's http.exthandler requires TLS to be configured
# Ownership set to xrootd user (194:194) so non-root container can read the key
RUN openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
    -keyout /etc/xrootd/certs/server.key \
    -out /etc/xrootd/certs/server.crt \
    -subj "/CN=localhost" \
    -batch && \
    chmod 600 /etc/xrootd/certs/server.key && \
    chmod 644 /etc/xrootd/certs/server.crt && \
    chown -R 194:194 /etc/xrootd/certs

# Expose HTTP port (Spin Ingress will terminate TLS and forward to this port)
EXPOSE 8080

# Health check - verify XRootD HTTP is responding
# We check port 8080 is accepting connections (the /ssi endpoint requires POST data)
HEALTHCHECK --interval=30s --timeout=10s --start-period=10s --retries=3 \
    CMD curl -sf -o /dev/null http://localhost:8080/ || exit 1

ENTRYPOINT ["/docker-entrypoint.sh"]
