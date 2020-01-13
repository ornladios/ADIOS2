FROM fedora:latest

# Install core dev packages
RUN dnf -y install gcc gcc-c++ gcc-gfortran git make curl file patch \
        zlib-devel bzip2 bzip2-libs bzip2-devel blosc-devel vim valgrind \
        zeromq-devel libpng-devel hdf5-devel openmpi-devel hdf5-openmpi-devel \
        python3-devel python3-numpy python3-mpi4py-openmpi \
        libasan libtsan libubsan clang llvm-devel libcxx-devel

# Cleanup headers and packages
RUN dnf clean all

# Install CMake nightly
WORKDIR /opt/cmake
RUN curl -L https://cmake.org/files/dev/cmake-3.16.20200111-g8b29a83-Linux-x86_64.tar.gz |tar --strip-components=1 -xzv
ENV PATH=/opt/cmake/bin:${PATH}

# Installing Kitware Ninja
WORKDIR /usr/local/bin
RUN curl -L https://github.com/Kitware/ninja/releases/download/v1.9.0.g99df1.kitware.dyndep-1.jobserver-1/ninja-1.9.0.g99df1.kitware.dyndep-1.jobserver-1_x86_64-linux-gnu.tar.gz | tar --strip-components=1 -xzv

# Building an msan-instrumented libc++
WORKDIR /opt/llvm
RUN git clone --branch llvmorg-9.0.0 --depth 1 \
        https://github.com/llvm/llvm-project.git source && \
    mkdir -p build/libcxx-msan && \
    cd build/libcxx-msan && \
    export \
        CC=/usr/bin/clang \
        CXX=/usr/bin/clang++ && \
    /opt/cmake/bin/cmake \
        -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/opt/llvm/9.0.0/libcxx-msan \
        -DCMAKE_INSTALL_RPATH='$ORIGIN' \
        -DLLVM_ENABLE_PROJECTS="libcxx;libcxxabi" \
        -DLLVM_USE_SANITIZER=MemoryWithOrigins \
        -DLIBCXX_ENABLE_ABI_LINKER_SCRIPT=OFF \
        ../../source/llvm && \
    ninja install-cxxabi install-cxx && \
    cd ../.. && \
    rm -rf source build

# Setup clang with msan-enabled libc++
#ENV CC=/usr/bin/clang
#ENV CFLAGS="-fsanitize=memory -fsanitize-memory-track-origins=2 -fno-omit-frame-pointer"
#ENV CXX=/usr/bin/clang++
#ENV CXXFLAGS="-fsanitize=memory -fsanitize-memory-track-origins=2 -fno-omit-frame-pointer -stdlib=libc++ -nostdinc++ -I/opt/llvm/9.0.0/libcxx-msan/include/c++/v1 -L/opt/llvm/9.0.0/libcxx-msan/lib64 -Wl,rpath=/opt/llvm/9.0.0/libcxx-msan/lib64"

# Misc cleanup of unneeded files
RUN rm -rfv /tmp/* /var/cache/dnf
