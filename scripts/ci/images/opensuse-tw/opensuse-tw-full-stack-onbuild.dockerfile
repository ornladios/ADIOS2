FROM ghcr.io/ornladios/adios2:ci-opensuse-tw-sanitizer-base

# Set up some arguments
ONBUILD ARG INSTALL_PREFIX
ONBUILD ARG TOOLCHAIN_FILE
ONBUILD ARG CFLAGS
ONBUILD ARG CXXFLAGS
ONBUILD ARG LLVM_USE_SANITIZER

ENV CC=clang
ENV CXX=clang++

# Build and install libc++
ONBUILD WORKDIR /root/llvm
ONBUILD RUN git clone --branch llvmorg-17.0.6 --depth 1 \
        https://github.com/llvm/llvm-project.git source
ONBUILD RUN cmake -GNinja -B build -S source/runtimes \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} \
            -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi" \
            -DLLVM_USE_SANITIZER=${LLVM_USE_SANITIZER} \
             && \
        cmake --build build --target install-cxxabi install-cxx

# Copy in the toolchain
ONBUILD COPY \
    ${TOOLCHAIN_FILE} \
    ${INSTALL_PREFIX}/toolchain.cmake

# Build and install zlib
ONBUILD WORKDIR /root/zlib
ONBUILD RUN git clone --branch v1.2.11 --depth 1 \
        https://github.com/madler/zlib.git source && \
    cmake -GNinja -S source -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=${INSTALL_PREFIX}/toolchain.cmake \
        -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} \
        && \
    cmake --build build && \
    cmake --install build

# Build and install bzip2
ONBUILD WORKDIR /root/bzip2
ONBUILD RUN git clone --branch bzip2-1.0.8 --depth 1 \
        https://sourceware.org/git/bzip2.git source && \
    cd source && \
    sed -e "s_^CC=.*\$_CC=/usr/bin/clang_" \
        -e "s_^CFLAGS=.*\$_CFLAGS=-fpic -fPIC -Wall -Winline -O2 ${CFLAGS} \$(BIGFILES)_" \
        -i Makefile-libbz2_so && \
    make -f Makefile-libbz2_so && \
    sed -e "s_^CC=.*\$_CC=/usr/bin/clang_" \
        -e "s_^PREFIX=.*\$_PREFIX=${INSTALL_PREFIX}_" \
        -e "s_^CFLAGS=.*\$_CFLAGS=-Wall -Winline -O2 ${CFLAGS} \$(BIGFILES)_" \
        -i Makefile && \
    make install && \
    install libbz2.so.1.0.8 ${INSTALL_PREFIX}/lib && \
    ln -s -T libbz2.so.1.0.8 ${INSTALL_PREFIX}/lib/libbz2.so.1.0 && \
    ln -s -T libbz2.so.1.0 ${INSTALL_PREFIX}/lib/libbz2.so

# Build and install zeromq
ONBUILD WORKDIR /root/zeromq
ONBUILD RUN git clone --branch v4.3.2 --depth 1 \
        https://github.com/zeromq/libzmq.git source && \
    cmake -GNinja -S source -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=${INSTALL_PREFIX}/toolchain.cmake \
        -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} \
        && \
    cmake --build build && \
    cmake --install build


# Build and install libpng
ONBUILD WORKDIR /root/libpng
ONBUILD RUN git clone --branch v1.6.9 --depth 1 \
        https://git.code.sf.net/p/libpng/code.git source && \
    cmake -GNinja -S source -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=${INSTALL_PREFIX}/toolchain.cmake \
        -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} \
        && \
    cmake --build build && \
    cmake --install build

# Build and install hdf5
ONBUILD WORKDIR /root/hdf5
ONBUILD RUN git clone --branch hdf5-1_14_3 --depth=1 \
        https://github.com/HDFGroup/hdf5.git source && \
    cmake -GNinja -S source -B build \
        -DBUILD_TESTING=OFF \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=${INSTALL_PREFIX}/toolchain.cmake \
        -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} \
        && \
    cmake --build build && \
    cmake --install build

# Build and install libfabric
ONBUILD WORKDIR /root/libfabric
ONBUILD RUN mkdir -p source build && \
    curl -L \
        https://github.com/ofiwg/libfabric/releases/download/v1.18.1/libfabric-1.18.1.tar.bz2 | \
    tar -C source -xj --strip-components=1 && \
    cd build && \
    ../source/configure \
        --prefix=${INSTALL_PREFIX} \
        --disable-efa \
        --disable-shm \
        --disable-verbs \
        CC="/usr/bin/clang ${CFLAGS} -L${INSTALL_PREFIX}/lib -Wl,-rpath,${INSTALL_PREFIX}/lib -Wno-unused-command-line-argument" \
        CXX="/usr/bin/clang++ ${CXXFLAGS} -L${INSTALL_PREFIX}/lib -Wl,-rpath,${INSTALL_PREFIX}/lib -Wno-unused-command-line-argument -nostdinc++ -isystem ${INSTALL_PREFIX}/include/c++/v1 -stdlib=libc++" && \
    make -j4 install

# Build and install libffi
ONBUILD WORKDIR /root/libffi
ONBUILD RUN mkdir -p source build && \
    curl -L \
        https://github.com/libffi/libffi/releases/download/v3.3/libffi-3.3.tar.gz | \
    tar -C source -xz --strip-components=1 && \
    cd build && \
    ../source/configure \
        --prefix=${INSTALL_PREFIX} \
        CC="/usr/bin/clang ${CFLAGS} -L${INSTALL_PREFIX}/lib -Wl,-rpath,${INSTALL_PREFIX}/lib -Wno-unused-command-line-argument" \
        CXX="/usr/bin/clang++ ${CXXFLAGS} -L${INSTALL_PREFIX}/lib -Wl,-rpath,${INSTALL_PREFIX}/lib -Wno-unused-command-line-argument -nostdinc++ -isystem ${INSTALL_PREFIX}/include/c++/v1 -stdlib=libc++" && \
    make -j4 install 
