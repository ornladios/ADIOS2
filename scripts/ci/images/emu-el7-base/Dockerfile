ARG TARGET_CPU=power8
FROM ornladios/adios2:ci-x86_64-${TARGET_CPU}-el7

# Install core dev packages
RUN yum upgrade -y && \
    yum -y install make curl file valgrind vim bison flex sudo gdb \
        pkgconfig bison flex pkgconfig gcc gcc-c++ gcc-gfortran \
        zlib zlib-devel bzip2 bzip2-libs bzip2-devel libpng-devel \
        libfabric-devel libffi-devel
RUN yum -y install epel-release && \
    yum -y install zeromq-devel blosc-devel libzstd-devel

# Install and setup newer version of git
WORKDIR /opt/git
RUN yum install -y gettext openssl-devel curl-devel expat-devel && \
    mkdir tmp && \
    cd tmp && \
    curl -L https://mirrors.edge.kernel.org/pub/software/scm/git/git-2.26.0.tar.gz | tar -xz && \
    cd git-2.26.0 && \
    make -j$(grep -c '^processor' /proc/cpuinfo) prefix=/opt/git/2.26.0 all && \
    make prefix=/opt/git/2.26.0 install && \
    cd ../.. && \
    rm -rf tmp
ENV PATH=/opt/git/2.26.0/bin:${PATH}

# Install the most recent CMake from source
WORKDIR /opt/cmake
RUN yum install -y \
        bzip2-devel libcurl-devel expat-devel \
        xz-devel rhash-devel zlib-devel libzstd-devel && \
    mkdir tmp && \
    cd tmp && \
    curl -L https://github.com/Kitware/CMake/releases/download/v3.18.0/cmake-3.18.0.tar.gz | \
        tar -xz && \
    mkdir build && \
    cd build && \
    ../cmake-3.17.0/bootstrap \
      --system-libs \
      --no-qt-gui \
      --no-system-libarchive \
      --no-system-libuv \
      --no-system-jsoncpp \
      --prefix=/opt/cmake/3.18.0 \
      --parallel=$(grep -c '^processor' /proc/cpuinfo) && \
    make -j$(grep -c '^processor' /proc/cpuinfo) install && \
    cd ../.. && \
    rm -rf tmp 
ENV PATH=/opt/cmake/3.18.0/bin:${PATH}

# Misc cleanup of unneeded files
RUN yum clean all && \
    rm -rfv /tmp/* /var/cache/yum
