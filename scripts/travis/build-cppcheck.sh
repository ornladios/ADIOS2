#!/usr/bin/env bash

cd ${HOME}
wget https://github.com/danmar/cppcheck/archive/1.82.tar.gz
mkdir -p /opt/cppcheck/build /opt/cppcheck/install
cd /opt/cppcheck
tar -xzf ${HOME}/1.82.tar.gz
cd build

CC=$(which clang-3.8)
CXX=$(which clang++-3.8)

echo "Found clang C compiler: ${CC}"
echo "Found clang C++ compiler: ${CXX}"

cmake \
  -DCMAKE_INSTALL_PREFIX:PATH=/opt/cppcheck/install \
  -DCMAKE_C_COMPILER:FILEPATH=${CC} \
  -DCMAKE_CXX_COMPILER:FILEPATH=${CXX} \
  ../cppcheck-1.82

make -j8
make install
