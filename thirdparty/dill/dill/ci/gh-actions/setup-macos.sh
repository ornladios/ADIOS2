#!/bin/bash

echo
echo "Installing ninja"
brew install ninja

# Install cmake
echo
echo "Installing CMake"
mkdir -p ${CI_DEP_DIR}/tools
cd ${CI_DEP_DIR}/tools
curl -L https://github.com/Kitware/CMake/releases/download/v3.3.2/cmake-3.3.2-Darwin-x86_64.tar.gz | tar --strip-components=3 -xz

export PATH=${CI_DEP_DIR}/tools/bin:${PATH}
