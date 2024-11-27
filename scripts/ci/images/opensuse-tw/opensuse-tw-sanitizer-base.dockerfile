FROM opensuse/leap:15.6
LABEL maintainer "Vicente Adolfo Bolea Sanchez<vicente.bolea@kitware.com>"

# Base dependencies for building ADIOS2 projects
RUN zypper refresh && \
    zypper update -y && \
    zypper install -y --no-recommends \
      awk \
      bzip2 \
      gzip \
      bzip3-devel \
      clang9 \
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
      llvm9 \
      ninja \
      patch \
      python3-devel \
      python3-numpy \
      tar \
      zeromq-devel \
      zlib-devel \
      && \
    zypper clean --all
