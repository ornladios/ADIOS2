#!/bin/bash

USE_MPI=${1:-ON}
echo USE_MPI=$USE_MPI

BUILD_FULL=true
VALUE=AUTO
if [ x"$2" == x"lean" ]; then
    BUILD_FULL=false
    echo "Build ADIOS without extra packages"
    VALUE=OFF
fi

ADIOS2_SOURCE=${PWD}/..
echo "ADIOS2_SOURCE: " ${ADIOS2_SOURCE}

# -DCMAKE_BUILD_TYPE options: Debug / Release / RelWithDebInfo / MinSizeRel
# Change -DCMAKE_INSTALL_PREFIX to your preferred location
# Default mode is AUTO
# If mode option is ON, it will fail if cmake can't find the dependency location
# Pass *_ROOT for dependency locations e.g. -DZFP_ROOT=/opt/zfp

cmake -DCMAKE_INSTALL_PREFIX=${PWD}/install \
          -DADIOS2_USE_MPI=${USE_MPI} \
          -DADIOS2_USE_HDF5=${VALUE} \
          -DADIOS2_USE_ZeroMQ=${VALUE} \
          -DADIOS2_USE_Fortran=${VALUE} \
          -DADIOS2_USE_Python=${VALUE} \
          -DADIOS2_USE_SST=${VALUE} \
          -DADIOS2_USE_BZip2=${VALUE} \
          -DADIOS2_USE_ZFP=${VALUE} \
          -DBUILD_SHARED_LIBS=ON \
          -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
          -DCMAKE_BUILD_TYPE=Release \
          -DADIOS2_BUILD_TESTING=${VALUE} \
          -DADIOS2_BUILD_EXAMPLES=${VALUE} \
          ${ADIOS2_SOURCE}
          