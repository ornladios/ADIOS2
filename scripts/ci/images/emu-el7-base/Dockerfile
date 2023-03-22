ARG TARGET_CPU=power8
FROM ornladios/adios2:ci-x86_64-${TARGET_CPU}-el7

# Install core dev packages
RUN yum upgrade -y && \
    yum -y install centos-release-scl && \
    yum-config-manager --enable rhel-server-rhscl-7-rpms && \
    yum -y install \
        bison \
        bzip2 \
        bzip2-libs \
        curl \
        devtoolset-7 \
        devtoolset-7-gcc \
        devtoolset-7-gcc-c++ \
        devtoolset-7-gcc-gdb \
        devtoolset-7-gcc-gfortran \
        file \
        flex \
        gcc \
        gcc-c++ \
        gcc-gfortran \
        libfabric-devel \
        libffi-devel \
        libpng-devel \
        make \
        pkgconfig \
        sudo \
        valgrind \
        vim \
        zlib \
    yum -y install epel-release && \
    yum -y install \
        blosc-devel \
        bzip2-devel \
        curl-devel \
        expat-devel \
        gettext \
        libcurl-devel \
        libzstd-devel \
        openssl-devel \
        rhash-devel \
        xz-devel \
        zeromq-devel \
        zlib-devel && \
    yum clean all && \
    rm -rfv /tmp/* /var/cache/yum

ENV LAUNCHER="/usr/bin/scl enable devtoolset-7 --"

# Install and setup newer version of git
WORKDIR /opt/git
RUN mkdir tmp && \
    cd tmp && \
    curl -L https://mirrors.edge.kernel.org/pub/software/scm/git/git-2.26.0.tar.gz | tar -xz && \
    cd git-2.26.0 && \
    $LAUNCHER make -j$(grep -c '^processor' /proc/cpuinfo) prefix=/opt/git/2.26.0 all && \
    $LAUNCHER make prefix=/opt/git/2.26.0 install && \
    cd ../.. && \
    rm -rf tmp
ENV PATH=/opt/git/2.26.0/bin:${PATH}

# Install the most recent CMake from source
WORKDIR /opt/cmake
RUN mkdir tmp && \
    cd tmp && \
    curl -L https://github.com/Kitware/CMake/releases/download/v3.26.0/cmake-3.26.0.tar.gz | \
        tar -xz && \
    mkdir build && \
    cd build && \
    $LAUNCHER ../cmake-3.26.0/bootstrap \
      --system-libs \
      --no-qt-gui \
      --no-system-libarchive \
      --no-system-libuv \
      --no-system-jsoncpp \
      --prefix=/opt/cmake/3.26.0 \
      --parallel=$(grep -c '^processor' /proc/cpuinfo) && \
    make -j$(grep -c '^processor' /proc/cpuinfo) install && \
    cd ../.. && \
    rm -rf tmp
ENV PATH=/opt/cmake/3.26.0/bin:${PATH}
