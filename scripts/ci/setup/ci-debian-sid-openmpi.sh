#!/bin/sh

set -x
set -e

# This installation is done at test time rather than image build time because
# Debian Sid is a moving target. Packages are being updated every day. We want
# to make sure ADIOS is always tested against the latest Debian environment.

apt-get update
apt-get dist-upgrade -y
apt-get install -y --no-install-recommends \
  curl \
  python3-all \
  ca-certificates \
  git \
  cmake \
  ninja-build \
  make \
  g++ \
  gfortran \
  pkg-config \
  libpugixml-dev \
  libyaml-cpp-dev \
  pybind11-dev \
  libgtest-dev \
  nlohmann-json3-dev \
  libpython3-dev \
  python3-numpy \
  python3-mpi4py \
  libblosc-dev \
  libbz2-dev \
  libpng-dev \
  libopenmpi-dev \
  libczmq-dev \
  libhdf5-openmpi-dev \
  libfabric-dev \
  libffi-dev
