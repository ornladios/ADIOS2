#!/bin/bash

echo "Setting up default XCode version"
case "${GH_YML_JOBNAME}" in
  *xcode941*)
    sudo xcode-select --switch /Applications/Xcode_9.4.1.app
    ;;
  *xcode103*)
    sudo xcode-select --switch /Applications/Xcode_10.3.app
    ;;
  *xcode111*)
    sudo xcode-select --switch /Applications/Xcode_11.1.app
    ;;
  *)
    echo "  Unknown macOS image.  Using defaults."
    ;;
esac

echo "Installing CMake"
curl -L https://github.com/Kitware/CMake/releases/download/v3.17.3/cmake-3.17.3-Darwin-x86_64.tar.gz | sudo tar -C /Applications --strip-components=1 -xz

echo "Removing all existing brew packages"
brew remove --force $(brew list)

echo "Installing cmake and ninja"
brew install cmake ninja

echo "Installing GCC"
brew install gcc

echo "Installing blosc compression"
brew install c-blosc

echo "Installing python and numpy"
brew install python
pip3 install numpy

if [[ "${GH_YML_JOBNAME}" =~ .*openmpi.* ]]
then
  echo "Installing OpenMPI"
  brew install openmpi

  echo "Installing mpi4py"
  pip3 install mpi4py
fi
