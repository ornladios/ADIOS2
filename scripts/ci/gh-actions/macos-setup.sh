#!/bin/bash

echo "Setting up default XCode version"
if [ -z "${GH_YML_MATRIX_COMPILER}" ]
then
  echo "Error: GH_YML_MATRIX_COMPILER variable is not defined"
  exit 1
fi
XCODE_VER="$(echo ${GH_YML_MATRIX_COMPILER} | sed -e 's|_|.|g' -e 's|xcode||')"
if [ ! -d /Applications/Xcode_${XCODE_VER}.app ]
then
  echo "Error: XCode installation directory /Applications/Xcode_${XCODE_VER}.app does not exist"
  exit 2
fi
sudo xcode-select --switch /Applications/Xcode_${XCODE_VER}.app

echo "Installing CMake"
brew install cmake

echo "Installing Ninja"
brew install ninja

echo "Installing GCC"
brew install gcc
sudo ln -v -s $(which gfortran-11) /usr/local/bin/gfortran

echo "Installing blosc compression"
brew install c-blosc

echo "Installing python3"
brew install python numpy

if [[ "$GH_YML_JOBNAME" =~ -mpi ]]
then
  echo "Installing OpenMPI"
  brew install openmpi mpi4py
fi
