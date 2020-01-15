FROM fedora:latest

# Install core dev packages
RUN dnf -y install git make curl file patch vim llvm clang

# Install CMake nightly
RUN mkdir -p /opt/cmake && \
    curl -L https://cmake.org/files/dev/cmake-3.16.20200111-g8b29a83-Linux-x86_64.tar.gz | \
        tar -C /opt/cmake --strip-components=1 -xzv
ENV PATH=/opt/cmake/bin:${PATH}

# Installing Kitware Ninja
RUN mkdir -p /usr/local/bin && \
    curl -L https://github.com/Kitware/ninja/releases/download/v1.9.0.g99df1.kitware.dyndep-1.jobserver-1/ninja-1.9.0.g99df1.kitware.dyndep-1.jobserver-1_x86_64-linux-gnu.tar.gz | \
        tar -C /usr/local/bin --strip-components=1 -xzv

# Misc cleanup
RUN dnf clean all && \
    rm -rfv /tmp/* /var/cache/dnf

# Building an msan-instrumented libc++
WORKDIR /opt/llvm
RUN git clone --branch llvmorg-9.0.0 --depth 1 \
        https://github.com/llvm/llvm-project.git source && \
    mkdir -p build/libcxx-msan && \
    cd build/libcxx-msan && \
    /opt/cmake/bin/cmake \
        -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/opt/llvm/9.0.0/libcxx-msan \
        -DCMAKE_C_COMPILER=/usr/bin/clang \
        -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
        -DLLVM_ENABLE_PROJECTS="libcxx;libcxxabi;libunwind" \
        -DLIBCXX_USE_LLVM_UNWINDER=ON \
        -DLLVM_USE_SANITIZER=MemoryWithOrigins \
        ../../source/llvm && \
    ninja install-cxxabi install-cxx install-unwind && \
    cd ../.. && \
    rm -rf source build

# Copy in the toolchains
COPY \
    toolchains/msan.cmake \
    /opt/llvm/9.0.0/toolchains/

# Build and install zlib
WORKDIR /opt/zlib
RUN git clone --branch v1.2.11 --depth 1 \
        https://github.com/madler/zlib.git source && \
    mkdir -p build && \
    cd build && \
    /opt/cmake/bin/cmake \
        -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=/opt/llvm/9.0.0/toolchains/msan.cmake \
        -DCMAKE_INSTALL_PREFIX=/opt/zlib/1.2.11/msan \
        ../source && \
    ninja install && \
    cd .. && \
    rm -rf source build

# Build and install bzip2
WORKDIR /opt/bzip2
RUN git clone --branch bzip2-1.0.8 \
        https://sourceware.org/git/bzip2.git source && \
    cd source && \
    sed -e 's_^CC=.*$_CC=/usr/bin/clang_' \
        -e 's_^CFLAGS=.*$_CFLAGS=-fpic -fPIC -Wall -Winline -O2 -fsanitize=memory -fsanitize-memory-track-origins $(BIGFILES)_' \
        -i Makefile-libbz2_so && \
    make -f Makefile-libbz2_so && \
    sed -e 's_^CC=.*$_CC=/usr/bin/clang_' \
        -e 's_^PREFIX=.*$_PREFIX=/opt/bzip2/1.0.8/msan_' \
        -e 's_^CFLAGS=.*$_CFLAGS=-Wall -Winline -O2 -fsanitize=memory -fsanitize-memory-track-origins $(BIGFILES)_' \
        -i Makefile && \
    make install && \
    install libbz2.so.1.0.8 /opt/bzip2/1.0.8/msan/lib && \
    ln -s -T libbz2.so.1.0.8 /opt/bzip2/1.0.8/msan/lib/libbz2.so.1.0 && \
    ln -s -T libbz2.so.1.0 /opt/bzip2/1.0.8/msan/lib/libbz2.so && \
    cd .. && \
    rm -rf source

# Build and install zeromq
WORKDIR /opt/zeromq
RUN git clone --branch v4.3.2 \
        https://github.com/zeromq/libzmq.git source && \
    mkdir -p build && \
    cd build && \
    /opt/cmake/bin/cmake \
        -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=/opt/llvm/9.0.0/toolchains/msan.cmake \
        -DCMAKE_INSTALL_PREFIX=/opt/zeromq/4.3.2/msan \
        ../source && \
    ninja install && \
    cd .. && \
    rm -rf source build

# Build and install libpng
WORKDIR /opt/libpng
RUN git clone --branch v1.6.9 \
        https://git.code.sf.net/p/libpng/code.git source && \
    mkdir -p build && \
    cd build && \
    /opt/cmake/bin/cmake \
        -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=/opt/llvm/9.0.0/toolchains/msan.cmake \
        -DCMAKE_INSTALL_PREFIX=/opt/libpng/1.6.9/msan \
        -DZLIB_ROOT=/opt/zlib/1.2.11/msan \
        ../source && \
    ninja install && \
    cd .. && \
    rm -rf source build

# Build and install hdf5
WORKDIR /opt/hdf5
RUN git clone --branch hdf5-1_10_6 \
        https://bitbucket.hdfgroup.org/scm/hdffv/hdf5.git source && \
    mkdir -p build && \
    cd build && \
    /opt/cmake/bin/cmake \
        -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=/opt/llvm/9.0.0/toolchains/msan.cmake \
        -DCMAKE_INSTALL_PREFIX=/opt/hdf5/1.10.6/msan \
        ../source && \
    ninja install && \
    cd .. && \
    rm -rf source build

# Build and install blosc
WORKDIR /opt/blosc
RUN git clone --branch v1.17.1 \
        https://github.com/Blosc/c-blosc.git source && \
    mkdir -p build && \
    cd build && \
    /opt/cmake/bin/cmake \
        -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=/opt/llvm/9.0.0/toolchains/msan.cmake \
        -DCMAKE_INSTALL_PREFIX=/opt/blosc/1.17.1/msan \
        ../source && \
    ninja install && \
    cd .. && \
    rm -rf source build
