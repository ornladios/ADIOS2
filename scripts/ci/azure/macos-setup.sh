#!/bin/bash

echo "Setting up default XCode version"
case "$SYSTEM_JOBNAME" in
  *macos1013*)
    sudo xcode-select --switch /Applications/Xcode_9.4.1.app
    ;;
  *macos1014*)
    sudo xcode-select --switch /Applications/Xcode_10.3.app
    ;;
  *)
    echo "  Unknown macOS image.  Using defaults."
    ;;
esac

echo "Installing Kitware Ninja"
curl -L https://github.com/Kitware/ninja/releases/download/v1.9.0.g99df1.kitware.dyndep-1.jobserver-1/ninja-1.9.0.g99df1.kitware.dyndep-1.jobserver-1_x86_64-apple-darwin.tar.gz | tar -C /usr/local/bin --strip-components=1 -xzv

echo "Installing GCC"
brew install gcc

if [[ "$SYSTEM_JOBNAME" =~ .*openmpi.* ]]
then
  echo "Installing OpenMPI"
  brew install openmpi
fi
