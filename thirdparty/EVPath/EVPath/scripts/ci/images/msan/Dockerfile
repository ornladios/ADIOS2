# MSan (MemorySanitizer) build environment for EVPath
#
# This container provides a clang environment with MSan-instrumented libc++
# for detecting uninitialized memory reads.
#
# Build: docker build -t evpath-msan .
# Run:   docker run -v $(pwd):/src evpath-msan

FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    clang \
    llvm \
    libc++-dev \
    libc++abi-dev \
    cmake \
    ninja-build \
    git \
    curl \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Build libc++ with MSan instrumentation
WORKDIR /tmp
ARG LLVM_VERSION=18
RUN git clone --depth 1 --branch llvmorg-${LLVM_VERSION}.1.0 https://github.com/llvm/llvm-project.git

WORKDIR /tmp/llvm-project
RUN cmake -G Ninja -S runtimes -B build \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind" \
    -DLLVM_USE_SANITIZER=MemoryWithOrigins \
    -DCMAKE_INSTALL_PREFIX=/opt/msan \
    -DLIBCXX_ENABLE_SHARED=OFF \
    -DLIBCXXABI_ENABLE_SHARED=OFF \
    && ninja -C build cxx cxxabi \
    && ninja -C build install-cxx install-cxxabi

# Final image
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    clang \
    llvm \
    cmake \
    ninja-build \
    git \
    curl \
    pkg-config \
    make \
    perl \
    bison \
    flex \
    && rm -rf /var/lib/apt/lists/*

# Copy MSan-instrumented libc++
COPY --from=builder /opt/msan /opt/msan

# Set up environment
ENV CC=clang
ENV CXX=clang++
ENV MSAN_PREFIX=/opt/msan

WORKDIR /build
