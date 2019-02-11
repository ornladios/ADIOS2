#!/bin/bash -e

rm -rf build
mkdir build
cd build

set -x

cmake -DCMAKE_C_COMPILER=`which gcc` -DCMAKE_CXX_COMPILER=`which g++` \
-DCMAKE_INSTALL_PREFIX=`pwd` -DCMAKE_BUILD_TYPE=Debug ..
make -j8 -l4
