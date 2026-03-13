# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

FROM ghcr.io/ornladios/adios2:ci-opensuse-tw-sanitizer-base-20260313
LABEL maintainer "Vicente Adolfo Bolea Sanchez<vicente.bolea@kitware.com>"

RUN zypper refresh && \
    zypper install -y --no-recommends \
      lcov \
      && \
    zypper clean --all

# Install ZFP
WORKDIR /opt/zfp
RUN curl -L https://github.com/LLNL/zfp/releases/download/1.0.1/zfp-1.0.1.tar.gz | tar -xvz && \
    cmake -GNinja -S zfp-1.0.1 -B build \
      -DBUILD_SHARED_LIBS=ON \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/opt/zfp/1.0.1 \
      && \
    cmake --build build && \
    cmake --install build && \
    rm -rf zfp-1.0.1 build
ENV PATH=/opt/zfp/1.0.1/bin:${PATH} \
    LD_LIBRARY_PATH=/opt/zfp/1.0.1/lib64:${LD_LIBRARY_PATH} \
    CMAKE_PREFIX_PATH=/opt/zfp/1.0.1:${CMAKE_PREFIX_PATH}
