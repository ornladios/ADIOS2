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

echo "Removing all existing brew packages"
brew remove --force $(brew list)
brew update

echo "Installing CMake and Ninja"
brew install cmake ninja

echo "Installing GCC"
brew install gcc

echo "Installing blosc compression"
brew install c-blosc

if [[ "$SYSTEM_JOBNAME" =~ .*openmpi.* ]]
then
  echo "Installing OpenMPI"
  brew install openmpi
fi
