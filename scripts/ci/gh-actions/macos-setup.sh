#!/bin/bash

# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

set -xe

echo "Setting up default XCode version"
if [ -z "${GH_YML_MATRIX_COMPILER}" ]
then
  echo "Error: GH_YML_MATRIX_COMPILER variable is not defined"
  exit 1
fi
XCODE_VER="$(echo "${GH_YML_MATRIX_COMPILER}" | sed -e 's|_|.|g' -e 's|xcode||')"
if [ ! -d "/Applications/Xcode_${XCODE_VER}.app" ]
then
  echo "Error: XCode installation directory /Applications/Xcode_${XCODE_VER}.app does not exist"
  exit 2
fi
sudo xcode-select --switch "/Applications/Xcode_${XCODE_VER}.app"
sudo ln -v -s "$(which gfortran-13)" /usr/local/bin/gfortran

echo "Installing Miniconda"

if [ "${RUNNER_ARCH}" = "X64" ]
then
  readonly checksum="9c88674b1a839eeb4cff006df397a05ea7d896472318fd84b7070278f9653dc6"
  readonly pkg="Miniconda3-py313_25.7.0-2-MacOSX-x86_64.sh"
elif [ "${RUNNER_ARCH}" = "ARM64" ]
then
  readonly checksum="5c0137ef38c153649da28ca31a420b9c12c94cf636319beb8c925396d797fe62"
  readonly pkg="Miniconda3-py313_25.7.0-2-MacOSX-arm64.sh"
else
  echo "Error: unknown platform: ${RUNNER_ARCH} "
  exit 3
fi
echo "${checksum}  ${pkg}" > miniconda.sha256sum

curl -OL "https://repo.anaconda.com/miniconda/${pkg}"
shasum -a 256 --check miniconda.sha256sum
bash "./${pkg}" -b

# shellcheck source=/dev/null
source "/Users/runner/miniconda3/bin/activate"

# Canonical installation of Miniconda
conda init --all
conda config --remove channels defaults
conda config --add channels conda-forge
conda config --set channel_priority strict
conda update -n base -c conda-forge conda -y
conda update --all -y

conda env create --verbose -f "gha/scripts/ci/gh-actions/conda-env-macos.yml"

conda list
conda list --export > ./conda-env.txt
conda info --verbose
echo 'conda activate adios2' >> /Users/runner/.bash_profile
