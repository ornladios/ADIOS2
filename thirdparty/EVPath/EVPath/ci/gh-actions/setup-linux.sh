#!/bin/bash

echo
echo "Installing ninja"
mkdir -p ${CI_DEP_DIR}/tools/bin
cd ${CI_DEP_DIR}/tools/bin
curl -O -L https://github.com/ninja-build/ninja/releases/download/v1.10.0/ninja-linux.zip
unzip ninja-linux.zip

echo
echo "Installing libfabric"
mkdir -p ${CI_DEP_DIR}/libfabric
cd ${CI_DEP_DIR}/libfabric
curl -L https://github.com/ofiwg/libfabric/releases/download/v1.9.1/libfabric-1.9.1.tar.bz2 | tar -xj
cd libfabric-1.9.1
./configure --prefix=${CI_DEP_DIR}/install --disable-static --enable-shared --enable-{shm,sockets,tcp,udp} --disable-{bgq,efa,gni,mrail,psm,psm2,rstream,rxd,rxm,usnic,verbs}
make -j2 install

# Install cmake
echo
echo "Installing CMake"
mkdir -p ${CI_DEP_DIR}/tools
cd ${CI_DEP_DIR}/tools
curl -L https://github.com/Kitware/CMake/releases/download/v3.3.2/cmake-3.3.2-Linux-x86_64.tar.gz | tar --strip-components=1 -xz

export PATH=${CI_DEP_DIR}/tools/bin:${PATH}
