#!/bin/bash

echo
echo "Installing ninja"
mkdir -p ${CI_DEP_DIR}/tools/bin
cd ${CI_DEP_DIR}/tools/bin
curl -O -L https://github.com/ninja-build/ninja/releases/download/v1.10.0/ninja-linux.zip
unzip ninja-linux.zip

# Install cmake
echo
echo "Installing CMake"
mkdir -p ${CI_DEP_DIR}/tools
cd ${CI_DEP_DIR}/tools
curl -L https://github.com/Kitware/CMake/releases/download/v3.3.2/cmake-3.3.2-Linux-x86_64.tar.gz | tar --strip-components=1 -xz

export PATH=${CI_DEP_DIR}/tools/bin:${PATH}
