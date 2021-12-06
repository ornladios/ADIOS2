#!/bin/bash

set -x

echo "Setting up default XCode version"
case "$SYSTEM_JOBNAME" in
  *xcode1211*)
    sudo xcode-select --switch /Applications/Xcode_12.1.1.app
    ;;
  *xcode131*)
    sudo xcode-select --switch /Applications/Xcode_13.1.app
    ;;
  *)
    echo "  Unknown Xcode mapping.  Using defaults."
    ;;
esac

echo "Removing all existing brew package and update the formula"
brew remove --force --ignore-dependencies $(brew list --formula)
brew update

echo "Installing GCC"
brew install gcc

echo "Installing blosc compression"
brew install c-blosc

echo "Installing python and friends"
brew install python numpy
brew link --overwrite python
brew link --overwrite numpy

echo "Installing CMake and Ninja"
brew install cmake ninja

if [[ "$SYSTEM_JOBNAME" =~ .*openmpi.* ]]
then
  echo "Installing OpenMPI"
  brew install openmpi
fi
