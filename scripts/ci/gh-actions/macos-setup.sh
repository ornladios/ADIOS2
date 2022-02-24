#!/bin/bash

echo "Setting up default XCode version"
if [ -z "${GH_YML_XCODE}" ]
then
  echo "Error: GH_YML_XCODE veriable is not defined"
  exit 1
fi
if [ ! -d /Applications/Xcode_${GH_YML_XCODE}.app ]
then
  echo "Error: XCode installation directory /Applications/Xcode_${GH_YML_XCODE}.app does not exist"
  exit 2
fi
sudo xcode-select --switch /Applications/Xcode_${GH_YML_XCODE}.app

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
