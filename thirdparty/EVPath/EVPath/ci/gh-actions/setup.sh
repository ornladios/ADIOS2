#!/bin/bash

export CI_ROOT_DIR="${GITHUB_WORKSPACE//\\//}/.."
export CI_SOURCE_DIR="${GITHUB_WORKSPACE//\\//}"
export CI_DEP_DIR="${CI_ROOT_DIR}/dependencies"

# Install ninja, pkgconfig, and libfabric
case "$(uname -s)" in
  Linux)
    . $(dirname ${BASH_SOURCE[0]})/setup-linux.sh
    ;;
  Darwin)
    . $(dirname ${BASH_SOURCE[0]})/setup-macos.sh
    ;;
esac

export CMAKE_PREFIX_PATH=${CI_DEP_DIR}/install

# Install atl
echo
echo "Installing atl"
mkdir -p ${CI_DEP_DIR}/atl
cd ${CI_DEP_DIR}/atl
git clone https://github.com/GTKorvo/atl.git source
mkdir build
cd build
cmake -GNinja -DCMAKE_INSTALL_PREFIX=${CI_DEP_DIR}/install ../source
ninja install

# Install dill
echo
echo "Installing dill"
mkdir -p ${CI_DEP_DIR}/dill
cd ${CI_DEP_DIR}/dill
git clone https://github.com/GTKorvo/dill.git source
mkdir build
cd build
cmake -GNinja -DCMAKE_INSTALL_PREFIX=${CI_DEP_DIR}/install ../source
ninja install

# Install ffs
echo
echo "Installing ffs"
mkdir -p ${CI_DEP_DIR}/ffs
cd ${CI_DEP_DIR}/ffs
git clone https://github.com/GTKorvo/ffs.git source
mkdir build
cd build
cmake -GNinja -DCMAKE_INSTALL_PREFIX=${CI_DEP_DIR}/install ../source
ninja install

# Install enet
echo
echo "Installing enet"
mkdir -p ${CI_DEP_DIR}/enet
cd ${CI_DEP_DIR}/enet
git clone https://github.com/GTKorvo/enet.git source
mkdir build
cd build
cmake -GNinja -DCMAKE_INSTALL_PREFIX=${CI_DEP_DIR}/install ../source
ninja install
