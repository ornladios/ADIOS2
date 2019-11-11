#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="yaml-cpp"
readonly ownership="yaml-cpp Upstream <robot@adios2>"
readonly subtree="thirdparty/yaml-cpp/yaml-cpp"
readonly repo="https://github.com/jbeder/yaml-cpp.git"
readonly tag="yaml-cpp-0.6.3"
readonly shortlog="true"
readonly paths="
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
