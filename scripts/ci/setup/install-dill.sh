#!/bin/bash

echo
echo
echo "****************************************"
echo "  Installing DILL"
echo "****************************************"

# Determine architecture flag for Windows builds
arch_flag=""
if [[ "${GH_YML_JOBNAME}" == *"win32"* ]]; then
  arch_flag="-A Win32"
elif [[ "${GH_YML_JOBNAME}" == *"windows"* ]]; then
  arch_flag="-A x64"
fi

mkdir dill
cd dill
git clone https://github.com/GTKorvo/dill.git source
mkdir build
cd build
cmake ${arch_flag} \
  -DCMAKE_BUILD_TYPE=$1 \
  -DBUILD_TESTING=OFF \
  -DCMAKE_INSTALL_PREFIX=${PWD}/../install \
  ../source
cmake --build . -j4 --config $1
cmake --install . --config $1
