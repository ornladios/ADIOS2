#!/bin/bash

echo
echo
echo "****************************************"
echo "  Installing FFS"
echo "****************************************"

export atl_ROOT="${PWD}/atl/install"

# Determine architecture flag for Windows builds
arch_flag=""
if [[ "${GH_YML_JOBNAME}" == *"win32"* ]]; then
  arch_flag="-A Win32"
elif [[ "${GH_YML_JOBNAME}" == *"windows"* ]]; then
  arch_flag="-A x64"
fi

export dill_ROOT="${PWD}/dill/install"
if [[ "${OS}" =~ "Windows" ]]
then
  extra_cmake_args=""
else
  extra_cmake_args="-DCMAKE_INSTALL_RPATH_USE_LINK_PATH=ON"
fi

mkdir ffs
cd ffs
git clone https://github.com/GTKorvo/ffs.git source
mkdir build
cd build
cmake ${arch_flag} ${extra_cmake_args} \
  -DCMAKE_BUILD_TYPE=$1 \
  -DBUILD_TESTING=OFF \
  -DCMAKE_INSTALL_PREFIX=${PWD}/../install \
  ../source
cmake --build . -j4 --config $1
cmake --install . --config $1
