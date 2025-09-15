#!/bin/bash
mkdir dill
cd dill
git clone https://github.com/GTKorvo/dill.git source
mkdir build
cd build
cmake \
  -DCMAKE_BUILD_TYPE=$1 \
  -DBUILD_TESTING=OFF \
  -DCMAKE_INSTALL_PREFIX=${PWD}/../install \
  ../source
cmake --build . -j4 --config $1
cmake --install . --config $1
if [ -f ${PWD}/../install/bin/dill.dll ] && [ -d /c/Windows/system32 ]; then
   # there's got to be a better way, but haven't found it
   cp ${PWD}/../install/bin/dill.dll /c/Windows/system32
fi
