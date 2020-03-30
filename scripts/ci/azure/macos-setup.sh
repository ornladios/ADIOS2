#!/bin/bash

echo "Setting up default XCode version"
case "$SYSTEM_JOBNAME" in
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

echo "Installing CMake Nightly"
curl -L https://cmake.org/files/dev/cmake-3.16.20191218-g8262562-Darwin-x86_64.tar.gz | sudo tar -C /Applications --strip-components=1 -xz

echo "Removing all existing brew packages"
brew remove --force $(brew list)

echo "Installing Kitware Ninja"
brew install ninja

echo "Installing GCC"
brew install gcc

echo "Installing blosc compression"
brew install c-blosc

if [[ "$SYSTEM_JOBNAME" =~ .*openmpi.* ]]
then
  echo "Installing OpenMPI"
  brew install openmpi
fi
