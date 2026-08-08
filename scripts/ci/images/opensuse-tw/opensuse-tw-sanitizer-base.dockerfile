# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

FROM opensuse/tumbleweed
LABEL maintainer "Vicente Adolfo Bolea Sanchez<vicente.bolea@kitware.com>"

# Base dependencies for building ADIOS2 projects
RUN zypper refresh && \
    zypper update -y && \
    zypper install -y --no-recommends \
      awk \
      bzip2 \
      gzip \
      bzip3-devel \
      clang18 \
      cmake \
      curl \
      file \
      gcc \
      gcc-c++ \
      git \
      git-lfs \
      hdf5-devel \
      libffi-devel \
      libpng16-devel \
      libunwind-devel \
      llvm18 \
      ninja \
      patch \
      python3-devel \
      python3-numpy \
      tar \
      zeromq-devel \
      zlib-devel \
      && \
    zypper clean --all
