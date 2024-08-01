#!/bin/bash

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
sudo ln -v -s "$(which gfortran-11)" /usr/local/bin/gfortran

echo "Installing Miniconda"

if [ "${RUNNER_ARCH}" = "X64" ]
then
  readonly checksum="6d7c1cc138adfc4bb2ccbb8a22eb8e9eb13a366b6af0d63245b643e6c3a3c708"
  readonly pkg="Miniconda3-py310_24.5.0-0-MacOSX-x86_64.sh"
elif [ "${RUNNER_ARCH}" = "ARM64" ]
then
  readonly checksum="e422602aa19140c600b5889e5b41a0d7187640107ea82fcb5da857dd25330148"
  readonly pkg="Miniconda3-py310_24.5.0-0-MacOSX-arm64.sh"
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
conda update --all -y
conda config --add channels conda-forge
conda config --set channel_priority strict

conda env create --verbose -f "gha/scripts/ci/gh-actions/conda-env-macos.yml"

conda list -n adios2
conda info --verbose
echo 'conda activate adios2' >> /Users/runner/.bash_profile
