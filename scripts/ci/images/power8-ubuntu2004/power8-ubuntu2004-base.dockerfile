# vim: ft=dockerfile:
ARG TARGET_CPU=power8
FROM ornladios/adios2:ci-x86_64-${TARGET_CPU}-ubuntu20.04

# Install core dev packages
RUN apt update && \
    apt upgrade -y && \
    DEBIAN_FRONTEND="noninteractive" apt install -y --no-install-recommends  \
        bison \
        build-essential \
        bzip2 \
        curl \
        diffutils \
        file \
        findutils \
        binutils \
        coreutils \
        flex \
        g++ \
        gcc \
        gettext \
        gfortran \
        git \
        gnupg2 \
        gzip \
        lbzip2 \
        libblosc-dev \
        libcurl4-gnutls-dev \
        libzmq3-dev \
        libexpat1-dev \
        libffi-dev \
        libpng-dev \
        librhash-dev \
        libzstd-dev \
        make \
        ninja-build \
        openssl \
        patch \
        patchelf \
        perl \
        pkg-config \
        python3 \
        python3-pip \
        sudo \
        tar \
        tcl \
        unzip \
        valgrind \
        vim \
        xz-utils \
        zlib1g-dev && \
    apt clean && \
    rm -rfv /tmp/*

RUN git clone -b v0.19.1 https://github.com/spack/spack.git /opt/spack
ENV SPACK_ROOT=/opt/spack

RUN . /opt/spack/share/spack/setup-env.sh && \
    spack arch && \
    spack compiler find --scope system && \
    spack external find && \
    spack config add concretizer:targets:granularity:generic && \
    spack config add concretizer:reuse:false && \
    spack clean -a && \
    spack mirror add E4S https://cache.e4s.io && \
    spack buildcache keys --install --trust && \
    spack spec zlib

RUN . /opt/spack/share/spack/setup-env.sh && \
    spack env create adios2-ci && \
    spack -e adios2-ci add cmake@3.24 zfp sz hdf5 && \
    spack -e adios2-ci concretize --force --fresh && \
    spack -e adios2-ci install \
      --no-checksum \
      --fail-fast \
      -v \
      -j$((2 * $(grep -c '^processor' /proc/cpuinfo)))

# Setup default login environment
RUN . /etc/profile.d/modules.sh && \
    echo "source /opt/spack/share/spack/setup-env.sh" >> /etc/profile.d/zz-adios2-ci-env.sh && \
    echo "spack env activate adios2-ci" >> /etc/profile.d/zz-adios2-ci-env.sh

