#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="yaml-cpp"
readonly ownership="yaml-cpp Upstream <robot@adios2>"
readonly subtree="thirdparty/yaml-cpp/yaml-cpp"
readonly repo="https://github.com/jbeder/yaml-cpp.git"
readonly tag="yaml-cpp-0.7.0"
readonly shortlog="true"
readonly paths="
  LICENSE
  include/
  src/
"

extract_source () {
    git_archive
    sed -i '2i#include <cstdint>' "${extractdir}/${name}-reduced/src/emitterutils.cpp"
}

. "${BASH_SOURCE%/*}/../update-common.sh"
